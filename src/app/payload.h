#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <zephyr/types.h>

#define PAYLOAD_SIZE 17

typedef struct {
    uint8_t device_id[16]; // UUID or Serial
    uint8_t status_code;   // 0x01 = Opened
} __packed seal_payload_t;

void payload_encode(seal_payload_t *payload, uint8_t *buffer);

#endif
