#include "fsm.h"
#include "storage.h"
#include "payload.h"
#include "watchdog_mgr.h" 
#include "../drivers/veml6035.h"
#include "../drivers/npm1300.h"
#include "../power/power_mgr.h" 
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/sys/reboot.h>
#include <errno.h>

LOG_MODULE_REGISTER(fsm);

#include <hal/nrf_power.h> // For GPREGRET

/* Double Tap Magic Value */
#define DOUBLE_RESET_MAGIC 0xA5
#define MIN_BOOT_WINDOW_MS 2000

static int64_t boot_time_ms = 0;

/* Hardware Definitions */
#define I2C_DEV_NODE DT_NODELABEL(i2c2)
static const struct device *i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);

#define SENSOR_INT_NODE DT_ALIAS(veml_int)
static const struct gpio_dt_spec sensor_int = GPIO_DT_SPEC_GET(SENSOR_INT_NODE, gpios);

/* Networking Config */
#define SERVER_ADDR "203.0.113.10" /* TEST-NET-3 Public IP (Replace with actual) */
#define SERVER_PORT 5000

static enum app_state current_state = STATE_BOOT;

// Forward declarations
static void process_provisioning(void);
static void process_arming(void);
static void process_monitoring(void);
static void process_triggered(void);
static void process_transmission(void);
static void process_termination(void);

int fsm_init(void)
{
    int rc;
    uint32_t flags = 0;

    // Double Tap Reset Check
    uint8_t gpregret = (uint8_t)nrf_power_gpregret_get(NRF_POWER, 0);
    
    if (gpregret == DOUBLE_RESET_MAGIC) {
        LOG_WRN("!!! DOUBLE TAP DETECTED - FACTORY RESET !!!");
        
        nrf_power_gpregret_set(NRF_POWER, 0, 0); 
        
        storage_init();
        storage_reset();
        
        LOG_INF("Reset complete. Rebooting...");
        k_sleep(K_SECONDS(1));
        sys_reboot(SYS_REBOOT_COLD);
    } else {
        nrf_power_gpregret_set(NRF_POWER, 0, DOUBLE_RESET_MAGIC);
    }

    boot_time_ms = k_uptime_get();

    // Storage Init
    rc = storage_init();
    if (rc < 0) {
        LOG_ERR("Storage init failed");
        return rc;
    }

    // State Restoration
    storage_get_flags(&flags);

    if (flags & FLAG_TERMINATED) {
        current_state = STATE_TERMINATED;
    } else if (flags & FLAG_TRIGGERED) {
        current_state = STATE_TRANSMISSION; 
    } else if (flags & FLAG_PROVISIONED) {
        current_state = STATE_MONITORING;
    } else {
        current_state = STATE_PROVISIONING;
    }

    // Hardware Checks 
    if (current_state != STATE_TERMINATED) {
        if (!device_is_ready(i2c_dev)) {
            LOG_ERR("I2C device not ready");
            return -ENODEV;
        }

        if (!gpio_is_ready_dt(&sensor_int)) {
            LOG_ERR("Sensor GPIO not ready");
            return -ENODEV;
        }
        gpio_pin_configure_dt(&sensor_int, GPIO_INPUT);
        
        // Initialize Modem - DEFERRED to STATE_TRANSMISSION
        // rc = nrf_modem_lib_init();
        // if (rc < 0) {
        //      LOG_ERR("Modem init failed: %d", rc);
        // }

        
        // Init Sensor
        veml6035_init(i2c_dev);
    }
    
    // Check Wake Reason / Sensor State for Immediate Trigger
    if (current_state == STATE_MONITORING) {
         // If we woke up from System OFF, and GPIO is high -> Triggered
         int pin_state = gpio_pin_get_dt(&sensor_int);
         if (pin_state == 1) {
             LOG_INF("Wakeup detected on Sensor Pin!");
             current_state = STATE_TRIGGERED;
         }
    }

    LOG_INF("FSM Init Complete. State=%d", current_state);
    return 0;
}

int fsm_run(void)
{
    // Clear Double Tap Magic if window expired
    int64_t now = k_uptime_get();
    if ((now - boot_time_ms) > MIN_BOOT_WINDOW_MS) {
        if ((uint8_t)nrf_power_gpregret_get(NRF_POWER, 0) == DOUBLE_RESET_MAGIC) {
            nrf_power_gpregret_set(NRF_POWER, 0, 0);
        }
    }

    switch (current_state) {
        case STATE_PROVISIONING:
            process_provisioning();
            break;
        case STATE_ARMING:
            process_arming();
            break;
        case STATE_MONITORING:
            process_monitoring();
            break;
        case STATE_TRIGGERED:
            process_triggered();
            break;
        case STATE_TRANSMISSION:
            process_transmission(); 
            break;
        case STATE_TERMINATED:
            process_termination();
            break;
        default:
            break;
    }
    return 0;
}

