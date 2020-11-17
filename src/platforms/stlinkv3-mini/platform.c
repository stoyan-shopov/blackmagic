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
#include <libopencm3/stm32/spi.h>

uint16_t led_idle_run;
uint16_t srst_pin;
static uint32_t rev;

int platform_hwversion(void)
{
	return rev;
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

void platform_request_boot(void)
{
	/* Use top of ITCM RAM as magic marker*/
	volatile uint32_t *magic = (volatile uint32_t *) 0x3ff8;
	magic[0] = BOOTMAGIC0;
	magic[1] = BOOTMAGIC1;
	scb_reset_system();
}

void platform_init(void)
{

	rcc_periph_clock_enable(RCC_APB2ENR_SYSCFGEN);
	rcc_clock_setup_hse(rcc_3v3 + RCC_CLOCK_3V3_216MHZ, 25);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOH);
	rcc_periph_clock_enable(RCC_GPIOF);

	/* Configure spi pins - used for swd bus driving. */
	rcc_periph_clock_enable(RCC_SPI5);
	gpio_mode_setup(STLINKV3_MINI_SPI_MOSI_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, STLINKV3_MINI_SPI_MOSI_PIN);
	gpio_set_output_options(STLINKV3_MINI_SPI_MOSI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, STLINKV3_MINI_SPI_MOSI_PIN);
	gpio_set_af(STLINKV3_MINI_SPI_MOSI_PORT, STLINKV3_MINI_SPI_AF_NUMBER, STLINKV3_MINI_SPI_MOSI_PIN);

	gpio_mode_setup(STLINKV3_MINI_SPI_MISO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, STLINKV3_MINI_SPI_MISO_PIN);
	gpio_set_af(STLINKV3_MINI_SPI_MISO_PORT, STLINKV3_MINI_SPI_AF_NUMBER, STLINKV3_MINI_SPI_MISO_PIN);

	gpio_mode_setup(STLINKV3_MINI_SPI_SCK_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, STLINKV3_MINI_SPI_SCK_PIN);
	gpio_set_output_options(STLINKV3_MINI_SPI_SCK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, STLINKV3_MINI_SPI_SCK_PIN);
	gpio_set_af(STLINKV3_MINI_SPI_SCK_PORT, STLINKV3_MINI_SPI_AF_NUMBER, STLINKV3_MINI_SPI_SCK_PIN);

	/* Set spi port. */
	spi_init_master(STLINKV3_MINI_SPI, SPI_CR1_BAUDRATE_FPCLK_DIV_8, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
			SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_LSBFIRST);
	SPI_CR2(STLINKV3_MINI_SPI) |= /* FRXTH */ 1 << 12;

	/* By default, drive the swd bus by gpio bitbanging. */
	spi_disable(STLINKV3_MINI_SPI);

	/* Float the swdio pin. */
	gpio_mode_setup(STLINKV3_MINI_SPI_MOSI_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, STLINKV3_MINI_SPI_MOSI_PIN);
	gpio_set_af(STLINKV3_MINI_SPI_MOSI_PORT, STLINKV3_MINI_SPI_AF_NUMBER, STLINKV3_MINI_SPI_MOSI_PIN);

	/* Drive the swck pin low. */
	gpio_mode_setup(STLINKV3_MINI_SPI_SCK_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, STLINKV3_MINI_SPI_SCK_PIN);
	gpio_set_output_options(STLINKV3_MINI_SPI_SCK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, STLINKV3_MINI_SPI_SCK_PIN);
	gpio_set_af(STLINKV3_MINI_SPI_SCK_PORT, STLINKV3_MINI_SPI_AF_NUMBER, STLINKV3_MINI_SPI_SCK_PIN);

	gpio_clear(STLINKV3_MINI_SPI_SCK_PORT, STLINKV3_MINI_SPI_SCK_PIN); 

	platform_timing_init();
	cdcacm_init();
	usbuart_init();
}
