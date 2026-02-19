#include "linux_generic_plugin.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <sys/stat.h>

#include "ota/core/error.h"
#include "ota/core/types.h"
#include "cJSON.h"

namespace ota::agent {

struct LinuxGenericCtx {
    std::string base_dir;
    std::string metadata_path;
    std::string slot_a_path;
    std::string slot_b_path;
};

static void init_default_metadata(cJSON *root) {
    cJSON *slot_a = cJSON_CreateObject();
    cJSON_AddNumberToObject(slot_a, "slot_index", 0);
    cJSON_AddStringToObject(slot_a, "label", "slot_a");
    cJSON_AddBoolToObject(slot_a, "is_active", 1);
    cJSON_AddBoolToObject(slot_a, "is_valid", 1);
    cJSON_AddStringToObject(slot_a, "version", "");
    cJSON_AddItemToObject(root, "slot_a", slot_a);

    cJSON *slot_b = cJSON_CreateObject();
    cJSON_AddNumberToObject(slot_b, "slot_index", 1);
    cJSON_AddStringToObject(slot_b, "label", "slot_b");
    cJSON_AddBoolToObject(slot_b, "is_active", 0);
    cJSON_AddBoolToObject(slot_b, "is_valid", 1);
    cJSON_AddStringToObject(slot_b, "version", "");
    cJSON_AddItemToObject(root, "slot_b", slot_b);

    cJSON_AddNumberToObject(root, "boot_count", 0);
    cJSON_AddNumberToObject(root, "boot_limit", 3);
    cJSON_AddBoolToObject(root, "pending_activation", 0);
}

static cJSON *read_metadata(const char *path) {
    FILE *fp = std::fopen(path, "rb");
    if (!fp) {
        /* Create default metadata */
        cJSON *root = cJSON_CreateObject();
        init_default_metadata(root);
        return root;
    }

    std::fseek(fp, 0, SEEK_END);
    long len = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);

    if (len <= 0) {
        std::fclose(fp);
        cJSON *root = cJSON_CreateObject();
        init_default_metadata(root);
        return root;
    }

    auto *buf = static_cast<char *>(std::malloc(static_cast<size_t>(len) + 1));
    if (!buf) {
        std::fclose(fp);
        return nullptr;
    }

    size_t read_bytes = std::fread(buf, 1, static_cast<size_t>(len), fp);
    buf[read_bytes] = '\0';
    std::fclose(fp);

    cJSON *root = cJSON_Parse(buf);
    std::free(buf);

    if (!root) {
        root = cJSON_CreateObject();
        init_default_metadata(root);
    }

    return root;
}

static ota_error_t write_metadata(const char *path, cJSON *root) {
    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str) {
        return OTA_ERR_IO;
    }

    FILE *fp = std::fopen(path, "wb");
    if (!fp) {
        cJSON_free(json_str);
        return OTA_ERR_IO;
    }

    size_t len = std::strlen(json_str);
    size_t written = std::fwrite(json_str, 1, len, fp);
    std::fclose(fp);
    cJSON_free(json_str);

    if (written != len) {
        return OTA_ERR_IO;
    }

    return OTA_OK;
}

static void slot_from_json(cJSON *slot_json, ota_slot_info_t *out) {
    std::memset(out, 0, sizeof(*out));

    cJSON *idx = cJSON_GetObjectItemCaseSensitive(slot_json, "slot_index");
    if (cJSON_IsNumber(idx)) {
        out->slot_index = static_cast<uint8_t>(idx->valueint);
    }

    cJSON *label = cJSON_GetObjectItemCaseSensitive(slot_json, "label");
    if (cJSON_IsString(label) && label->valuestring) {
        std::strncpy(out->label, label->valuestring, OTA_MAX_LABEL_LEN - 1);
        out->label[OTA_MAX_LABEL_LEN - 1] = '\0';
    }

    cJSON *active = cJSON_GetObjectItemCaseSensitive(slot_json, "is_active");
    if (cJSON_IsBool(active)) {
        out->is_active = cJSON_IsTrue(active);
    }

    cJSON *valid = cJSON_GetObjectItemCaseSensitive(slot_json, "is_valid");
    if (cJSON_IsBool(valid)) {
        out->is_valid = cJSON_IsTrue(valid);
    }

    cJSON *ver = cJSON_GetObjectItemCaseSensitive(slot_json, "version");
    if (cJSON_IsString(ver) && ver->valuestring) {
        std::strncpy(out->version, ver->valuestring, OTA_MAX_VERSION_LEN - 1);
        out->version[OTA_MAX_VERSION_LEN - 1] = '\0';
    }
}

