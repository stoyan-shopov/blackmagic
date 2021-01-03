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

/* This file implements a the USB Communications Device Class - Abstract
 * Control Model (CDC-ACM) as defined in CDC PSTN subclass 1.2.
 * A Device Firmware Upgrade (DFU 1.1) class interface is provided for
 * field firmware upgrade.
 *
 * The device's unique id is used as the USB serial number string.
 */

#include "general.h"
#include "gdb_if.h"
#include "cdcacm.h"
#if defined(PLATFORM_HAS_TRACESWO)
#	include "traceswo.h"
#endif
#include "usbuart.h"
#include "serialno.h"
#include "version.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/usb/dfu.h>
#include <stdlib.h>

usbd_device * usbdev;

#define TRACE_ENDPOINT_SIZE	512

int configured;

static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,		/* Vendor Specific Device */
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	/* The USB specification requires that the control endpoint size for high
	 * speed devices (e.g., stlinkv3) is 64 bytes.
	 * Best to have its size set to 64 bytes in all cases. */
	.bMaxPacketSize0 = 64,
	.idVendor = 0x1D50,
	.idProduct = 0x6020,
	.bcdDevice = 0x0100,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor trace_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x81,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = TRACE_ENDPOINT_SIZE,
	.bInterval = 0,
}};

const struct usb_interface_descriptor trace_iface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = 0xFF,
	.bInterfaceSubClass = 0xFF,
	.bInterfaceProtocol = 0xFF,
	.iInterface = 7,
	.endpoint = trace_endp,
	.extra = 0,
	.extralen = 0,
};


static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = &trace_iface,
}};

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};
static char serial_no[DFU_SERIAL_LENGTH];

#define BOARD_IDENT "Black Magic Probe" PLATFORM_IDENT FIRMWARE_VERSION
#define DFU_IDENT   "Black Magic Firmware Upgrade" PLATFORM_IDENT FIRMWARE_VERSION

static const char *usb_strings[] = {
	"Black Sphere Technologies",
	BOARD_IDENT,
	serial_no,
	"Black Magic GDB Server",
	"Black Magic UART Port",
	DFU_IDENT,
	"Black Magic Trace Capture",
};

int cdcacm_get_config(void)
{
	return configured;
}

int cdcacm_get_dtr(void)
{
	return 0;
}

static void cdcacm_set_config(usbd_device *dev, uint16_t wValue)
{
	configured = wValue;

	/* Trace interface */
	//usbd_ep_setup(dev, 0x81, USB_ENDPOINT_ATTR_BULK, TRACE_ENDPOINT_SIZE, trace_buf_drain);
	usbd_ep_setup(dev, 0x81, USB_ENDPOINT_ATTR_BULK, TRACE_ENDPOINT_SIZE, 0);
}

/* We need a special large control buffer for this device: */
uint8_t usbd_control_buffer[256];

void cdcacm_init(void)
{
	void exti15_10_isr(void);

	serial_no_read(serial_no);

	usbdev = usbd_init(&USB_DRIVER, &dev, &config, usb_strings,
			    sizeof(usb_strings)/sizeof(char *),
			    usbd_control_buffer, sizeof(usbd_control_buffer));

	usbd_register_set_config_callback(usbdev, cdcacm_set_config);

	nvic_set_priority(USB_IRQ, IRQ_PRI_USB);
	nvic_enable_irq(USB_IRQ);
}

void USB_ISR(void)
{
	usbd_poll(usbdev);
}
