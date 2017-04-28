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
 
#include "si446x.h"

#include <stdio.h>

void delay(uint32_t millis);
void si446x_wakeup();
void si446x_shutdown();
void si446x_select();
void si446x_release();
uint8_t si446x_xfer(uint8_t out);

static uint32_t si446x_VCOXFrequency;

static uint8_t si446x_setNCOModulo(uint8_t osr);

uint8_t si446x_waitForCTS()
{
    int reply = 0x00;
    int nTry = 20;

    // Wait for CTS
    while (nTry > 0) {       
        si446x_select();

        si446x_xfer(0x44);        
        reply = si446x_xfer(0xFF);

        if (reply == 0xFF) break;

        si446x_release();
        nTry--;
    }
    
    if (nTry == 0) {
        LOG("CTS timeout\n");
        return 1;
    }    
    else {
        LOG("Received CTS\n");
        return 0;        
    }
}

uint8_t si446x_sendcmd(uint8_t tx_len, const uint8_t* txData, uint8_t rx_len, uint8_t *rxData)
{
    int j, k;
    
    LOG("Sending ");
    si446x_select();    
    for (j = 0; j < tx_len; j++) {
        uint8_t wordb = txData[j];
        si446x_xfer(wordb);
        LOG("%02x ", wordb);
    } 
    si446x_release();
    LOG("\n");

    if (rx_len == 0) {
        return 0;
    }
    
    if (0 != si446x_waitForCTS())
    {
        si446x_release();
        return 1;
    }

    if (rx_len > 1) {
        LOG("Receiving ");
        for (k = 1; k < rx_len; k++) {
            uint8_t recv = si446x_xfer(0xFF);
            if (rxData) rxData[k - 1] = recv;
            LOG("%02x ", recv);
        }
        LOG("\n");
    }
    si446x_release();

    return 0;
}

uint8_t si446x_boot(uint32_t VCOXFrequency) 
{
    uint8_t rc = 0;
    
    si446x_VCOXFrequency = VCOXFrequency;
    
    // Divide si446x_VCOXFrequency into its bytes; MSB first  
    uint8_t x3 = (uint8_t)(si446x_VCOXFrequency >> 24);
    uint8_t x2 = (uint8_t)(si446x_VCOXFrequency >> 16);
    uint8_t x1 = (uint8_t)(si446x_VCOXFrequency >> 8);
    uint8_t x0 = (uint8_t)(si446x_VCOXFrequency >> 0); 

    // Power up radio module
    // No patch, boot main app. img, TCXO, return 1 byte
    const uint8_t init_command[] = {0x02, 0x01, 0x01, x3, x2, x1, x0};
    rc = si446x_sendcmd(7, init_command, 1, NULL);
    if (rc) return rc;
        
    // Radio ready: clear all pending interrupts and get the interrupt status back
    const uint8_t get_int_status_command[] = {0x20, 0x00, 0x00, 0x00}; 
    rc = si446x_sendcmd(4, get_int_status_command, 9, NULL);
    if (rc) return rc;
        
    return rc;
}

uint8_t si446x_setupGPIO(uint8_t gpio0, uint8_t gpio1, uint8_t gpio2, uint8_t gpio3, uint8_t nirq, uint8_t sdo, uint8_t genConfig) 
{
    const uint8_t gpio_pin_cfg_command[] = { 0x13, gpio0, gpio1, gpio2, gpio3, nirq, sdo, genConfig }; 
    return si446x_sendcmd(8, gpio_pin_cfg_command, 8, NULL);
}

uint8_t si446x_setupGPIO0(uint8_t gpio0) 
{
    uint8_t unmod = EZR_GPIO_MODE_UNMODIFIED;
    return si446x_setupGPIO(gpio0, unmod, unmod, unmod, unmod, unmod, 0); 
}

uint8_t si446x_setupGPIO1(uint8_t gpio1) 
{
    uint8_t unmod = EZR_GPIO_MODE_UNMODIFIED;
    return si446x_setupGPIO(unmod, gpio1, unmod, unmod, unmod, unmod, 0); 
}

