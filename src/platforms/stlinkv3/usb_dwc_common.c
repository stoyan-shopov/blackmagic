/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2011 Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright (C) 2020 Stoyan Shopov <stoyan.shopov@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <libopencm3/cm3/common.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/dwc/otg_common.h>
#include "usb_private.h"
#include "usb_dwc_common.h"

/* The FS core and the HS core have the same register layout.
 * As the code can be used on both cores, the registers offset is modified
 * according to the selected cores base address. */
#define dev_base_address (usbd_dev->driver->base_address)
#define REBASE(x)        MMIO32((x) + (dev_base_address))

/* TODO: this does not belong here; maybe add it as a new field in the driver structure */
enum
{
	MAX_BULK_PACKET_SIZE	= 512,
	USB_ENDPOINT_COUNT	= 9,
};

/* Note: (shopov - 15122020) - the original 'usb_dwc_common.c' source code
 * handles incoming OUT and SETUP usb packets as soon as they are available
 * in the packet FIFO.
 *
 * I inspected the stm-cube sources, and there, the incoming OUT and
 * SETUP packets are handled only when a XFRC - transfer complete interrupt is
 * received. This means that, at this point, it is guaranteed that the respective
 * usb endpoint 'enable' bit is cleared by the usb core. From the st manual:
 *
 * The core clears this (EPENA) bit before setting any of the following interrupts on this endpoint:
 * - SETUP phase done
 * - Endpoint disabled
 * - Transfer completed
 *
 * Maybe this is not relevant, but it seems to me that such handling is more
 * robust. I spent a significant amount of time debugging some other usb issues,
 * and while debugging, I adjusted the code to behave more like the stm-cube
 * sources. When I was done with debugging, I did not investigate if the original
 * usb_dwc_common driver code for handling incoming (OUT) usb packages would work with.
 * the adjustments I made. It seems to me that the stm-cube handling of incoming (OUT)
 * usb packages is more robust, and I decided to keep it this way.
 *
 * However, for that to work, it is needed that the packets are read from the usb FIFO packet
 * memory in some temporary storage, and are handed to the upper usb layers only
 * when the corresponding XFRC interrupts are received. This means that some temporary
 * storage is necessary, so this variable serves this purpose.
 *
 * Also, note that in usb_control.c, function '_usbd_control_setup()' expects that
 * the setup data packet (8 bytes) has already been read in the
 * 'usbd_dev->control_state.req' buffer prior to invoking the '_usbd_control_setup()'
 * function. I am not very familiar with the usb stack, but it seems more natural
 * to me that the setup packet is actually read in the '_usbd_control_setup()'
 * function, instead of handling it as a special case here.
 */
static struct incoming_packet
{
	bool	is_packet_present;
	int	packet_length;
	uint8_t	packet_data[MAX_BULK_PACKET_SIZE];
} stashed_packets[USB_ENDPOINT_COUNT];

void dwc_set_address(usbd_device *usbd_dev, uint8_t addr)
{
	REBASE(OTG_DCFG) = (REBASE(OTG_DCFG) & ~OTG_DCFG_DAD) | (addr << 4);
}

