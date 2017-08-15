#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>

#include "i2c.h"

#define BUS_I2C I2C1

#if defined(STM32F0)
void i2c_setup(void)
{
    
}

#elif defined(STM32F1)
void i2c_setup(void)
{
	/* Enable clocks for I2C2 and AFIO. */
	rcc_periph_clock_enable(RCC_I2C2);
	rcc_periph_clock_enable(RCC_AFIO);

	/* Set alternate functions for the SCL and SDA pins of I2C2. */
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
		      GPIO_I2C2_SCL | GPIO_I2C2_SDA);

	/* Disable the I2C before changing any configuration. */
	i2c_peripheral_disable(BUS_I2C);

	/* APB1 is running at 36MHz. */
	i2c_set_clock_frequency(BUS_I2C, I2C_CR2_FREQ_36MHZ);

	/* 400KHz - I2C Fast Mode */
	i2c_set_fast_mode(BUS_I2C);

	/*
	 * fclock for I2C is 36MHz APB2 -> cycle time 28ns, low time at 400kHz
	 * incl trise -> Thigh = 1600ns; CCR = tlow/tcycle = 0x1C,9;
	 * Datasheet suggests 0x1e.
	 */
	i2c_set_ccr(BUS_I2C, 0x1e);

	/*
	 * fclock for I2C is 36MHz -> cycle time 28ns, rise time for
	 * 400kHz => 300ns and 100kHz => 1000ns; 300ns/28ns = 10;
	 * Incremented by 1 -> 11.
	 */
	i2c_set_trise(BUS_I2C, 0x0b);

	/*
	 * This is our slave address - needed only if we want to receive from
	 * other masters.
	 */
	i2c_set_own_7bit_slave_address(BUS_I2C, 0x32);

	/* If everything is configured -> enable the peripheral. */
	i2c_peripheral_enable(BUS_I2C);
}
#endif
    
void i2c_write_bytes(uint8_t addr, const uint8_t *buffer, uint8_t count)
{
    i2c_transfer7(BUS_I2C, addr, (uint8_t *)buffer, count, 0, 0);
}

void i2c_read_bytes(uint8_t addr, uint8_t *buffer, uint8_t count)
{
    i2c_transfer7(BUS_I2C, addr, 0, 0, buffer, count);
}

void i2c_write_reg8(uint8_t addr, uint8_t reg, const uint8_t data)
{
    uint8_t buffer[2] = {reg, data};
    i2c_transfer7(BUS_I2C, addr, buffer, 2, 0, 0);
}

uint8_t i2c_read_reg8(uint8_t addr, uint8_t reg)
{
    uint8_t buffer[1] = {reg};
    i2c_transfer7(BUS_I2C, addr, buffer, 1, buffer, 1);
    return buffer[0];
}
