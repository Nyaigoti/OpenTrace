#ifndef VEML6035_H
#define VEML6035_H

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

// VEML6035 I2C Address
#define VEML6035_I2C_ADDR 0x29

// Register Map
#define VEML6035_REG_ALS_CONF 0x00
#define VEML6035_REG_ALS_WH   0x01
#define VEML6035_REG_ALS_WL   0x02
#define VEML6035_REG_ALS_PSM  0x03
#define VEML6035_REG_ALS      0x04
#define VEML6035_REG_WHITE    0x05
#define VEML6035_REG_ALS_INT  0x06

// Function Prototypes
int veml6035_init(const struct device *i2c_dev);
int veml6035_configure(const struct device *i2c_dev);
int veml6035_read_als(const struct device *i2c_dev, uint16_t *lux_counts);
int veml6035_enable_interrupt(const struct device *i2c_dev, bool enable);
int veml6035_shutdown(const struct device *i2c_dev);

#endif 
