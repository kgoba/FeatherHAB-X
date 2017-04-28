#ifndef _DEBUG_H_INCLUDED_
#define _DEBUG_H_INCLUDED_

#include <stdint.h>

#define LED_RCC     RCC_GPIOA
#define LED_GPIO    GPIOA
#define LED_PIN     GPIO12

#define LED_ON      gpio_set(LED_GPIO, LED_PIN)
#define LED_OFF     gpio_clear(LED_GPIO, LED_PIN)

void debug_setup(void);

/** Halts with an error indication */
void testFail(uint8_t error);

#endif
