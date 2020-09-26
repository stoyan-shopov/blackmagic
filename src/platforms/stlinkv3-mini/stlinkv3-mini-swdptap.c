/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2011  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 * 2020 Stoyan Shopov
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

/* This file implements the SW-DP interface. */

/* TODO: currently, this code needs much cleaning. Here are some notes:
 * - the libopencm3 functions for configuring mcu pins (gpio_mode_setup(),
 * gpio_set_output_options()) are very slow, they need replacement with
 * more straightforward code
 * - the serial clock (SWCK) on the SWD bus needs better handling now
 * that the hardware spi is being used instead of bitbanging
 */

#include <libopencm3/stm32/spi.h>

#include "general.h"
#include "swdptap.h"


enum {
	SWDIO_STATUS_FLOAT = 0,
	SWDIO_STATUS_DRIVE
};
static void swdptap_turnaround(int dir) __attribute__ ((optimize(3)));
static uint32_t swdptap_seq_in(int ticks) __attribute__ ((optimize(3)));
static bool swdptap_seq_in_parity(uint32_t *ret, int ticks)
	__attribute__ ((optimize(3)));
static void swdptap_seq_out(uint32_t MS, int ticks)
	__attribute__ ((optimize(3)));
static void swdptap_seq_out_parity(uint32_t MS, int ticks)
	__attribute__ ((optimize(3)));

static int olddir = SWDIO_STATUS_FLOAT;

static void set_swdio_direction_to_current_direction(void)
{
	if (olddir == SWDIO_STATUS_FLOAT)
		SWDIO_MODE_FLOAT();
	else
		SWDIO_MODE_DRIVE();
}

static void do_gpio_to_spi(unsigned clock_phase, unsigned clock_polarity)
{
	(clock_phase ? spi_set_clock_phase_1 : spi_set_clock_phase_0)(STLINKV3_MINI_SPI);
	(clock_polarity ? spi_set_clock_polarity_1 : spi_set_clock_polarity_0)(STLINKV3_MINI_SPI);

	if (olddir == SWDIO_STATUS_DRIVE)
	{
		gpio_mode_setup(STLINKV3_MINI_SPI_MOSI_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, STLINKV3_MINI_SPI_MOSI_PIN);
		gpio_set_output_options(STLINKV3_MINI_SPI_MOSI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, STLINKV3_MINI_SPI_MOSI_PIN);
		gpio_set_af(STLINKV3_MINI_SPI_MOSI_PORT, STLINKV3_MINI_SPI_AF_NUMBER, STLINKV3_MINI_SPI_MOSI_PIN);
	}
	else
		SWDIO_MODE_FLOAT();

	gpio_set_af(STLINKV3_MINI_SPI_SCK_PORT, STLINKV3_MINI_SPI_AF_NUMBER, STLINKV3_MINI_SPI_SCK_PIN);
	gpio_mode_setup(STLINKV3_MINI_SPI_SCK_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, STLINKV3_MINI_SPI_SCK_PIN);
	gpio_set_output_options(STLINKV3_MINI_SPI_SCK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, STLINKV3_MINI_SPI_SCK_PIN);

	spi_enable(STLINKV3_MINI_SPI);
}

static void do_spi_to_gpio(void)
{
	spi_disable(STLINKV3_MINI_SPI);

	set_swdio_direction_to_current_direction();

	gpio_mode_setup(STLINKV3_MINI_SPI_SCK_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, STLINKV3_MINI_SPI_SCK_PIN);
	gpio_set_output_options(STLINKV3_MINI_SPI_SCK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, STLINKV3_MINI_SPI_SCK_PIN);
}

static bool is_turnaround_neeed(int dir) __attribute__ ((optimize(3)));
static bool is_turnaround_neeed(int dir)
{
	return dir != olddir;
}

static void swdptap_turnaround(int dir)
{
	/* Don't turnaround if direction not changing */
	if(dir == olddir) return;
	olddir = dir;

#ifdef DEBUG_SWD_BITS
	DEBUG("%s", dir ? "\n-> ":"\n<- ");
#endif

	if(dir == SWDIO_STATUS_FLOAT)
		SWDIO_MODE_FLOAT();
	gpio_set(SWCLK_PORT, SWCLK_PIN);
	gpio_set(SWCLK_PORT, SWCLK_PIN);
	gpio_set(SWCLK_PORT, SWCLK_PIN);
	gpio_set(SWCLK_PORT, SWCLK_PIN);
	gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
	if(dir == SWDIO_STATUS_DRIVE)
		SWDIO_MODE_DRIVE();
}

static uint32_t swdptap_seq_in(int ticks)
{
	uint32_t index = 1;
	uint32_t ret = 0;
	int len = ticks;

	swdptap_turnaround(SWDIO_STATUS_FLOAT);
	while (len--) {
		int res;
		res = gpio_get(SWDIO_PORT, SWDIO_PIN);
		gpio_set(SWCLK_PORT, SWCLK_PIN);
		gpio_set(SWCLK_PORT, SWCLK_PIN);
		gpio_set(SWCLK_PORT, SWCLK_PIN);
		gpio_set(SWCLK_PORT, SWCLK_PIN);
		if (res)
			ret |= index;
		index <<= 1;
		gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
	}

#ifdef DEBUG_SWD_BITS
	for (int i = 0; i < len; i++)
		DEBUG("%d", (ret & (1 << i)) ? 1 : 0);
#endif
	return ret;
}

