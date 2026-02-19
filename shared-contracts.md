# Shared Contracts

## Rules
1. ONLY the architect defines contracts. Developers NEVER invent new names.
2. Every identifier that a test references MUST appear in this document.
3. Only define identifiers for elements that will actually exist.

---

## 1. Naming Conventions

| Context | Convention | Example |
|---------|-----------|---------|
| C source files | snake_case | `manifest.c`, `crypto_openssl.c` |
| C header files | snake_case | `manifest.h`, `protocol.h` |
| C++ source files | snake_case | `download_manager.cpp` |
| C++ header files | snake_case | `download_manager.hpp` |
| C functions | `ota_` prefix + snake_case | `ota_manifest_parse()` |
| C types (structs/enums) | `ota_` prefix + snake_case + `_t` suffix | `ota_error_t`, `ota_slot_info_t` |
| C constants / enum values | `OTA_` prefix + UPPER_SNAKE | `OTA_OK`, `OTA_MSG_HEARTBEAT` |
| C++ namespaces | `ota::` nested by module | `ota::master`, `ota::agent` |
| C++ classes | PascalCase | `DownloadManager`, `AgentRegistry` |
| C++ methods | camelCase | `startTransfer()`, `getBootStatus()` |
| C++ member variables | `m_` prefix + camelCase | `m_agents`, `m_downloadDir` |
| SQLite tables | snake_case | `campaign_targets` |
| SQLite columns | snake_case | `agent_id`, `last_heartbeat` |
| Config JSON keys | snake_case | `listen_unix`, `db_path` |
| File paths (runtime) | snake_case | `/var/lib/ota/master.db` |
| Test files (C++) | `test_` prefix + snake_case | `test_manifest.cpp` |
| Test files (C) | `test_` prefix + snake_case | `test_agent_task.c` |
| Test functions (GTest) | `TEST(SuiteName, TestName)` PascalCase | `TEST(Manifest, ParseValidManifest)` |
| Test functions (Unity) | `test_` prefix + snake_case | `test_agent_task_initial_state` |
| CMake targets | kebab-case | `ota-core`, `ota-master`, `ota-agent` |
| Package IDs | kebab-case | `nav-service`, `sensor-fw` |
| Commit messages | `feat(module):` / `test(module):` / `fix(module):` | `feat(master): add download manager` |

---

## 2. Include Path Convention

All public headers are accessed via `ota/` prefix:

```c
#include "ota/core/error.h"
#include "ota/core/manifest.h"
#include "ota/core/protocol.h"
#include "ota/core/state.h"
#include "ota/core/version.h"
#include "ota/core/crypto.h"
#include "ota/core/persistence.h"
#include "ota/core/types.h"
#include "ota/bootloader/interface.h"
```

---

## 3. Shared Types (`ota/core/types.h`)

```c
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
```

---

## 4. Error Codes (`ota/core/error.h`)

```c
#ifndef OTA_CORE_ERROR_H
#define OTA_CORE_ERROR_H

typedef enum {
    /* Success */
    OTA_OK                          = 0,

    /* General errors (1-99) */
    OTA_ERR_INVALID_ARGUMENT        = 1,
    OTA_ERR_OUT_OF_MEMORY           = 2,
    OTA_ERR_IO                      = 3,
    OTA_ERR_TIMEOUT                 = 4,
    OTA_ERR_NOT_FOUND               = 5,
    OTA_ERR_ALREADY_EXISTS          = 6,
    OTA_ERR_INVALID_STATE           = 7,

    /* Protocol errors (100-199) */
    OTA_ERR_PROTOCOL_MAGIC          = 100,
    OTA_ERR_PROTOCOL_VERSION        = 101,
    OTA_ERR_PROTOCOL_PAYLOAD_SIZE   = 102,
    OTA_ERR_PROTOCOL_PARSE          = 103,
    OTA_ERR_PROTOCOL_UNKNOWN_TYPE   = 104,

    /* Verification errors (200-299) */
    OTA_ERR_VERIFY_SIZE_MISMATCH    = 200,
    OTA_ERR_VERIFY_CRC32_MISMATCH   = 201,
    OTA_ERR_VERIFY_SHA256_MISMATCH  = 202,
    OTA_ERR_VERIFY_SIGNATURE_FAILED = 203,
    OTA_ERR_VERIFY_KEY_LOAD_FAILED  = 204,

    /* Download errors (300-399) */
    OTA_ERR_DOWNLOAD_NETWORK        = 300,
    OTA_ERR_DOWNLOAD_HTTP           = 301,
    OTA_ERR_DOWNLOAD_STORAGE        = 302,
    OTA_ERR_DOWNLOAD_SUPERSEDED     = 303,

    /* Transfer errors (400-499) */
    OTA_ERR_TRANSFER_REJECTED       = 400,
    OTA_ERR_TRANSFER_STORAGE        = 401,
    OTA_ERR_TRANSFER_CHECKSUM       = 402,
    OTA_ERR_TRANSFER_TIMEOUT        = 403,

    /* Installation errors (500-599) */
    OTA_ERR_INSTALL_EXTRACT         = 500,
    OTA_ERR_INSTALL_MANIFEST        = 501,
    OTA_ERR_INSTALL_STORAGE         = 502,
    OTA_ERR_INSTALL_SYMLINK         = 503,
    OTA_ERR_INSTALL_SLOT            = 504,

    /* Activation errors (600-699) */
    OTA_ERR_ACTIVATE_SERVICE_FAIL   = 600,
    OTA_ERR_ACTIVATE_HEALTH_CHECK   = 601,
    OTA_ERR_ACTIVATE_BOOT_FAIL      = 602,
    OTA_ERR_ACTIVATE_TIMEOUT        = 603,

    /* Compatibility errors (700-799) */
    OTA_ERR_COMPAT_NODE_MISMATCH    = 700,
    OTA_ERR_COMPAT_OS_VERSION       = 701,
    OTA_ERR_COMPAT_SDK_VERSION      = 702,
    OTA_ERR_COMPAT_REGION           = 703,
    OTA_ERR_COMPAT_OEM_RULE         = 704,

    /* Bootloader errors (800-899) */
    OTA_ERR_BOOTLOADER_SLOT         = 800,
    OTA_ERR_BOOTLOADER_STAGE        = 801,
    OTA_ERR_BOOTLOADER_ACTIVATE     = 802,
    OTA_ERR_BOOTLOADER_QUERY        = 803,
} ota_error_t;

/* Convert error code to string */
const char *ota_error_str(ota_error_t err);

/* Check if error code falls within a range */
static inline bool ota_error_is_protocol(ota_error_t e) { return e >= 100 && e < 200; }
static inline bool ota_error_is_verify(ota_error_t e)   { return e >= 200 && e < 300; }
static inline bool ota_error_is_download(ota_error_t e)  { return e >= 300 && e < 400; }
static inline bool ota_error_is_transfer(ota_error_t e)  { return e >= 400 && e < 500; }
static inline bool ota_error_is_install(ota_error_t e)   { return e >= 500 && e < 600; }
static inline bool ota_error_is_activate(ota_error_t e)  { return e >= 600 && e < 700; }
static inline bool ota_error_is_compat(ota_error_t e)    { return e >= 700 && e < 800; }
static inline bool ota_error_is_bootloader(ota_error_t e){ return e >= 800 && e < 900; }

#endif /* OTA_CORE_ERROR_H */
```

