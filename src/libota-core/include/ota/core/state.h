#ifndef OTA_CORE_STATE_H
#define OTA_CORE_STATE_H

#include <stdbool.h>

/* Campaign states (Master-side, tracked in campaigns.state) */
typedef enum {
    OTA_CAMPAIGN_IDLE              = 0,
    OTA_CAMPAIGN_DOWNLOADING       = 1,
    OTA_CAMPAIGN_DOWNLOAD_FAILED   = 2,
    OTA_CAMPAIGN_VALIDATING        = 3,
    OTA_CAMPAIGN_VALIDATION_FAILED = 4,
    OTA_CAMPAIGN_TRANSFERRING      = 5,
    OTA_CAMPAIGN_TRANSFER_FAILED   = 6,
    OTA_CAMPAIGN_AWAITING_SAFE     = 7,
    OTA_CAMPAIGN_INSTALLING        = 8,
    OTA_CAMPAIGN_INSTALL_FAILED    = 9,
    OTA_CAMPAIGN_ACTIVATING        = 10,
    OTA_CAMPAIGN_ACTIVATE_FAILED   = 11,
    OTA_CAMPAIGN_MONITORING        = 12,
    OTA_CAMPAIGN_MONITORING_FAILED = 13,
    OTA_CAMPAIGN_COMPLETED         = 14,
    OTA_CAMPAIGN_ROLLED_BACK       = 15,
    OTA_CAMPAIGN_ABORTED           = 16,
} ota_campaign_state_t;

/* Per-agent target states (tracked in campaign_targets.state) */
typedef enum {
    OTA_TARGET_PENDING              = 0,
    OTA_TARGET_TRANSFER_IN_PROGRESS = 1,
    OTA_TARGET_TRANSFERRED          = 2,
    OTA_TARGET_INSTALLING           = 3,
    OTA_TARGET_INSTALLED            = 4,
    OTA_TARGET_ACTIVATING           = 5,
    OTA_TARGET_ACTIVATED            = 6,
    OTA_TARGET_MONITORING           = 7,
    OTA_TARGET_COMPLETED            = 8,
    OTA_TARGET_FAILED               = 9,
    OTA_TARGET_ROLLING_BACK         = 10,
    OTA_TARGET_ROLLED_BACK          = 11,
} ota_target_state_t;

/* Milestones reported by agents */
typedef enum {
    OTA_MILESTONE_TRANSFER_STARTED   = 1,
    OTA_MILESTONE_TRANSFER_COMPLETED = 2,
    OTA_MILESTONE_INSTALL_STARTED    = 3,
    OTA_MILESTONE_INSTALL_COMPLETED  = 4,
    OTA_MILESTONE_ACTIVATE_STARTED   = 5,
    OTA_MILESTONE_ACTIVATE_COMPLETED = 6,
    OTA_MILESTONE_ROLLBACK_STARTED   = 7,
    OTA_MILESTONE_ROLLBACK_COMPLETED = 8,
    OTA_MILESTONE_FAILURE            = 9,
} ota_milestone_t;

/* Download states (tracked in downloads.state) */
typedef enum {
    OTA_DOWNLOAD_PENDING     = 0,
    OTA_DOWNLOAD_IN_PROGRESS = 1,
    OTA_DOWNLOAD_PAUSED      = 2,
    OTA_DOWNLOAD_COMPLETED   = 3,
    OTA_DOWNLOAD_FAILED      = 4,
    OTA_DOWNLOAD_SUPERSEDED  = 5,
} ota_download_state_t;

/* Convert enums to/from string (for JSON and DB storage) */
const char *ota_campaign_state_str(ota_campaign_state_t s);
ota_campaign_state_t ota_campaign_state_from_str(const char *s);

const char *ota_target_state_str(ota_target_state_t s);
ota_target_state_t ota_target_state_from_str(const char *s);

const char *ota_milestone_str(ota_milestone_t m);
ota_milestone_t ota_milestone_from_str(const char *s);

const char *ota_download_state_str(ota_download_state_t s);
ota_download_state_t ota_download_state_from_str(const char *s);

/* State transition validation */
bool ota_campaign_can_transition(ota_campaign_state_t from, ota_campaign_state_t to);
bool ota_target_can_transition(ota_target_state_t from, ota_target_state_t to);

#endif /* OTA_CORE_STATE_H */
