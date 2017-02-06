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

#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "aprs.h"
#include "gps.h"
#include "adc.h"
#include "ax25.h"


int32_t meters_to_feet(int32_t m)
{
  // 10000 ft = 3048 m
  return (float)m / 0.3048;
}

void aprs_send(void)
{
  struct s_address addresses[] = { 
    {D_CALLSIGN, D_CALLSIGN_ID},  // Destination callsign
    {"", S_CALLSIGN_ID},  // Source callsign (-11 = balloon, -9 = car)
		//{S_CALLSIGN, S_CALLSIGN_ID},
#ifdef DIGI_PATH1
    {DIGI_PATH1, DIGI_PATH1_TTL}, // Digi1 (first digi in the chain)
#endif
#ifdef DIGI_PATH2
    {DIGI_PATH2, DIGI_PATH2_TTL}, // Digi2 (second digi in the chain)
#endif
  };

  strncpy(addresses[1].callsign, S_CALLSIGN, 7);
  
	// emz: modified this to get the size of the first address rather than the size of the struct itself, which fails
  ax25_send_header(addresses, sizeof(addresses)/sizeof(addresses[0]));
  ax25_send_byte('/');                // Report w/ timestamp, no APRS messaging. $ = NMEA raw data
  // ax25_send_string("021709z");     // 021709z = 2nd day of the month, 17:09 zulu (UTC/GMT)
  ax25_send_string(get_dayofmonth()); ///! Needs to be day hour minute        // 170915 = 17h:09m:15s zulu (not allowed in Status Reports)
  ax25_send_string(get_timestamp()); 
  ax25_send_byte('z'); // zulu time. h for nonzulu
  //ax25_send_string("4215.37");//get_latitudeTrimmed());     // Lat: 38deg and 22.20 min (.20 are NOT seconds, but 1/100th of minutes)
  ax25_send_string(get_latitudeTrimmed());     // Lat: 38deg and 22.20 min (.20 are NOT seconds, but 1/100th of minutes)
  ax25_send_byte('N');
  ax25_send_byte('/');                // Symbol table
  ax25_send_string(get_longitudeTrimmed());     // Lon: 000deg and 25.80 min
  ax25_send_byte('W');
  ax25_send_byte('O');                // Symbol: O=balloon, -=QTH
  
  //snprintf(temp, 4, "%03d", (int)(get_course() + 0.5));  
  // TODO: ENSURE THAT THE COURSE IS FORMATTED CORRECTLY!
  ax25_send_string(get_course());             // Course (degrees)
  
  ax25_send_byte('/');                // and
  
  ax25_send_string(get_speedKnots());             // speed (knots)
  
  // TODO: Rework altitude
  //int32_t alt = 100; // meters_to_feet(strtol(/* get_gpsaltitude()*/ 1337, '\0' /*NULL*/, 10));
  //char temp[7];
  ax25_send_string("/A=");            // Altitude (feet). Goes anywhere in the comment area
  //snprintf(temp, 7, "%ld", get_gpsaltitude());
  ax25_send_string(get_gpsaltitude());


  char temp[10];
  itoa(adc_gettemp(), temp, 10);
  //snprintf(temp, 7, "%d", adc_gettemp());
  ax25_send_string("/T=");
  ax25_send_string(temp);
  //ax25_send_string("/HAB Propagation Test");
  ax25_send_string("/FeatherHab Mission 2");
  ax25_send_byte(' ');
  
  #define COMMENTBUFFER_SIZE 128
  //char commentBuffer[COMMENTBUFFER_SIZE];
  //ax25_send_string(slavesensors_getAPRScomment(commentBuffer, COMMENTBUFFER_SIZE));
  
  ax25_send_footer();
  ax25_flush_frame();
}

// vim:softtabstop=4 shiftwidth=4 expandtab 
