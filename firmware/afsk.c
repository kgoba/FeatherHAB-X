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
 * Karlis Goba
 *
 */
 
/** @file
 * 
 * Implements AFSK used by AX.25 on VHF (1200 baud) by PWM modulating 
 * a pin which controls frequency deviation. 
 * 
 * Mark frequency: 1200 Hz, space frequency: 2200 Hz.
 * 
 * LSB first.
 * 
 * NRZ(S) encoding, where  logical 0 bit is marked by a change of state, 
 * and a logical 1 bit is marked by no change of state.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>

#include "afsk.h"

#define RCC_MOD     RCC_GPIOB
#define PORT_MOD    GPIOB
#define PIN_MOD     GPIO1
#define AF_MOD      GPIO_AF0

#define RCC_PWM     RCC_TIM14
#define TIM_PWM     TIM14
#define IRQ_PWM     NVIC_TIM14_IRQ

#define SYMBOL_RATE 1200        /**< AFSK symbol rate (baud) */
#define FREQ_MARK   1200        /**< AFSK mark tone frequency (Hz) */
#define FREQ_SPACE  2200        /**< AFSK space tone frequency (Hz) */

/** Defines AFSK oversampling in PWM cycles per symbol. Should be at least
 * 10 (?) to ensure good sinewave approximation by PWM.
 * 
 * SAMPLES_PER_SYMBOL * PWM_PRESCALER * PWM_PERIOD * SYMBOL_RATE should
 * equal to the frequency of timer source (normally APB clock). 
 */
#define SAMPLES_PER_SYMBOL  32

#define PWM_PRESCALER       5
#define PWM_PERIOD          125

#define WAVETABLE_SIZE      256

/** Number of fractional bits for phase computation */
#define FRAC                6

/* Compute useful constants */
#define PHASE_MAX           ((uint32_t)WAVETABLE_SIZE << FRAC)
#define PHASE_DELTA_MARK    ((uint32_t)PHASE_MAX * FREQ_MARK / SYMBOL_RATE / SAMPLES_PER_SYMBOL)
#define PHASE_DELTA_SPACE   ((uint32_t)PHASE_MAX * FREQ_SPACE / SYMBOL_RATE / SAMPLES_PER_SYMBOL) 

/** PWM values between 0 and 124 inclusive corresponding to a period of sine wave */
static const uint8_t waveTable[WAVETABLE_SIZE] = {
    62, 64, 65, 67, 68, 70, 71, 73, 74, 76, 77, 79, 80, 81, 83, 84, 86, 87, 89, 90, 91, 93, 94, 95, 
    96, 98, 99, 100, 101, 102, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 114, 115, 116, 
    117, 117, 118, 119, 119, 120, 120, 121, 121, 122, 122, 122, 123, 123, 123, 124, 124, 124, 124, 
    124, 124, 124, 124, 124, 124, 124, 123, 123, 123, 122, 122, 122, 121, 121, 120, 120, 119, 119, 
    118, 117, 117, 116, 115, 114, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 102, 101, 
    100, 99, 98, 96, 95, 94, 93, 91, 90, 89, 87, 86, 84, 83, 81, 80, 79, 77, 76, 74, 73, 71, 70, 68, 
    67, 65, 64, 62, 60, 59, 57, 56, 54, 53, 51, 50, 48, 47, 45, 44, 43, 41, 40, 38, 37, 35, 34, 33, 
    31, 30, 29, 28, 26, 25, 24, 23, 22, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 10, 9, 8, 7, 7, 
    6, 5, 5, 4, 4, 3, 3, 2, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 
    4, 4, 5, 5, 6, 7, 7, 8, 9, 10, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 22, 23, 24, 25, 26, 
    28, 29, 30, 31, 33, 34, 35, 37, 38, 40, 41, 43, 44, 45, 47, 48, 50, 51, 53, 54, 56, 57, 59, 60
};

static volatile uint8_t *txBuffer;
static volatile uint8_t  txBitMask;
static volatile uint16_t txBitsToSend;
static volatile uint16_t txSampleInSymbol;
static volatile uint32_t txPhase;
static volatile uint32_t txPhaseDelta;