static void process_provisioning(void)
{
    LOG_INF("State: PROVISIONING");
    current_state = STATE_ARMING;
}

static void process_arming(void)
{
    LOG_INF("State: ARMING (Waiting for Darkness)");
    
    // We need to confirm darkness for 2 mins
    int consecutive_dark_seconds = 0;
    const int target_dark_seconds = 120;
    uint16_t lux_counts = 0;
    int rc;
    
    veml6035_configure(i2c_dev);

    while (consecutive_dark_seconds < target_dark_seconds) {
        rc = veml6035_read_als(i2c_dev, &lux_counts);
        if (rc < 0) {
            LOG_ERR("Failed to read sensor");
            k_sleep(K_SECONDS(1));
            continue;
        }

        LOG_INF("Arming: Lux Counts=%d", lux_counts);

        // Threshold verify (5 counts ~ 0.05 lux)
        if (lux_counts < 5) {
            consecutive_dark_seconds++;
            LOG_INF("Darkness detected (%d/%d)", consecutive_dark_seconds, target_dark_seconds);
        } else {
            consecutive_dark_seconds = 0;
            LOG_INF("Light detected! Resetting arming timer.");
        }

        watchdog_mgr_kick();
        k_sleep(K_SECONDS(1));
    }
    
    LOG_INF("Arming Complete! Locking device.");
    storage_set_flag(FLAG_PROVISIONED);
    current_state = STATE_MONITORING;
}

// Helper to safely sleep
static void fsm_secure_sleep(void)
{
    int64_t now = k_uptime_get();
    int64_t diff = now - boot_time_ms;
    
    if (diff < MIN_BOOT_WINDOW_MS) {
        LOG_INF("Holding for Double Tap Window (%lld ms remaining)...", (MIN_BOOT_WINDOW_MS - diff));
        k_sleep(K_MSEC(MIN_BOOT_WINDOW_MS - diff));
    }

    nrf_power_gpregret_set(NRF_POWER, 0, 0);
    k_sleep(K_MSEC(100));    
    power_mgr_system_off();
}

static void process_monitoring(void)
{
    LOG_INF("State: MONITORING");
    
    // Arm Sensor
    veml6035_configure(i2c_dev);
    
    // Configure Wakeup GPIO
    gpio_pin_interrupt_configure_dt(&sensor_int, GPIO_INT_LEVEL_ACTIVE);
    
    // Enter System OFF (Deep Sleep)
    LOG_INF("Entering System OFF...");
    fsm_secure_sleep();
}

static void process_triggered(void)
{
    LOG_INF("State: TRIGGERED");
    storage_set_flag(FLAG_TRIGGERED);
    current_state = STATE_TRANSMISSION;
}

static void process_transmission(void)
{
    LOG_INF("State: TRANSMISSION");
    
    // Defer Modem Init to here
    int err = power_mgr_modem_init();
    if (err) {
        LOG_ERR("Modem init failed: %d", err);
        storage_set_flag(FLAG_TERMINATED);
        current_state = STATE_TERMINATED;
        return;
    }
    
    int sock = -1;
    struct sockaddr_in server;
    bool success = false;
    int retries_left = 3;

    seal_payload_t pkt = {0};
    pkt.status_code = 0x01; 
    uint8_t raw_buf[PAYLOAD_SIZE];
    payload_encode(&pkt, raw_buf);

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_ADDR, &server.sin_addr);

    while (!success && retries_left > 0) {
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock < 0) {
            LOG_ERR("Socket fail: %d", errno);
            goto retry;
        }

        // Set Socket Timeouts (Fix 5)
        struct timeval timeout = {
            .tv_sec = 60,
            .tv_usec = 0,
        };
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        err = connect(sock, (struct sockaddr *)&server, sizeof(server));
        if (err < 0) {
            LOG_ERR("Connect fail: %d", errno); 
            close(sock);
            goto retry;
        }

        err = send(sock, raw_buf, sizeof(seal_payload_t), 0);
        if (err < 0) {
             LOG_ERR("Send fail: %d", errno);
             close(sock);
             goto retry;
        } else {
             LOG_INF("Payload Sent!");
             success = true;
             close(sock);
             break;
        }

    retry:
        if (!success) {
            LOG_WRN("Retrying tx...");
            retries_left--;
            watchdog_mgr_kick();
            k_sleep(K_SECONDS(60));
        }
    }

    if (success) {
        storage_set_flag(FLAG_TERMINATED);
        current_state = STATE_TERMINATED;
    } else {
        LOG_ERR("Transmission Failed.");
        storage_set_flag(FLAG_TERMINATED); 
        current_state = STATE_TERMINATED;
    }
}

static void process_termination(void)
{
    LOG_INF("State: TERMINATED");
    veml6035_shutdown(i2c_dev);
    npm1300_hibernate(i2c_dev); 
    
    fsm_secure_sleep();
}

