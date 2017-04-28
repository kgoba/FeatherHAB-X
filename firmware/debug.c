#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>

#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "delay.h"

#define FD_DEBUG    1


void debug_setup(void)
{
	/* Setup GPIO pins for USART1 transmit/receive. */
    rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);    // TX
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10);   // RX
	gpio_set_af(GPIOA, GPIO_AF1, GPIO9);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO10);

	/* Setup USART parameters. */
    rcc_periph_clock_enable(RCC_USART1);
	usart_set_baudrate(USART1, 9600);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_CR2_STOP_1_0BIT);
	usart_set_mode(USART1, USART_MODE_TX_RX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART1);
    
    /* setup LED */
    rcc_periph_clock_enable(LED_RCC);
    gpio_mode_setup(LED_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);    
}


void testFail(uint8_t error)
{
    while (1) {
        uint8_t i;
        for (i = 0; i < (error & 0x0F); i++) {
            LED_ON; delay(100); LED_OFF; delay(100);
        }
        delay(500);
    }
}


int _write(int file, char *ptr, int len)
{
	int ret = 0;

	if (file == FD_DEBUG) {
        while (len) {
    		usart_send_blocking(USART1, (uint8_t)*ptr);
            ret++;
            ptr++;
            len--;
        }
		return ret;
	}

	errno = EIO;
	return -1;    
}    
