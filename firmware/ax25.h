#ifndef AX25_H_
#define AX25_H_

#include <inttypes.h>

struct s_address {
	char callsign[7];
	uint8_t ssid;
};

// Private
void update_crc(uint8_t a_bit);
void send_byte(uint8_t a_byte);

// Public
void ax25_send_byte(uint8_t a_byte);
void ax25_send_flag(void);
void ax25_send_string(const char *string);
void ax25_send_header(const struct s_address *addresses, uint16_t num_addresses);
void ax25_send_footer(void);
void ax25_flush_frame(void);

#endif /* AX25_H_ */
