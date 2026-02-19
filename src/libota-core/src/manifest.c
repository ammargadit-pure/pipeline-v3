#include "ota/core/manifest.h"
#include <cJSON.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void safe_strcpy(char *dst, size_t dst_size, const char *src)
{
    if (!src) {
        dst[0] = '\0';
        return;
    }
    size_t len = strlen(src);
    if (len >= dst_size) {
        len = dst_size - 1;
    }
    memcpy(dst, src, len);
    dst[len] = '\0';
}

static const char *cjson_get_string(const cJSON *obj, const char *key)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsString(item) && item->valuestring) {
        return item->valuestring;
    }
    return NULL;
}

ota_error_t ota_manifest_parse(const char *json_str, size_t json_len, ota_manifest_t *out)
{
    if (!json_str || !out) {
        return OTA_ERR_INVALID_ARGUMENT;
    }

    (void)json_len;

    memset(out, 0, sizeof(*out));

    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        return OTA_ERR_PROTOCOL_PARSE;
    }

    /* Required fields */
    const char *pkg_id = cjson_get_string(root, OTA_MANIFEST_KEY_PACKAGE_ID);
    const char *version = cjson_get_string(root, OTA_MANIFEST_KEY_VERSION);
    const char *artifact = cjson_get_string(root, OTA_MANIFEST_KEY_ARTIFACT_TYPE);
    const char *node_id = cjson_get_string(root, OTA_MANIFEST_KEY_NODE_ID);

    if (!pkg_id || !version || !artifact || !node_id) {
        cJSON_Delete(root);
        return OTA_ERR_INVALID_ARGUMENT;
    }

    safe_strcpy(out->package_id, sizeof(out->package_id), pkg_id);
    safe_strcpy(out->version, sizeof(out->version), version);
    safe_strcpy(out->node_identifier, sizeof(out->node_identifier), node_id);

    if (strcmp(artifact, "firmware") == 0) {
        out->artifact_type = OTA_ARTIFACT_FIRMWARE;
    } else {
        out->artifact_type = OTA_ARTIFACT_SOFTWARE;
    }

    /* Optional fields */
    const char *min_os = cjson_get_string(root, OTA_MANIFEST_KEY_MIN_OS_VER);
    safe_strcpy(out->min_os_version, sizeof(out->min_os_version), min_os);

    const char *max_os = cjson_get_string(root, OTA_MANIFEST_KEY_MAX_OS_VER);
    safe_strcpy(out->max_os_version, sizeof(out->max_os_version), max_os);

    const char *min_sdk = cjson_get_string(root, OTA_MANIFEST_KEY_MIN_SDK_VER);
    safe_strcpy(out->min_sdk_version, sizeof(out->min_sdk_version), min_sdk);

    const char *max_sdk = cjson_get_string(root, OTA_MANIFEST_KEY_MAX_SDK_VER);
    safe_strcpy(out->max_sdk_version, sizeof(out->max_sdk_version), max_sdk);

    const char *region = cjson_get_string(root, OTA_MANIFEST_KEY_REGION);
    safe_strcpy(out->targeted_region, sizeof(out->targeted_region), region);

    const char *sha256 = cjson_get_string(root, OTA_MANIFEST_KEY_SHA256);
    safe_strcpy(out->sha256, sizeof(out->sha256), sha256);

    const char *created = cjson_get_string(root, OTA_MANIFEST_KEY_CREATED_AT);
    safe_strcpy(out->created_at, sizeof(out->created_at), created);

    /* Numeric fields */
    const cJSON *size_item = cJSON_GetObjectItemCaseSensitive(root, OTA_MANIFEST_KEY_SIZE);
    if (cJSON_IsNumber(size_item)) {
        out->size = (uint64_t)size_item->valuedouble;
    }

    const cJSON *crc_item = cJSON_GetObjectItemCaseSensitive(root, OTA_MANIFEST_KEY_CRC32);
    if (cJSON_IsNumber(crc_item)) {
        out->crc32 = (ota_crc32_t)crc_item->valuedouble;
    }

    /* OEM rules (heap-allocated raw JSON) */
    const cJSON *oem_item = cJSON_GetObjectItemCaseSensitive(root, OTA_MANIFEST_KEY_OEM_RULES);
    if (oem_item && !cJSON_IsNull(oem_item)) {
        char *oem_str = cJSON_PrintUnformatted(oem_item);
        out->custom_oem_rules_json = oem_str;
    }

    cJSON_Delete(root);
    return OTA_OK;
}

void ota_manifest_free(ota_manifest_t *manifest)
{
    if (!manifest) return;
    if (manifest->custom_oem_rules_json) {
        free(manifest->custom_oem_rules_json);
        manifest->custom_oem_rules_json = NULL;
    }
}

ota_error_t ota_manifest_validate(const ota_manifest_t *manifest)
{
    if (!manifest) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    if (manifest->package_id[0] == '\0') {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    if (manifest->version[0] == '\0') {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    if (manifest->node_identifier[0] == '\0') {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    return OTA_OK;
}

char *ota_manifest_serialize(const ota_manifest_t *manifest)
{
    if (!manifest) return NULL;

    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON_AddStringToObject(root, OTA_MANIFEST_KEY_PACKAGE_ID, manifest->package_id);
    cJSON_AddStringToObject(root, OTA_MANIFEST_KEY_VERSION, manifest->version);
    cJSON_AddStringToObject(root, OTA_MANIFEST_KEY_ARTIFACT_TYPE,
        manifest->artifact_type == OTA_ARTIFACT_FIRMWARE ? "firmware" : "software");
    cJSON_AddStringToObject(root, OTA_MANIFEST_KEY_NODE_ID, manifest->node_identifier);

    if (manifest->min_os_version[0]) {
        cJSON_AddStringToObject(root, OTA_MANIFEST_KEY_MIN_OS_VER, manifest->min_os_version);
    }
    if (manifest->max_os_version[0]) {
        cJSON_AddStringToObject(root, OTA_MANIFEST_KEY_MAX_OS_VER, manifest->max_os_version);
    }
    if (manifest->min_sdk_version[0]) {
        cJSON_AddStringToObject(root, OTA_MANIFEST_KEY_MIN_SDK_VER, manifest->min_sdk_version);
    }
    if (manifest->max_sdk_version[0]) {
        cJSON_AddStringToObject(root, OTA_MANIFEST_KEY_MAX_SDK_VER, manifest->max_sdk_version);
    }
    if (manifest->targeted_region[0]) {
        cJSON_AddStringToObject(root, OTA_MANIFEST_KEY_REGION, manifest->targeted_region);
    }

    if (manifest->size > 0) {
        cJSON_AddNumberToObject(root, OTA_MANIFEST_KEY_SIZE, (double)manifest->size);
    }
    if (manifest->crc32 > 0) {
        cJSON_AddNumberToObject(root, OTA_MANIFEST_KEY_CRC32, (double)manifest->crc32);
    }
    if (manifest->sha256[0]) {
        cJSON_AddStringToObject(root, OTA_MANIFEST_KEY_SHA256, manifest->sha256);
    }
    if (manifest->created_at[0]) {
        cJSON_AddStringToObject(root, OTA_MANIFEST_KEY_CREATED_AT, manifest->created_at);
    }

    if (manifest->custom_oem_rules_json) {
        cJSON *oem = cJSON_Parse(manifest->custom_oem_rules_json);
        if (oem) {
            cJSON_AddItemToObject(root, OTA_MANIFEST_KEY_OEM_RULES, oem);
        }
    }

    char *result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return result;
}
