#ifndef CONFIG_H
#define CONFIG_H

#include <libopencm3/stm32/gpio.h>

/*  PIN MAP
    -----------------------------
    PA0     Out     TCXO_nEN
    PA1     Out     TRX_SHDN
    PA2     UART2   GPS_TX      AF1
    PA3     UART2   GPS_RX      AF1
    PA4     Out     TRX_nCS
    PA5     SPI1    SCK         AF0
    PA6     SPI1    MISO        AF0
    PA7     SPI1    MOSI        AF0
    PA9     UART1   Dbg_TX      AF1
    PA10    UART1   Dbg_RX      AF1
    PA12    Out     LED
    PB0     In      TRX_nIRQ
    PB1     In/Out  TRX_GPIO1
    PB2     In/Out  TRX_GPIO0
    PB6     Out     FLASH_nCS
    PB7     Out     GPS_nEN
    PB10    I2C1    SCL         AF1
    PB11    I2C1    SDA         AF1
    PC13    In      GPS_PPS
*/

// --------------------------------------------------------------------------
// Transmitter config (si446x.c)
// --------------------------------------------------------------------------

#define TCXO_FREQ_HZ            26000000ul
#define TRANSMIT_FREQUENCY_HZ  144850000ul
#define AFSK_DEVIATION_HZ           1500ul

#define SI446x_FREQUENCY 146390000UL
//#define SI446x_FREQUENCY 144390000UL


// --------------------------------------------------------------------------
// ADC config (adc.c)
// --------------------------------------------------------------------------

// Temperature sensor offset (die temperature from ambient, esimate, in Celcius)
#define ADC_TEMPERATURE_OFFSET -10


// --------------------------------------------------------------------------
// AX.25 config (ax25.c)
// --------------------------------------------------------------------------

// TX delay in milliseconds
#define TX_DELAY      500

// Maximum packet delay
#define MAX_PACKET_LEN 512  // bytes


// --------------------------------------------------------------------------
// APRS config (aprs.c)
// --------------------------------------------------------------------------

// Set your callsign and SSID here. Common values for the SSID are
// (from http://zlhams.wikidot.com/aprs-ssidguide):
//
// - Balloons:  11
// - Cars:       9
// - Home:       0
// - IGate:      5
#define S_CALLSIGN      "KD8TDF"
#define S_CALLSIGN_ID   11

// Destination callsign: APRS (with SSID=0) is usually okay.
#define D_CALLSIGN      "APRS"
#define D_CALLSIGN_ID   0

// Digipeating paths:
// (read more about digipeating paths here: http://wa8lmf.net/DigiPaths/ )
// The recommended digi path for a balloon is WIDE2-1 or pathless. The default
// is pathless. Uncomment the following two lines for WIDE2-1 path:
#define DIGI_PATH1      "WIDE2"
#define DIGI_PATH1_TTL  1

// Transmit the APRS sentence every X milliseconds
#define APRS_TRANSMIT_PERIOD 60123



// --------------------------------------------------------------------------
// GPS config (gps.c)
// --------------------------------------------------------------------------

// How often to parse data from the GPS circular buffer
#define GPS_PARSE_PERIOD 50

// Baud rate of GPS USART
#define GPS_BAUDRATE 9600

// NMEA circular buffer size. Must be large enough to hold all received sentences
#define NMEABUFFER_SIZE 150




#endif

// vim:softtabstop=4 shiftwidth=4 expandtab 