---

## 5. Protocol Contract (`ota/core/protocol.h`)

### 5.1 Wire Frame

```c
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
```

### 5.2 JSON Payload Schemas

All JSON payloads use **cJSON** (C) or **nlohmann::json** (C++).
Field names are `snake_case`. The exact JSON keys are:

**REGISTER_REQ** (Agent → Master):
| Key | Type | Required | Example |
|-----|------|----------|---------|
| `node_identifier` | string | yes | `"hpc-primary-001"` |
| `hardware_revision` | string | yes | `"rev-B"` |
| `architecture` | string | yes | `"x86_64"` |
| `os_type` | string | yes | `"linux"` or `"freertos"` |
| `os_version` | string | no | `"5.15.0"` |
| `sdk_version` | string | no | `"1.0.0"` |
| `region` | string | no | `"EU"` |
| `installed_packages` | array | no | see below |

`installed_packages` array element:
| Key | Type | Example |
|-----|------|---------|
| `package_id` | string | `"nav-service"` |
| `version` | string | `"2.1.0"` |
| `type` | string | `"software"` or `"firmware"` |

**REGISTER_ACK** (Master → Agent):
| Key | Type | Example |
|-----|------|---------|
| `agent_id` | string (UUID) | `"a1b2c3d4-e5f6-7890-abcd-ef1234567890"` |
| `status` | string | `"registered"` |

**HEARTBEAT** (Agent → Master):
| Key | Type | Example |
|-----|------|---------|
| `agent_id` | string (UUID) | `"a1b2c3d4-..."` |
| `timestamp` | integer (unix) | `1708300000` |

**HEARTBEAT_ACK** (Master → Agent):
| Key | Type | Example |
|-----|------|---------|
| `timestamp` | integer (unix) | `1708300000` |

**TRANSFER_START** (Master → Agent):
| Key | Type | Example |
|-----|------|---------|
| `package_id` | string | `"nav-service"` |
| `version` | string | `"2.2.0"` |
| `artifact_type` | string | `"software"` or `"firmware"` |
| `total_size` | integer | `5242880` |
| `sha256` | string (hex) | `"a3f2b8c1d4..."` |
| `chunk_size` | integer | `65536` |

**TRANSFER_DATA** (Master → Agent):
Binary payload — no JSON. Raw bytes of chunk data.

**TRANSFER_ACK** (Agent → Master):
| Key | Type | Example |
|-----|------|---------|
| `package_id` | string | `"nav-service"` |
| `version` | string | `"2.2.0"` |
| `received_bytes` | integer | `131072` |
| `status` | string | `"ok"` |

**TRANSFER_NACK** (Agent → Master):
| Key | Type | Example |
|-----|------|---------|
| `package_id` | string | `"nav-service"` |
| `version` | string | `"2.2.0"` |
| `error_code` | integer | `401` |
| `error_message` | string | `"Storage full"` |

**TRANSFER_COMPLETE** (Master → Agent):
| Key | Type | Example |
|-----|------|---------|
| `package_id` | string | `"nav-service"` |
| `version` | string | `"2.2.0"` |
| `sha256` | string (hex) | `"a3f2b8c1d4..."` |

**INSTALL_CMD** (Master → Agent):
| Key | Type | Example |
|-----|------|---------|
| `package_id` | string | `"nav-service"` |
| `version` | string | `"2.2.0"` |
| `artifact_type` | string | `"software"` or `"firmware"` |

**ACTIVATE_CMD** (Master → Agent):
| Key | Type | Example |
|-----|------|---------|
| `package_id` | string | `"nav-service"` |
| `version` | string | `"2.2.0"` |
| `artifact_type` | string | `"software"` or `"firmware"` |

