/*
 * FeatherHAB 
 *
 * This file is part of FeatherHAB.
 *
 * FeatherHab is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FeatherHab is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with FeatherHAB. If not, see <http://www.gnu.org/licenses/>.
 * 
 * Karlis Goba
 *
 */
 
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>

#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include "delay.h"
#include "debug.h"

#include "si446x.h"
#include "radio.h"
#include "afsk.h"

#include "config.h"

void gps_setup(void)
{
	/* Setup GPIO pins for USART1 transmit/receive. */
    rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);    // TX
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO3);   // RX
	gpio_set_af(GPIOA, GPIO_AF1, GPIO2);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO3);

	/* Setup USART parameters. */
    rcc_periph_clock_enable(RCC_USART2);
	usart_set_baudrate(USART2, 9600);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_CR2_STOP_1_0BIT);
	usart_set_mode(USART2, USART_MODE_TX_RX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART2);
    
    /* setup GPS_nEN, GPS_PPS pins */
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO7);
    gpio_set(GPIOB, GPIO7);

    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO13);
}

void gps_enable()
{
    gpio_clear(GPIOB, GPIO7);
}

void gps_disable()
{
    gpio_set(GPIOB, GPIO7);
}

void gpsTest()
{
    LED_ON; delay(200); LED_OFF; delay(200);
    
    gps_setup();
    
    LED_ON; delay(200); LED_OFF;
    
    gps_enable();
    
    while (1) {
        usart_send_blocking(USART2, 'X');
    }
}

void partNumberTest()
{
    while (1) {        
        uint8_t rc;
        rc = si446x_getPartID(NULL);
        //if (!rc) testFail(4);
    }
}

void prnTest() 
{
    uint8_t rc = 0;
    
    if (!rc) rc = si446x_setFrequency(TRANSMIT_FREQUENCY_HZ, 5000);
    if (!rc) rc = si446x_setModulation(EZR_MOD_SOURCE_PRN | EZR_MOD_TYPE_2FSK);
    if (!rc) rc = si446x_setDataRate(1000);
    if (!rc) rc = si446x_setPower(0x10);
    if (rc) testFail(3);  
    
    si446x_tune();
    delay(100);
    
    while (1) {
        LED_ON;
        si446x_txOn();        
        delay(200);
        
        LED_OFF;
        si446x_txOff();
        delay(2000);       
    }
}

void afskTest() 
{
    uint8_t rc = 0;
    uint8_t modulation = EZR_MOD_TYPE_2FSK | EZR_MOD_SOURCE_DIRECT_MODE | 
                         EZR_MOD_TX_MODE_ASYNC | EZR_MOD_TX_MODE_GPIO1;

    uint8_t message[128];
    uint16_t messageLength = 128;
    
    for (uint16_t i = 0; i < messageLength; i++) {
        message[i] = ' ' + i;
    }

    afsk_setup();    
    
    if (!rc) rc = si446x_setFrequency(TRANSMIT_FREQUENCY_HZ, AFSK_DEVIATION_HZ);
    if (!rc) rc = si446x_setModulation(modulation);
    if (!rc) rc = si446x_setDataRate(1200*64);
    if (!rc) rc = si446x_setPower(0x10);
    if (!rc) rc = si446x_setupGPIO1(EZR_GPIO_MODE_TX_DATA);
    if (rc) testFail(3);
        
    si446x_tune();
    delay(100);

    while (1) {
        LED_ON;
        si446x_txOn();
        delay(20);

        afsk_send(message, 8 * messageLength);
        while (afsk_busy());

        LED_OFF;
        delay(20);
        si446x_txOff();
        delay(2000);
    }
}

int test_setup()
{
    uint8_t rc;

    /* Configure hardware peripherals */
    systick_setup();
    spi_setup();
    si446x_setup();
    debug_setup();
    LED_OFF;

    /* Start up SI446x */
    si446x_shutdown();
    delay(200);
    si446x_wakeup();
    delay(200);
  
    rc = si446x_boot(TCXO_FREQ_HZ);
    if (rc) testFail(2);
    
    //gpsTest();
    //partNumberTest();
    //prnTest();
    //afskTest();
}
