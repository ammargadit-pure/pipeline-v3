#ifndef OTA_CORE_PROTOCOL_H
#define OTA_CORE_PROTOCOL_H

#include "ota/core/types.h"
#include "ota/core/error.h"

#define OTA_PROTOCOL_MAGIC     0x4F54   /* "OT" */
#define OTA_PROTOCOL_VERSION   0x01
#define OTA_HEADER_SIZE        12
#define OTA_MAX_PAYLOAD_SIZE   (16 * 1024 * 1024)  /* 16 MiB */

typedef struct __attribute__((packed)) {
    uint16_t magic;           /* OTA_PROTOCOL_MAGIC (network byte order) */
    uint8_t  version;         /* OTA_PROTOCOL_VERSION */
    uint8_t  msg_type;        /* ota_msg_type_t value */
    uint32_t payload_length;  /* Payload bytes (network byte order) */
    uint32_t sequence_num;    /* Per-connection monotonic (network byte order) */
} ota_header_t;

/* Message type enum */
typedef enum {
    OTA_MSG_REGISTER_REQ      = 0x01,
    OTA_MSG_REGISTER_ACK      = 0x02,
    OTA_MSG_HEARTBEAT         = 0x10,
    OTA_MSG_HEARTBEAT_ACK     = 0x11,
    OTA_MSG_TRANSFER_START    = 0x20,
    OTA_MSG_TRANSFER_DATA     = 0x21,
    OTA_MSG_TRANSFER_ACK      = 0x22,
    OTA_MSG_TRANSFER_NACK     = 0x23,
    OTA_MSG_TRANSFER_COMPLETE = 0x24,
    OTA_MSG_INSTALL_CMD       = 0x30,
    OTA_MSG_ACTIVATE_CMD      = 0x31,
    OTA_MSG_ROLLBACK_CMD      = 0x32,
    OTA_MSG_STATUS_REPORT     = 0x40,
    OTA_MSG_HEALTH_REPORT     = 0x41,
    OTA_MSG_CAMPAIGN_STATUS   = 0x50,
} ota_msg_type_t;

/* Serialize header to network byte order buffer (exactly OTA_HEADER_SIZE bytes) */
ota_error_t ota_header_serialize(const ota_header_t *hdr, uint8_t *buf, size_t buf_len);

/* Deserialize header from network byte order buffer */
ota_error_t ota_header_deserialize(const uint8_t *buf, size_t buf_len, ota_header_t *out);

/* Validate header fields (magic, version, payload size) */
ota_error_t ota_header_validate(const ota_header_t *hdr);

/* Check if message type uses binary payload (TRANSFER_DATA) vs JSON */
bool ota_msg_is_binary(ota_msg_type_t type);

/* Convert message type to string for logging */
const char *ota_msg_type_str(ota_msg_type_t type);

#endif /* OTA_CORE_PROTOCOL_H */
