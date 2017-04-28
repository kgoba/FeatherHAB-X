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
 * Karlis Goba
 *
 */
 
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

#include "radio.h"

void delay(uint32_t millis);

#define SPI_RADIO   SPI1
#define RCC_SPI_RADIO   RCC_SPI1
#define RCC_GPIO_SPI    RCC_GPIOA

#define AF_SPI      GPIO_AF0
#define PORT_SPI    GPIOA
#define PIN_MOSI    GPIO7
#define PIN_MISO    GPIO6
#define PIN_SCK     GPIO5

#define RCC_CTRL    RCC_GPIOA
#define PORT_CTRL   GPIOA
#define PIN_NSEL    GPIO4
#define PIN_SHDN    GPIO1
#define PIN_TCXO    GPIO0

void spi_setup()
{
    /* Enable all clocks needed for SPI_RADIO */
    rcc_periph_clock_enable(RCC_GPIO_SPI);
    rcc_periph_clock_enable(RCC_SPI_RADIO);

    /* Configure GPIOs: SCK=PA5, MISO=PA6 and MOSI=PA7 */
    gpio_mode_setup(PORT_SPI, GPIO_MODE_AF, GPIO_PUPD_NONE, PIN_MOSI);
    gpio_set_output_options(PORT_SPI, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, PIN_MOSI);
    gpio_set_af(PORT_SPI, AF_SPI, PIN_MOSI);

    gpio_mode_setup(PORT_SPI, GPIO_MODE_AF, GPIO_PUPD_NONE, PIN_MISO);
    gpio_set_output_options(PORT_SPI, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, PIN_MISO);
    gpio_set_af(PORT_SPI, AF_SPI, PIN_MISO);

    gpio_mode_setup(PORT_SPI, GPIO_MODE_AF, GPIO_PUPD_NONE, PIN_SCK);
    gpio_set_output_options(PORT_SPI, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, PIN_SCK);
    gpio_set_af(PORT_SPI, AF_SPI, PIN_SCK);    
    
    /* Set SPI to mode 0, master, MSB first, low baudrate */
    spi_init_master(SPI_RADIO, SPI_CR1_BAUDRATE_FPCLK_DIV_256, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_CRCL_8BIT, SPI_CR1_MSBFIRST);
    
    /* Configure NSS (slave select) behaviour */
    spi_disable_ss_output(SPI_RADIO);                // SSOE = 0
    spi_enable_software_slave_management(SPI_RADIO); // SSM = 1
    spi_set_nss_high(SPI_RADIO);                     // SSI = 1

    /* Switch to 8 bit frames (needed on F0 series) */
    spi_set_data_size(SPI_RADIO, SPI_CR2_DS_8BIT);
    spi_fifo_reception_threshold_8bit(SPI_RADIO);
        
    /* Enable SPI_RADIO periph. */
    spi_enable(SPI_RADIO);
}

/*
void spi_test()
{
    while (1) {        
        uint8_t reply;
        
        si446x_select();
        delay(1);
        spi_send8(SPI_RADIO, (uint8_t)0x44);

        spi_send8(SPI_RADIO, (uint8_t)0xFF);
        reply = spi_read8(SPI_RADIO);
        delay(1);
        si446x_release();

        LED_ON;
        delay((reply == 0xFF) ? 800 : 200);
        LED_OFF;
        delay((reply == 0xFF) ? 200 : 800);
    }
}
*/

void si446x_setup()
{
    rcc_periph_clock_enable(RCC_CTRL);
    gpio_mode_setup(PORT_CTRL, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PIN_NSEL | PIN_SHDN | PIN_TCXO);
    gpio_set_output_options(PORT_CTRL, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, PIN_NSEL | PIN_SHDN | PIN_TCXO);

    gpio_set(PORT_CTRL, PIN_NSEL);
    gpio_set(PORT_CTRL, PIN_SHDN);
    gpio_set(PORT_CTRL, PIN_TCXO);

    delay(100);
}

void si446x_wakeup()
{
    /* start TCXO */
    gpio_clear(PORT_CTRL, PIN_TCXO);
    /* enable Si446x */
    gpio_clear(PORT_CTRL, PIN_SHDN);
}

void si446x_shutdown()
{
    /* disable Si446x */
    gpio_set(PORT_CTRL, PIN_SHDN);
    /* disable TCXO */
    gpio_set(PORT_CTRL, PIN_TCXO);
}

void si446x_select()
{
    gpio_clear(PORT_CTRL, PIN_NSEL);
    delay(2);
}

void si446x_release()
{
    delay(2);
    gpio_set(PORT_CTRL, PIN_NSEL);
    delay(1);
}

uint8_t si446x_xfer(uint8_t out)
{
    spi_send8(SPI_RADIO, out);
    return spi_read8(SPI_RADIO);
}
