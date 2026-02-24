#ifndef FSM_H
#define FSM_H

/**
 * @file fsm.h
 * @brief Finite State Machine for Security Seal
 */

enum app_state {
    STATE_BOOT,
    STATE_PROVISIONING,
    STATE_ARMING,
    STATE_MONITORING,
    STATE_TRIGGERED,
    STATE_TRANSMISSION,
    STATE_TERMINATED
};

/**
 * @brief Initialize the application FSM.
 * Restores state from NVS.
 * @return 0 on success.
 */
int fsm_init(void);

/**
 * @brief Execute one iteration of the FSM.
 * This should be called in the main loop.
 */
int fsm_run(void);

#endif
