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


// TODO: Transition to using https://github.com/cuspaceflight/joey-m/blob/master/firmware/gps.c requesting UBX data

#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <string.h>

#include "config.h"
#include "gps.h"
#include "delay.h"
/*

void serial0_sendString(const char* string) {
	while(*string != 0x00)
	{
		usart_send_blocking(USART1, *string);
		string++;
	}
}
*/
// Circular buffer for incoming data
uint8_t nmeaBuffer[NMEABUFFER_SIZE];

// Location of parser in the buffer
uint8_t nmeaBufferParsePosition = 0;

// Location of receive byte interrupt in the buffer
volatile uint16_t nmeaBufferDataPosition = 0;

// holds the byte ALREADY PARSED. includes starting character
int bytesReceived = 0;

//data (and checksum) of most recent transmission
char data[16];

//used to skip over bytes during parse
int skipBytes = 0;

//used to index data arrays during data collection
int numBytes = 0;

//variables to store data from transmission
//least significant digit is stored at location 0 of arrays
char tramsmissionType[7];


void gps_poweron(void)
{
    // NOTE: pchannel
    gpio_clear(GPS_ONOFF);
}

void gps_poweroff(void)
{
    // NOTE: pchannel
    gpio_set(GPS_ONOFF);
}

char timestamp[12];	//hhmmss.ss
char* get_timestamp() 
{
	return timestamp;
}
	
char latitude[14];	//lllll.lla
char latitudeTmpTRIM[8];
char latitudeTmpLSB[4];
char* get_latitudeTrimmed() 
{
	strncpy(latitudeTmpTRIM, &latitude[0], 7);
	latitudeTmpTRIM[7] = 0x00;
	return latitudeTmpTRIM;
}
char* get_latitudeLSBs()
{
	strncpy(latitudeTmpLSB, &latitude[7], 3);
	latitudeTmpLSB[3] = 0x00;
	return latitudeTmpLSB;
}

char longitude[14];	//yyyyy.yyb
char longitudeTmpTRIM[9];
char longitudeTmpLSB[4];

char* get_longitudeTrimmed() 
{
	strncpy(longitudeTmpTRIM, &longitude[0], 8);
	longitudeTmpTRIM[8] = 0x00;
	return longitudeTmpTRIM;
}
char* get_longitudeLSBs()
{
	strncpy(longitudeTmpLSB, &longitude[8], 3);
	longitudeTmpLSB[3] = 0x00;
	return longitudeTmpLSB;
}

char quality;		//quality for GGA and validity for RMC
char numSatellites[4];
char* get_sv() 
{
	return numSatellites;
}

char hdop[6];		//xx.x
char* get_hdop() 
{
	return hdop;
}

char altitude[10];	//xxxxxx.x
char* get_gpsaltitude()
{
	return altitude;
}

char wgs84Height[8];	//sxxx.x
char lastUpdated[8];	//blank - included for testing
char stationID[8];	//blank - included for testing
char checksum[3];	//xx

char knots[8];		//xxx.xx
char* get_speedKnots() 
{
	return knots;
}

char course[8];		//xxx.x
char* get_course() 
{
	return course;
}
	
char dayofmonth[9];	//ddmmyy
char* get_dayofmonth() 
{
	return dayofmonth;
}


uint8_t gps_hadfix = 0;

uint8_t gps_hasfix() 
{
	uint8_t hasFix = get_latitudeTrimmed()[0] != 0x00;
	gps_hadfix = hasFix;
	return hasFix;
}

char variation[9];	//xxx.xb
int calculatedChecksum;
int receivedChecksum;

// transmission state machine
enum decodeState {
	//shared fields
	INITIALIZE=0,
	GET_TYPE,
	GPS_CHECKSUM,	//XOR of all the bytes between the $ and the * (not including the delimiters themselves), written in hexadecimal
	//GGA data fields
	GGA_TIME,
	GGA_LATITUDE,
	GGA_LONGITUDE,
	GGA_QUALITY,
	GGA_SATELLITES,
	GGA_HDOP,
	GGA_ALTITUDE,
	GGA_WGS84,
	GGA_LAST_UPDATE,
	GGA_STATION_ID,
	//RMC data fields
	RMC_TIME,
	RMC_VALIDITY,
	RMC_LATITUDE,
	RMC_LONGITUDE,
	RMC_KNOTS,
	RMC_COURSE,
	RMC_DATE,
	RMC_MAG_VARIATION,
	
}decodeState;


void usart1_isr(void)
{
	uint8_t recv = usart_recv(GPS_USART);
	//ECHO debug: usart_send_blocking(GPS_USART, recv);
	nmeaBuffer[nmeaBufferDataPosition % NMEABUFFER_SIZE] = recv;
	nmeaBufferDataPosition = (nmeaBufferDataPosition + 1) % NMEABUFFER_SIZE;
}