**ROLLBACK_CMD** (Master → Agent):
| Key | Type | Example |
|-----|------|---------|
| `package_id` | string | `"nav-service"` |
| `version` | string | `"2.2.0"` |
| `reason` | string | `"health_threshold_exceeded"` |

**STATUS_REPORT** (Agent → Master):
| Key | Type | Example |
|-----|------|---------|
| `agent_id` | string (UUID) | `"a1b2c3d4-..."` |
| `package_id` | string | `"nav-service"` |
| `version` | string | `"2.2.0"` |
| `milestone` | string | see milestone values below |
| `error_code` | integer | `0` |
| `error_message` | string | `""` |
| `timestamp` | integer (unix) | `1708300000` |

Milestone string values (used in JSON, mapped from `ota_milestone_t`):
- `"transfer_started"`
- `"transfer_completed"`
- `"install_started"`
- `"install_completed"`
- `"activate_started"`
- `"activate_completed"`
- `"rollback_started"`
- `"rollback_completed"`
- `"failure"`

**HEALTH_REPORT** (Agent → Master):
| Key | Type | Example |
|-----|------|---------|
| `agent_id` | string (UUID) | `"a1b2c3d4-..."` |
| `driving_cycle` | integer | `42` |
| `packages` | array | see below |

`packages` array element:
| Key | Type | Example |
|-----|------|---------|
| `package_id` | string | `"nav-service"` |
| `version` | string | `"2.2.0"` |
| `error_count` | integer | `0` |
| `crash_count` | integer | `0` |
| `uptime_seconds` | integer | `3600` |
| `status` | string | `"healthy"` or `"degraded"` or `"failed"` |

**CAMPAIGN_STATUS** (Master → Agent):
| Key | Type | Example |
|-----|------|---------|
| `campaign_id` | string (UUID) | `"..."` |
| `package_id` | string | `"nav-service"` |
| `version` | string | `"2.2.0"` |
| `state` | string | campaign state value |

---

## 6. State Machine Enums (`ota/core/state.h`)

```c
#ifndef OTA_CORE_STATE_H
#define OTA_CORE_STATE_H

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
```

### State String Values (stored in SQLite and JSON)

| Enum | String stored in DB / JSON |
|------|---------------------------|
| `OTA_CAMPAIGN_IDLE` | `"idle"` |
| `OTA_CAMPAIGN_DOWNLOADING` | `"downloading"` |
| `OTA_CAMPAIGN_DOWNLOAD_FAILED` | `"download_failed"` |
| `OTA_CAMPAIGN_VALIDATING` | `"validating"` |
| `OTA_CAMPAIGN_VALIDATION_FAILED` | `"validation_failed"` |
| `OTA_CAMPAIGN_TRANSFERRING` | `"transferring"` |
| `OTA_CAMPAIGN_TRANSFER_FAILED` | `"transfer_failed"` |
| `OTA_CAMPAIGN_AWAITING_SAFE` | `"awaiting_safe_state"` |
| `OTA_CAMPAIGN_INSTALLING` | `"installing"` |
| `OTA_CAMPAIGN_INSTALL_FAILED` | `"install_failed"` |
| `OTA_CAMPAIGN_ACTIVATING` | `"activating"` |
| `OTA_CAMPAIGN_ACTIVATE_FAILED` | `"activate_failed"` |
| `OTA_CAMPAIGN_MONITORING` | `"monitoring"` |
| `OTA_CAMPAIGN_MONITORING_FAILED` | `"monitoring_failed"` |
| `OTA_CAMPAIGN_COMPLETED` | `"completed"` |
| `OTA_CAMPAIGN_ROLLED_BACK` | `"rolled_back"` |
| `OTA_CAMPAIGN_ABORTED` | `"aborted"` |
| `OTA_TARGET_PENDING` | `"pending"` |
| `OTA_TARGET_TRANSFER_IN_PROGRESS` | `"transfer_in_progress"` |
| `OTA_TARGET_TRANSFERRED` | `"transferred"` |
| `OTA_TARGET_INSTALLING` | `"installing"` |
| `OTA_TARGET_INSTALLED` | `"installed"` |
| `OTA_TARGET_ACTIVATING` | `"activating"` |
| `OTA_TARGET_ACTIVATED` | `"activated"` |
| `OTA_TARGET_MONITORING` | `"monitoring"` |
| `OTA_TARGET_COMPLETED` | `"completed"` |
| `OTA_TARGET_FAILED` | `"failed"` |
| `OTA_TARGET_ROLLING_BACK` | `"rolling_back"` |
| `OTA_TARGET_ROLLED_BACK` | `"rolled_back"` |
| `OTA_DOWNLOAD_PENDING` | `"pending"` |
| `OTA_DOWNLOAD_IN_PROGRESS` | `"in_progress"` |
| `OTA_DOWNLOAD_PAUSED` | `"paused"` |
| `OTA_DOWNLOAD_COMPLETED` | `"completed"` |
| `OTA_DOWNLOAD_FAILED` | `"failed"` |
| `OTA_DOWNLOAD_SUPERSEDED` | `"superseded"` |

---

## 7. Manifest Contract (`ota/core/manifest.h`)

```c
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
```

---

## 8. Version Comparison (`ota/core/version.h`)

