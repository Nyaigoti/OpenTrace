#ifndef STORAGE_H
#define STORAGE_H

#include <zephyr/types.h>

/* NVS IDs */
#define NVS_ID_STATE_FLAGS 1

/* Flags */
#define FLAG_PROVISIONED  (1 << 0)
#define FLAG_TRIGGERED    (1 << 1)
#define FLAG_TERMINATED   (1 << 2)

int storage_init(void);

int storage_set_flag(uint32_t flag);
int storage_get_flags(uint32_t *flags);
/**
 * @brief Wipes the state flags from NVS (Factory Reset).
 * @return 0 on success.
 */
int storage_reset(void);

#endif
