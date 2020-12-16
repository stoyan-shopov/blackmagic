/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2011  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This file implements a transparent channel over which the GDB Remote
 * Serial Debugging protocol is implemented.  This implementation for STM32
 * uses the USB CDC-ACM device bulk endpoints to implement the channel.
 */
#include "general.h"
#include "cdcacm.h"
#include "gdb_if.h"

static uint32_t count_out;
static uint32_t count_in;
static uint32_t out_ptr;
static uint8_t buffer_out[CDCACM_PACKET_SIZE];
/*
 * When receiving data packets that are larger than the usb IN endpoint
 * size, the usb host expects that the large data packet is split into
 * smaller data packets, of length equal to the usb IN endpoint size, and
 * only the last packet must be of length strictly smaller than the usb
 * IN endpoint size. In case that the large data packet is an integral
 * multiple of the usb IN endpoint size, the last packet sent to the host
 * must be of length zero, so that the usb host recognizes the completion
 * of the transfer of the large data packet. Otherwise, some usb hosts
 * may wait indefinitely waiting for a short packet.
 *
 * Currently, the libopencm3 'usbd_ep_write_packet()' function cannot
 * report a successful write of a zero-length packet - it
 * returns the number of bytes written, on success, and 0 on failure.
 * This makes it impossible to check for a successful write of a
 * zero-length packet.
 *
 * So, the code here used to perform a workaround, by sending a packet
 * of 1 zero byte, instead of a zero-length packet. Generally, this is
 * not correct for generic usb cdcacm devices, but in this particular
 * case this does not cause problems, because the gdb communication is
 * packetized, and these 1 byte, zero data packets, are silently ignored
 * by gdb.
 *
 * To simplify the code, set the usb IN buffer for the gdb cdcacm interface to
 * be less than the advertised usb IN endpoint size. This makes sure
 * that packets of size equal to the usb IN endpoint size will never
 * be sent to the usb host, and therefore sending zero-length packets
 * will never be needed.  */
static uint8_t buffer_in[CDCACM_PACKET_SIZE - 1];
#ifdef STM32F4
static volatile uint32_t count_new;
static uint8_t double_buffer_out[CDCACM_PACKET_SIZE];
#endif

void gdb_if_putchar(unsigned char c, int flush)
{
	buffer_in[count_in++] = c;
	if(flush || (count_in == sizeof buffer_in)) {
		/* Refuse to send if USB isn't configured, and
		 * don't bother if nobody's listening */
		if((cdcacm_get_config() != 1) || !cdcacm_get_dtr()) {
			count_in = 0;
			return;
		}
		while(usbd_ep_write_packet(usbdev, CDCACM_GDB_ENDPOINT,
			buffer_in, count_in) <= 0);

		}

		count_in = 0;
	}
}

#ifdef STM32F4
void gdb_usb_out_cb(usbd_device *dev, uint8_t ep)
{
	(void)ep;
	usbd_ep_nak_set(dev, CDCACM_GDB_ENDPOINT, 1);
	count_new = usbd_ep_read_packet(dev, CDCACM_GDB_ENDPOINT,
	                                double_buffer_out, CDCACM_PACKET_SIZE);
	if(!count_new) {
		usbd_ep_nak_set(dev, CDCACM_GDB_ENDPOINT, 0);
	}
}
#endif

static void gdb_if_update_buf(void)
{
	while (cdcacm_get_config() != 1);
#ifdef STM32F4
	asm volatile ("cpsid i; isb");
	if (count_new) {
		memcpy(buffer_out, double_buffer_out, count_new);
		count_out = count_new;
		count_new = 0;
		out_ptr = 0;
		usbd_ep_nak_set(usbdev, CDCACM_GDB_ENDPOINT, 0);
	}
	asm volatile ("cpsie i; isb");
#else
	count_out = usbd_ep_read_packet(usbdev, CDCACM_GDB_ENDPOINT,
	                                buffer_out, CDCACM_PACKET_SIZE);
	out_ptr = 0;
#endif
}

unsigned char gdb_if_getchar(void)
{

	while (!(out_ptr < count_out)) {
		/* Detach if port closed */
		if (!cdcacm_get_dtr())
			return 0x04;

		gdb_if_update_buf();
	}

	return buffer_out[out_ptr++];
}

unsigned char gdb_if_getchar_to(int timeout)
{
	platform_timeout t;
	platform_timeout_set(&t, timeout);

	if (!(out_ptr < count_out)) do {
		/* Detach if port closed */
		if (!cdcacm_get_dtr())
			return 0x04;

		gdb_if_update_buf();
	} while (!platform_timeout_is_expired(&t) && !(out_ptr < count_out));

	if(out_ptr < count_out)
		return gdb_if_getchar();

	return -1;
}