uint8_t si446x_setFrequency(uint32_t frequency, uint32_t deviation)
{
    uint8_t rc = 0;
    // Set the output divider according to recommended ranges given in si446x datasheet  
    int outdiv = 4;
    int band = 0;
    if (frequency < 705000000UL) { outdiv = 6;  band = 1;};
    if (frequency < 525000000UL) { outdiv = 8;  band = 2;};
    if (frequency < 353000000UL) { outdiv = 12; band = 3;};
    if (frequency < 239000000UL) { outdiv = 16; band = 4;};
    if (frequency < 177000000UL) { outdiv = 24; band = 5;};

    unsigned long f_pfd = 2 * si446x_VCOXFrequency / outdiv;

    unsigned int n = ((unsigned int)(frequency / f_pfd)) - 1;

    float ratio = (float)frequency / (float)f_pfd;
    float rest  = ratio - (float)n;

    unsigned long m = (unsigned long)(rest * 524288UL); 

    // Set the band parameter
    unsigned int sy_sel = 8;  
    uint8_t set_band_property_command[] = {0x11, 0x20, 0x01, 0x51, (band + sy_sel)};
    rc = si446x_sendcmd(5, set_band_property_command, 1, NULL);
    if (rc) return rc;

    // Set the pll parameters
    unsigned int m2 = m / 0x10000;
    unsigned int m1 = (m - m2 * 0x10000) / 0x100;
    unsigned int m0 = (m - m2 * 0x10000 - m1 * 0x100); 

    // Assemble parameter string
    uint8_t set_frequency_property_command[] = {0x11, 0x40, 0x04, 0x00, n, m2, m1, m0};
    rc = si446x_sendcmd(8, set_frequency_property_command, 1, NULL);
    if (rc) return rc;

    // Set frequency deviation
    // ...empirically 0xF9 looks like about 5khz. Sketchy.
    //uint8_t set_frequency_separation[] = {0x11, 0x20, 0x03, 0x0a, 0x00, 0x03, 0x00}; 
    //rc = si446x_sendcmd(7, 1, set_frequency_separation);
    
    uint32_t x = ((uint64_t)(1ul << 18) * outdiv * deviation) / si446x_VCOXFrequency;
    uint8_t set_frequency_deviation[] = { 
      0x11, 0x20, 0x03, 0x0A,
      (uint8_t)(x >> 16),
      (uint8_t)(x >> 8),
      (uint8_t)(x >> 0)
    };
    rc = si446x_sendcmd(7, set_frequency_deviation, 1, NULL);
    
    /* Needed for data rate to function correctly */
    if (!rc) rc = si446x_setNCOModulo(0);

    return rc;
}

uint8_t si446x_setPower(uint8_t powerlevel)
{
    if (powerlevel > 0x7f)
        powerlevel = 0x7f;
    uint8_t set_power_level_command[] = {0x11, 0x22, 0x01, 0x01, powerlevel};
    return si446x_sendcmd(5, set_power_level_command, 1, NULL);
}

uint8_t si446x_setModulation(uint8_t modulation)
{
    uint8_t set_modem_mod_type_command[] = {0x11,     0x20,     0x01,     0x00,       modulation}; 
    return si446x_sendcmd(5, set_modem_mod_type_command, 1, NULL);
}

uint8_t si446x_setDataRate(uint32_t rate) 
{
    uint8_t x2 = (uint8_t)(rate >> 16);
    uint8_t x1 = (uint8_t)(rate >> 8);
    uint8_t x0 = (uint8_t)(rate >> 0);
    uint8_t set_data_rate_command[] = {0x11, 0x20, 0x03, 0x03, x2, x1, x0};
    return si446x_sendcmd(7, set_data_rate_command, 1, NULL);
}

static uint8_t si446x_setNCOModulo(uint8_t osr) 
{
    uint32_t ncoFreq;
    switch (osr) {
        case 0: 
            ncoFreq = si446x_VCOXFrequency / 10; break;
  //  case kModulo20: ncoFreq = si446x_VCOXFrequency / 20; break;
  //  case kModulo40: ncoFreq = si446x_VCOXFrequency / 40; break;  
        default:
            return -1;
    }
    uint8_t set_nco_modulo_command[] = { 
        0x11, 0x20, 0x04, 0x06,
        (uint8_t)(ncoFreq >> 24) & 0x03 | (uint8_t)(osr << 2),
        (uint8_t)(ncoFreq >> 16),
        (uint8_t)(ncoFreq >> 8),
        (uint8_t)(ncoFreq),
    };

    return si446x_sendcmd(8, set_nco_modulo_command, 1, NULL);
}

uint8_t si446x_getPartID(uint8_t *outPartID)
{
    uint8_t cmd[] = {0x01};
    return si446x_sendcmd(1, cmd, 9, outPartID);
}

uint8_t si446x_tune()
{
    // Tune tx 
    uint8_t change_state_command[] = {0x34, 0x05};
    return si446x_sendcmd(2, change_state_command, 1, NULL);
}

uint8_t si446x_txOn(void)
{ 
    // Change to TX state
    uint8_t change_state_command[] = {0x34, 0x07};
    return si446x_sendcmd(2, change_state_command, 1, NULL);
}

uint8_t si446x_txOff(void)
{
    // Change to ready state
    uint8_t change_state_command[] = {0x34, 0x03}; 
    return si446x_sendcmd(2, change_state_command, 1, NULL);
}