void dwc_ep_setup(usbd_device *usbd_dev, uint8_t addr, uint8_t type,
			uint16_t max_size,
			void (*callback) (usbd_device *usbd_dev, uint8_t ep))
{
	/*
	 * Configure endpoint address and type. Allocate FIFO memory for
	 * endpoint. Install callback function.
	 */
	uint8_t dir = addr & 0x80;
	addr &= 0x7f;

	if (addr == 0) { /* For the default control endpoint */
		/* Configure IN part. */
		if (max_size >= 64) {
			REBASE(OTG_DIEPCTL0) = OTG_DIEPCTL0_MPSIZ_64;
		} else if (max_size >= 32) {
			REBASE(OTG_DIEPCTL0) = OTG_DIEPCTL0_MPSIZ_32;
		} else if (max_size >= 16) {
			REBASE(OTG_DIEPCTL0) = OTG_DIEPCTL0_MPSIZ_16;
		} else {
			REBASE(OTG_DIEPCTL0) = OTG_DIEPCTL0_MPSIZ_8;
		}

		REBASE(OTG_DIEPTSIZ0) =
			(max_size & OTG_DIEPSIZ0_XFRSIZ_MASK);
		REBASE(OTG_DIEPCTL0) |=
			/* OTG_DIEPCTL0_EPENA | */OTG_DIEPCTL0_SNAK;

		/* Configure OUT part. */
		usbd_dev->doeptsiz[0] = OTG_DIEPSIZ0_STUPCNT_1 |
			OTG_DIEPSIZ0_PKTCNT |
			(max_size & OTG_DIEPSIZ0_XFRSIZ_MASK);
		REBASE(OTG_DOEPTSIZ(0)) = usbd_dev->doeptsiz[0];
		REBASE(OTG_DOEPCTL(0)) |=
		    OTG_DOEPCTL0_EPENA | OTG_DIEPCTL0_SNAK;

		REBASE(OTG_GNPTXFSIZ) = ((max_size / 4) << 16) |
					 usbd_dev->driver->rx_fifo_size;
		usbd_dev->fifo_mem_top += max_size / 4;
		usbd_dev->fifo_mem_top_ep0 = usbd_dev->fifo_mem_top;

		return;
	}

	if (dir) {
		REBASE(OTG_DIEPTXF(addr)) = ((max_size / 4) << 16) |
					     usbd_dev->fifo_mem_top;
		usbd_dev->fifo_mem_top += max_size / 4;

		REBASE(OTG_DIEPTSIZ(addr)) =
		    (max_size & OTG_DIEPSIZ0_XFRSIZ_MASK);
		REBASE(OTG_DIEPCTL(addr)) |=
		    /*OTG_DIEPCTL0_EPENA |*/ OTG_DIEPCTL0_SNAK | (type << 18)
		    | OTG_DIEPCTL0_USBAEP | OTG_DIEPCTLX_SD0PID
		    | (addr << 22) | max_size;

		if (callback) {
			usbd_dev->user_callback_ctr[addr][USB_TRANSACTION_IN] =
			    (void *)callback;
		}
	}

	if (!dir) {
		usbd_dev->doeptsiz[addr] = OTG_DIEPSIZ0_PKTCNT |
				 (max_size & OTG_DIEPSIZ0_XFRSIZ_MASK);
		REBASE(OTG_DOEPTSIZ(addr)) = usbd_dev->doeptsiz[addr];
		REBASE(OTG_DOEPCTL(addr)) |= OTG_DOEPCTL0_EPENA |
		    OTG_DOEPCTL0_USBAEP | OTG_DIEPCTL0_CNAK |
		    OTG_DOEPCTLX_SD0PID | (type << 18) | max_size;

		if (callback) {
			usbd_dev->user_callback_ctr[addr][USB_TRANSACTION_OUT] =
			    (void *)callback;
		}
	}
}

void dwc_endpoints_reset(usbd_device *usbd_dev)
{
	int i;
	/* The core resets the endpoints automatically on reset. */
	usbd_dev->fifo_mem_top = usbd_dev->fifo_mem_top_ep0;

	/* Disable any currently active endpoints */
	for (i = 1; i < USB_ENDPOINT_COUNT; i++) {
		if (REBASE(OTG_DOEPCTL(i)) & OTG_DOEPCTL0_EPENA) {
			REBASE(OTG_DOEPCTL(i)) |= OTG_DOEPCTL0_EPDIS;
		}
		if (REBASE(OTG_DIEPCTL(i)) & OTG_DIEPCTL0_EPENA) {
			REBASE(OTG_DIEPCTL(i)) |= OTG_DIEPCTL0_EPDIS;
		}
	}

	/* Flush all tx/rx fifos */
	REBASE(OTG_GRSTCTL) = OTG_GRSTCTL_TXFFLSH | OTG_GRSTCTL_TXFNUM_ALL
			      | OTG_GRSTCTL_RXFFLSH;
}

