/*
 * Copyright (c) 2024
 * Reliable Security Seal Firmware
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include "app/fsm.h"
#include "app/watchdog_mgr.h"
#include "drivers/npm1300.h"

LOG_MODULE_REGISTER(main);

int main(void)
{
    LOG_INF("Security Seal Booting...");

    /* --- LED Indication for Reset/Boot --- */
    const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
    if (gpio_is_ready_dt(&led)) {
        gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
        // Blink 3 times
        for (int i = 0; i < 3; i++) {
            gpio_pin_set_dt(&led, 1);
            k_sleep(K_MSEC(200));
            gpio_pin_set_dt(&led, 0);
            k_sleep(K_MSEC(200));
        }
    } else {
        LOG_ERR("LED device not ready");
    }


    /* --- NPM1300 PMIC Init --- */
    const struct device *pmic_i2c = DEVICE_DT_GET(DT_NODELABEL(i2c1));
    if (npm1300_init(pmic_i2c) == 0) {
        npm1300_enable_bucks(pmic_i2c);
    } else {
        LOG_ERR("NPM1300 Init Failed! Power rails may be down.");
    }

    int rc = watchdog_mgr_init(180000); // 3 minutes
    if (rc < 0) {
        LOG_ERR("Watchdog Init Failed: %d", rc);
    }

    rc = fsm_init();
    if (rc < 0) {
        LOG_ERR("FSM Init Failed: %d", rc);
        return -1;
    }

    // FSM loop (though our FSM currently handles flow in run once then sleep)
    while (1) {
        watchdog_mgr_kick();
        rc = fsm_run();
        if (rc < 0) {
             LOG_ERR("FSM Critical Failure: %d", rc);
        }
        k_sleep(K_SECONDS(1));
    }
}
