#ifndef CONFIG_H
#define CONFIG_H

#include <libopencm3/stm32/gpio.h>

// --------------------------------------------------------------------------
// Transmitter config (si446x.c)
// --------------------------------------------------------------------------

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
