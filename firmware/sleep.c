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


#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/cm3/scb.h>

#include "sleep.h"

void sleep_init(void)
{
    SCB_SCR &= ~4; // No deep sleep

    /* To be able to debug in stop mode */
//    DBGMCU->CR |= DBGMCU_CR_STANDBY | DBGMCU_CR_STOP | DBGMCU_CR_SLEEP;

}
inline __attribute__((always_inline)) void sleep_now(void)
{
    __asm volatile ("wfi");
}

void rtc_init(void)
{
    //////////////////////////////////// 
    // Setup RTC
    rcc_periph_clock_enable(RCC_PWR);
    rcc_periph_clock_enable(RCC_RTC);

    // Fire up the low-speed oscillator
    rcc_osc_on(LSI);
    rcc_wait_for_osc_ready(LSI);

    pwr_disable_backup_domain_write_protect();
    rtc_unlock();
    // Select the LSI as rtc clock  TODO: Add define
    RCC_BDCR |= (1<<9); 

    // Set the RTCEN bit TODO: Add define
    RCC_BDCR |= (1<<15); 

    rtc_unlock();

    // enter init mode 
    RTC_ISR |= RTC_ISR_INIT;
    while ((RTC_ISR & RTC_ISR_INITF) == 0);

    // set synch prescaler, using defaults for 1Hz out 
    uint32_t sync = 255;
    uint32_t async = 127;
    rtc_set_prescaler(sync, async);

    // load time and date here if desired, and hour format 
    // exit init mode 
    RTC_ISR &= ~(RTC_ISR_INIT);

    // EMZ new:
    rtc_set_wakeup_time(0xFF, RTC_CR_WUCLKSEL_RTC_DIV16);


    rtc_lock();
    RCC_BDCR |= 0x8000; // Set the RTCEN bit TODO: Add define
    rtc_wait_for_synchro();
    //////////////////////////////////////////
}

// vim:softtabstop=4 shiftwidth=4 expandtab 