void dwc_ep_stall_set(usbd_device *usbd_dev, uint8_t addr, uint8_t stall)
{
	if (addr == 0) {
		if (stall) {
			REBASE(OTG_DIEPCTL(addr)) |= OTG_DIEPCTL0_STALL;
		} else {
			REBASE(OTG_DIEPCTL(addr)) &= ~OTG_DIEPCTL0_STALL;
		}
	}

	if (addr & 0x80) {
		addr &= 0x7F;

		if (stall) {
			REBASE(OTG_DIEPCTL(addr)) |= OTG_DIEPCTL0_STALL;
		} else {
			REBASE(OTG_DIEPCTL(addr)) &= ~OTG_DIEPCTL0_STALL;
			REBASE(OTG_DIEPCTL(addr)) |= OTG_DIEPCTLX_SD0PID;
		}
	} else {
		if (stall) {
			REBASE(OTG_DOEPCTL(addr)) |= OTG_DOEPCTL0_STALL;
		} else {
			REBASE(OTG_DOEPCTL(addr)) &= ~OTG_DOEPCTL0_STALL;
			REBASE(OTG_DOEPCTL(addr)) |= OTG_DOEPCTLX_SD0PID;
		}
	}
}

uint8_t dwc_ep_stall_get(usbd_device *usbd_dev, uint8_t addr)
{
	/* Return non-zero if STALL set. */
	if (addr & 0x80) {
		return (REBASE(OTG_DIEPCTL(addr & 0x7f)) &
				OTG_DIEPCTL0_STALL) ? 1 : 0;
	} else {
		return (REBASE(OTG_DOEPCTL(addr)) &
				OTG_DOEPCTL0_STALL) ? 1 : 0;
	}
}

void dwc_ep_nak_set(usbd_device *usbd_dev, uint8_t addr, uint8_t nak)
{
	/* It does not make sense to force NAK on IN endpoints. */
	if (addr & 0x80) {
		return;
	}

	usbd_dev->force_nak[addr] = nak;

	if (nak) {
		REBASE(OTG_DOEPCTL(addr)) |= OTG_DOEPCTL0_SNAK;
	} else {
		REBASE(OTG_DOEPCTL(addr)) |= OTG_DOEPCTL0_CNAK;
	}
}

