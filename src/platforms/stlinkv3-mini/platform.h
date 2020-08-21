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

/* This file implements the platform specific functions for the STM32
 * implementation.
 */
#ifndef __PLATFORM_H
#define __PLATFORM_H

#include "gpio.h"
#include "timing.h"
#include "timing_stm32.h"

#include <libopencm3/cm3/common.h>
#include <libopencm3/stm32/f1/memorymap.h>
#include <libopencm3/usb/usbd.h>

#ifdef ENABLE_DEBUG
# define PLATFORM_HAS_DEBUG
# define USBUART_DEBUG
extern bool debug_bmp;
int usbuart_debug_write(const char *buf, size_t len);
#endif

#define PLATFORM_IDENT    "(STLINK-V3) "

/* Hardware definitions... */
#define TDI_PORT	GPIOA
#define TMS_PORT	GPIOF
#define TCK_PORT	GPIOB
#define TDO_PORT	GPIOA
#define TDI_PIN		GPIO7
#define TMS_PIN		GPIO9
#define TCK_PIN		GPIO2
#define TDO_PIN		GPIO6

#define SWDIO_PORT 	TMS_PORT
#define SWCLK_PORT 	TCK_PORT
#define SWDIO_PIN	TMS_PIN
#define SWCLK_PIN	TCK_PIN

#define SRST_PORT	GPIOB
#define SRST_PIN_V1	GPIO1
#define SRST_PIN_V2	GPIO0

#define LED_PORT	GPIOA
/* Use PC14 for a "dummy" uart led. So we can observe at least with scope*/
#define LED_PORT_UART	GPIOC
#define LED_UART	GPIO14

//#define PLATFORM_HAS_TRACESWO	0
#define NUM_TRACE_PACKETS		(128)		/* This is an 8K buffer */
#define TRACESWO_PROTOCOL		2			/* 1 = Manchester, 2 = NRZ / async */

//# define SWD_CR   GPIO_CRH(SWDIO_PORT)
//# define SWD_CR_MULT (1 << ((14 - 8) << 2))

#define SWDIO_MODE_FLOAT()	do { gpio_mode_setup(SWDIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, SWDIO_PIN); } while (0)
#define SWDIO_MODE_DRIVE()	do {\
	gpio_mode_setup(SWDIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SWDIO_PIN);\
	gpio_set_output_options(SWDIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, SWDIO_PIN);\
} while (0)
#define TMS_SET_MODE()		SWDIO_MODE_DRIVE()
#define UART_PIN_SETUP() /* TODO: write this. */

#define USB_DRIVER      stm32f723_usb_driver
#define USB_IRQ	        NVIC_OTG_HS_IRQ
#define USB_ISR	        otg_hs_isr
/* Interrupt priorities.  Low numbers are high priority.
 * For now USART2 preempts USB which may spin while buffer is drained.
 */
#define IRQ_PRI_USB		(2 << 4)
#define IRQ_PRI_USBUSART	(1 << 4)
#define IRQ_PRI_USBUSART_TIM	(3 << 4)
#define IRQ_PRI_USB_VBUS	(14 << 4)
#define IRQ_PRI_SWO_DMA			(1 << 4)

#define USBUSART USART2
#define USBUSART_CR1 USART2_CR1
#define USBUSART_IRQ NVIC_USART2_IRQ
#define USBUSART_CLK RCC_USART2
#define USBUSART_PORT GPIOA
#define USBUSART_TX_PIN GPIO2
#define USBUSART_ISR usart2_isr
#define USBUSART_TIM TIM4
#define USBUSART_TIM_CLK_EN() rcc_periph_clock_enable(RCC_TIM4)
#define USBUSART_TIM_IRQ NVIC_TIM4_IRQ
#define USBUSART_TIM_ISR tim4_isr

/* On F103, only USART1 is on AHB2 and can reach 4.5 MBaud at 72 MHz.*/
#define SWO_UART				USART1
#define SWO_UART_DR				USART1_DR
#define SWO_UART_CLK			RCC_USART1
#define SWO_UART_PORT			GPIOA
#define SWO_UART_RX_PIN			GPIO10

/* This DMA channel is set by the USART in use */
#define SWO_DMA_BUS				DMA1
#define SWO_DMA_CLK				RCC_DMA1
#define SWO_DMA_CHAN			DMA_CHANNEL5
#define SWO_DMA_IRQ				NVIC_DMA1_CHANNEL5_IRQ
#define SWO_DMA_ISR(x)			dma1_channel5_isr(x)

extern uint16_t led_idle_run;
#define LED_IDLE_RUN            led_idle_run
#define SET_RUN_STATE(state)	{running_status = (state);}
#define SET_IDLE_STATE(state)	{gpio_set_val(LED_PORT, led_idle_run, state);}
#define SET_ERROR_STATE(x)

extern uint32_t detect_rev(void);

/*
 * Use newlib provided integer only stdio functions
 */

/* sscanf */
#ifdef sscanf
#undef sscanf
#define sscanf siscanf
#else
#define sscanf siscanf
#endif
/* sprintf */
#ifdef sprintf
#undef sprintf
#define sprintf siprintf
#else
#define sprintf siprintf
#endif
/* vasprintf */
#ifdef vasprintf
#undef vasprintf
#define vasprintf vasiprintf
#else
#define vasprintf vasiprintf
#endif
/* snprintf */
#ifdef snprintf
#undef snprintf
#define snprintf sniprintf
#else
#define snprintf sniprintf
#endif


#endif

