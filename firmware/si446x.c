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

#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/rcc.h>

#include "si446x.h"
#include "config.h"
#include "delay.h"


void si446x_setup(void)
{
    // Init clocks
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_SPI1);
    // Init GPIO for CS/MOSI/MISO/CLK
    gpio_mode_setup(SI446x_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SI446x_CS_PIN);
    gpio_set_output_options(SI446x_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, SI446x_CS_PIN);
    gpio_set(SI446x_CS);

    gpio_mode_setup(SI446x_SHDN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SI446x_SHDN_PIN);
    gpio_set_output_options(SI446x_SHDN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, SI446x_SHDN_PIN);
    gpio_clear(SI446x_SHDN);

    gpio_mode_setup(SI446x_MOSI_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, SI446x_MOSI_PIN);
    gpio_set_output_options(SI446x_MOSI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, SI446x_MOSI_PIN);
    gpio_set_af(SI446x_MOSI_PORT, SI446x_MOSI_AF, SI446x_MOSI_PIN);

    gpio_mode_setup(SI446x_MISO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, SI446x_MISO_PIN);
    gpio_set_output_options(SI446x_MISO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, SI446x_MISO_PIN);
    gpio_set_af(SI446x_MISO_PORT, SI446x_MISO_AF, SI446x_MISO_PIN);

    gpio_mode_setup(SI446x_SCK_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, SI446x_SCK_PIN);
    gpio_set_output_options(SI446x_SCK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, SI446x_SCK_PIN);
    gpio_set_af(SI446x_SCK_PORT, SI446x_SCK_AF, SI446x_SCK_PIN);

    // Init SPI interface
    spi_init_master(SI446x_SPI, SPI_CR1_BAUDRATE_FPCLK_DIV_64, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_CRCL_8BIT, SPI_CR1_MSBFIRST);
    spi_enable_ss_output(SI446x_SPI);
    spi_enable(SI446x_SPI);
    spi_set_crcl_8bit(SPI1);

    // Initialize in shutdown state
    si446x_shutdown();
    //delay(20);
}

void si446x_prepare(void)
{
    si446x_wakeup();

    // Not really sure what this is for
    const char PART_INFO_command[] = {0x01}; // Part Info
    si446x_sendcmd(1, 9, PART_INFO_command);

    // Divide SI446x_VCXO_FREQ into its bytes; MSB first  
    uint16_t x3 = SI446x_VCXO_FREQ / 0x1000000;
    uint16_t x2 = (SI446x_VCXO_FREQ - x3 * 0x1000000) / 0x10000;
    uint16_t x1 = (SI446x_VCXO_FREQ - x3 * 0x1000000 - x2 * 0x10000) / 0x100;
    uint16_t x0 = (SI446x_VCXO_FREQ - x3 * 0x1000000 - x2 * 0x10000 - x1 * 0x100); 

    // Power up radio module
    // No patch, boot main app. img, FREQ_VCXO, return 1 byte
    const char init_command[] = {0x02, 0x01, 0x01, x3, x2, x1, x0};
    si446x_sendcmd(7, 1 ,init_command); 

    // Radio ready: clear all pending interrupts and get the interrupt status back
    const char get_int_status_command[] = {0x20, 0x00, 0x00, 0x00}; 
    si446x_sendcmd(4, 9, get_int_status_command);

    // GPIO config: Set all GPIOs LOW; Link NIRQ to CTS; Link SDO to MISO; Max drive strength
                                        // cmd    gp0   gp1   gp2   gp3  nirq  sdo   gencfg
    const char gpio_pin_cfg_command[] = {0x13, 0x02, 0x04, 0x02, 0x02, 0x08, 0x11, 0x00}; 
    si446x_sendcmd(8, 8, gpio_pin_cfg_command);

    si446x_setfreq(SI446x_FREQUENCY);

    // Set to 2FSK direct mode
                            //        set prop   group     numprops  startprop   data
    char set_modem_mod_type_command[] = {0x11,     0x20,     0x01,     0x00,       0xAA}; 
    si446x_sendcmd(5, 1, set_modem_mod_type_command);
    /* 7: 1 = direct mode (asynch)
       6: 0 = >| choose gpio1 as data source
       5: 1 = >| choose gpio1 as data source
       4: 0 = >> | mod source is direct
       3: 1 = >> | mod source is direct
       2: 0 = >|
       1: 1 = >| 2fsk modulation (0x2
       0: 0 = >|
    */
  

    si446x_setpower(0x7F);

    // Set data rate (unsure if this actually affects direct modulation)
                         //        set prop   group     numprops  startprop   data
    char set_data_rate_command[] = {0x11,     0x20,     0x03,     0x03,       0x01, 0x09, 0x00}; //set data rate to 2.3kbps or so
    si446x_sendcmd(7, 1, set_data_rate_command);

    // Tune tx 
    char change_state_command[] = {0x34, 0x05}; //  Change to TX tune state
    si446x_sendcmd(2, 1, change_state_command);

}