uint16_t dwc_ep_write_packet(usbd_device *usbd_dev, uint8_t addr,
			      const void *buf, uint16_t len)
{
	const uint32_t *buf32 = buf;
#if defined(__ARM_ARCH_6M__)
	const uint8_t *buf8 = buf;
	uint32_t word32;
#endif /* defined(__ARM_ARCH_6M__) */
	int i;

	addr &= 0x7F;

	/* Return if endpoint is already enabled. */
	if (REBASE(OTG_DIEPCTL(addr)) & OTG_DIEPCTL0_EPENA)
		return 0xffff;
	if (REBASE(OTG_DTXFSTS(addr) & 0xffff) < (unsigned)((len + 3) >> 2))
		return 0xffff;

	/* Enable endpoint for transmission. */
	REBASE(OTG_DIEPTSIZ(addr)) = OTG_DIEPSIZ0_PKTCNT | len;

	/* WARNING: it is not explicitly stated in the ST documentation,
	 * but the usb core fifo memory read/write accesses should not
	 * be interrupted by usb core fifo memory write/read accesses.
	 *
	 * For example, this function can be run from within the usb interrupt
	 * context, and also from outside of the usb interrupt context.
	 * When this function executes outside of the usb interrupt context,
	 * if it gets interrupted by the usb interrupt while it writes to
	 * the usb core fifo memory, and from within the usb
	 * interrupt the usb core fifo memory is read, then when control returns
	 * to this function, the usb core fifo memory write accesses can not
	 * simply continue, this will result in a transaction error on the
	 * usb line.
	 *
	 * In order to avoid such situation, the usb interrupt is masked here
	 * prior to writing the data to the usb core fifo memory.
	 */
	uint32_t saved_interrupt_mask = REBASE(OTG_GINTMSK);
	REBASE(OTG_GINTMSK) = 0;
	REBASE(OTG_DIEPCTL(addr)) |= OTG_DIEPCTL0_EPENA |
				     OTG_DIEPCTL0_CNAK;

	/* Copy buffer to endpoint FIFO, note - memcpy does not work.
	 * ARMv7M supports non-word-aligned accesses, ARMv6M does not. */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
	for (i = len; i > 0; i -= 4) {
		REBASE(OTG_FIFO(addr)) = *buf32++;
	}
#endif /* defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) */

#if defined(__ARM_ARCH_6M__)
	/* Take care of word-aligned and non-word-aligned buffers */
	if (((uint32_t)buf8 & 0x3) == 0) {
		for (i = len; i > 0; i -= 4) {
			REBASE(OTG_FIFO(addr)) = *buf32++;
		}
	} else {
		for (i = len; i > 0; i -= 4) {
			memcpy(&word32, buf8, 4);
			REBASE(OTG_FIFO(addr)) = word32;
			buf8 += 4;
		}
	}
#endif /* defined(__ARM_ARCH_6M__) */

	REBASE(OTG_GINTMSK) = saved_interrupt_mask;
	return len;
}

uint16_t dwc_ep_read_packet(usbd_device *usbd_dev, uint8_t addr,
				  void *buf, uint16_t len)
{
	(void) usbd_dev;
	struct incoming_packet * packet = stashed_packets + addr;
	if (!packet->is_packet_present)
		return 0;
	len = MIN(len, packet->packet_length);
	packet->is_packet_present = false;
	memcpy(buf, packet->packet_data, len);
	return len;
}

/* TODO: this does not currently handle the case for __ARM_ARCH_6M__ */
void dwc_ep_read_packet_internal(usbd_device *usbd_dev, int ep)
{
	int i;
	struct incoming_packet * packet = stashed_packets + ep;
	uint32_t *buf32 = (uint32_t *) packet->packet_data;
	uint32_t extra;
	uint16_t len = sizeof packet->packet_data;

	len = MIN(len, usbd_dev->rxbcnt);

	/* ARMv7M supports non-word-aligned accesses, ARMv6M does not. */
	for (i = len; i >= 4; i -= 4) {
		*buf32++ = REBASE(OTG_FIFO(0));
		usbd_dev->rxbcnt -= 4;
	}

	if (i) {
		extra = REBASE(OTG_FIFO(0));
		/* we read 4 bytes from the fifo, so update rxbcnt */
		if (usbd_dev->rxbcnt < 4) {
			/* Be careful not to underflow (rxbcnt is unsigned) */
			usbd_dev->rxbcnt = 0;
		} else {
			usbd_dev->rxbcnt -= 4;
		}
		memcpy(buf32, &extra, i);
	}
	packet->is_packet_present = true;

	packet->packet_length = len;
}

