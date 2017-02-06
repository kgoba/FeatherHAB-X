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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

#include "delay.h"

void systick_setup(int freq)
{
    systick_set_reload(rcc_ahb_frequency / freq);
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    STK_CVR = 0;
    systick_interrupt_enable();
    systick_counter_enable();
}

volatile uint32_t system_millis;

void delay(uint32_t delay)
{
    uint32_t wake = system_millis + delay;
    while (wake > system_millis);
}

void sys_tick_handler(void)
{
    system_millis++;
}

uint32_t get_millis_elapsed(void)
{
    return system_millis;
}

// vim:softtabstop=4 shiftwidth=4 expandtab 