```c
#ifndef OTA_CORE_VERSION_H
#define OTA_CORE_VERSION_H

#include "ota/core/types.h"
#include "ota/core/error.h"

/* Parsed semantic version */
typedef struct {
    int major;
    int minor;
    int patch;
} ota_version_t;

/* Parse "MAJOR.MINOR.PATCH" string */
ota_error_t ota_version_parse(const char *str, ota_version_t *out);

/* Compare two versions. Returns: <0 if a<b, 0 if a==b, >0 if a>b */
int ota_version_compare(const ota_version_t *a, const ota_version_t *b);

/* Check if version is within [min, max] range (inclusive) */
bool ota_version_in_range(const ota_version_t *ver,
                          const ota_version_t *min,
                          const ota_version_t *max);

/* Format version to string buffer (must be >= OTA_MAX_VERSION_LEN) */
ota_error_t ota_version_to_str(const ota_version_t *ver, char *buf, size_t buf_len);

#endif /* OTA_CORE_VERSION_H */
```

---

## 9. Crypto Abstraction (`ota/core/crypto.h`)

```c
#ifndef OTA_CORE_CRYPTO_H
#define OTA_CORE_CRYPTO_H

#include "ota/core/types.h"
#include "ota/core/error.h"

/* CRC32 — compute over a buffer (ITU-T / zlib polynomial) */
ota_crc32_t ota_crc32(const uint8_t *data, size_t len);

/* CRC32 — incremental: init, update, finalize */
typedef struct { uint32_t state; } ota_crc32_ctx_t;
void        ota_crc32_init(ota_crc32_ctx_t *ctx);
void        ota_crc32_update(ota_crc32_ctx_t *ctx, const uint8_t *data, size_t len);
ota_crc32_t ota_crc32_finalize(ota_crc32_ctx_t *ctx);

/* SHA-256 — compute digest over buffer */
ota_error_t ota_sha256(const uint8_t *data, size_t len, uint8_t digest[OTA_SHA256_DIGEST_LEN]);

/* SHA-256 — incremental */
typedef struct { uint8_t opaque[128]; } ota_sha256_ctx_t;
ota_error_t ota_sha256_init(ota_sha256_ctx_t *ctx);
ota_error_t ota_sha256_update(ota_sha256_ctx_t *ctx, const uint8_t *data, size_t len);
ota_error_t ota_sha256_finalize(ota_sha256_ctx_t *ctx, uint8_t digest[OTA_SHA256_DIGEST_LEN]);

/* SHA-256 — compute over file */
ota_error_t ota_sha256_file(const char *path, uint8_t digest[OTA_SHA256_DIGEST_LEN]);

/* Convert SHA-256 digest to hex string (must be >= OTA_MAX_SHA256_HEX_LEN) */
void ota_sha256_to_hex(const uint8_t digest[OTA_SHA256_DIGEST_LEN], char *hex_out);

/* Compare hex string against digest */
bool ota_sha256_hex_matches(const char *hex, const uint8_t digest[OTA_SHA256_DIGEST_LEN]);

/* ECDSA P-256 — verify signature of data using PEM public key file */
ota_error_t ota_ecdsa_verify(const uint8_t *data, size_t data_len,
                              const uint8_t *signature, size_t sig_len,
                              const char *pubkey_pem_path);

/* ECDSA P-256 — sign data using PEM private key file (for packaging tool) */
ota_error_t ota_ecdsa_sign(const uint8_t *data, size_t data_len,
                            const char *privkey_pem_path,
                            uint8_t *sig_out, size_t *sig_len);

#endif /* OTA_CORE_CRYPTO_H */
```

---

## 10. Persistence Utilities (`ota/core/persistence.h`)

```c
#ifndef OTA_CORE_PERSISTENCE_H
#define OTA_CORE_PERSISTENCE_H

#include "ota/core/types.h"
#include "ota/core/error.h"

/* Atomic file write: write to temp file, fsync, rename to target path */
ota_error_t ota_atomic_write(const char *path, const uint8_t *data, size_t len);

/* Atomic JSON write: serialize JSON string, write atomically */
ota_error_t ota_atomic_write_json(const char *path, const char *json_str);

/* Read entire file into heap-allocated buffer. Caller frees *out_data. */
ota_error_t ota_read_file(const char *path, uint8_t **out_data, size_t *out_len);

/* Read file as null-terminated string. Caller frees returned pointer. */
char *ota_read_file_str(const char *path);

/* Check if file exists */
bool ota_file_exists(const char *path);

/* Create directory and parents (like mkdir -p) */
ota_error_t ota_mkdir_p(const char *path);

#endif /* OTA_CORE_PERSISTENCE_H */
```

---

## 11. Bootloader Plugin Interface (`ota/bootloader/interface.h`)

```c
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
```

---

## 12. SQLite Schema (Exact DDL)

