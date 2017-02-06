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
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include "afsk.h"
#include "si446x.h"
#include "config.h"
#include "delay.h"


const uint8_t afsk_sine_table[512] = 
{127, 129, 130, 132, 133, 135, 136, 138, 139, 141, 143, 144, 146, 147, 149, 150, 152, 153, 155, 156, 158, 159, 161, 162, 164, 165, 167, 168, 170, 171, 173, 174, 176, 177, 178, 180, 181, 183, 184, 185, 187, 188, 190, 191, 192, 194, 195, 196, 198, 199, 200, 201, 203, 204, 205, 206, 208, 209, 210, 211, 212, 213, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 233, 234, 235, 236, 237, 238, 238, 239, 240, 240, 241, 242, 242, 243, 244, 244, 245, 245, 246, 247, 247, 248, 248, 249, 249, 249, 250, 250, 251, 251, 251, 252, 252, 252, 252, 253, 253, 253, 253, 253, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 253, 253, 253, 253, 253, 252, 252, 252, 252, 251, 251, 251, 250, 250, 249, 249, 249, 248, 248, 247, 247, 246, 245, 245, 244, 244, 243, 242, 242, 241, 240, 240, 239, 238, 238, 237, 236, 235, 234, 233, 233, 232, 231, 230, 229, 228, 227, 226, 225, 224, 223, 222, 221, 220, 219, 218, 217, 216, 215, 213, 212, 211, 210, 209, 208, 206, 205, 204, 203, 201, 200, 199, 198, 196, 195, 194, 192, 191, 190, 188, 187, 185, 184, 183, 181, 180, 178, 177, 176, 174, 173, 171, 170, 168, 167, 165, 164, 162, 161, 159, 158, 156, 155, 153, 152, 150, 149, 147, 146, 144, 143, 141, 139, 138, 136, 135, 133, 132, 130, 129, 127, 125, 124, 122, 121, 119, 118, 116, 115, 113, 111, 110, 108, 107, 105, 104, 102, 101, 99, 98, 96, 95, 93, 92, 90, 89, 87, 86, 84, 83, 81, 80, 78, 77, 76, 74, 73, 71, 70, 69, 67, 66, 64, 63, 62, 60, 59, 58, 56, 55, 54, 53, 51, 50, 49, 48, 46, 45, 44, 43, 42, 41, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 21, 20, 19, 18, 17, 16, 16, 15, 14, 14, 13, 12, 12, 11, 10, 10, 9, 9, 8, 7, 7, 6, 6, 5, 5, 5, 4, 4, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 5, 6, 6, 7, 7, 8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 14, 15, 16, 16, 17, 18, 19, 20, 21, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 41, 42, 43, 44, 45, 46, 48, 49, 50, 51, 53, 54, 55, 56, 58, 59, 60, 62, 63, 64, 66, 67, 69, 70, 71, 73, 74, 76, 77, 78, 80, 81, 83, 84, 86, 87, 89, 90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 108, 110, 111, 113, 115, 116, 118, 119, 121, 122, 124, 125 };

const uint16_t TABLE_SIZE = sizeof(afsk_sine_table);

static inline uint8_t afsk_read_sample(uint16_t phase)
{
    return afsk_sine_table[phase];
}


// constants
//#define MODEM_CLOCK_RATE F_CPU
//#define PLAYBACK_RATE MODEM_CLOCK_RATE / 256  // Fast PWM

#define BAUD_RATE 1200
//#define SAMPLES_PER_BAUD PLAYBACK_RATE / BAUD_RATE // = 156
//#define SAMPLES_PER_BAUD 36

// factor of 4 for the 4x clcokdiv on the counter (maybe kill that)
//#define SAMPLES_PER_BAUD 156 / 4 
#define SAMPLES_PER_BAUD 156 / 2 / 3


// phase offset of 830 gives ~1900 Hz
// phase offset of 1550 gives ~2200 Hz
//#define PHASE_DELTA_1200 1800
//#define PHASE_DELTA_2200 2200
//#define PHASE_DELTA_1200 8000
//#define PHASE_DELTA_2200 10000 

#define PHASE_DELTA_1200 830 * 3 //1800
#define PHASE_DELTA_2200 1550*3  //1550 * 6


// Module globals
volatile unsigned char current_byte;
volatile uint8_t current_sample_in_baud;    // 1 bit = SAMPLES_PER_BAUD samples
volatile uint16_t phase = 10;
volatile uint16_t phasedelta = 3300;

volatile uint8_t request_cwoff = 0;
volatile uint8_t go = 0;                 

