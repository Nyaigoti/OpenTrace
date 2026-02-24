# Device Firmware

## Project Overview
This project implements the firmware for a cellular-connected package device. The device is designed to detect if a package has been opened and immediately reports via an NB-IoT cellular connection.

The firmware is built on the **nRF Connect SDK (Zephyr RTOS)** and runs on the **Nordic nRF9160 SiP**. It is optimized for ultra-low power consumption, remaining in a deep sleep state until it needs to perform its critical function.

### Key Features
*   **Opening Detection**: Uses a VEML6035 Ambient Light Sensor to detect opening.
*   **Instant Alerting**: Connects to an NB-IoT network to transmit an alert payload.
*   **Ultra-Low Power**: Operates in System OFF mode (~shutdown) to maximize battery life, waking only on specific events.
*   **Robust State Machine**: clearly defined lifecycle (Provisioning -> Arming -> Monitoring -> Triggered).
*   **Double-Tap Reset**: Hidden feature to factory reset the device for re-use.

## Hardware Requirements
To run this firmware, you need the following hardware:

1.  **MCU/Modem**: Nordic Semiconductor **nRF9160** SiP.
2.  **PMIC**: Nordic **NPM1300** Power Management IC.
3.  **Sensor**: Vishay **VEML6035** Ambient Light Sensor (connected via I2C).
4.  **Antenna**: Antenna forLTE-M/NB-IoT bands.
5.  **SIM Card**: An active iBasis or other compatible NB-IoT/LTE-M SIM card.

## Software Architecture
The core logic is driven by a Finite State Machine (FSM) implemented in `src/app/fsm.c`.

### Device Lifecycle
1.  **PROVISIONING (`STATE_PROVISIONING`)**: The initial state after first boot. The device prepares itself for deployment.
2.  **ARMING (`STATE_ARMING`)**: The device waits for a sustained period of darkness (2 minutes by default) to confirm it is inside the package.
3.  **MONITORING (`STATE_MONITORING`)**: The device is "Armed".
    *   It configures the light sensor to fire a hardware interrupt upon detecting light.
    *   It enters **System OFF** (Deep Sleep). The CPU is off.
4.  **TRIGGERED (`STATE_TRIGGERED`)**: Wakes up immediately when the sensor interrupt fires (package opened).
5.  **TRANSMISSION (`STATE_TRANSMISSION`)**:
    *   Initializes the LTE Modem.
    *   Connects to the server
    *   Sends a UDP packet indicating the opening.
    *   Retries up to 3 times if transmission fails.
6.  **TERMINATED (`STATE_TERMINATED`)**: Final state. The device shuts down sensors and modem and enters permanent deep sleep to save power.