```sql
-- Master database: /var/lib/ota/master.db

CREATE TABLE IF NOT EXISTS agents (
    agent_id           TEXT PRIMARY KEY,
    node_identifier    TEXT NOT NULL,
    hardware_revision  TEXT NOT NULL,
    architecture       TEXT NOT NULL,
    os_type            TEXT NOT NULL,
    os_version         TEXT,
    sdk_version        TEXT,
    region             TEXT,
    ip_address         TEXT NOT NULL,
    port               INTEGER NOT NULL,
    transport_type     TEXT NOT NULL,
    status             TEXT NOT NULL DEFAULT 'registered',
    last_heartbeat     INTEGER NOT NULL,
    registered_at      INTEGER NOT NULL,
    capabilities_json  TEXT
);

CREATE TABLE IF NOT EXISTS campaigns (
    campaign_id   TEXT PRIMARY KEY,
    package_id    TEXT NOT NULL,
    version       TEXT NOT NULL,
    artifact_type TEXT NOT NULL,
    state         TEXT NOT NULL DEFAULT 'idle',
    created_at    INTEGER NOT NULL,
    updated_at    INTEGER NOT NULL,
    manifest_json TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS campaign_targets (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    campaign_id   TEXT NOT NULL REFERENCES campaigns(campaign_id),
    agent_id      TEXT NOT NULL REFERENCES agents(agent_id),
    state         TEXT NOT NULL DEFAULT 'pending',
    error_code    INTEGER DEFAULT 0,
    error_message TEXT DEFAULT '',
    retry_count   INTEGER NOT NULL DEFAULT 0,
    healthy_cycles INTEGER NOT NULL DEFAULT 0,
    updated_at    INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS downloads (
    download_id     TEXT PRIMARY KEY,
    package_id      TEXT NOT NULL,
    version         TEXT NOT NULL,
    url             TEXT NOT NULL,
    total_size      INTEGER NOT NULL,
    downloaded_bytes INTEGER NOT NULL DEFAULT 0,
    expected_crc32  INTEGER,
    expected_sha256 TEXT,
    signature       TEXT,
    state           TEXT NOT NULL DEFAULT 'pending',
    local_path      TEXT NOT NULL,
    created_at      INTEGER NOT NULL,
    updated_at      INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS audit_log (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp     INTEGER NOT NULL,
    campaign_id   TEXT,
    agent_id      TEXT,
    event_type    TEXT NOT NULL,
    details_json  TEXT
);

CREATE INDEX IF NOT EXISTS idx_campaign_targets_campaign ON campaign_targets(campaign_id);
CREATE INDEX IF NOT EXISTS idx_campaign_targets_agent ON campaign_targets(agent_id);
CREATE INDEX IF NOT EXISTS idx_downloads_package ON downloads(package_id, version);
CREATE INDEX IF NOT EXISTS idx_audit_log_campaign ON audit_log(campaign_id);
CREATE INDEX IF NOT EXISTS idx_audit_log_timestamp ON audit_log(timestamp);
```

### Audit Event Types (exact strings for `audit_log.event_type`)

- `"agent_registered"`
- `"agent_heartbeat_timeout"`
- `"campaign_created"`
- `"campaign_state_changed"`
- `"download_started"`
- `"download_completed"`
- `"download_failed"`
- `"download_superseded"`
- `"verification_passed"`
- `"verification_failed"`
- `"transfer_started"`
- `"transfer_completed"`
- `"transfer_failed"`
- `"install_commanded"`
- `"install_completed"`
- `"install_failed"`
- `"activate_commanded"`
- `"activate_completed"`
- `"activate_failed"`
- `"rollback_commanded"`
- `"rollback_completed"`
- `"health_report_received"`
- `"health_threshold_exceeded"`

---

## 13. Module Contracts

### 13.1 Module: libota-core

**CMake target**: `ota-core`
**Library output**: `libota-core.a` (static library)

Source files:
| File | Public header | Exports |
|------|--------------|---------|
| `src/libota-core/src/error.c` | `ota/core/error.h` | `ota_error_str()`, range check inlines |
| `src/libota-core/src/manifest.c` | `ota/core/manifest.h` | `ota_manifest_parse()`, `_free()`, `_validate()`, `_serialize()` |
| `src/libota-core/src/protocol.c` | `ota/core/protocol.h` | `ota_header_serialize()`, `_deserialize()`, `_validate()`, `ota_msg_is_binary()`, `ota_msg_type_str()` |
| `src/libota-core/src/state.c` | `ota/core/state.h` | All `*_str()`, `*_from_str()`, `*_can_transition()` functions |
| `src/libota-core/src/version.c` | `ota/core/version.h` | `ota_version_parse()`, `_compare()`, `_in_range()`, `_to_str()` |
| `src/libota-core/src/crypto_openssl.c` | `ota/core/crypto.h` | All crypto functions (OpenSSL backend) |
| `src/libota-core/src/persistence.c` | `ota/core/persistence.h` | `ota_atomic_write()`, `_write_json()`, `ota_read_file()`, etc. |

### 13.2 Module: ota-master

**CMake target**: `ota-master`
**Binary output**: `ota-master`
**Namespace**: `ota::master`

Key C++ classes and their files:

| Class | Header | Source | Responsibility |
|-------|--------|--------|---------------|
| `Config` | `config/config.hpp` | `config/config.cpp` | Load and validate `/etc/ota/ota-master.json` |
| `PolicyEngine` | `config/policy_engine.hpp` | (header-only or in config.cpp) | Evaluate safe-state rules, bandwidth, network policy |
| `AgentRegistry` | `registry/agent_registry.hpp` | `registry/agent_registry.cpp` | Register agents, track heartbeats, mark unreachable |
| `DownloadManager` | `download/download_manager.hpp` | `download/download_manager.cpp` | Resumable download, HTTP range, state persistence |
| `DownloadState` | `download/download_state.hpp` | `download/download_state.cpp` | Per-download persistent state (JSON file) |
| `VerificationPipeline` | `verification/verification_pipeline.hpp` | `verification/verification_pipeline.cpp` | Size → CRC32 → SHA-256 → ECDSA chain |
| `CompatibilityEngine` | `compatibility/compatibility_engine.hpp` | `compatibility/compatibility_engine.cpp` | Per-agent manifest eligibility check |
| `PackageTransfer` | `transfer/package_transfer.hpp` | `transfer/package_transfer.cpp` | Chunked transfer to agents via protocol |
| `ActivationController` | `activation/activation_controller.hpp` | `activation/activation_controller.cpp` | Safe-state gating, issue install/activate commands |
| `HealthAggregator` | `monitoring/health_aggregator.hpp` | `monitoring/health_aggregator.cpp` | Aggregate health reports, threshold evaluation |
| `RollbackCoordinator` | `rollback/rollback_coordinator.hpp` | `rollback/rollback_coordinator.cpp` | Issue rollback commands, handle responses |
| `CampaignManager` | `campaign/campaign_manager.hpp` | `campaign/campaign_manager.cpp` | Campaign lifecycle, state transitions, audit |
| `TransportServer` | `transport/transport_server.hpp` | `transport/transport_server.cpp` | Accept Unix + TCP connections, dispatch messages |
| `Connection` | `transport/connection.hpp` | `transport/connection.cpp` | Single connection read/write, framing |
| `CloudClient` | `cloud/cloud_client.hpp` | `cloud/cloud_client.cpp` | HTTP client for mock cloud API |
| `Database` | `db/database.hpp` | `db/database.cpp` | SQLite wrapper, schema init, CRUD |

### 13.3 Module: ota-agent (Linux)

**CMake target**: `ota-agent`
**Binary output**: `ota-agent`
**Namespace**: `ota::agent`

| Class | Header | Source | Responsibility |
|-------|--------|--------|---------------|
| `TransportClient` | `transport/transport_client.hpp` | `transport/transport_client.cpp` | Connect to Master (unix/tcp), send/receive |
| `SwInstaller` | `installer/sw_installer.hpp` | `installer/sw_installer.cpp` | Extract .ota, create version dir, handle libs/res/exe |
| `FwInstaller` | `installer/fw_installer.hpp` | `installer/fw_installer.cpp` | Stage firmware via bootloader plugin |
| `SwActivator` | `activator/sw_activator.hpp` | `activator/sw_activator.cpp` | Symlink swap (newer→current→prev), service restart |
| `FwActivator` | `activator/fw_activator.hpp` | `activator/fw_activator.cpp` | Bootloader prepare_activation + reboot trigger |
| `RollbackExecutor` | `rollback/rollback_executor.hpp` | `rollback/rollback_executor.cpp` | SW: revert symlinks. FW: mark_slot_invalid. |
| `HealthMonitor` | `health/health_monitor.hpp` | `health/health_monitor.cpp` | Track error/crash counts, generate health reports |
| `PluginFactory` | `bootloader/plugin_factory.hpp` | `bootloader/plugin_factory.cpp` | Instantiate bootloader plugin from config |

### 13.4 Module: ota-agent-freertos (x86 simulation)

**CMake target**: `ota-agent-freertos`
**Binary output**: `ota-agent-freertos`
**Language**: C11

| Function/Module | Header | Source | Responsibility |
|----------------|--------|--------|---------------|
| `main()` | — | `main.c` | Entry point, init, start agent task thread |
| `ota_agent_task_*` | `agent_task.h` | `agent_task.c` | Core agent loop: register, heartbeat, handle commands |
| `ota_freertos_transport_*` | `transport_client.h` | `transport_client.c` | TCP socket connect/send/recv to Master |
| `ota_freertos_fw_install_*` | `fw_installer.h` | `fw_installer.c` | Stage firmware via bootloader plugin |
| `ota_freertos_fw_activate_*` | `fw_activator.h` | `fw_activator.c` | Prepare activation, trigger simulated reset |
| `ota_freertos_health_*` | `health_monitor.h` | `health_monitor.c` | Error counters, health report generation |
| `ota_hal_*` | `hal/hal.h` | `hal/hal_sim.c` | Flash read/write/erase, watchdog, reset (x86 sim) |

HAL interface (`hal/hal.h`):
```c
#ifndef OTA_HAL_H
#define OTA_HAL_H

#include <stdint.h>
#include <stddef.h>
#include "ota/core/error.h"

ota_error_t ota_hal_flash_write(uint32_t addr, const uint8_t *data, size_t len);
ota_error_t ota_hal_flash_read(uint32_t addr, uint8_t *buf, size_t len);
ota_error_t ota_hal_flash_erase(uint32_t addr, size_t len);
void        ota_hal_system_reset(void);
void        ota_hal_watchdog_feed(void);

#endif /* OTA_HAL_H */
```

### 13.5 Module: ota-pack (CLI tool)

**CMake target**: `ota-pack`
**Binary output**: `ota-pack`
**Namespace**: `ota::pack`

CLI commands (exact syntax):
```bash
# Create package archive
ota-pack create --manifest <manifest.json> --dir <source_dir> --output <output.ota>

# Sign package
ota-pack sign --key <private_key.pem> --package <package.ota>

# Verify package
ota-pack verify --pubkey <public_key.pem> --package <package.ota>

# Inspect package contents
ota-pack inspect --package <package.ota>
```

### 13.6 Module: mock-cloud

**Language**: Python 3.10+ / FastAPI
**Base URL**: `http://localhost:8080/api/v1`