volatile uint32_t packet_pos;                 // Next bit to be sent out

volatile uint32_t afsk_packet_size = 10;
volatile const uint8_t *afsk_packet;


#define PRESCALE_1200 155 * 6
#define PRESCALE_2200 84 * 6

////////////////////////////////////////////////
void afsk_init(void) 
{
    // Init gpio of pin
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10); //pa10 radio gp1 
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,  GPIO10);
    gpio_set_af(GPIOA, GPIO_AF2, GPIO10);

    // Reset and configure timer
    timer_reset(TIM1);
    //TIM_CR1_CKD_CK_INT_MUL_4
    timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, //TIM_CR1_CMS_CENTER_1,
                       TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM1, 1); // 155 is 1200hz, 84 is 2200hz


    timer_set_oc_mode(TIM1, TIM_OC3, TIM_OCM_PWM1);
    timer_enable_oc_output(TIM1, TIM_OC3);
    timer_enable_break_main_output(TIM1);
    timer_set_oc_value(TIM1, TIM_OC3, 300);
    timer_set_period(TIM1, 255); // should this be 256? FIXME
    timer_enable_counter(TIM1);

    // Enable interrupt for timer capture/compare
    timer_enable_irq(TIM1, TIM_DIER_CC3IE);

    while(afsk_busy());
}


void afsk_output_sample(uint8_t s)
{
    timer_set_oc_value(TIM1, TIM_OC3, s);
}

uint8_t afsk_request_cwoff(void)
{
    if(request_cwoff)
    {
        request_cwoff = 0;
        return 1;
    }
    else
    {
        return 0;
    }
}

volatile uint32_t freqctr = 0;
void tim1_cc_isr(void)
{
    if (timer_get_flag(TIM1, TIM_SR_CC3IF)) {

	if (go) {
            if (packet_pos == afsk_packet_size) 
            {
                    request_cwoff = 1;
                    gpio_toggle(GPIOB, GPIO7);
                    go = 0;         // End of transmission
                    afsk_timer_stop();  // Disable modem
                    return; // done
            }

            // If sent SAMPLES_PER_BAUD already, go to the next bit
            if (current_sample_in_baud == 0)  // Load up next bit
            {   
                    if ((packet_pos & 7) == 0) // Load up next byte
                    {          
                            current_byte = afsk_packet[packet_pos >> 3];
                    }				
                    else 
                    {
                            current_byte = current_byte / 2;  // ">>1" forces int conversion
                    }			
                    if ((current_byte & 1) == 0) 
                    {
                            // Toggle tone (1200 <> 2200)
                            phasedelta ^= (PHASE_DELTA_1200 ^ PHASE_DELTA_2200);
                    }
            }
    
            phase += phasedelta;
            uint8_t s = afsk_read_sample((phase >> 7) & (TABLE_SIZE - 1));

            afsk_output_sample(s); //real afsk

            if(++current_sample_in_baud == SAMPLES_PER_BAUD) 
            {
                    current_sample_in_baud = 0;
                    packet_pos++;
            }
            
	}

        timer_clear_flag(TIM1, TIM_SR_CC3IF);
    }


}


void afsk_timer_start()
{
    // Clear the overflow flag, so that the interrupt doesn't go off
    // immediately and overrun the next one (p.163).
    // Enable interrupt when TCNT2 reaches TOP (0xFF) (p.151, 163)
    nvic_enable_irq(NVIC_TIM1_CC_IRQ);
    timer_enable_counter(TIM1);
}

void afsk_timer_stop()
{
	// Resting duty cycle
	// Output 0v (could be 255/2, which is 0v after coupling... doesn't really matter)
	//OCR2A = 0x80;

	// Disable playback interrupt
	//TIMSK2 &= ~_BV(TOIE2);
    nvic_disable_irq(NVIC_TIM1_CC_IRQ);
    timer_disable_counter(TIM1);
}


// External

void afsk_start(void)
{
	phasedelta = PHASE_DELTA_1200;
	phase = 0;
	packet_pos = 0;
	current_sample_in_baud = 0;
	go = 1;

    // Wake up and configure radio
    si446x_prepare();
    delay(10);

	// Key the radio
	si446x_cw_on();

	// Start transmission
	afsk_timer_start();
}

uint8_t afsk_busy(void)
{
	return go;
}

void afsk_send(const uint8_t *buffer, uint16_t len)
{
	afsk_packet_size = len;
	afsk_packet = buffer;
}


// vim:softtabstop=4 shiftwidth=4 expandtab 
