#include "power_mgr.h"
#include <zephyr/sys/poweroff.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(main);

#include "../app/watchdog_mgr.h"
#include <zephyr/kernel.h>

static bool modem_active = false;
static K_SEM_DEFINE(lte_connected, 0, 1);

static void lte_handler(const struct lte_lc_evt *const evt)
{
     switch (evt->type) {
     case LTE_LC_EVT_NW_REG_STATUS:
        if ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
             (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
             LOG_INF("Network Registered!");
             k_sem_give(&lte_connected);
        }
        break;
     default:
        break;
     }
}

int power_mgr_modem_init(void)
{
    int err = nrf_modem_lib_init();
    if (err) {
        LOG_ERR("Modem lib init failed: %d", err);
        return err;
    }
    
    LOG_INF("Connecting to LTE network (Async)...");
    
    err = lte_lc_connect_async(lte_handler);
    if (err) {
        LOG_ERR("LTE connection request failed: %d", err);
        return err;
    }

    int retries = 300; // 300 seconds timeout
    while (k_sem_take(&lte_connected, K_NO_WAIT) != 0) {
        if (retries-- <= 0) {
            LOG_ERR("LTE Connection Timeout!");
            lte_lc_power_off();
            return -ETIMEDOUT;
        }
        watchdog_mgr_kick();
        k_sleep(K_SECONDS(1));
    }

    LOG_INF("LTE Connected!");
    modem_active = true;
    return 0;
}

void power_mgr_system_off(void)
{
    // Shutdown Modem
    if (modem_active) {
        LOG_INF("Shutting down LTE...");
        lte_lc_power_off();
        nrf_modem_lib_shutdown();
        modem_active = false;
    }

    // Enter System OFF
    LOG_INF("System Power Off");
    sys_poweroff();
}