| Method | Path | Handler Function | Module |
|--------|------|-----------------|--------|
| GET | `/updates` | `list_updates()` | `routes/updates.py` |
| GET | `/updates/{package_id}/{version}/manifest` | `get_manifest()` | `routes/updates.py` |
| GET | `/updates/{package_id}/{version}/package` | `download_package()` | `routes/updates.py` |
| GET | `/updates/{package_id}/{version}/metadata` | `get_metadata()` | `routes/updates.py` |
| POST | `/admin/inject-error` | `inject_error()` | `routes/admin.py` |
| POST | `/admin/add-package` | `add_package()` | `routes/admin.py` |
| DELETE | `/admin/reset` | `reset_state()` | `routes/admin.py` |

Error injection types (exact string values for `type` field):
- `"network_error"` — connection reset, timeout
- `"corrupted_download"` — flip bytes in package stream
- `"truncated_download"` — cut off after N bytes
- `"slow_response"` — add latency to responses
- `"version_supersede"` — add a newer version mid-download

Error injection targets (exact string values for `target` field):
- `"download"` — affects package download endpoint
- `"manifest"` — affects manifest endpoint
- `"metadata"` — affects metadata endpoint

---

## 14. Configuration File Keys (Exact)

### Master Config Keys (`/etc/ota/ota-master.json`)
```
master.listen_unix          : string  (Unix socket path)
master.listen_tcp           : string  (host:port)
master.db_path              : string  (SQLite file path)
master.download_dir         : string  (download directory)
master.trusted_pubkey_path  : string  (EC public key PEM)
master.log_level            : string  ("debug"|"info"|"warn"|"error")
master.log_file             : string  (log file path)
cloud.base_url              : string  (mock cloud base URL)
cloud.timeout_seconds       : int
cloud.retry_count           : int
policy.check_on_ignition    : bool
policy.allowed_network_types: array<string>
policy.bandwidth_limit_kbps : int     (0 = unlimited)
policy.require_safe_state_for_activation: bool
policy.safe_states          : array<string>  ("parked"|"short_parking")
policy.health_monitoring_cycles: int
policy.error_threshold_percent : int
policy.transfer_retry_count : int
policy.transfer_chunk_size  : int     (bytes)
policy.version_supersede_policy: string ("supersede"|"queue")
```

### Agent Config Keys (`/etc/ota/ota-agent.json`)
```
agent.node_identifier   : string
agent.hardware_revision : string
agent.architecture      : string
agent.os_type           : string  ("linux"|"freertos")
agent.region            : string
agent.install_base_dir  : string  (e.g. "/opt")
agent.state_file        : string  (persistent state JSON)
agent.log_level         : string
agent.log_file          : string
master.address          : string  (socket path or host:port)
master.transport        : string  ("unix"|"tcp")
heartbeat.interval_seconds : int
heartbeat.timeout_seconds  : int
bootloader.plugin       : string  ("sim"|"linux-generic"|"mcuboot")
bootloader.config       : object  (plugin-specific JSON)
```

---

## 15. File Path Patterns (Exact)

### Master Runtime Paths
| Path | Description |
|------|-------------|
| `/etc/ota/ota-master.json` | Master config file |
| `/etc/ota/keys/ota-cloud.pub.pem` | Trusted EC public key |
| `/var/lib/ota/master.db` | SQLite database |
| `/var/lib/ota/downloads/` | Download directory |
| `/var/lib/ota/downloads/{pkg_id}_{version}.ota.partial` | In-progress download |
| `/var/lib/ota/downloads/{pkg_id}_{version}.ota` | Completed download |
| `/var/lib/ota/downloads/{pkg_id}_{version}.state.json` | Download state file |
| `/var/log/ota/master.log` | Log file |
| `/var/run/ota/master.sock` | Unix domain socket |

### Agent Runtime Paths (Linux)
| Path | Description |
|------|-------------|
| `/etc/ota/ota-agent.json` | Agent config file |
| `/var/lib/ota/agent-state.json` | Persistent agent state |
| `/var/log/ota/agent.log` | Log file |
| `/opt/{package_id}/{version}/` | Installed version directory |
| `/opt/{package_id}/{version}/exe` | Executable |
| `/opt/{package_id}/{version}/libs/` | Libraries |
| `/opt/{package_id}/{version}/res/` | Resources |
| `/opt/{package_id}/newer` → `{version}/` | Staged symlink |
| `/opt/{package_id}/current` → `{version}/` | Active symlink |
| `/opt/{package_id}/prev` → `{version}/` | Previous version symlink |

### Test Fixture Paths (Repository)
| Path | Description |
|------|-------------|
| `tests/fixtures/keys/test_private.pem` | ECDSA P-256 test private key |
| `tests/fixtures/keys/test_public.pem` | ECDSA P-256 test public key |
| `tests/fixtures/packages/valid_sw_package.ota` | Valid software package |
| `tests/fixtures/packages/valid_fw_package.ota` | Valid firmware package |
| `tests/fixtures/packages/corrupted_package.ota` | Tampered package (bad hash) |
| `tests/fixtures/packages/bad_signature.ota` | Wrong signature |
| `tests/fixtures/manifests/valid_sw_manifest.json` | Valid SW manifest |
| `tests/fixtures/manifests/valid_fw_manifest.json` | Valid FW manifest |
| `tests/fixtures/manifests/incompatible_manifest.json` | Manifest targeting non-existent node |
| `tests/fixtures/configs/test_master.json` | Master config for tests |
| `tests/fixtures/configs/test_agent.json` | Agent config for tests |

---

## 16. Test Data Conventions

### Test Agent Identities