static void slot_to_json(cJSON *slot_json, const ota_slot_info_t *slot) {
    cJSON_ReplaceItemInObjectCaseSensitive(slot_json, "slot_index",
                                            cJSON_CreateNumber(slot->slot_index));
    cJSON_ReplaceItemInObjectCaseSensitive(slot_json, "label",
                                            cJSON_CreateString(slot->label));
    cJSON_ReplaceItemInObjectCaseSensitive(slot_json, "is_active",
                                            cJSON_CreateBool(slot->is_active));
    cJSON_ReplaceItemInObjectCaseSensitive(slot_json, "is_valid",
                                            cJSON_CreateBool(slot->is_valid));
    cJSON_ReplaceItemInObjectCaseSensitive(slot_json, "version",
                                            cJSON_CreateString(slot->version));
}

static ota_error_t lg_detect_inactive_slot(void *ctx, ota_slot_info_t *out) {
    if (!ctx || !out) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    auto *c = static_cast<LinuxGenericCtx *>(ctx);

    cJSON *root = read_metadata(c->metadata_path.c_str());
    if (!root) {
        return OTA_ERR_BOOTLOADER_QUERY;
    }

    cJSON *sa = cJSON_GetObjectItemCaseSensitive(root, "slot_a");
    cJSON *sb = cJSON_GetObjectItemCaseSensitive(root, "slot_b");

    ota_slot_info_t slot_a;
    ota_slot_info_t slot_b;
    slot_from_json(sa, &slot_a);
    slot_from_json(sb, &slot_b);

    if (slot_a.is_active) {
        *out = slot_b;
    } else {
        *out = slot_a;
    }

    cJSON_Delete(root);
    return OTA_OK;
}

static ota_error_t lg_stage_firmware(void *ctx, const uint8_t *data,
                                      size_t len, size_t offset) {
    if (!ctx || !data) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    auto *c = static_cast<LinuxGenericCtx *>(ctx);

    /* Determine inactive slot path */
    cJSON *root = read_metadata(c->metadata_path.c_str());
    if (!root) {
        return OTA_ERR_BOOTLOADER_QUERY;
    }

    cJSON *sa = cJSON_GetObjectItemCaseSensitive(root, "slot_a");
    ota_slot_info_t slot_a;
    slot_from_json(sa, &slot_a);

    const std::string &path = slot_a.is_active ? c->slot_b_path : c->slot_a_path;
    cJSON_Delete(root);

    FILE *fp = std::fopen(path.c_str(), offset == 0 ? "wb" : "r+b");
    if (!fp && offset != 0) {
        fp = std::fopen(path.c_str(), "wb");
    }
    if (!fp) {
        return OTA_ERR_BOOTLOADER_STAGE;
    }

    if (offset > 0) {
        if (std::fseek(fp, static_cast<long>(offset), SEEK_SET) != 0) {
            std::fclose(fp);
            return OTA_ERR_BOOTLOADER_STAGE;
        }
    }

    size_t written = std::fwrite(data, 1, len, fp);
    std::fclose(fp);

    if (written != len) {
        return OTA_ERR_BOOTLOADER_STAGE;
    }

    return OTA_OK;
}

static ota_error_t lg_prepare_activation(void *ctx) {
    if (!ctx) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    auto *c = static_cast<LinuxGenericCtx *>(ctx);

    cJSON *root = read_metadata(c->metadata_path.c_str());
    if (!root) {
        return OTA_ERR_BOOTLOADER_ACTIVATE;
    }

    cJSON *sa = cJSON_GetObjectItemCaseSensitive(root, "slot_a");
    cJSON *sb = cJSON_GetObjectItemCaseSensitive(root, "slot_b");

    ota_slot_info_t slot_a;
    ota_slot_info_t slot_b;
    slot_from_json(sa, &slot_a);
    slot_from_json(sb, &slot_b);

    /* Swap active slot */
    slot_a.is_active = !slot_a.is_active;
    slot_b.is_active = !slot_b.is_active;

    slot_to_json(sa, &slot_a);
    slot_to_json(sb, &slot_b);

    /* Set pending and increment boot count */
    cJSON_ReplaceItemInObjectCaseSensitive(root, "pending_activation",
                                            cJSON_CreateBool(1));

    cJSON *bc = cJSON_GetObjectItemCaseSensitive(root, "boot_count");
    int boot_count = cJSON_IsNumber(bc) ? bc->valueint + 1 : 1;
    cJSON_ReplaceItemInObjectCaseSensitive(root, "boot_count",
                                            cJSON_CreateNumber(boot_count));

    ota_error_t err = write_metadata(c->metadata_path.c_str(), root);
    cJSON_Delete(root);
    return err;
}

