#ifndef _RADIO_HAL_INCLUDED_
#define _RADIO_HAL_INCLUDED_

void spi_setup();
void si446x_setup();

void si446x_shutdown();
void si446x_wakeup();

void si446x_directLow();
void si446x_directHigh();

#endif