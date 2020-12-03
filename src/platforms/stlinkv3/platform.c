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

#define SCB_CCR_IC_Pos                      17U                                           /*!< SCB CCR: Instruction cache enable bit Position */
#define SCB_CCR_IC_Msk                     (1UL << SCB_CCR_IC_Pos)                        /*!< SCB CCR: Instruction cache enable bit Mask */

#define SCB_CCR_DC_Pos                      16U                                           /*!< SCB CCR: Cache enable bit Position */
#define SCB_CCR_DC_Msk                     (1UL << SCB_CCR_DC_Pos)                        /*!< SCB CCR: DC Mask */

#define SCB_CCSIDR_NUMSETS_Msk             (0x7FFFUL << SCB_CCSIDR_NUMSETS_Pos)           /*!< SCB CCSIDR: NumSets Mask */
#define SCB_CCSIDR_NUMSETS_Pos             13U                                            /*!< SCB CCSIDR: NumSets Position */

#define SCB_CCSIDR_ASSOCIATIVITY_Pos        3U                                            /*!< SCB CCSIDR: Associativity Position */
#define SCB_CCSIDR_ASSOCIATIVITY_Msk       (0x3FFUL << SCB_CCSIDR_ASSOCIATIVITY_Pos)      /*!< SCB CCSIDR: Associativity Mask */
#define CCSIDR_WAYS(x)         (((x) & SCB_CCSIDR_ASSOCIATIVITY_Msk) >> SCB_CCSIDR_ASSOCIATIVITY_Pos)

#define SCB_DCISW_SET_Pos                   5U                                            /*!< SCB DCISW: Set Position */
#define SCB_DCISW_SET_Msk                  (0x1FFUL << SCB_DCISW_SET_Pos)                 /*!< SCB DCISW: Set Mask */

#define SCB_DCISW_WAY_Pos                  30U                                            /*!< SCB DCISW: Way Position */
#define SCB_DCISW_WAY_Msk                  (3UL << SCB_DCISW_WAY_Pos)                     /*!< SCB DCISW: Way Mask */

#define CCSIDR_SETS(x)         (((x) & SCB_CCSIDR_NUMSETS_Msk      ) >> SCB_CCSIDR_NUMSETS_Pos      )

static void __DSB(void)
{
  asm volatile ("dsb 0xF":::"memory");
}

static void __ISB(void)
{
  asm volatile ("isb 0xF":::"memory");
}

static void SCB_EnableICache (void)
{
	volatile uint32_t *SCB_ICIALLU =  (volatile uint32_t *)(SCB_BASE + 0x250);
    __DSB();
    __ISB();
    *SCB_ICIALLU = 0UL;                     /* invalidate I-Cache */
    __DSB();
    __ISB();
    SCB_CCR |=  (uint32_t)SCB_CCR_IC_Msk;  /* enable I-Cache */
    __DSB();
    __ISB();
}

static void SCB_EnableDCache (void)
{
	volatile uint32_t *SCB_CCSIDR = (volatile uint32_t *)(SCB_BASE +  0x80);
	volatile uint32_t *SCB_CSSELR = (volatile uint32_t *)(SCB_BASE +  0x84);
	volatile uint32_t *SCB_DCISW  =  (volatile uint32_t *)(SCB_BASE + 0x260);

    uint32_t ccsidr;
    uint32_t sets;
    uint32_t ways;

    *SCB_CSSELR = 0U; /*(0U << 1U) | 0U;*/  /* Level 1 data cache */
    __DSB();

    ccsidr = *SCB_CCSIDR;

    sets = (uint32_t)(CCSIDR_SETS(ccsidr));
    do {
      ways = (uint32_t)(CCSIDR_WAYS(ccsidr));
      do {
        *SCB_DCISW = (((sets << SCB_DCISW_SET_Pos) & SCB_DCISW_SET_Msk) |
                      ((ways << SCB_DCISW_WAY_Pos) & SCB_DCISW_WAY_Msk)  );
        #if defined ( __CC_ARM )
          __schedule_barrier();
        #endif
      } while (ways-- != 0U);
    } while(sets-- != 0U);
    __DSB();

    SCB_CCR |=  (uint32_t)SCB_CCR_DC_Msk;  /* enable D-Cache */

    __DSB();
    __ISB();
}

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
	SCB_EnableICache();
	SCB_EnableDCache();
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOH);
	rcc_periph_clock_enable(RCC_GPIOF);
  
	TMS_SET_MODE();

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
	/* By default, do not drive the swd bus too fast. */
	platform_max_frequency_set(6000000);
}
