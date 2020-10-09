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
	while (1);
}

#if RUN_SFORTH == 1 || 1
#include "engine.h"
#include "sf-arch.h"

#include "sf-word-wizard.h"

static void do_stlinkv3_mini_help(void)
{ print_str("stlinkv3-mini support for the blackmagic debug probe\n\n"); }

static void do_help(void)
{ print_str("TODO: help is not available at this moment; please, provide help here\n"); }

static void do_spi_send8(void)
{ spi_send8(STLINKV3_MINI_SPI, sf_pop()); sf_push(spi_read8(STLINKV3_MINI_SPI)); }

static void do_spi_send(void)
{ sf_push(spi_xfer(STLINKV3_MINI_SPI, sf_pop())); }

static void do_spi_cpol0(void) { spi_set_clock_polarity_0(STLINKV3_MINI_SPI); }
static void do_spi_cpol1(void) { spi_set_clock_polarity_1(STLINKV3_MINI_SPI); }

static void do_spi_cphase0(void) { spi_set_clock_phase_0(STLINKV3_MINI_SPI); }
static void do_spi_cphase1(void) { spi_set_clock_phase_1(STLINKV3_MINI_SPI); }

static void do_spi_set_baud_prescaler(void) { spi_set_baudrate_prescaler(STLINKV3_MINI_SPI, sf_pop() & 7); }
static void do_spi_set_data_bitsize(void)
{
	static const uint16_t data_sizes[16] =
	{
		SPI_CR2_DS_4BIT,
		SPI_CR2_DS_4BIT,
		SPI_CR2_DS_4BIT,
		SPI_CR2_DS_4BIT,
		SPI_CR2_DS_5BIT,
		SPI_CR2_DS_6BIT,
		SPI_CR2_DS_7BIT,
		SPI_CR2_DS_8BIT,
		SPI_CR2_DS_9BIT,
		SPI_CR2_DS_10BIT,
		SPI_CR2_DS_11BIT,
		SPI_CR2_DS_12BIT,
		SPI_CR2_DS_13BIT,
		SPI_CR2_DS_14BIT,
		SPI_CR2_DS_15BIT,
		SPI_CR2_DS_16BIT,
	};
	unsigned t = sf_pop() & 15;
	spi_set_data_size(STLINKV3_MINI_SPI, data_sizes[t]);
}

static void do_swdio_float(void)
{
	gpio_mode_setup(STLINKV3_MINI_SPI_MOSI_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, STLINKV3_MINI_SPI_MOSI_PIN);
	gpio_set_af(STLINKV3_MINI_SPI_MOSI_PORT, STLINKV3_MINI_SPI_AF_NUMBER, STLINKV3_MINI_SPI_MOSI_PIN);
}

static void do_swdio_drive(void)
{
	gpio_mode_setup(STLINKV3_MINI_SPI_MOSI_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, STLINKV3_MINI_SPI_MOSI_PIN);
	gpio_set_output_options(STLINKV3_MINI_SPI_MOSI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, STLINKV3_MINI_SPI_MOSI_PIN);
	gpio_set_af(STLINKV3_MINI_SPI_MOSI_PORT, STLINKV3_MINI_SPI_AF_NUMBER, STLINKV3_MINI_SPI_MOSI_PIN);
}

static void do_spi_to_gpio(void)
{
	spi_disable(STLINKV3_MINI_SPI);

	do_swdio_float();

	gpio_mode_setup(STLINKV3_MINI_SPI_SCK_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, STLINKV3_MINI_SPI_SCK_PIN);
	gpio_set_output_options(STLINKV3_MINI_SPI_SCK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, STLINKV3_MINI_SPI_SCK_PIN);
	gpio_set_af(STLINKV3_MINI_SPI_SCK_PORT, STLINKV3_MINI_SPI_AF_NUMBER, STLINKV3_MINI_SPI_SCK_PIN);
}

static void do_gpio_to_spi(void)
{
	gpio_mode_setup(STLINKV3_MINI_SPI_MOSI_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, STLINKV3_MINI_SPI_MOSI_PIN);
	gpio_set_output_options(STLINKV3_MINI_SPI_MOSI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, STLINKV3_MINI_SPI_MOSI_PIN);
	gpio_set_af(STLINKV3_MINI_SPI_MOSI_PORT, STLINKV3_MINI_SPI_AF_NUMBER, STLINKV3_MINI_SPI_MOSI_PIN);

	gpio_mode_setup(STLINKV3_MINI_SPI_MISO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, STLINKV3_MINI_SPI_MISO_PIN);
	gpio_set_af(STLINKV3_MINI_SPI_MISO_PORT, STLINKV3_MINI_SPI_AF_NUMBER, STLINKV3_MINI_SPI_MISO_PIN);

	gpio_mode_setup(STLINKV3_MINI_SPI_SCK_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, STLINKV3_MINI_SPI_SCK_PIN);
	gpio_set_output_options(STLINKV3_MINI_SPI_SCK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, STLINKV3_MINI_SPI_SCK_PIN);
	gpio_set_af(STLINKV3_MINI_SPI_SCK_PORT, STLINKV3_MINI_SPI_AF_NUMBER, STLINKV3_MINI_SPI_SCK_PIN);

	spi_enable(STLINKV3_MINI_SPI);
}

static void do_swdio_read(void) { sf_push(gpio_get(STLINKV3_MINI_SPI_MOSI_PORT, STLINKV3_MINI_SPI_MOSI_PIN) ? 1 : 0); }