```c
/* Test agent #1 — Linux HPC */
#define TEST_AGENT1_NODE_ID       "hpc-primary-001"
#define TEST_AGENT1_HW_REV        "rev-B"
#define TEST_AGENT1_ARCH          "x86_64"
#define TEST_AGENT1_OS_TYPE       "linux"
#define TEST_AGENT1_OS_VERSION    "5.15.0"
#define TEST_AGENT1_SDK_VERSION   "1.0.0"
#define TEST_AGENT1_REGION        "EU"

/* Test agent #2 — Linux remote ECU */
#define TEST_AGENT2_NODE_ID       "ecu-body-001"
#define TEST_AGENT2_HW_REV        "rev-A"
#define TEST_AGENT2_ARCH          "x86_64"
#define TEST_AGENT2_OS_TYPE       "linux"
#define TEST_AGENT2_OS_VERSION    "5.10.0"
#define TEST_AGENT2_SDK_VERSION   "1.0.0"
#define TEST_AGENT2_REGION        "EU"

/* Test agent #3 — FreeRTOS MCU */
#define TEST_AGENT3_NODE_ID       "mcu-sensor-001"
#define TEST_AGENT3_HW_REV        "rev-C"
#define TEST_AGENT3_ARCH          "x86_64"
#define TEST_AGENT3_OS_TYPE       "freertos"
#define TEST_AGENT3_OS_VERSION    "10.5.0"
#define TEST_AGENT3_SDK_VERSION   "1.0.0"
#define TEST_AGENT3_REGION        "EU"
```

### Test Package Identities

```c
/* Software package */
#define TEST_SW_PACKAGE_ID        "nav-service"
#define TEST_SW_VERSION_OLD       "2.1.0"
#define TEST_SW_VERSION_NEW       "2.2.0"
#define TEST_SW_VERSION_NEWER     "2.3.0"
#define TEST_SW_NODE_ID           "hpc-primary"

/* Firmware package */
#define TEST_FW_PACKAGE_ID        "sensor-fw"
#define TEST_FW_VERSION_OLD       "1.3.2"
#define TEST_FW_VERSION_NEW       "1.4.0"
#define TEST_FW_NODE_ID           "mcu-sensor"

/* Incompatible package (targets non-existent node) */
#define TEST_INCOMPAT_PACKAGE_ID  "other-service"
#define TEST_INCOMPAT_NODE_ID     "nonexistent-node"
```

### Test Configuration Values

```c
#define TEST_MASTER_UNIX_SOCK     "/tmp/ota-test/master.sock"
#define TEST_MASTER_TCP_ADDR      "127.0.0.1"
#define TEST_MASTER_TCP_PORT      19735
#define TEST_MASTER_DB_PATH       "/tmp/ota-test/master.db"
#define TEST_DOWNLOAD_DIR         "/tmp/ota-test/downloads"
#define TEST_INSTALL_BASE_DIR     "/tmp/ota-test/opt"
#define TEST_AGENT_STATE_FILE     "/tmp/ota-test/agent-state.json"
#define TEST_LOG_DIR              "/tmp/ota-test/log"
#define TEST_MOCK_CLOUD_PORT      18080
#define TEST_MOCK_CLOUD_URL       "http://127.0.0.1:18080/api/v1"
#define TEST_PUBKEY_PATH          "tests/fixtures/keys/test_public.pem"
#define TEST_PRIVKEY_PATH         "tests/fixtures/keys/test_private.pem"
#define TEST_HEARTBEAT_INTERVAL   2
#define TEST_HEARTBEAT_TIMEOUT    6
#define TEST_HEALTH_CYCLES        2
#define TEST_ERROR_THRESHOLD      50
#define TEST_CHUNK_SIZE           4096
```

### Test UUID Generation

For deterministic tests, use fixed UUIDs:
```c
#define TEST_AGENT1_UUID    "00000000-0000-0000-0000-000000000001"
#define TEST_AGENT2_UUID    "00000000-0000-0000-0000-000000000002"
#define TEST_AGENT3_UUID    "00000000-0000-0000-0000-000000000003"
#define TEST_CAMPAIGN1_UUID "10000000-0000-0000-0000-000000000001"
#define TEST_CAMPAIGN2_UUID "10000000-0000-0000-0000-000000000002"
#define TEST_DOWNLOAD1_UUID "20000000-0000-0000-0000-000000000001"
```

---

## 17. CMake Target Summary

| Target | Type | Language | Depends On |
|--------|------|----------|-----------|
| `ota-core` | STATIC library | C11 | OpenSSL or mbedTLS, cJSON |
| `ota-master` | Executable | C++17 | `ota-core`, nlohmann-json, spdlog, sqlite3, libcurl, libarchive |
| `ota-agent` | Executable | C++17 | `ota-core`, nlohmann-json, spdlog, libarchive |
| `ota-agent-freertos` | Executable | C11 | `ota-core` |
| `ota-pack` | Executable | C++17 | `ota-core`, nlohmann-json, libarchive |

Test targets (linked with GoogleTest/Unity):

| Target | Type | Tests For |
|--------|------|-----------|
| `unit_core_tests` | Test executable (GTest) | `libota-core` |
| `unit_master_tests` | Test executable (GTest) | `ota-master` classes |
| `unit_agent_tests` | Test executable (GTest) | `ota-agent` classes |
| `unit_agent_freertos_tests` | Test executable (Unity) | `ota-agent-freertos` |
| `unit_bootloader_tests` | Test executable (GTest) | Bootloader plugins |
| `integ_tests` | Test executable (GTest) | Integration scenarios |
| `e2e_tests` | Test executable (GTest) | End-to-end flows |