void dwc_poll(usbd_device *usbd_dev)
{
	/* Read interrupt status register. */
	uint32_t intsts = REBASE(OTG_GINTSTS);
	if (!(intsts & REBASE(OTG_GINTMSK)))
		/* No interrupts to handle - can happen if this function is
		 * not invoked from the usb interrupt handler. */
		 return;
	int i;

	if (intsts & OTG_GINTSTS_ENUMDNE) {
		/* Handle USB RESET condition. */
		REBASE(OTG_GINTSTS) = OTG_GINTSTS_ENUMDNE;
		usbd_dev->fifo_mem_top = usbd_dev->driver->rx_fifo_size;
		_usbd_reset(usbd_dev);
		return;
	}

	/* Handle IN endpoint interrupt requests. */
	if (intsts & OTG_GINTSTS_IEPINT)
	{
		for (i = 0; i < USB_ENDPOINT_COUNT; i++) { /* Iterate over endpoints. */
			if (REBASE(OTG_DIEPINT(i)) & OTG_DIEPINTX_XFRC) {
				/* Transfer complete. */
				REBASE(OTG_DIEPINT(i)) = OTG_DIEPINTX_XFRC;
				if (usbd_dev->user_callback_ctr[i]
						[USB_TRANSACTION_IN]) {
					usbd_dev->user_callback_ctr[i]
						[USB_TRANSACTION_IN](usbd_dev, i);
				}
			}
		}
	}

	if (intsts & OTG_GINTSTS_RXFLVL) {
		/* Receive FIFO non-empty. */
		uint32_t rxstsp = REBASE(OTG_GRXSTSP);
		uint32_t pktsts = rxstsp & OTG_GRXSTSP_PKTSTS_MASK;
		uint8_t ep = rxstsp & OTG_GRXSTSP_EPNUM_MASK;

		/* Save packet size for dwc_ep_read_packet(). */
		usbd_dev->rxbcnt = (rxstsp & OTG_GRXSTSP_BCNT_MASK) >> 4;
		struct incoming_packet * packet = stashed_packets + ep;

		if (pktsts == OTG_GRXSTSP_PKTSTS_OUT)
		{
			if (usbd_dev->rxbcnt)
				dwc_ep_read_packet_internal(usbd_dev, ep);
			else
				packet->is_packet_present = true, packet->packet_length = 0;
		}
		else if (pktsts == OTG_GRXSTSP_PKTSTS_SETUP)
		{
			if (usbd_dev->rxbcnt)
				dwc_ep_read_packet_internal(usbd_dev, ep);
			else
				packet->is_packet_present = true, packet->packet_length = 0;
		}
	}

	/* Handle OUT endpoint interrupt requests. */
	if (intsts & OTG_GINTSTS_OEPINT)
	{
		uint32_t daint = REBASE(OTG_DAINT);
		int epnum;
		for (epnum = 0; epnum < USB_ENDPOINT_COUNT; epnum ++)
			if (daint & (1 << (16 + epnum)))
			{
				uint32_t t = REBASE(OTG_DOEPINT(epnum));
				REBASE(OTG_DOEPINT(epnum)) = t;

				if (t & OTG_DOEPINTX_XFRC)
				{
					REBASE(OTG_DOEPINT(epnum)) = OTG_DOEPINTX_XFRC;
					if (usbd_dev->user_callback_ctr[epnum][USB_TRANSACTION_OUT]) {
						usbd_dev->user_callback_ctr[epnum][USB_TRANSACTION_OUT] (usbd_dev, epnum);
					}
					REBASE(OTG_DOEPTSIZ(epnum)) = usbd_dev->doeptsiz[epnum];
					REBASE(OTG_DOEPCTL(epnum)) |= OTG_DOEPCTL0_EPENA |
						(usbd_dev->force_nak[epnum] ?
						 OTG_DOEPCTL0_SNAK : OTG_DOEPCTL0_CNAK);
				}
				if (t & OTG_DOEPINTX_STUP)
				{
					/* Special case for control endpoints - reception of OUT packets is
					 * always enabled. */
					REBASE(OTG_DOEPINT(epnum)) = OTG_DOEPINTX_STUP;
					if (usbd_dev->user_callback_ctr[epnum][USB_TRANSACTION_SETUP]) {
						usbd_dev->user_callback_ctr[epnum][USB_TRANSACTION_SETUP] (usbd_dev, epnum);
					}
					REBASE(OTG_DOEPTSIZ(epnum)) = usbd_dev->doeptsiz[epnum];
					REBASE(OTG_DOEPCTL(epnum)) |= OTG_DOEPCTL0_EPENA |

						(usbd_dev->force_nak[epnum] ?
						 OTG_DOEPCTL0_SNAK : OTG_DOEPCTL0_CNAK);
				}
				if (t & OTG_DOEPINTX_OTEPDIS)
					REBASE(OTG_DOEPINT(epnum)) = OTG_DOEPINTX_OTEPDIS;
			}
	}

	if (intsts & OTG_GINTSTS_USBSUSP) {
		if (usbd_dev->user_callback_suspend) {
			usbd_dev->user_callback_suspend();
		}
		REBASE(OTG_GINTSTS) = OTG_GINTSTS_USBSUSP;
	}

	if (intsts & OTG_GINTSTS_WKUPINT) {
		if (usbd_dev->user_callback_resume) {
			usbd_dev->user_callback_resume();
		}
		REBASE(OTG_GINTSTS) = OTG_GINTSTS_WKUPINT;
	}

	if (intsts & OTG_GINTSTS_SOF) {
		if (usbd_dev->user_callback_sof) {
			usbd_dev->user_callback_sof();
		}
		REBASE(OTG_GINTSTS) = OTG_GINTSTS_SOF;
	}

	if (usbd_dev->user_callback_sof) {
		REBASE(OTG_GINTMSK) |= OTG_GINTMSK_SOFM;
	} else {
		REBASE(OTG_GINTMSK) &= ~OTG_GINTMSK_SOFM;
	}
}

