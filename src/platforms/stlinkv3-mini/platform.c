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

/* This file implements the platform specific functions for the ST-Link
 * implementation.
 */

#include "general.h"
#include "cdcacm.h"
#include "usbuart.h"
#include "gdb_if.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/scs.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/stm32/adc.h>

uint16_t led_idle_run;
uint16_t srst_pin;
static uint32_t rev;

int platform_hwversion(void)
{
	return rev;
}

void platform_init(void)
{

	rcc_periph_clock_enable(RCC_APB2ENR_SYSCFGEN);
	rcc_clock_setup_hse(rcc_3v3 + RCC_CLOCK_3V3_216MHZ, 25000000);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOH);

	/* Configure swd pins */
	rcc_periph_clock_enable(RCC_GPIOF);
	gpio_mode_setup(SWDIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SWDIO_PIN);
	gpio_set_output_options(SWDIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, SWDIO_PIN);
	//gpio_set_af(GPIOB, GPIO_AF12, GPIO14 | GPIO15);

	gpio_mode_setup(SWCLK_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SWCLK_PIN);
	gpio_set_output_options(SWCLK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, SWCLK_PIN);

	/* Configure and set the TPWR_EN pin. */
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0);
	gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO0);
	GPIOB_BSRR = 1 << 0;

	platform_timing_init();
	cdcacm_init();
}

void platform_srst_set_val(bool assert)
{
	// TODO: fill this in
	(void)assert;
}

bool platform_srst_get_val()
{
	// TODO: fill this in
	return 0;
}

const char *platform_target_voltage(void)
{
	// TODO: fill this in
	static char ret[] = "3.23V";

	return ret;
}

void usbuart_set_line_coding(struct usb_cdc_line_coding *coding)
{
	(void)coding;
}

void platform_request_boot(void)
{
	while (1);
}

void usbuart_usb_in_cb(usbd_device *dev, uint8_t ep)
{
	(void) dev;
	(void) ep;
}

void usbuart_usb_out_cb(usbd_device *dev, uint8_t ep)
{
	(void)ep;
	(void)dev;
}


#if RUN_SFORTH == 1
#include "engine.h"
#include "sf-arch.h"

#include "sf-word-wizard.h"

static void do_stlinkv3_mini_help(void)
{ print_str("stlinkv3-mini support for the blackmagic debug probe\n\n"); }

static void do_help(void)
{ print_str("TODO: help is not available at this moment; please, provide help here\n"); }

static struct word dict_base_dummy_word[1] = { MKWORD(0, 0, "", 0), };
static const struct word custom_dict[] = {
	MKWORD(dict_base_dummy_word,	0,		"stlinkv3-mini-help",	do_stlinkv3_mini_help),
	MKWORD(custom_dict,		__COUNTER__,	"help",	do_help),

}, * custom_dict_start = custom_dict + __COUNTER__;

static void sf_stlinkv3_mini_dictionary(void) __attribute__((constructor));
static void sf_stlinkv3_mini_dictionary(void)
{
	sf_merge_custom_dictionary(dict_base_dummy_word, custom_dict_start);
}

int sfgetc(void)
{
	return gdb_if_getchar();
}
int sfputc(int c)
{
	gdb_if_putchar(c, c == '\n' || c == '\r');
	return 1;
}

#endif
