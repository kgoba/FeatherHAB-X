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

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/pwr.h>

#include "config.h"
#include "gps.h"
#include "usart.h"
#include "afsk.h"
#include "aprs.h"
#include "adc.h"
#include "delay.h"
#include "sleep.h"
#include "si446x.h"

static void clockdisable(void)
{
    // Enable clocks 
    rcc_periph_clock_disable(RCC_GPIOB);
    rcc_periph_clock_disable(RCC_GPIOA);
//    rcc_periph_clock_enable(RCC_GPIOF);
    rcc_periph_clock_disable(RCC_TIM1);
    rcc_periph_clock_disable(RCC_SPI1);
 
}
static void clockenable(void)
{
    rcc_clock_setup_in_hsi_out_16mhz(); // may want to slow down for power
    // Enable clocks 
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOA);
//    rcc_periph_clock_enable(RCC_GPIOF);
    rcc_periph_clock_enable(RCC_TIM1);
    rcc_periph_clock_enable(RCC_SPI1);
 
}

int main(void)
{
    // Init clocks

    clockenable();

    rtc_init();
    sleep_init();
    

    adc_init();

    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO7);

    systick_setup(1000);

    si446x_setup();

    gps_init();

    afsk_init();
    usart_init();

    // Main loop
    uint32_t last_aprs = APRS_TRANSMIT_PERIOD;
    uint32_t last_gps  = GPS_PARSE_PERIOD;
    
    // TODO: Leave GPS on for 10 minutes at the start, then only turn it on a bit before each transmission

    while(1)
    {

        if(get_millis_elapsed() - last_aprs > APRS_TRANSMIT_PERIOD) 
        {
            aprs_send();
            last_aprs = get_millis_elapsed();
        }

        if(get_millis_elapsed() - last_gps > GPS_PARSE_PERIOD)
        {
            parse_gps_transmission();
            last_gps = get_millis_elapsed();
        }

        if(afsk_request_cwoff())
        {
            si446x_cw_off();
            si446x_shutdown();
        }

//        pwr_set_stop_mode();
        sleep_now();
 //       clockenable();
    }
}

// vim:softtabstop=4 shiftwidth=4 expandtab 