void dwc_disconnect(usbd_device *usbd_dev, bool disconnected)
{
	if (disconnected) {
		REBASE(OTG_DCTL) |= OTG_DCTL_SDIS;
	} else {
		REBASE(OTG_DCTL) &= ~OTG_DCTL_SDIS;
	}
}


































/** @defgroup usb_control_file Generic USB Control Requests

@ingroup USB

@brief <b>Generic USB Control Requests</b>

@version 1.0.0

@author @htmlonly &copy; @endhtmlonly 2010
Gareth McMullin <gareth@blacksphere.co.nz>

@date 10 March 2013

LGPL License Terms @ref lgpl_license
*/

/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

/**@{*/

#include <stdlib.h>
#include <libopencm3/usb/usbd.h>
#include "usb_private.h"

/*
 * According to the USB 2.0 specification, section 8.5.3, when a control
 * transfer is stalled, the pipe becomes idle. We provide one utility to stall
 * a transaction to reduce boilerplate code.
 */
static void stall_transaction(usbd_device *usbd_dev)
{
	usbd_ep_stall_set(usbd_dev, 0, 1);
	usbd_dev->control_state.state = IDLE;
}

/**
 * If we're replying with _some_ data, but less than the host is expecting,
 * then we normally just do a short transfer.  But if it's short, but a
 * multiple of the endpoint max packet size, we need an explicit ZLP.
 * @param len how much data we want to transfer
 * @param wLength how much the host asked for
 * @param ep_size
 * @return
 */
static bool needs_zlp(uint16_t len, uint16_t wLength, uint8_t ep_size)
{
	if (len < wLength) {
		if (len && (len % ep_size == 0)) {
			return true;
		}
	}
	return false;
}

/* Register application callback function for handling USB control requests. */
int usbd_register_control_callback(usbd_device *usbd_dev, uint8_t type,
				   uint8_t type_mask,
				   usbd_control_callback callback)
{
	int i;

	for (i = 0; i < MAX_USER_CONTROL_CALLBACK; i++) {
		if (usbd_dev->user_control_callback[i].cb) {
			continue;
		}

		usbd_dev->user_control_callback[i].type = type;
		usbd_dev->user_control_callback[i].type_mask = type_mask;
		usbd_dev->user_control_callback[i].cb = callback;
		return 0;
	}

	return -1;
}