void gps_init() 
{
    timestamp[0] = 0x00;
    latitude[0] = 0x00;
    longitude[0] = 0x00;
    numSatellites[0] = 0x00;
    hdop[0] = 0x00;
    knots[0] = 0x00;
    course[0] = 0x00;
    dayofmonth[0] = 0x00;

    gps_poweron();
    delay(100); // Make sure GPS is awake and alive

    // Disable GLONASS mode
    uint8_t disable_glonass[20] = {0xB5, 0x62, 0x06, 0x3E, 0x0C, 0x00, 0x00, 0x00, 0x20, 0x01, 0x06, 0x08, 0x0E, 0x00, 0x00, 0x00, 0x01, 0x01, 0x8F, 0xB2};

    gps_sendubx(disable_glonass, 20);

    // Enable power saving
    uint8_t enable_powersave[10] = {0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x01, 0x22, 0x92};
    gps_sendubx(enable_powersave, 10);


    // Set dynamic model 6 (<1g airborne platform)
    uint8_t airborne_model[] = { 0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0xDC };
    gps_sendubx(airborne_model, sizeof(airborne_model)/sizeof(uint8_t));

}

void gps_sendubx(uint8_t* dat, uint8_t size)
{
    uint8_t sendctr;
    for(sendctr = 0; sendctr < size; sendctr++)
    {
        usart_send(GPS_USART, dat[sendctr]);
    }
}

// Could inline if program space available
static void setParserState(uint8_t state)
{
	decodeState = state;

	// If resetting, clear vars
	if(state == INITIALIZE)
	{
		calculatedChecksum = 0;
	}
	
	// Every time we change state, we have parsed a byte
	nmeaBufferParsePosition = (nmeaBufferParsePosition + 1) % NMEABUFFER_SIZE;
}



