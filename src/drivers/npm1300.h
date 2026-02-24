#ifndef NPM1300_H
#define NPM1300_H

#include <zephyr/drivers/i2c.h>
#include <stdint.h>

// I2C Address (7-bit)
#define NPM1300_I2C_ADDR 0x6B

// Register Definitions
#define NPM1300_REG_BUCK1_NORM_VOUT 0x404
#define NPM1300_REG_BUCK1_CTRL      0x408
#define NPM1300_REG_BUCK1_STATUS    0x40A

#define NPM1300_REG_BUCK2_NORM_VOUT 0x40C
#define NPM1300_REG_BUCK2_CTRL      0x410
#define NPM1300_REG_BUCK2_STATUS    0x412

// Control Bits
#define NPM1300_BUCK_CTRL_ENABLE (1 << 0)

// VOUT = 0.6V + (Code * 0.1V) 
// 3.0V -> (3.0 - 0.6) / 0.1 = 24 (0x18)
// 1.8V -> (1.8 - 0.6) / 0.1 = 12 (0x0C)

#define NPM1300_VOUT_3V0 0x18
#define NPM1300_VOUT_1V8 0x0C

/**
 * @brief Initialize the NPM1300 and verify presence.
 */
int npm1300_init(const struct device *i2c_dev);

/**
 * @brief Enable Buck1 (System/Modem) and Buck2 (GPIO)
 */
int npm1300_enable_bucks(const struct device *i2c_dev);

/**
 * @brief Hibernate the NPM1300 (Disable Bucks)
 */
int npm1300_hibernate(const struct device *i2c_dev);

#endif // NPM1300_H
