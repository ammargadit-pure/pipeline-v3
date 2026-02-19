#include "sim_plugin.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <sys/stat.h>

#include "ota/core/error.h"
#include "ota/core/types.h"
#include "cJSON.h"

namespace ota::agent {

struct SimPluginCtx {
    std::string base_dir;
    std::string slot_a_path;
    std::string slot_b_path;
    ota_slot_info_t slot_a;
    ota_slot_info_t slot_b;
    uint32_t boot_count;
    uint32_t boot_limit;
    bool pending_activation;
};

static void init_slot(ota_slot_info_t *slot, uint8_t index, const char *label,
                       bool active, bool valid) {
    slot->slot_index = index;
    std::strncpy(slot->label, label, OTA_MAX_LABEL_LEN - 1);
    slot->label[OTA_MAX_LABEL_LEN - 1] = '\0';
    slot->is_active = active;
    slot->is_valid = valid;
    slot->version[0] = '\0';
}

static ota_error_t sim_detect_inactive_slot(void *ctx, ota_slot_info_t *out) {
    if (!ctx || !out) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    auto *s = static_cast<SimPluginCtx *>(ctx);
    if (s->slot_a.is_active) {
        *out = s->slot_b;
    } else {
        *out = s->slot_a;
    }
    return OTA_OK;
}

static ota_error_t sim_stage_firmware(void *ctx, const uint8_t *data,
                                       size_t len, size_t offset) {
    if (!ctx || !data) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    auto *s = static_cast<SimPluginCtx *>(ctx);

    /* Write to inactive slot's file */
    const std::string &path = s->slot_a.is_active ? s->slot_b_path : s->slot_a_path;

    FILE *fp = std::fopen(path.c_str(), offset == 0 ? "wb" : "r+b");
    if (!fp && offset != 0) {
        /* File doesn't exist yet but offset > 0, create it */
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

static ota_error_t sim_prepare_activation(void *ctx) {
    if (!ctx) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    auto *s = static_cast<SimPluginCtx *>(ctx);

    /* Swap active slot (simulates reboot into new firmware) */
    s->slot_a.is_active = !s->slot_a.is_active;
    s->slot_b.is_active = !s->slot_b.is_active;
    s->pending_activation = true;
    s->boot_count++;

    return OTA_OK;
}

static ota_error_t sim_query_boot_status(void *ctx, ota_boot_status_t *out) {
    if (!ctx || !out) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    auto *s = static_cast<SimPluginCtx *>(ctx);

    if (s->slot_a.is_active) {
        out->active_slot = s->slot_a;
        out->inactive_slot = s->slot_b;
    } else {
        out->active_slot = s->slot_b;
        out->inactive_slot = s->slot_a;
    }
    out->boot_count = s->boot_count;
    out->boot_limit = s->boot_limit;
    out->pending_activation = s->pending_activation;

    return OTA_OK;
}

static ota_error_t sim_mark_active_valid(void *ctx) {
    if (!ctx) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    auto *s = static_cast<SimPluginCtx *>(ctx);
    s->pending_activation = false;
    return OTA_OK;
}

static ota_error_t sim_mark_slot_invalid(void *ctx, uint8_t slot_index) {
    if (!ctx) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    auto *s = static_cast<SimPluginCtx *>(ctx);

    if (slot_index == 0) {
        s->slot_a.is_valid = false;
    } else if (slot_index == 1) {
        s->slot_b.is_valid = false;
    } else {
        return OTA_ERR_BOOTLOADER_SLOT;
    }

    return OTA_OK;
}

static std::string parse_base_dir(const char *config_json) {
    std::string base_dir = "/tmp/ota-test/bootloader";

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
    mkdir(path, 0755);
}

ota_bootloader_interface_t *create_sim_plugin(const char *config_json) {
    std::string base_dir = parse_base_dir(config_json);
    ensure_dir(base_dir.c_str());

    auto *ctx = new SimPluginCtx();
    ctx->base_dir = base_dir;
    ctx->slot_a_path = base_dir + "/slot_a.bin";
    ctx->slot_b_path = base_dir + "/slot_b.bin";
    ctx->boot_count = 0;
    ctx->boot_limit = 3;
    ctx->pending_activation = false;

    init_slot(&ctx->slot_a, 0, "slot_a", true, true);
    init_slot(&ctx->slot_b, 1, "slot_b", false, true);

    auto *iface = new ota_bootloader_interface_t();
    iface->detect_inactive_slot = sim_detect_inactive_slot;
    iface->stage_firmware = sim_stage_firmware;
    iface->prepare_activation = sim_prepare_activation;
    iface->query_boot_status = sim_query_boot_status;
    iface->mark_active_valid = sim_mark_active_valid;
    iface->mark_slot_invalid = sim_mark_slot_invalid;
    iface->ctx = ctx;
    iface->name = OTA_BOOTLOADER_PLUGIN_SIM;

    return iface;
}

void destroy_sim_plugin(ota_bootloader_interface_t *iface) {
    if (!iface) {
        return;
    }
    delete static_cast<SimPluginCtx *>(iface->ctx);
    delete iface;
}

} // namespace ota::agent
