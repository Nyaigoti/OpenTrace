#include "veml6035.h"
#include <zephyr/logging/log.h>
#include <errno.h>

LOG_MODULE_REGISTER(veml6035);

#define VEML6035_CONF_SD_POS      0
#define VEML6035_CONF_INT_EN_POS  1
#define VEML6035_CONF_SENS_POS    12
#define VEML6035_CONF_IT_POS      6

// Write
static int veml6035_write_reg(const struct device *i2c_dev, uint8_t reg, uint16_t value)
{
    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = (uint8_t)(value & 0xFF);
    buf[2] = (uint8_t)((value >> 8) & 0xFF);
    return i2c_write(i2c_dev, buf, 3, VEML6035_I2C_ADDR);
}

// Read 
static int veml6035_read_reg(const struct device *i2c_dev, uint8_t reg, uint16_t *value)
{
    uint8_t buf[2];
    int ret = i2c_write_read(i2c_dev, VEML6035_I2C_ADDR, &reg, 1, buf, 2);
    if (ret == 0) {
        *value = (buf[1] << 8) | buf[0];
    }
    return ret;
}

int veml6035_read_als(const struct device *i2c_dev, uint16_t *lux_counts)
{
    return veml6035_read_reg(i2c_dev, VEML6035_REG_ALS, lux_counts);
}

int veml6035_init(const struct device *i2c_dev)
{
    if (!device_is_ready(i2c_dev)) {
        return -ENODEV;
    }
    
    uint16_t dummy_val;
    int ret = veml6035_read_reg(i2c_dev, VEML6035_REG_ALS_CONF, &dummy_val);
    if (ret < 0) {
        LOG_ERR("VEML6035 not found!");
        return -ENODEV;
    }

    return 0;
}

int veml6035_configure(const struct device *i2c_dev)
{
    int ret = 0;
    
    // 1. Shutdown first to configure safely
    ret = veml6035_shutdown(i2c_dev);
    if (ret < 0) return ret;

    // 2. Set Thresholds for Interrupt
    uint16_t high_threshold = 0x00A0; 
    uint16_t low_threshold = 0x0000;

    ret = veml6035_write_reg(i2c_dev, VEML6035_REG_ALS_WH, high_threshold);
    if (ret < 0) return ret;
    ret = veml6035_write_reg(i2c_dev, VEML6035_REG_ALS_WL, low_threshold);
    if (ret < 0) return ret;

    // 3. Configure and Enable Interrupt
    uint16_t conf = 0;
    conf |= (0 << VEML6035_CONF_SD_POS);       // Power ON
    conf |= (1 << VEML6035_CONF_INT_EN_POS);   // Interrupt Enable
    conf |= (0 << VEML6035_CONF_SENS_POS);     // Sensitivity x1 (Normal)
    
    // Write Configuration
    ret = veml6035_write_reg(i2c_dev, VEML6035_REG_ALS_CONF, conf);
    
    return ret;
}

int veml6035_enable_interrupt(const struct device *i2c_dev, bool enable)
{
    uint16_t conf = 0;
    int ret = veml6035_read_reg(i2c_dev, VEML6035_REG_ALS_CONF, &conf);
    if (ret < 0) return ret;

    if (enable) {
        conf |= (1 << VEML6035_CONF_INT_EN_POS);
    } else {
        conf &= ~(1 << VEML6035_CONF_INT_EN_POS);
    }

    return veml6035_write_reg(i2c_dev, VEML6035_REG_ALS_CONF, conf);
}

int veml6035_shutdown(const struct device *i2c_dev)
{
    // Set SD bit = 1
    return veml6035_write_reg(i2c_dev, VEML6035_REG_ALS_CONF, (1 << VEML6035_CONF_SD_POS));
}