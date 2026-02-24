#ifndef WATCHDOG_MGR_H
#define WATCHDOG_MGR_H

#include <stdint.h>

/**
 * @brief Initialize the Independent Watchdog (WDT)
 * 
 * @param timeout_ms Safety timeout (e.g., 180000 for 3 minutes)
 * @return int 0 on success
 */
int watchdog_mgr_init(uint32_t timeout_ms);

/**
 * @brief Kick the watchdog (if needed, though mostly we use it as a hard limit)
 */
void watchdog_mgr_kick(void);

#endif
