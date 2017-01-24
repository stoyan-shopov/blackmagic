/*
Copyright (c) 2014-2016 stoyan shopov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>

#include "engine.h"
#include "sf-word-wizard.h"

static void do_squared(void) { cell x = sf_pop(); sf_push(x * x); }
static void do_gcd(void)
/* implements the FORTH word:
: gcd ( n1 n2 | n) begin over mod swap over 0= until nip ;
*/
{ do { do_over(); do_mod(); do_swap(); do_over(); do_zero_not_equals(); } while (sf_pop()); do_nip(); }

static void usbuart_init(void)
{
	rcc_periph_clock_enable(RCC_USART2);

	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, GPIO2);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO2);

	//gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO3);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO3);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO3);

	/* Setup UART parameters. */
	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_CR2_STOP_1_0BIT);
	usart_set_mode(USART2, USART_MODE_TX_RX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	/* Enable interrupts */
	USART2_CR1 |= USART_CR1_RXNEIE;
	nvic_set_priority(NVIC_USART2_IRQ, 1 << 4);
	nvic_enable_irq(NVIC_USART2_IRQ);

	/* Finally enable the USART. */
	usart_enable(USART2);
}

static struct
{
	char	data[64];
	int	index;
}
usart_data;
volatile uint32_t ustat, ustat1;
void usart2_isr(void)
{
	usart_data.data[usart_data.index ++] = USART2_RDR;
	usart_data.index &= sizeof usart_data.data - 1;
}

static void do_usart_emit(void)
{
	while (!(USART2_ISR & USART_ISR_TXE));
	USART2_TDR = sf_pop();
}

static void do_usart_send(void)
{
cell count = sf_pop();
const char * data = sf_pop();
	while (count --)
		sf_push(* data ++), do_usart_emit();
}

static struct word dict_base_dummy_word[1] = { MKWORD(0, 0, "", 0), };
static const struct word custom_dict[] = {
	/* override the sforth supplied engine reset */
	MKWORD(dict_base_dummy_word,	0,		"squared",	do_squared),
	MKWORD(custom_dict,		__COUNTER__,	"gcd",	do_gcd),
	MKWORD(custom_dict,		__COUNTER__,	"init-usart",	usbuart_init),
	MKWORD(custom_dict,		__COUNTER__,	"usart-emit",	do_usart_emit),
	MKWORD(custom_dict,		__COUNTER__,	"usart-send",	do_usart_send),

}, * custom_dict_start = custom_dict + __COUNTER__;


static void sf_opt_sample_init(void) __attribute__((constructor));
static void sf_opt_sample_init(void)
{
	sf_merge_custom_dictionary(dict_base_dummy_word, custom_dict_start);
}
