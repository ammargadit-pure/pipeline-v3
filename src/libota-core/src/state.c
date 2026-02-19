#include "ota/core/state.h"
#include <string.h>
#include <stddef.h>

/* --- Campaign state strings --- */

static const struct {
    ota_campaign_state_t state;
    const char          *str;
} campaign_state_map[] = {
    { OTA_CAMPAIGN_IDLE,              "idle" },
    { OTA_CAMPAIGN_DOWNLOADING,       "downloading" },
    { OTA_CAMPAIGN_DOWNLOAD_FAILED,   "download_failed" },
    { OTA_CAMPAIGN_VALIDATING,        "validating" },
    { OTA_CAMPAIGN_VALIDATION_FAILED, "validation_failed" },
    { OTA_CAMPAIGN_TRANSFERRING,      "transferring" },
    { OTA_CAMPAIGN_TRANSFER_FAILED,   "transfer_failed" },
    { OTA_CAMPAIGN_AWAITING_SAFE,     "awaiting_safe_state" },
    { OTA_CAMPAIGN_INSTALLING,        "installing" },
    { OTA_CAMPAIGN_INSTALL_FAILED,    "install_failed" },
    { OTA_CAMPAIGN_ACTIVATING,        "activating" },
    { OTA_CAMPAIGN_ACTIVATE_FAILED,   "activate_failed" },
    { OTA_CAMPAIGN_MONITORING,        "monitoring" },
    { OTA_CAMPAIGN_MONITORING_FAILED, "monitoring_failed" },
    { OTA_CAMPAIGN_COMPLETED,         "completed" },
    { OTA_CAMPAIGN_ROLLED_BACK,       "rolled_back" },
    { OTA_CAMPAIGN_ABORTED,           "aborted" },
};

#define CAMPAIGN_STATE_COUNT (sizeof(campaign_state_map) / sizeof(campaign_state_map[0]))

const char *ota_campaign_state_str(ota_campaign_state_t s)
{
    for (size_t i = 0; i < CAMPAIGN_STATE_COUNT; i++) {
        if (campaign_state_map[i].state == s) {
            return campaign_state_map[i].str;
        }
    }
    return "unknown";
}

ota_campaign_state_t ota_campaign_state_from_str(const char *s)
{
    if (!s) return OTA_CAMPAIGN_IDLE;
    for (size_t i = 0; i < CAMPAIGN_STATE_COUNT; i++) {
        if (strcmp(campaign_state_map[i].str, s) == 0) {
            return campaign_state_map[i].state;
        }
    }
    return OTA_CAMPAIGN_IDLE;
}

/* --- Target state strings --- */

static const struct {
    ota_target_state_t state;
    const char        *str;
} target_state_map[] = {
    { OTA_TARGET_PENDING,              "pending" },
    { OTA_TARGET_TRANSFER_IN_PROGRESS, "transfer_in_progress" },
    { OTA_TARGET_TRANSFERRED,          "transferred" },
    { OTA_TARGET_INSTALLING,           "installing" },
    { OTA_TARGET_INSTALLED,            "installed" },
    { OTA_TARGET_ACTIVATING,           "activating" },
    { OTA_TARGET_ACTIVATED,            "activated" },
    { OTA_TARGET_MONITORING,           "monitoring" },
    { OTA_TARGET_COMPLETED,            "completed" },
    { OTA_TARGET_FAILED,               "failed" },
    { OTA_TARGET_ROLLING_BACK,         "rolling_back" },
    { OTA_TARGET_ROLLED_BACK,          "rolled_back" },
};

#define TARGET_STATE_COUNT (sizeof(target_state_map) / sizeof(target_state_map[0]))

const char *ota_target_state_str(ota_target_state_t s)
{
    for (size_t i = 0; i < TARGET_STATE_COUNT; i++) {
        if (target_state_map[i].state == s) {
            return target_state_map[i].str;
        }
    }
    return "unknown";
}

ota_target_state_t ota_target_state_from_str(const char *s)
{
    if (!s) return OTA_TARGET_PENDING;
    for (size_t i = 0; i < TARGET_STATE_COUNT; i++) {
        if (strcmp(target_state_map[i].str, s) == 0) {
            return target_state_map[i].state;
        }
    }
    return OTA_TARGET_PENDING;
}

/* --- Milestone strings --- */

