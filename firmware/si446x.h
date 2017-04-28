#pragma once

#include <stdint.h>

#ifndef LOG
#define LOG(...)        ;
#endif

/* Bit field values for si446x_setmodulation() */
/* Select TX modulation source (FIFO/direct/PRN) - CHOOSE ONE */
#define EZR_MOD_SOURCE_FIFO          0x00
#define EZR_MOD_SOURCE_DIRECT_MODE   0x08
#define EZR_MOD_SOURCE_PRN           0x10
/* Select direct TX mode - CHOOSE ONE */
#define EZR_MOD_TX_MODE_SYNC    0x00
#define EZR_MOD_TX_MODE_ASYNC   0x80
/* Select direct TX input - CHOOSE ONE */
#define EZR_MOD_TX_MODE_GPIO0   0x00
#define EZR_MOD_TX_MODE_GPIO1   0x20
#define EZR_MOD_TX_MODE_GPIO2   0x40
#define EZR_MOD_TX_MODE_GPIO3   0x60
/* Select radio modulation type - CHOOSE ONE */
#define EZR_MOD_TYPE_CW         0x00
#define EZR_MOD_TYPE_OOK        0x01
#define EZR_MOD_TYPE_2FSK       0x02
#define EZR_MOD_TYPE_2GFSK      0x03
#define EZR_MOD_TYPE_4FSK       0x04
#define EZR_MOD_TYPE_4GFSK      0x05

/* setupGPIO mode values */
#define EZR_GPIO_MODE_UNMODIFIED        0x00
#define EZR_GPIO_MODE_DISABLED          0x01
#define EZR_GPIO_MODE_CMOS_OUTPUT_LOW   0x02
#define EZR_GPIO_MODE_CMOS_OUTPUT_HIGH  0x03
#define EZR_GPIO_MODE_CMOS_INPUT        0x04
#define EZR_GPIO_MODE_CTS_HIGH          0x08
#define EZR_GPIO_MODE_SERIAL_DATA_OUT   0x0B
#define EZR_GPIO_MODE_TX_DATA           0x13
/* This can be combined with the previous */
#define EZR_GPIO_MODE_PULLUP            0x40

uint8_t si446x_sendcmd(uint8_t tx_len, const uint8_t* txData, uint8_t rx_len, uint8_t *rxData);

uint8_t si446x_boot(uint32_t VCOXFrequency);

uint8_t si446x_getPartID(uint8_t *outPartID);

uint8_t si446x_setFrequency(uint32_t frequency, uint32_t deviation);
uint8_t si446x_setModulation(uint8_t modulation);
uint8_t si446x_setDataRate(uint32_t rate);
uint8_t si446x_setPower(uint8_t powerlevel);

uint8_t si446x_setupGPIO0(uint8_t mode);
uint8_t si446x_setupGPIO1(uint8_t mode); 

/** Change to TX tune state */
uint8_t si446x_tune();

uint8_t si446x_txOn();
uint8_t si446x_txOff();
