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
#include <libopencm3/stm32/adc.h>

#include "config.h"
#include "adc.h"


// Configure ADC
void adc_init(void)
{
    rcc_periph_clock_enable(RCC_ADC);
    rcc_periph_clock_enable(RCC_GPIOA);

    uint8_t channel_array[] = { ADC_CHANNEL_TEMP };

    adc_power_off(ADC1);

    adc_set_clk_source(ADC1, ADC_CLKSOURCE_ADC);

    adc_calibrate_start(ADC1);
    adc_calibrate_wait_finish(ADC1);

    adc_set_single_conversion_mode(ADC1);

    adc_disable_external_trigger_regular(ADC1);
    adc_set_right_aligned(ADC1);

    adc_enable_temperature_sensor();
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_239DOT5); // Slow samplerate

    adc_set_regular_sequence(ADC1, 1, channel_array);
    adc_set_resolution(ADC1, ADC_RESOLUTION_12BIT);
    adc_disable_analog_watchdog(ADC1);
    adc_power_on(ADC1);

}


//See RM0091 section 13.9 and appendix A.7.16
//Temperature sensor raw value at 30 degrees C, VDDA=3.3V
#define TEMP30_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFFF7B8))
//Temperature sensor raw value at 110 degrees C, VDDA=3.3V
#define TEMP110_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFFF7C2))

// Read internal temperature from ADC
// TODO: Power on ADC, sample, power off ADC.
int16_t adc_gettemp(void)
{
    // Read first channel (internal temperature)
    adc_start_conversion_regular(ADC1);
    while (!(adc_eoc(ADC1)));
    int32_t tempraw = adc_read_regular(ADC1);


    int32_t temperature;
    temperature = tempraw - ((int32_t)*TEMP30_CAL_ADDR);
    temperature = temperature * (110L - 30L);
    temperature = temperature / (*TEMP110_CAL_ADDR - *TEMP30_CAL_ADDR);
    temperature = temperature + 30L;
    
    temperature = temperature + ADC_TEMPERATURE_OFFSET;
    // temp30 cal is 5144
    // temp110cal is 3630 
    // for some reason, these calculations aren't returning what I would expect...
    return temperature;

}