void afsk_send(uint8_t *message, uint16_t lengthInBits)
{
    txBuffer = message;
    txBitsToSend = lengthInBits;

    txPhase = 0;
    txPhaseDelta = PHASE_DELTA_SPACE;

    txBitMask = 1;  /* LSB first */
    txSampleInSymbol = 0;
    
    timer_enable_oc_output(TIM_PWM, TIM_OC1);
    timer_enable_counter(TIM_PWM);
}


void afsk_stop()
{
    timer_disable_counter(TIM_PWM);
    timer_disable_oc_output(TIM_PWM, TIM_OC1);
}


uint8_t afsk_busy()
{
    return (txBitsToSend > 0);
}


void afsk_setup()
{
    txBitsToSend = 0;

    /* Configure modulation pin (GPIO1 on SI446x) */
    rcc_periph_clock_enable(RCC_MOD);

    /* Setup pin for TIM14_CH1 function */
    gpio_mode_setup(PORT_MOD, GPIO_MODE_AF, GPIO_PUPD_NONE, PIN_MOD);
    gpio_set_output_options(PORT_MOD, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, PIN_MOD);
    gpio_set_af(PORT_MOD, AF_MOD, PIN_MOD);

    // Reset and configure timer
    rcc_periph_clock_enable(RCC_PWM);
    timer_reset(TIM_PWM);
    timer_set_mode(TIM_PWM, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM_PWM, PWM_PRESCALER - 1);
    timer_set_period(TIM_PWM, PWM_PERIOD - 1);
    timer_enable_break_main_output(TIM_PWM);

    timer_set_oc_mode(TIM_PWM, TIM_OC1, TIM_OCM_PWM1);
    timer_set_oc_value(TIM_PWM, TIM_OC1, PWM_PERIOD / 2);

    /* Enables the timer update interrupt, which is called every PWM cycle */
    timer_enable_irq(TIM_PWM, TIM_DIER_UIE);
    nvic_enable_irq(IRQ_PWM);
}


void afsk_shutdown()
{
    timer_disable_irq(TIM_PWM, TIM_DIER_UIE);
    nvic_disable_irq(IRQ_PWM);

    rcc_periph_clock_disable(RCC_MOD);
    rcc_periph_clock_disable(RCC_PWM);
}


void afsk_low()
{
    gpio_clear(PORT_MOD, PIN_MOD);
}


void afsk_high()
{
    gpio_set(PORT_MOD, PIN_MOD);
}


/** Interrupt Service Routine. Performs AFSK output modulation.
 */
void tim14_isr(void)
{
    if (timer_get_flag(TIM_PWM, TIM_SR_UIF)) 
    {
        timer_clear_flag(TIM_PWM, TIM_SR_UIF);

        /* We get here 19200 times per second or 16 times per symbol */
        if (txBitsToSend == 0) {
            afsk_stop();
            return;
        }

        if (txSampleInSymbol == 0) {
            /* Load new symbol (bit) to transmit */
            if (0 == (*txBuffer & txBitMask)) {
                /* Logical 0 - toggle the AFSK frequency */
                txPhaseDelta ^= (PHASE_DELTA_MARK ^ PHASE_DELTA_SPACE);
            }
            
            /* Update bit extraction mask */
            if (txBitMask == 0x80) {
                /* Whole byte was processed, move to the next one */
                txBitMask = 1;
                txBuffer++;
            } else {
                /* LSB first */
                txBitMask <<= 1;
            }

            txBitsToSend--;
        }

        txSampleInSymbol++;
        if (txSampleInSymbol >= SAMPLES_PER_SYMBOL) {
            txSampleInSymbol = 0;
        }

        /* Update PWM amount and increment phase */
        timer_set_oc_value(TIM_PWM, TIM_OC1, waveTable[txPhase >> FRAC]);
        txPhase = (txPhase + txPhaseDelta) % PHASE_MAX;
    }
}
