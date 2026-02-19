#ifndef OTA_CORE_TYPES_H
#define OTA_CORE_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Fixed-size ID types */
#define OTA_UUID_STR_LEN 37  /* 36 chars + null terminator */

typedef char ota_uuid_t[OTA_UUID_STR_LEN];

/* Artifact type */
typedef enum {
    OTA_ARTIFACT_SOFTWARE = 0,
    OTA_ARTIFACT_FIRMWARE = 1,
} ota_artifact_type_t;

/* Transport type */
typedef enum {
    OTA_TRANSPORT_UNIX = 0,
    OTA_TRANSPORT_TCP  = 1,
} ota_transport_type_t;

/* Agent OS type */
typedef enum {
    OTA_OS_LINUX    = 0,
    OTA_OS_FREERTOS = 1,
} ota_os_type_t;

/* Agent connection status */
typedef enum {
    OTA_AGENT_STATUS_REGISTERED  = 0,
    OTA_AGENT_STATUS_ACTIVE      = 1,
    OTA_AGENT_STATUS_UNREACHABLE = 2,
} ota_agent_status_t;

/* Maximum string lengths */
#define OTA_MAX_PACKAGE_ID_LEN    64
#define OTA_MAX_VERSION_LEN       32
#define OTA_MAX_NODE_ID_LEN       64
#define OTA_MAX_HW_REV_LEN        32
#define OTA_MAX_ARCH_LEN          16
#define OTA_MAX_REGION_LEN        8
#define OTA_MAX_PATH_LEN          256
#define OTA_MAX_SHA256_HEX_LEN    65  /* 64 hex chars + null */
#define OTA_MAX_ERROR_MSG_LEN     256
#define OTA_MAX_LABEL_LEN         32
#define OTA_MAX_PLUGIN_NAME_LEN   32

/* SHA-256 digest (raw bytes) */
#define OTA_SHA256_DIGEST_LEN     32

/* CRC32 value type */
typedef uint32_t ota_crc32_t;

#endif /* OTA_CORE_TYPES_H */
