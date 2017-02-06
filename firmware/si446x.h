#ifndef SI446X_H
#define SI446X_H

#include <libopencm3/stm32/gpio.h>

// Hardware Options ///////////////////
#define SI446x_VCXO_FREQ  30000000UL
#define SI446x_SPI SPI1

#define SI446x_CS_PORT GPIOA
#define SI446x_CS_PIN GPIO15
#define SI446x_CS_AF GPIO_AF0

#define SI446x_GPIO_PORT GPIOA
#define SI446x_GPIO_PIN GPIO10

#define SI446x_MOSI_PORT GPIOB
#define SI446x_MOSI_PIN GPIO5
#define SI446x_MOSI_AF GPIO_AF0

#define SI446x_MISO_PORT GPIOB
#define SI446x_MISO_PIN GPIO4
#define SI446x_MISO_AF GPIO_AF0

#define SI446x_SCK_PORT GPIOB
#define SI446x_SCK_PIN GPIO3
#define SI446x_SCK_AF GPIO_AF0

#define SI446x_SHDN_PORT GPIOA
#define SI446x_SHDN_PIN GPIO9
///////////////////////////////////////


// Internal definitions
#define SI446x_CS SI446x_CS_PORT, SI446x_CS_PIN
#define SI446x_MOSI SI446x_MOSI_PORT, SI446x_MOSI_PIN
#define SI446x_MISO SI446x_MISO_PORT, SI446x_MISO_PIN
#define SI446x_SCK SI446x_SCK_PORT, SI446x_SCK_PIN
#define SI446x_SHDN SI446x_SHDN_PORT, SI446x_SHDN_PIN
#define SI446x_GPIO SI446x_GPIO_PORT, SI446x_GPIO_PIN


void si446x_setup(void);
void si446x_prepare(void);

void si446x_sendcmd(uint8_t tx_len, uint8_t rx_len, const char* data);

void si446x_setfreq(uint32_t frequency);
void si446x_setpower(uint8_t powerlevel);

void si446x_cw_on(void);
void si446x_cw_off(void);

void si446x_shutdown(void);
void si446x_wakeup(void);

#endif