static void usb_control_send_chunk(usbd_device *usbd_dev)
{
	if (usbd_dev->desc->bMaxPacketSize0 <
			usbd_dev->control_state.ctrl_len) {
		/* Data stage, normal transmission */
		usbd_ep_write_packet(usbd_dev, 0,
				     usbd_dev->control_state.ctrl_buf,
				     usbd_dev->desc->bMaxPacketSize0);
		usbd_dev->control_state.state = DATA_IN;
		usbd_dev->control_state.ctrl_buf +=
			usbd_dev->desc->bMaxPacketSize0;
		usbd_dev->control_state.ctrl_len -=
			usbd_dev->desc->bMaxPacketSize0;
	} else {
		/* Data stage, end of transmission */
		usbd_ep_write_packet(usbd_dev, 0,
				     usbd_dev->control_state.ctrl_buf,
				     usbd_dev->control_state.ctrl_len);

		usbd_dev->control_state.state =
			usbd_dev->control_state.needs_zlp ?
			DATA_IN : LAST_DATA_IN;
		usbd_dev->control_state.needs_zlp = false;
		usbd_dev->control_state.ctrl_len = 0;
		usbd_dev->control_state.ctrl_buf = NULL;
	}
}

static int usb_control_recv_chunk(usbd_device *usbd_dev)
{
	uint16_t packetsize = MIN(usbd_dev->desc->bMaxPacketSize0,
			usbd_dev->control_state.req.wLength -
			usbd_dev->control_state.ctrl_len);
	uint16_t size = usbd_ep_read_packet(usbd_dev, 0,
				       usbd_dev->control_state.ctrl_buf +
				       usbd_dev->control_state.ctrl_len,
				       packetsize);

	if (size != packetsize) {
		stall_transaction(usbd_dev);
		return -1;
	}

	usbd_dev->control_state.ctrl_len += size;

	return packetsize;
}

static enum usbd_request_return_codes
usb_control_request_dispatch(usbd_device *usbd_dev,
			     struct usb_setup_data *req)
{
	int i, result = 0;
	struct user_control_callback *cb = usbd_dev->user_control_callback;

	/* Call user command hook function. */
	for (i = 0; i < MAX_USER_CONTROL_CALLBACK; i++) {
		if (cb[i].cb == NULL) {
			break;
		}

		if ((req->bmRequestType & cb[i].type_mask) == cb[i].type) {
			result = cb[i].cb(usbd_dev, req,
					  &(usbd_dev->control_state.ctrl_buf),
					  &(usbd_dev->control_state.ctrl_len),
					  &(usbd_dev->control_state.complete));
			if (result == USBD_REQ_HANDLED ||
			    result == USBD_REQ_NOTSUPP) {
				return result;
			}
		}
	}

	/* Try standard request if not already handled. */
	return _usbd_standard_request(usbd_dev, req,
				      &(usbd_dev->control_state.ctrl_buf),
				      &(usbd_dev->control_state.ctrl_len));
}

/* Handle commands and read requests. */
static void usb_control_setup_read(usbd_device *usbd_dev,
		struct usb_setup_data *req)
{
	usbd_dev->control_state.ctrl_buf = usbd_dev->ctrl_buf;
	usbd_dev->control_state.ctrl_len = req->wLength;

	if (usb_control_request_dispatch(usbd_dev, req)) {
		if (req->wLength) {
			usbd_dev->control_state.needs_zlp =
				needs_zlp(usbd_dev->control_state.ctrl_len,
					req->wLength,
					usbd_dev->desc->bMaxPacketSize0);
			/* Go to data out stage if handled. */
			usb_control_send_chunk(usbd_dev);
		} else {
			/* Go to status stage if handled. */
			usbd_ep_write_packet(usbd_dev, 0, NULL, 0);
			usbd_dev->control_state.state = STATUS_IN;
		}
	} else {
		/* Stall endpoint on failure. */
		stall_transaction(usbd_dev);
	}
}

static void usb_control_setup_write(usbd_device *usbd_dev,
				    struct usb_setup_data *req)
{
	if (req->wLength > usbd_dev->ctrl_buf_len) {
		stall_transaction(usbd_dev);
		return;
	}

