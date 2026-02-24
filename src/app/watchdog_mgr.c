#include "watchdog_mgr.h"
#include <zephyr/drivers/watchdog.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

static const struct device *wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));
static int wdt_channel_id;

int watchdog_mgr_init(uint32_t timeout_ms)
{
    if (!device_is_ready(wdt)) {
        return -1;
    }

    struct wdt_timeout_cfg wdt_config = {
        .window.min = 0,
        .window.max = timeout_ms,
        .callback = NULL, // Reset on expiration
        .flags = WDT_FLAG_RESET_SOC,
    };

    wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
    if (wdt_channel_id < 0) {
        return wdt_channel_id;
    }

    return wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
}

void watchdog_mgr_kick(void)
{
    wdt_feed(wdt, wdt_channel_id);
}
