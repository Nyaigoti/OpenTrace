#include "npm1300.h"
#include <zephyr/logging/log.h>
#include <errno.h>

LOG_MODULE_REGISTER(npm1300, CONFIG_LOG_DEFAULT_LEVEL);

// Helper for register writing
static int npm1300_write_reg(const struct device *i2c_dev, uint16_t reg, uint8_t value)
{
    // Format: [RegAddr High] [RegAddr Low] [Data]
    uint8_t buf[3];
    buf[0] = (uint8_t)((reg >> 8) & 0xFF);
    buf[1] = (uint8_t)(reg & 0xFF);
    buf[2] = value;
    
    return i2c_write(i2c_dev, buf, 3, NPM1300_I2C_ADDR);
}

// Helper for register reading
static int npm1300_read_reg(const struct device *i2c_dev, uint16_t reg, uint8_t *value)
{
    uint8_t reg_addr[2];
    reg_addr[0] = (uint8_t)((reg >> 8) & 0xFF);
    reg_addr[1] = (uint8_t)(reg & 0xFF);
    
    return i2c_write_read(i2c_dev, NPM1300_I2C_ADDR, reg_addr, 2, value, 1);
}

int npm1300_init(const struct device *i2c_dev)
{
    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }

    uint8_t dummy;
    int ret = npm1300_read_reg(i2c_dev, NPM1300_REG_BUCK1_STATUS, &dummy);
    if (ret < 0) {
        LOG_ERR("NPM1300 not found at 0x%02X", NPM1300_I2C_ADDR);
        return -ENODEV;
    }

    LOG_INF("NPM1300 found! ID/Status Val: 0x%02X", dummy);
    return 0;
}

int npm1300_enable_bucks(const struct device *i2c_dev)
{
    int ret = 0;

    // 1. Configure BUCK1 (System/Modem) -> 3.0V
    LOG_INF("Setting BUCK1 to 3.0V...");
    ret = npm1300_write_reg(i2c_dev, NPM1300_REG_BUCK1_NORM_VOUT, NPM1300_VOUT_3V0);
    if (ret < 0) return ret;

    // Enable BUCK1
    ret = npm1300_write_reg(i2c_dev, NPM1300_REG_BUCK1_CTRL, NPM1300_BUCK_CTRL_ENABLE);
    if (ret < 0) return ret;


    // 2. Configure BUCK2 (GPIO/Aux) -> 1.8V
    LOG_INF("Setting BUCK2 to 1.8V...");
    ret = npm1300_write_reg(i2c_dev, NPM1300_REG_BUCK2_NORM_VOUT, NPM1300_VOUT_1V8);
    if (ret < 0) return ret;

    // Enable BUCK2
    ret = npm1300_write_reg(i2c_dev, NPM1300_REG_BUCK2_CTRL, NPM1300_BUCK_CTRL_ENABLE);
    if (ret < 0) return ret;

    LOG_INF("NPM1300 Bucks Enabled.");
    return 0;
}

int npm1300_hibernate(const struct device *i2c_dev)
{
    int ret = 0;
    
    LOG_INF("Hibernating NPM1300 (Disabling BUCKs)...");

    // Disable BUCK2
    ret = npm1300_write_reg(i2c_dev, NPM1300_REG_BUCK2_CTRL, 0x00); // Disable
    if (ret < 0) {
        LOG_ERR("Failed to disable BUCK2");
    }

    // Disable BUCK1 (System Power usually)
    ret = npm1300_write_reg(i2c_dev, NPM1300_REG_BUCK1_CTRL, 0x00); // Disable
    if (ret < 0) {
        LOG_ERR("Failed to disable BUCK1");
    }

    return ret;
}
