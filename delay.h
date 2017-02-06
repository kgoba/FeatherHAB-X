#ifndef DELAY_H
#define DELAY_H

uint32_t get_millis_elapsed(void);
void systick_setup(int freq);
void delay(uint32_t delay);

#endif