static const struct {
    ota_milestone_t milestone;
    const char     *str;
} milestone_map[] = {
    { OTA_MILESTONE_TRANSFER_STARTED,   "transfer_started" },
    { OTA_MILESTONE_TRANSFER_COMPLETED, "transfer_completed" },
    { OTA_MILESTONE_INSTALL_STARTED,    "install_started" },
    { OTA_MILESTONE_INSTALL_COMPLETED,  "install_completed" },
    { OTA_MILESTONE_ACTIVATE_STARTED,   "activate_started" },
    { OTA_MILESTONE_ACTIVATE_COMPLETED, "activate_completed" },
    { OTA_MILESTONE_ROLLBACK_STARTED,   "rollback_started" },
    { OTA_MILESTONE_ROLLBACK_COMPLETED, "rollback_completed" },
    { OTA_MILESTONE_FAILURE,            "failure" },
};

#define MILESTONE_COUNT (sizeof(milestone_map) / sizeof(milestone_map[0]))

const char *ota_milestone_str(ota_milestone_t m)
{
    for (size_t i = 0; i < MILESTONE_COUNT; i++) {
        if (milestone_map[i].milestone == m) {
            return milestone_map[i].str;
        }
    }
    return "unknown";
}

ota_milestone_t ota_milestone_from_str(const char *s)
{
    if (!s) return OTA_MILESTONE_FAILURE;
    for (size_t i = 0; i < MILESTONE_COUNT; i++) {
        if (strcmp(milestone_map[i].str, s) == 0) {
            return milestone_map[i].milestone;
        }
    }
    return OTA_MILESTONE_FAILURE;
}

/* --- Download state strings --- */

static const struct {
    ota_download_state_t state;
    const char          *str;
} download_state_map[] = {
    { OTA_DOWNLOAD_PENDING,     "pending" },
    { OTA_DOWNLOAD_IN_PROGRESS, "in_progress" },
    { OTA_DOWNLOAD_PAUSED,      "paused" },
    { OTA_DOWNLOAD_COMPLETED,   "completed" },
    { OTA_DOWNLOAD_FAILED,      "failed" },
    { OTA_DOWNLOAD_SUPERSEDED,  "superseded" },
};

#define DOWNLOAD_STATE_COUNT (sizeof(download_state_map) / sizeof(download_state_map[0]))

const char *ota_download_state_str(ota_download_state_t s)
{
    for (size_t i = 0; i < DOWNLOAD_STATE_COUNT; i++) {
        if (download_state_map[i].state == s) {
            return download_state_map[i].str;
        }
    }
    return "unknown";
}

ota_download_state_t ota_download_state_from_str(const char *s)
{
    if (!s) return OTA_DOWNLOAD_PENDING;
    for (size_t i = 0; i < DOWNLOAD_STATE_COUNT; i++) {
        if (strcmp(download_state_map[i].str, s) == 0) {
            return download_state_map[i].state;
        }
    }
    return OTA_DOWNLOAD_PENDING;
}

/* --- Campaign state transitions --- */

typedef struct {
    ota_campaign_state_t from;
    ota_campaign_state_t to;
} campaign_transition_t;

