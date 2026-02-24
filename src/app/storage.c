#include "storage.h"
#include <zephyr/drivers/flash.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_REGISTER(storage);

static struct nvs_fs fs;
#define NVS_PARTITION		storage_partition 

int storage_init(void)
{
    int rc;
    struct flash_pages_info info;

    /* Define the Flash device and offset from the Device Tree Partition */
    fs.flash_device = FIXED_PARTITION_DEVICE(NVS_PARTITION);
    if (!device_is_ready(fs.flash_device)) {
        LOG_ERR("Flash device not ready");
        return -1;
    }

    fs.offset = FIXED_PARTITION_OFFSET(NVS_PARTITION);
    
    /* Get sector info to configure NVS */
    rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
    if (rc) {
        LOG_ERR("Unable to get page info");
        return rc;
    }
    
    fs.sector_size = info.size;
    fs.sector_count = 3U; 

    rc = nvs_mount(&fs);
    if (rc) {
        LOG_ERR("NVS mount failed: %d", rc);
        return rc;
    }

    return 0;
}

int storage_set_flag(uint32_t flag)
{
    uint32_t current_flags = 0;
    storage_get_flags(&current_flags);
    
    current_flags |= flag;
    
    return nvs_write(&fs, NVS_ID_STATE_FLAGS, &current_flags, sizeof(current_flags));
}

int storage_get_flags(uint32_t *flags)
{
    int rc = nvs_read(&fs, NVS_ID_STATE_FLAGS, flags, sizeof(uint32_t));
    if (rc > 0) {
        // Success
        return 0; 
    } else if (rc == -ENOENT) {
        // Not found, first run
        *flags = 0;
        return 0;   
    }
    return rc; 
}

int storage_reset(void)
{
    // Delete the NVS ID to wipe all flags
    int rc = nvs_delete(&fs, NVS_ID_STATE_FLAGS);
    if (rc == 0) {
        LOG_INF("*** STORAGE FACTORY RESET ***");
    } else if (rc == -ENOENT) {
        LOG_WRN("Reset requested but storage was empty.");
        return 0; 
    } else {
        LOG_ERR("Failed to reset storage: %d", rc);
    }
    return rc;
}