static bool swdptap_seq_in_parity(uint32_t *ret, int ticks)
{
	uint32_t index = 1;
	uint32_t res = 0;
	bool bit;
	int len = ticks;

	swdptap_turnaround(SWDIO_STATUS_FLOAT);
//PULSE_SCOPE_TRIGGER();
	if (ticks == 32 && 1)
	{
//gpio_clear(SWCLK_PORT, SWCLK_PIN);
		uint32_t lsb = (gpio_get(SWDIO_PORT, SWDIO_PIN) ? 1 : 0);
		do_gpio_to_spi(1, 0);
		spi_set_data_size(STLINKV3_MINI_SPI, SPI_CR2_DS_15BIT);
		res = spi_xfer(STLINKV3_MINI_SPI, 0);
		res &= 0x7fff;
		res <<= 1;
		res |= lsb;
		spi_set_data_size(STLINKV3_MINI_SPI, SPI_CR2_DS_16BIT);
		res |= spi_xfer(STLINKV3_MINI_SPI, 0) << 16;
		do_spi_to_gpio();
// TODO: clean this.		
#if 1
PULSE_SCOPE_TRIGGER();
gpio_set(SWCLK_PORT, SWCLK_PIN);
gpio_set(SWCLK_PORT, SWCLK_PIN);
gpio_set(SWCLK_PORT, SWCLK_PIN);
gpio_set(SWCLK_PORT, SWCLK_PIN);
gpio_set(SWCLK_PORT, SWCLK_PIN);
gpio_set(SWCLK_PORT, SWCLK_PIN);
gpio_set(SWCLK_PORT, SWCLK_PIN);
gpio_set(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
#endif

	}
	else
	{
		while (len--) {
			bit = gpio_get(SWDIO_PORT, SWDIO_PIN);
			gpio_set(SWCLK_PORT, SWCLK_PIN);
			gpio_set(SWCLK_PORT, SWCLK_PIN);
			gpio_set(SWCLK_PORT, SWCLK_PIN);
			if (bit)
				res |= index;
			index <<= 1;
			gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
		}
	}
	int parity = __builtin_popcount(res);
	bit = gpio_get(SWDIO_PORT, SWDIO_PIN);
	gpio_set(SWCLK_PORT, SWCLK_PIN);
	if (bit)
		parity++;
	else
		gpio_set(SWCLK_PORT, SWCLK_PIN);
	gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
#ifdef DEBUG_SWD_BITS
	for (int i = 0; i < len; i++)
		DEBUG("%d", (res & (1 << i)) ? 1 : 0);
#endif
	*ret = res;
	return (parity & 1);
}

static void swdptap_seq_out(uint32_t MS, int ticks)
{
#ifdef DEBUG_SWD_BITS
	for (int i = 0; i < ticks; i++)
		DEBUG("%d", (MS & (1 << i)) ? 1 : 0);
#endif
	swdptap_turnaround(SWDIO_STATUS_DRIVE);
	gpio_set_val(SWDIO_PORT, SWDIO_PIN, MS & 1);
	while (ticks--) {
		gpio_set(SWCLK_PORT, SWCLK_PIN);
		MS >>= 1;
		gpio_set_val(SWDIO_PORT, SWDIO_PIN, MS & 1);
		gpio_clear(SWCLK_PORT, SWCLK_PIN);
		gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
	}
}

static void swdptap_seq_out_parity(uint32_t MS, int ticks)
{
	int parity = __builtin_popcount(MS);
#ifdef DEBUG_SWD_BITS
	for (int i = 0; i < ticks; i++)
		DEBUG("%d", (MS & (1 << i)) ? 1 : 0);
#endif
	swdptap_turnaround(SWDIO_STATUS_DRIVE);
	if (ticks == 32)
	{
		do_gpio_to_spi(0, 0);
		spi_set_data_size(STLINKV3_MINI_SPI, SPI_CR2_DS_16BIT);
		spi_xfer(STLINKV3_MINI_SPI, MS);
		spi_xfer(STLINKV3_MINI_SPI, MS >> 16);
		do_spi_to_gpio();
	}
	else
	{

		gpio_set_val(SWDIO_PORT, SWDIO_PIN, MS & 1);
		MS >>= 1;
		while (ticks--) {
			gpio_set(SWCLK_PORT, SWCLK_PIN);
			gpio_set_val(SWDIO_PORT, SWDIO_PIN, MS & 1);
			MS >>= 1;
			gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
		}
	}
	gpio_set_val(SWDIO_PORT, SWDIO_PIN, parity & 1);
	gpio_set(SWCLK_PORT, SWCLK_PIN);
	gpio_set(SWCLK_PORT, SWCLK_PIN);
	gpio_set(SWCLK_PORT, SWCLK_PIN);
	gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
gpio_clear(SWCLK_PORT, SWCLK_PIN);
}

swd_proc_t swd_proc;

int swdptap_init(void)
{
	swd_proc.swdptap_seq_in  = swdptap_seq_in;
	swd_proc.swdptap_seq_in_parity  = swdptap_seq_in_parity;
	swd_proc.swdptap_seq_out = swdptap_seq_out;
	swd_proc.swdptap_seq_out_parity  = swdptap_seq_out_parity;

	return 0;
}