static const campaign_transition_t campaign_transitions[] = {
    { OTA_CAMPAIGN_IDLE,              OTA_CAMPAIGN_DOWNLOADING },
    { OTA_CAMPAIGN_DOWNLOADING,       OTA_CAMPAIGN_VALIDATING },
    { OTA_CAMPAIGN_DOWNLOADING,       OTA_CAMPAIGN_DOWNLOAD_FAILED },
    { OTA_CAMPAIGN_DOWNLOAD_FAILED,   OTA_CAMPAIGN_DOWNLOADING },
    { OTA_CAMPAIGN_DOWNLOAD_FAILED,   OTA_CAMPAIGN_ABORTED },
    { OTA_CAMPAIGN_VALIDATING,        OTA_CAMPAIGN_TRANSFERRING },
    { OTA_CAMPAIGN_VALIDATING,        OTA_CAMPAIGN_VALIDATION_FAILED },
    { OTA_CAMPAIGN_VALIDATION_FAILED, OTA_CAMPAIGN_ABORTED },
    { OTA_CAMPAIGN_TRANSFERRING,      OTA_CAMPAIGN_AWAITING_SAFE },
    { OTA_CAMPAIGN_TRANSFERRING,      OTA_CAMPAIGN_TRANSFER_FAILED },
    { OTA_CAMPAIGN_TRANSFER_FAILED,   OTA_CAMPAIGN_TRANSFERRING },
    { OTA_CAMPAIGN_TRANSFER_FAILED,   OTA_CAMPAIGN_ABORTED },
    { OTA_CAMPAIGN_AWAITING_SAFE,     OTA_CAMPAIGN_INSTALLING },
    { OTA_CAMPAIGN_INSTALLING,        OTA_CAMPAIGN_ACTIVATING },
    { OTA_CAMPAIGN_INSTALLING,        OTA_CAMPAIGN_INSTALL_FAILED },
    { OTA_CAMPAIGN_INSTALL_FAILED,    OTA_CAMPAIGN_ROLLED_BACK },
    { OTA_CAMPAIGN_INSTALL_FAILED,    OTA_CAMPAIGN_ABORTED },
    { OTA_CAMPAIGN_ACTIVATING,        OTA_CAMPAIGN_MONITORING },
    { OTA_CAMPAIGN_ACTIVATING,        OTA_CAMPAIGN_ACTIVATE_FAILED },
    { OTA_CAMPAIGN_ACTIVATE_FAILED,   OTA_CAMPAIGN_ROLLED_BACK },
    { OTA_CAMPAIGN_ACTIVATE_FAILED,   OTA_CAMPAIGN_ABORTED },
    { OTA_CAMPAIGN_MONITORING,        OTA_CAMPAIGN_COMPLETED },
    { OTA_CAMPAIGN_MONITORING,        OTA_CAMPAIGN_MONITORING_FAILED },
    { OTA_CAMPAIGN_MONITORING_FAILED, OTA_CAMPAIGN_ROLLED_BACK },
};

#define CAMPAIGN_TRANSITION_COUNT \
    (sizeof(campaign_transitions) / sizeof(campaign_transitions[0]))

bool ota_campaign_can_transition(ota_campaign_state_t from, ota_campaign_state_t to)
{
    for (size_t i = 0; i < CAMPAIGN_TRANSITION_COUNT; i++) {
        if (campaign_transitions[i].from == from &&
            campaign_transitions[i].to == to) {
            return true;
        }
    }
    return false;
}

/* --- Target state transitions --- */

typedef struct {
    ota_target_state_t from;
    ota_target_state_t to;
} target_transition_t;

static const target_transition_t target_transitions[] = {
    { OTA_TARGET_PENDING,              OTA_TARGET_TRANSFER_IN_PROGRESS },
    { OTA_TARGET_TRANSFER_IN_PROGRESS, OTA_TARGET_TRANSFERRED },
    { OTA_TARGET_TRANSFER_IN_PROGRESS, OTA_TARGET_FAILED },
    { OTA_TARGET_TRANSFERRED,          OTA_TARGET_INSTALLING },
    { OTA_TARGET_INSTALLING,           OTA_TARGET_INSTALLED },
    { OTA_TARGET_INSTALLING,           OTA_TARGET_FAILED },
    { OTA_TARGET_INSTALLED,            OTA_TARGET_ACTIVATING },
    { OTA_TARGET_ACTIVATING,           OTA_TARGET_ACTIVATED },
    { OTA_TARGET_ACTIVATING,           OTA_TARGET_FAILED },
    { OTA_TARGET_ACTIVATED,            OTA_TARGET_MONITORING },
    { OTA_TARGET_MONITORING,           OTA_TARGET_COMPLETED },
    { OTA_TARGET_MONITORING,           OTA_TARGET_FAILED },
    { OTA_TARGET_MONITORING,           OTA_TARGET_ROLLING_BACK },
    { OTA_TARGET_FAILED,               OTA_TARGET_ROLLING_BACK },
    { OTA_TARGET_ROLLING_BACK,         OTA_TARGET_ROLLED_BACK },
};

#define TARGET_TRANSITION_COUNT \
    (sizeof(target_transitions) / sizeof(target_transitions[0]))

bool ota_target_can_transition(ota_target_state_t from, ota_target_state_t to)
{
    for (size_t i = 0; i < TARGET_TRANSITION_COUNT; i++) {
        if (target_transitions[i].from == from &&
            target_transitions[i].to == to) {
            return true;
        }
    }
    return false;
}