//// MKa GPS transmission parser START
void parse_gps_transmission(void){
	
	// Pull byte off of the buffer
	char byte;
	
	while(nmeaBufferDataPosition != nmeaBufferParsePosition) 
	{
		byte = nmeaBuffer[nmeaBufferParsePosition];
		
		if(decodeState == INITIALIZE) //start of transmission sentence
		{
			if(byte == '$') 
			{
				setParserState(GET_TYPE);
				numBytes = 0; //prep for next phases
				skipBytes = 0;
				calculatedChecksum = 0;
			}		
			
			else 
			{
				setParserState(INITIALIZE);
			}
		}
	
		//parse transmission type
		else if (decodeState == GET_TYPE)
		{
			tramsmissionType[numBytes] = byte;
			numBytes++;
			
			if(byte == ',') //end of this data type
			{
				tramsmissionType[5] = 0x00;
				
				if (tramsmissionType[2] == 'G' &&
				tramsmissionType[3] == 'G' &&
				tramsmissionType[4] == 'A')
				{
					setParserState(GGA_TIME);
					numBytes = 0;
				}
				else if (tramsmissionType[2] == 'R' &&
				tramsmissionType[3] == 'M' &&
				tramsmissionType[4] == 'C')
				{
					setParserState(RMC_TIME);
					numBytes = 0;
				}
				else //this is an invalid transmission type
				{
					setParserState(INITIALIZE);
				}
			}
			else {
				// continue
				setParserState(GET_TYPE);
			}
			
		}
	
		///parses GGA transmissions START
		/// $--GGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*xx
		//timestamp
		else if (decodeState == GGA_TIME)
		{
			if (byte == ',') //end of this data type
			{
				timestamp[4] = 0x00; // Cut off at 4 (no seconds) for APRS
				setParserState(GGA_LATITUDE);
				skipBytes = 0; //prep for next phase of parse
				numBytes = 0;
			}
			else //store data
			{
				setParserState(GGA_TIME);
				timestamp[numBytes] = byte; //byte; //adjust number of bytes to fit array
				numBytes++;
			}
		}
	
		//latitude
		else if (decodeState == GGA_LATITUDE)
		{
			if (byte == ',' && skipBytes == 0) //discard this byte
			{
				skipBytes = 1;
				setParserState(GGA_LATITUDE);
			}
			else if (byte == ',') //end of this data type
			{
				
				latitude[numBytes] = 0x00; // null terminate
				
				setParserState(GGA_LONGITUDE);
				skipBytes = 0; //prep for next phase of parse
				numBytes = 0;
			}
			else //store data
			{
				latitude[numBytes] = byte; //adjust number of bytes to fit array
				numBytes++;
				setParserState(GGA_LATITUDE);
			}
		}
	
		//longitude
		else if (decodeState == GGA_LONGITUDE)
		{
			if (byte == ',' && skipBytes == 0) //discard this byte
			{
				skipBytes = 1;
				setParserState(GGA_LONGITUDE);
			}
			else if (byte == ',') //end of this data type
			{
				longitude[numBytes] = 0x00;
				setParserState(GGA_QUALITY);
				numBytes = 0; //prep for next phase of parse
				skipBytes = 0;
			}
			else //store data
			{
				longitude[numBytes] = byte; //adjust number of bytes to fit array
				numBytes++;
				setParserState(GGA_LONGITUDE);
			}
		}
	
		//GGA quality
		else if (decodeState == GGA_QUALITY)
		{
			if (byte == ',') //end of this data type
			{
				setParserState(GGA_SATELLITES);
				numBytes = 0; //prep for next phase of parse
			}
			else //store data
			{
				quality = byte; //maybe reset if invalid data ??
				setParserState(GGA_QUALITY);
			}
		}
	
		//number of satellites
		else if (decodeState == GGA_SATELLITES)
		{
			if (byte == ',') //end of this data type
			{
				numSatellites[numBytes] = 0x00;
				setParserState(GGA_HDOP);
				numBytes = 0; //prep for next phase of parse
			}
			else //store data
			{
				numSatellites[numBytes] = byte; //adjust number of bytes to fit array
				numBytes++;
				setParserState(GGA_SATELLITES);
			}
		}
	
		//HDOP
		else if (decodeState == GGA_HDOP)
		{
			if (byte == ',' ) //end of this data type
			{
				hdop[numBytes] = 0x00;
				setParserState(GGA_ALTITUDE);
				numBytes = 0; //prep for next phase of parse
				skipBytes = 0;
			}
			else //store data
			{
				hdop[numBytes] = byte; //adjust number of bytes to fit array
				numBytes++;
				setParserState(GGA_HDOP);
			}
		}
	
		//altitude
		else if (decodeState == GGA_ALTITUDE)
		{
			if (byte == ',' && skipBytes == 0) //discard this byte
			{
				altitude[numBytes] = 0x00;
				skipBytes = 1;
				setParserState(GGA_ALTITUDE);
			}
			else if(byte == ',') //end of this data type
			{
				// If we actually have an altitude
				if(numBytes>0) 
				{
					altitude[numBytes-1] = 0x00; // Cut off the "M" from the end of the altitude
				}
				else 
				{
					altitude[numBytes] = 0x00;
				}					
				setParserState(GGA_WGS84);
				numBytes = 0; //prep for next phase of parse
			}
			else //store data
			{
				altitude[numBytes] = byte; //adjust number of bytes to fit array
				numBytes++;
				setParserState(GGA_ALTITUDE);
			}
		}
	
		//WGS84 Height
		else if (decodeState == GGA_WGS84)
		{
			if (byte == ',' && skipBytes == 0) //discard this byte
			{
				skipBytes = 1;
				setParserState(GGA_WGS84);
			}
			else if(byte == ',') //end of this data type
			{
				wgs84Height[numBytes] = 0x00;
				setParserState(GGA_LAST_UPDATE);
				skipBytes = 0; //prep for next phase of parse
				numBytes = 0;
			}
			else //store data
			{
				wgs84Height[numBytes] = byte; //adjust number of bytes to fit array
				numBytes++;
				setParserState(GGA_WGS84);
			}
		}
	
		//last GGA DGPS update
		else if (decodeState == GGA_LAST_UPDATE)
		{
			if (byte == ',') //end of this data type
			{
				lastUpdated[numBytes] = 0x00;
				setParserState(GGA_STATION_ID);
				numBytes = 0; //prep for next phase of parse
			}
			else //store data - this should be blank
			{
				lastUpdated[numBytes] = byte; //adjust number of bytes to fit array
				numBytes++;
				setParserState(GGA_LAST_UPDATE);
			}
		}
	
		//GGA DGPS station ID
		else if (decodeState == GGA_STATION_ID)
		{
			if (byte == ',' || byte == '*') //end of this data type
			{
				stationID[numBytes] = 0x00;
				setParserState(GPS_CHECKSUM);
				numBytes = 0; //prep for next phase of parse
			}
			else //store data - this should be blank
			{
				stationID[numBytes] = byte; //adjust number of bytes to fit array
				numBytes++;
				setParserState(GGA_STATION_ID);
			}
		}
		///parses GGA transmissions END
	
		/// $GPRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a*hh
		///parses RMC transmissions
		//time
		// emz: commented setter, GMC time better?
		else if(decodeState == RMC_TIME)
		{
			if (byte == ',') //end of this data type
			{
				//timestamp[numBytes] = 0x00;
				setParserState(RMC_VALIDITY);
				numBytes = 0; //prep for next phase of parse
			}
			else //store data
			{
				//timestamp[numBytes] = byte; //adjust number of bytes to fit array
				numBytes++;
				setParserState(RMC_TIME);
			}
		}
	
		//validity
		// not needed? dupe gga
		else if(decodeState == RMC_VALIDITY)
		{
			if (byte == ',') //end of this data type
			{
				setParserState(RMC_LATITUDE);
				skipBytes = 0; //prep for next phase of parse
				numBytes = 0;
			}
			else //store data
			{
				//quality = byte;
				numBytes++;
				setParserState(RMC_VALIDITY);
			}
		}
	
		//latitude RMC (we don't need this, commented out setter)
		else if(decodeState == RMC_LATITUDE)
		{
			if (byte == ',' && skipBytes == 0) //discard this byte
			{
				skipBytes = 1; 
				setParserState(RMC_LATITUDE);
			}
			else if (byte == ',') //end of this data type
			{
				setParserState(RMC_LONGITUDE);
				skipBytes = 0; //prep for next phase of parse
				numBytes = 0;
			}
			else //store data
			{
				//latitude[numBytes]= byte; //adjust number of bytes to fit array
				numBytes++;
				setParserState(RMC_LATITUDE);
			}
		}
	
		//longitude RMC (we don't need this, commented out setter)
		else if(decodeState == RMC_LONGITUDE)
		{
			if (byte == ',' && skipBytes == 0) //discard this byte
			{
				skipBytes = 1; 
				setParserState(RMC_LONGITUDE);
			}
			else if (byte == ',') //end of this data type
			{
				setParserState(RMC_KNOTS);
				skipBytes = 0;
				numBytes = 0;
			}
			else //store data
			{
				//longitude[numBytes]= byte; //adjust number of bytes to fit array
				numBytes++;
				setParserState(RMC_LONGITUDE);
			}
		}
	
		//knots
		else if(decodeState == RMC_KNOTS)
		{
			if (byte == ',') //end of this data type
			{
				knots[numBytes] = 0x00;
				setParserState(RMC_COURSE);
				numBytes = 0; //prep for next phase of parse
			}
			else //store data
			{
				setParserState(RMC_KNOTS);
				knots[numBytes]= byte; //adjust number of bytes to fit array
				numBytes++;
			}
		}
	
		//course
		else if(decodeState == RMC_COURSE)
		{
			if (byte == ',') //end of this data type
			{
				course[numBytes] = 0x00;
				setParserState(RMC_DATE);
				numBytes = 0; //prep for next phase of parse
			}
			else //store data
			{
				setParserState(RMC_COURSE);
				course[numBytes] = byte; //adjust number of bytes to fit array
				numBytes++;
			}
		}
	
		//date
		else if(decodeState == RMC_DATE)
		{
			if (byte == ',') //end of this data type
			{
				// Cut it off at day of month. Also has month and year if we ever need it.
				dayofmonth[2] = 0x00;
				setParserState(RMC_MAG_VARIATION);
				skipBytes = 0; //prep for next phase of parse
				numBytes = 0;
			}
			else //store data
			{
				setParserState(RMC_DATE);
				dayofmonth[numBytes] = byte; //adjust number of bytes to fit array
				numBytes++;
			}
		}
	
		//magnetic variation
		else if(decodeState == RMC_MAG_VARIATION)
		{
			if (byte == '*') //end of this data type
			{
				variation[numBytes] = 0x00;
				setParserState(GPS_CHECKSUM);
				numBytes = 0; //prep for next phase of parse
			}
			else //store data
			{
				setParserState(RMC_MAG_VARIATION);
				variation[numBytes] = byte; //adjust number of bytes to fit array
				numBytes++;
			}
		}
		///parses RMC transmissions END
	
	
		//checksum
		else if (decodeState == GPS_CHECKSUM)
		{
			if (numBytes == 2) //end of data - terminating character ??
			{
				//checksum calculator for testing http://www.hhhh.org/wiml/proj/nmeaxor.html
				//TODO: must determine what to do with correct and incorrect messages
				receivedChecksum = checksum[0] + (checksum[1]*16);	//convert bytes to int
				if(calculatedChecksum==receivedChecksum)
				{
				
				}
				else
				{
				
				}
				
				setParserState(INITIALIZE);
				numBytes = 0; //prep for next phase of parse
			}
			else //store data
			{
				setParserState(GPS_CHECKSUM);
				checksum[numBytes] = byte; //adjust number of bytes to fit array
				numBytes++;
			}
		}
		else {
			setParserState(INITIALIZE);
		}
	
		if (decodeState!=GPS_CHECKSUM && decodeState!=INITIALIZE)	//want bytes between '$' and '*'
		{
			//input byte into running checksum
			XORbyteWithChecksum(byte);
		}
	}	
	

}

void XORbyteWithChecksum(uint8_t byte)
{
	calculatedChecksum ^= (int)byte; //this may need to be re-coded
}




// vim:softtabstop=4 shiftwidth=4 expandtab 