void si446x_sendcmd(uint8_t tx_len, uint8_t rx_len, const char* data)
{
    // extra byte to get rid of trash? maybe? why?!
    // see https://github.com/Si4463Project/RadioCode/blob/master/Source/drv_Si4463.c
    if (tx_len == 1)
        tx_len++;
    
    gpio_clear(SI446x_CS);
    
    uint8_t j;
    for (j = 0; j < tx_len; j++) // Loop through the bytes of the pData
    {
      uint8_t wordb = data[j];
      spi_send8(SI446x_SPI, wordb);
      delay(1);
    } 
    
    gpio_set(SI446x_CS);

    //_delay_us(20);
    delay(1); // TODO: Fixme

    gpio_clear(SI446x_CS);

    int reply = 0x00;
    while (reply != 0xFF)
    {       
       //reply = SPI.transfer(0x44);
        spi_send8(SI446x_SPI, 0x44);
        reply = spi_read8(SI446x_SPI);
        if (reply != 0xFF)
        {
         gpio_set(SI446x_CS);
         delay(1); // FIXME _delay_us(20);
         gpio_clear(SI446x_CS);
         delay(1); // FIXME _delay_us(20);
        }
    }

    uint8_t k;
    for (k = 1; k < rx_len; k++)
    {
      spi_send8(SI446x_SPI, 0x44);
    }
       
    gpio_set(SI446x_CS);

    // Wait half a second to prevent Serial buffer overflow
    delay(10); 

}


void si446x_setfreq(uint32_t frequency)
{
    // Set the output divider according to recommended ranges given in si446x datasheet  
    int outdiv = 4;
    int band = 0;
    if (frequency < 705000000UL) { outdiv = 6;  band = 1;};
    if (frequency < 525000000UL) { outdiv = 8;  band = 2;};
    if (frequency < 353000000UL) { outdiv = 12; band = 3;};
    if (frequency < 239000000UL) { outdiv = 16; band = 4;};
    if (frequency < 177000000UL) { outdiv = 24; band = 5;};

    unsigned long f_pfd = 2 * SI446x_VCXO_FREQ / outdiv;

    unsigned int n = ((unsigned int)(frequency / f_pfd)) - 1;

    float ratio = (float)frequency / (float)f_pfd;
    float rest  = ratio - (float)n;

    unsigned long m = (unsigned long)(rest * 524288UL); 

    // Set the band parameter
    unsigned int sy_sel = 8;  
    char set_band_property_command[] = {0x11, 0x20, 0x01, 0x51, (band + sy_sel)};
    si446x_sendcmd(5, 1, set_band_property_command);

    // Set the pll parameters
    unsigned int m2 = m / 0x10000;
    unsigned int m1 = (m - m2 * 0x10000) / 0x100;
    unsigned int m0 = (m - m2 * 0x10000 - m1 * 0x100); 

    // Assemble parameter string
    char set_frequency_property_command[] = {0x11, 0x40, 0x04, 0x00, n, m2, m1, m0};
    si446x_sendcmd(8, 1, set_frequency_property_command);

    // Set frequency deviation
    // ...empirically 0xF9 looks like about 5khz. Sketchy.
    char set_frequency_separation[] = {0x11, 0x20, 0x03, 0x0a, 0x00, 0x03, 0x00}; 
    si446x_sendcmd(7, 1, set_frequency_separation);

}

void si446x_setpower(uint8_t powerlevel)
{
    if(powerlevel > 0x7f)
        powerlevel = 0x7f;
    char set_power_level_command[] = {0x11, 0x22, 0x01, 0x01, powerlevel};
    si446x_sendcmd(5, 1, set_power_level_command);
}


// Turn on transmitter
void si446x_cw_on(void)
{ 
    // Change to TX state
    char change_state_command[] = {0x34, 0x07};
    si446x_sendcmd(2, 1, change_state_command);
    
    // Wait for TX to warm up
    delay(10);
    
}


// Turn off transmitter (note: called from AFSK ISR)
void si446x_cw_off(void)
{
    // Change to ready state
    char change_state_command[] = {0x34, 0x03}; 
    si446x_sendcmd(2, 1, change_state_command);
}
 
void si446x_shutdown(void)
{
    gpio_set(SI446x_SHDN);
}

void si446x_wakeup(void)
{
    gpio_clear(SI446x_SHDN);
}


// vim:softtabstop=4 shiftwidth=4 expandtab 