static void do_swdio_hi(void) { gpio_set(STLINKV3_MINI_SPI_MOSI_PORT, STLINKV3_MINI_SPI_MOSI_PIN); }
static void do_swdio_low(void) { gpio_clear(STLINKV3_MINI_SPI_MOSI_PORT, STLINKV3_MINI_SPI_MOSI_PIN); }

static void do_swck_hi(void) { gpio_set(STLINKV3_MINI_SPI_SCK_PORT, STLINKV3_MINI_SPI_SCK_PIN); }
static void do_swck_low(void) { gpio_clear(STLINKV3_MINI_SPI_SCK_PORT, STLINKV3_MINI_SPI_SCK_PIN); }

static struct word dict_base_dummy_word[1] = { MKWORD(0, 0, "", 0), };
static const struct word custom_dict[] = {
	MKWORD(dict_base_dummy_word,	0,		"stlinkv3-mini-help",	do_stlinkv3_mini_help),
	MKWORD(custom_dict,		__COUNTER__,	"spi-send8",	do_spi_send8),
	MKWORD(custom_dict,		__COUNTER__,	"spi-send",	do_spi_send),
	MKWORD(custom_dict,		__COUNTER__,	"spi-cpol0",	do_spi_cpol0),
	MKWORD(custom_dict,		__COUNTER__,	"spi-cpol1",	do_spi_cpol1),
	MKWORD(custom_dict,		__COUNTER__,	"spi-cphase0",	do_spi_cphase0),
	MKWORD(custom_dict,		__COUNTER__,	"spi-cphase1",	do_spi_cphase1),

	MKWORD(custom_dict,		__COUNTER__,	"spi-set-baud-prescaler",	do_spi_set_baud_prescaler),
	MKWORD(custom_dict,		__COUNTER__,	"spi-set-data-bitsize",		do_spi_set_data_bitsize),

	MKWORD(custom_dict,		__COUNTER__,	"spi>gpio",	do_spi_to_gpio),
	MKWORD(custom_dict,		__COUNTER__,	"gpio>spi",	do_gpio_to_spi),

	MKWORD(custom_dict,		__COUNTER__,	"swdio-float",	do_swdio_float),
	MKWORD(custom_dict,		__COUNTER__,	"swdio-drive",	do_swdio_drive),

	MKWORD(custom_dict,		__COUNTER__,	"swdio-hi",	do_swdio_hi),
	MKWORD(custom_dict,		__COUNTER__,	"swdio-low",	do_swdio_low),

	MKWORD(custom_dict,		__COUNTER__,	"swdio-read",	do_swdio_read),

	MKWORD(custom_dict,		__COUNTER__,	"swck-hi",	do_swck_hi),
	MKWORD(custom_dict,		__COUNTER__,	"swck-low",	do_swck_low),

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

void platform_init(void)
{

	rcc_periph_clock_enable(RCC_APB2ENR_SYSCFGEN);
	rcc_clock_setup_hse(rcc_3v3 + RCC_CLOCK_3V3_216MHZ, 25);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOH);
	rcc_periph_clock_enable(RCC_GPIOF);

	/* Configure and set the TPWR_EN pin. */
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0);
	gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO0);
	/* Output 5V. */
	gpio_set(GPIOB, GPIO0);

	/* Configure scope trigger signal. */
	gpio_mode_setup(SCOPE_TRIGGER_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SCOPE_TRIGGER_PIN);
	gpio_set_output_options(SCOPE_TRIGGER_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, SCOPE_TRIGGER_PIN);

	rcc_periph_clock_enable(RCC_SPI5);
	/* Configure spi pins - used for swd bus driving. */
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
	do_spi_to_gpio();
	do_swck_low();
#if 0
void spi_enable(uint32_t spi);
void spi_disable(uint32_t spi);
void spi_write(uint32_t spi, uint16_t data);
void spi_send(uint32_t spi, uint16_t data);
uint16_t spi_read(uint32_t spi);
uint16_t spi_xfer(uint32_t spi, uint16_t data);
void spi_set_bidirectional_mode(uint32_t spi);
void spi_disable_crc(uint32_t spi);
void spi_set_full_duplex_mode(uint32_t spi);
void spi_enable_software_slave_management(uint32_t spi);
void spi_send_lsb_first(uint32_t spi);
void spi_send_msb_first(uint32_t spi);
void spi_set_baudrate_prescaler(uint32_t spi, uint8_t baudrate);
void spi_set_master_mode(uint32_t spi);
void spi_set_clock_polarity_1(uint32_t spi);
void spi_set_clock_polarity_0(uint32_t spi);
void spi_set_clock_phase_1(uint32_t spi);
void spi_set_clock_phase_0(uint32_t spi);
void spi_enable_ss_output(uint32_t spi);
void spi_disable_ss_output(uint32_t spi);
void spi_enable_tx_dma(uint32_t spi);
void spi_disable_tx_dma(uint32_t spi);
void spi_enable_rx_dma(uint32_t spi);
void spi_disable_rx_dma(uint32_t spi);
void spi_set_standard_mode(uint32_t spi, uint8_t mode);

int spi_init_master(uint32_t spi, uint32_t br, uint32_t cpol, uint32_t cpha, 
		uint32_t lsbfirst);
void spi_set_crcl_8bit(uint32_t spi);
void spi_set_crcl_16bit(uint32_t spi);
void spi_set_data_size(uint32_t spi, uint16_t data_s);
void spi_fifo_reception_threshold_8bit(uint32_t spi);
void spi_fifo_reception_threshold_16bit(uint32_t spi);
void spi_i2s_mode_spi_mode(uint32_t spi);
void spi_send8(uint32_t spi, uint8_t data);
uint8_t spi_read8(uint32_t spi);
#endif
	platform_timing_init();
	cdcacm_init();
	usbuart_init();
}
