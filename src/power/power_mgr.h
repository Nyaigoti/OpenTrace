#ifndef POWER_MGR_H
#define POWER_MGR_H

#include <zephyr/kernel.h>

/**
 * @brief Initialize power management subsystems (if any specific init needed)
 */


/**
 * @brief Initialize the modem library and track its state.
 * 
 * @return 0 on success, negative errno code on failure.
 */
int power_mgr_modem_init(void);

/**
 * @brief Enter System OFF state (Deep Sleep)
 * 
 * This function will shut down the modem, disable peripherals, and 
 * put the nRF9160 into System OFF mode. It does NOT return (wake happens via reset).
 */
void power_mgr_system_off(void);

#endif // POWER_MGR_H
