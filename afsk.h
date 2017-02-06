#ifndef AFSK_H
#define AFSK_H

void afsk_init(void);
void afsk_send(const uint8_t *buffer, uint16_t len);
void afsk_start(void);
uint8_t afsk_busy(void);

void afsk_ptt_on(void);
void afsk_ptt_off(void);
void afsk_output_sample(uint8_t s);
uint8_t afsk_request_cwoff(void);
void afsk_timer_stop(void);
void afsk_timer_start(void);

#endif
