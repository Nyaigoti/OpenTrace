#include "payload.h"
#include <string.h>

void payload_encode(seal_payload_t *payload, uint8_t *buffer)
{
    // Simple memcpy 
    memcpy(buffer, payload, sizeof(seal_payload_t));
}