static ota_error_t lg_query_boot_status(void *ctx, ota_boot_status_t *out) {
    if (!ctx || !out) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    auto *c = static_cast<LinuxGenericCtx *>(ctx);

    cJSON *root = read_metadata(c->metadata_path.c_str());
    if (!root) {
        return OTA_ERR_BOOTLOADER_QUERY;
    }

    cJSON *sa = cJSON_GetObjectItemCaseSensitive(root, "slot_a");
    cJSON *sb = cJSON_GetObjectItemCaseSensitive(root, "slot_b");

    ota_slot_info_t slot_a;
    ota_slot_info_t slot_b;
    slot_from_json(sa, &slot_a);
    slot_from_json(sb, &slot_b);

    if (slot_a.is_active) {
        out->active_slot = slot_a;
        out->inactive_slot = slot_b;
    } else {
        out->active_slot = slot_b;
        out->inactive_slot = slot_a;
    }

    cJSON *bc = cJSON_GetObjectItemCaseSensitive(root, "boot_count");
    out->boot_count = cJSON_IsNumber(bc) ? static_cast<uint32_t>(bc->valueint) : 0;

    cJSON *bl = cJSON_GetObjectItemCaseSensitive(root, "boot_limit");
    out->boot_limit = cJSON_IsNumber(bl) ? static_cast<uint32_t>(bl->valueint) : 3;

    cJSON *pa = cJSON_GetObjectItemCaseSensitive(root, "pending_activation");
    out->pending_activation = cJSON_IsTrue(pa);

    cJSON_Delete(root);
    return OTA_OK;
}

static ota_error_t lg_mark_active_valid(void *ctx) {
    if (!ctx) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    auto *c = static_cast<LinuxGenericCtx *>(ctx);

    cJSON *root = read_metadata(c->metadata_path.c_str());
    if (!root) {
        return OTA_ERR_BOOTLOADER_ACTIVATE;
    }

    cJSON_ReplaceItemInObjectCaseSensitive(root, "pending_activation",
                                            cJSON_CreateBool(0));

    ota_error_t err = write_metadata(c->metadata_path.c_str(), root);
    cJSON_Delete(root);
    return err;
}

static ota_error_t lg_mark_slot_invalid(void *ctx, uint8_t slot_index) {
    if (!ctx) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    auto *c = static_cast<LinuxGenericCtx *>(ctx);

    cJSON *root = read_metadata(c->metadata_path.c_str());
    if (!root) {
        return OTA_ERR_BOOTLOADER_SLOT;
    }

    const char *key = (slot_index == 0) ? "slot_a" : (slot_index == 1) ? "slot_b" : nullptr;
    if (!key) {
        cJSON_Delete(root);
        return OTA_ERR_BOOTLOADER_SLOT;
    }

    cJSON *slot_json = cJSON_GetObjectItemCaseSensitive(root, key);
    ota_slot_info_t slot;
    slot_from_json(slot_json, &slot);
    slot.is_valid = false;
    slot_to_json(slot_json, &slot);

    ota_error_t err = write_metadata(c->metadata_path.c_str(), root);
    cJSON_Delete(root);
    return err;
}

static std::string parse_base_dir(const char *config_json) {
    std::string base_dir = "/tmp/ota-test/bootloader-lg";

    if (!config_json || config_json[0] == '\0') {
        return base_dir;
    }

    cJSON *root = cJSON_Parse(config_json);
    if (!root) {
        return base_dir;
    }

    cJSON *dir_item = cJSON_GetObjectItemCaseSensitive(root, "base_dir");
    if (cJSON_IsString(dir_item) && dir_item->valuestring) {
        base_dir = dir_item->valuestring;
    }

    cJSON_Delete(root);
    return base_dir;
}

static void ensure_dir(const char *path) {
    std::string p(path);
    for (size_t i = 1; i < p.size(); ++i) {
        if (p[i] == '/') {
            p[i] = '\0';
            mkdir(p.c_str(), 0755);
            p[i] = '/';
        }
    }
    mkdir(path, 0755);
}

ota_bootloader_interface_t *create_linux_generic_plugin(const char *config_json) {
    std::string base_dir = parse_base_dir(config_json);
    ensure_dir(base_dir.c_str());

    auto *ctx = new LinuxGenericCtx();
    ctx->base_dir = base_dir;
    ctx->metadata_path = base_dir + "/boot_metadata.json";
    ctx->slot_a_path = base_dir + "/slot_a.bin";
    ctx->slot_b_path = base_dir + "/slot_b.bin";

    /* Write initial metadata if it doesn't exist */
    FILE *fp = std::fopen(ctx->metadata_path.c_str(), "rb");
    if (!fp) {
        cJSON *root = cJSON_CreateObject();
        init_default_metadata(root);
        write_metadata(ctx->metadata_path.c_str(), root);
        cJSON_Delete(root);
    } else {
        std::fclose(fp);
    }

    auto *iface = new ota_bootloader_interface_t();
    iface->detect_inactive_slot = lg_detect_inactive_slot;
    iface->stage_firmware = lg_stage_firmware;
    iface->prepare_activation = lg_prepare_activation;
    iface->query_boot_status = lg_query_boot_status;
    iface->mark_active_valid = lg_mark_active_valid;
    iface->mark_slot_invalid = lg_mark_slot_invalid;
    iface->ctx = ctx;
    iface->name = OTA_BOOTLOADER_PLUGIN_LINUX_GENERIC;

    return iface;
}

void destroy_linux_generic_plugin(ota_bootloader_interface_t *iface) {
    if (!iface) {
        return;
    }
    delete static_cast<LinuxGenericCtx *>(iface->ctx);
    delete iface;
}

} // namespace ota::agent