	/* Buffer into which to write received data. */
	usbd_dev->control_state.ctrl_buf = usbd_dev->ctrl_buf;
	usbd_dev->control_state.ctrl_len = 0;
	/* Wait for DATA OUT stage. */
	if (req->wLength > usbd_dev->desc->bMaxPacketSize0) {
		usbd_dev->control_state.state = DATA_OUT;
	} else {
		usbd_dev->control_state.state = LAST_DATA_OUT;
	}

	usbd_ep_nak_set(usbd_dev, 0, 0);
}

/* Do not appear to belong to the API, so are omitted from docs */
/**@}*/

void _usbd_control_setup(usbd_device *usbd_dev, uint8_t ea)
{
	struct usb_setup_data *req = &usbd_dev->control_state.req;
	(void)ea;

	usbd_dev->control_state.complete = NULL;

	usbd_ep_nak_set(usbd_dev, 0, 1);

	if (usbd_ep_read_packet(usbd_dev, 0, req, 8) != 8) {
		stall_transaction(usbd_dev);
		return;
	}
	if (req->wLength == 0) {
		usb_control_setup_read(usbd_dev, req);
	} else if (req->bmRequestType & 0x80) {
		usb_control_setup_read(usbd_dev, req);
	} else {
		usb_control_setup_write(usbd_dev, req);
	}
}

void _usbd_control_out(usbd_device *usbd_dev, uint8_t ea)
{
	(void)ea;

	switch (usbd_dev->control_state.state) {
	case DATA_OUT:
		if (usb_control_recv_chunk(usbd_dev) < 0) {
			break;
		}
		if ((usbd_dev->control_state.req.wLength -
					usbd_dev->control_state.ctrl_len) <=
					usbd_dev->desc->bMaxPacketSize0) {
			usbd_dev->control_state.state = LAST_DATA_OUT;
		}
		break;
	case LAST_DATA_OUT:
		if (usb_control_recv_chunk(usbd_dev) < 0) {
			break;
		}
		/*
		 * We have now received the full data payload.
		 * Invoke callback to process.
		 */
		if (usb_control_request_dispatch(usbd_dev,
					&(usbd_dev->control_state.req))) {
			/* Go to status stage on success. */
			usbd_ep_write_packet(usbd_dev, 0, NULL, 0);
			usbd_dev->control_state.state = STATUS_IN;
		} else {
			stall_transaction(usbd_dev);
		}
		break;
	case STATUS_OUT:
		usbd_ep_read_packet(usbd_dev, 0, NULL, 0);
		usbd_dev->control_state.state = IDLE;
		if (usbd_dev->control_state.complete) {
			usbd_dev->control_state.complete(usbd_dev,
					&(usbd_dev->control_state.req));
		}
		usbd_dev->control_state.complete = NULL;
		break;
	default:
		stall_transaction(usbd_dev);
	}
}

void _usbd_control_in(usbd_device *usbd_dev, uint8_t ea)
{
	(void)ea;
	struct usb_setup_data *req = &(usbd_dev->control_state.req);

	switch (usbd_dev->control_state.state) {
	case DATA_IN:
		usb_control_send_chunk(usbd_dev);
		break;
	case LAST_DATA_IN:
		usbd_dev->control_state.state = STATUS_OUT;
		usbd_ep_nak_set(usbd_dev, 0, 0);
		break;
	case STATUS_IN:
		if (usbd_dev->control_state.complete) {
			usbd_dev->control_state.complete(usbd_dev,
					&(usbd_dev->control_state.req));
		}

		/* Exception: Handle SET ADDRESS function here... */
		if ((req->bmRequestType == 0) &&
		    (req->bRequest == USB_REQ_SET_ADDRESS)) {
			usbd_dev->driver->set_address(usbd_dev, req->wValue);
		}
		usbd_dev->control_state.state = IDLE;
		break;
	default:
		stall_transaction(usbd_dev);
	}
}


