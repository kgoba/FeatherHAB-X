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
 * Ethan Zonca
 *
 */

#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "usart.h"
#include "gps.h"
#include "config.h"


void usart_init(void)
{
    // Init clocks
    rcc_periph_clock_enable(RCC_USART1);

    // GPS FET control
    gpio_mode_setup(GPS_ONOFF_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPS_ONOFF_PIN);
    gpio_set_output_options(GPS_ONOFF_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LOW, GPS_ONOFF_PIN);
    gps_poweron();

    // USART TX
    gpio_mode_setup(GPS_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, GPS_TX_PIN);
    gpio_set_output_options(GPS_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, GPS_TX_PIN);
    gpio_set_af(GPS_TX_PORT, GPS_TX_AF, GPS_TX_PIN);

    // USART RX
    gpio_mode_setup(GPS_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, GPS_RX_PIN);
    gpio_set_output_options(GPS_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, GPS_RX_PIN);
    gpio_set_af(GPS_RX_PORT, GPS_RX_AF, GPS_RX_PIN);

    // USART Config
    usart_set_baudrate(GPS_USART, GPS_BAUDRATE);
    usart_set_databits(GPS_USART, 8);
    usart_set_parity(GPS_USART, USART_PARITY_NONE);
    usart_set_stopbits(GPS_USART, USART_CR2_STOP_1_0BIT);
    usart_set_mode(GPS_USART, USART_MODE_TX_RX);
    usart_set_flow_control(GPS_USART, USART_FLOWCONTROL_NONE);

    // Enable interrupts
    usart_enable_rx_interrupt(GPS_USART);
    nvic_enable_irq(GPS_IRQ);

    // Enable USART
    usart_enable(GPS_USART);

}

// vim:softtabstop=4 shiftwidth=4 expandtab 
