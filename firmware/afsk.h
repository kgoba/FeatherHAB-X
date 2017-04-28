#ifndef _AFSK_H_INCLUDED_
#define _AFSK_H_INCLUDED_

#include <stdint.h>


/** Configures the hardware resources for AFSK (timer, GPIO, interrupts).
 */
void afsk_setup(void);


/** Starts to transmit a message bit by bit asynchronously. 
 * Message length is indicated in bits.
 */
void afsk_send(uint8_t *message, uint16_t lengthInBits);


/** Stops any ongoing transmission immediately. 
 */
void afsk_stop(void);


/** Checks whether a transmission is ongoing. Returns nonzero if so. 
 */
uint8_t afsk_busy(void);


#endif
