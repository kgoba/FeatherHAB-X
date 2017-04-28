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

#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
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
#include "radio.h"

/* Set up system clock for lower frequency (24 MHz instead of the default 48 MHz).
 * This is a separate function now as helper functions for underclocked rates 
 * have been removed from libopencm3.
 */
void clock_setup(void)
{
    rcc_osc_on(RCC_HSI);
    rcc_wait_for_osc_ready(RCC_HSI);
    rcc_set_sysclk_source(RCC_HSI);

    rcc_set_hpre(RCC_CFGR_HPRE_NODIV);
    rcc_set_ppre(RCC_CFGR_PPRE_NODIV);

    /* 8MHz / 2 * 6 = 24MHz */
    rcc_set_pll_source(RCC_CFGR_PLLSRC_HSI_CLK_DIV2);
    rcc_set_pll_multiplication_factor(RCC_CFGR_PLLMUL_MUL6);

    rcc_osc_on(RCC_PLL);
    rcc_wait_for_osc_ready(RCC_PLL);
    rcc_set_sysclk_source(RCC_PLL);

    flash_set_ws(FLASH_ACR_LATENCY_000_024MHZ);

    rcc_apb1_frequency = 24000000;
    rcc_ahb_frequency = 24000000;
}

static void disableClocks(void)
{
    // Disable all peripheral clocks 
    rcc_periph_clock_disable(RCC_GPIOB);
    rcc_periph_clock_disable(RCC_GPIOA);
    rcc_periph_clock_disable(RCC_TIM14);
    rcc_periph_clock_disable(RCC_SPI1);
 
}

static void enableClocks(void)
{
    // Enable all peripheral clocks 
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_TIM14);
    rcc_periph_clock_enable(RCC_SPI1); 
}

int main(void)
{
    clock_setup();
    
    enableClocks();

    systick_setup();

    rtc_init();
    sleep_init();
    adc_init();
    gps_init();
    usart_init();

    si446x_setup();
    afsk_setup();

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

        /*
        if(afsk_request_cwoff())
        {
            si446x_cw_off();
            si446x_shutdown();
        }
        */

//        pwr_set_stop_mode();
        sleep_now();
 //       clockenable();
    }
}

// vim:softtabstop=4 shiftwidth=4 expandtab 
