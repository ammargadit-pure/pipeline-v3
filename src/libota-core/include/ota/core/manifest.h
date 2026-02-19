#ifndef OTA_CORE_MANIFEST_H
#define OTA_CORE_MANIFEST_H

#include "ota/core/types.h"
#include "ota/core/error.h"

/* Manifest JSON key names — EXACT strings */
#define OTA_MANIFEST_KEY_PACKAGE_ID       "package_identifier"
#define OTA_MANIFEST_KEY_VERSION          "version"
#define OTA_MANIFEST_KEY_ARTIFACT_TYPE    "artifact_type"
#define OTA_MANIFEST_KEY_NODE_ID          "node_identifier"
#define OTA_MANIFEST_KEY_MIN_OS_VER       "minimum_os_version"
#define OTA_MANIFEST_KEY_MAX_OS_VER       "maximum_os_version"
#define OTA_MANIFEST_KEY_MIN_SDK_VER      "minimum_sdk_version"
#define OTA_MANIFEST_KEY_MAX_SDK_VER      "maximum_sdk_version"
#define OTA_MANIFEST_KEY_REGION           "targeted_region"
#define OTA_MANIFEST_KEY_OEM_RULES        "custom_oem_rules"
#define OTA_MANIFEST_KEY_SIZE             "size"
#define OTA_MANIFEST_KEY_CRC32            "crc32"
#define OTA_MANIFEST_KEY_SHA256           "sha256"
#define OTA_MANIFEST_KEY_CREATED_AT       "created_at"

/* Parsed manifest structure */
typedef struct {
    char              package_id[OTA_MAX_PACKAGE_ID_LEN];
    char              version[OTA_MAX_VERSION_LEN];
    ota_artifact_type_t artifact_type;
    char              node_identifier[OTA_MAX_NODE_ID_LEN];
    char              min_os_version[OTA_MAX_VERSION_LEN];
    char              max_os_version[OTA_MAX_VERSION_LEN];
    char              min_sdk_version[OTA_MAX_VERSION_LEN];
    char              max_sdk_version[OTA_MAX_VERSION_LEN];
    char              targeted_region[OTA_MAX_REGION_LEN];
    char              *custom_oem_rules_json;  /* Raw JSON string, heap-allocated. NULL if absent. */
    uint64_t          size;
    ota_crc32_t       crc32;
    char              sha256[OTA_MAX_SHA256_HEX_LEN];
    char              created_at[32];           /* ISO 8601 string */
} ota_manifest_t;

/* Parse manifest from JSON string. Caller must call ota_manifest_free(). */
ota_error_t ota_manifest_parse(const char *json_str, size_t json_len, ota_manifest_t *out);

/* Free heap-allocated fields in manifest. */
void ota_manifest_free(ota_manifest_t *manifest);

/* Validate all required fields are present and well-formed. */
ota_error_t ota_manifest_validate(const ota_manifest_t *manifest);

/* Serialize manifest to JSON string. Caller frees returned pointer. */
char *ota_manifest_serialize(const ota_manifest_t *manifest);

#endif /* OTA_CORE_MANIFEST_H */
