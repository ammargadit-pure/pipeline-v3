#ifndef OTA_BOOTLOADER_INTERFACE_H
#define OTA_BOOTLOADER_INTERFACE_H

#include "ota/core/types.h"
#include "ota/core/error.h"

/* Slot identifier */
typedef struct {
    uint8_t  slot_index;                     /* 0 = A (primary), 1 = B (secondary) */
    char     label[OTA_MAX_LABEL_LEN];       /* "slot_a", "slot_b" */
    bool     is_active;
    bool     is_valid;
    char     version[OTA_MAX_VERSION_LEN];   /* Version currently in this slot */
} ota_slot_info_t;

/* Boot status */
typedef struct {
    ota_slot_info_t active_slot;
    ota_slot_info_t inactive_slot;
    uint32_t        boot_count;
    uint32_t        boot_limit;
    bool            pending_activation;
} ota_boot_status_t;

/* Plugin interface — one per platform */
typedef struct ota_bootloader_interface {
    ota_error_t (*detect_inactive_slot)(void *ctx, ota_slot_info_t *out);
    ota_error_t (*stage_firmware)(void *ctx, const uint8_t *data, size_t len, size_t offset);
    ota_error_t (*prepare_activation)(void *ctx);
    ota_error_t (*query_boot_status)(void *ctx, ota_boot_status_t *out);
    ota_error_t (*mark_active_valid)(void *ctx);
    ota_error_t (*mark_slot_invalid)(void *ctx, uint8_t slot_index);
    void       *ctx;
    const char *name;    /* e.g. "sim", "linux-generic", "mcuboot" */
} ota_bootloader_interface_t;

/* Plugin names (string constants used in config and factory) */
#define OTA_BOOTLOADER_PLUGIN_SIM           "sim"
#define OTA_BOOTLOADER_PLUGIN_LINUX_GENERIC "linux-generic"
#define OTA_BOOTLOADER_PLUGIN_MCUBOOT       "mcuboot"

/* Factory: create plugin by name. config_json is plugin-specific. Caller must destroy. */
ota_bootloader_interface_t *ota_bootloader_create(const char *plugin_name,
                                                   const char *config_json);
void ota_bootloader_destroy(ota_bootloader_interface_t *iface);

#endif /* OTA_BOOTLOADER_INTERFACE_H */
