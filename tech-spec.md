# Technical Specification: Automotive OTA Update System

## 1. Tech Stack

| Component | Technology | Version |
|-----------|-----------|---------|
| Language (Master, Linux Agent) | C++17 | GCC 11+ / Clang 14+ |
| Language (FreeRTOS Agent, Bootloader plugins, libota-core) | C11 | GCC 11+ / Clang 14+ |
| Build System | CMake | 3.20+ |
| Crypto (Linux) | OpenSSL | 3.x |
| Crypto (FreeRTOS) | mbedTLS | 3.x |
| HTTP Client | libcurl | 7.80+ |
| JSON (C++) | nlohmann-json | 3.11+ |
| JSON (C) | cJSON | 1.7+ |
| Database | SQLite | 3.39+ |
| Logging (C++) | spdlog | 1.11+ |
| Logging (C) | Custom lightweight (ota_log) | — |
| Archive | libarchive | 3.6+ (tar.gz read/write) |
| Test (C++) | GoogleTest + GoogleMock | 1.13+ |
| Test (C) | Unity + CMock | latest |
| Mock Cloud Server | Python 3.10+ / FastAPI | 0.100+ |
| Package format | tar.gz (.ota extension) | — |

## 2. Repository Structure

```
ota-system/
├── CMakeLists.txt                      # Root CMake — builds all targets
├── cmake/
│   ├── CompilerFlags.cmake             # -Wall -Wextra -Werror, sanitizers
│   ├── Dependencies.cmake              # FetchContent for all deps
│   └── Testing.cmake                   # CTest + coverage setup
├── src/
│   ├── libota-core/                    # C11 shared library
│   │   ├── CMakeLists.txt
│   │   ├── include/ota/
│   │   │   ├── core/
│   │   │   │   ├── error.h             # Error codes enum + helpers
│   │   │   │   ├── manifest.h          # Manifest struct + parse/validate
│   │   │   │   ├── protocol.h          # Wire protocol msg types + ser/de
│   │   │   │   ├── state.h             # Update state machine enums
│   │   │   │   ├── version.h           # Version comparison utilities
│   │   │   │   ├── crypto.h            # Hash/verify abstraction (compile-time switch openssl/mbedtls)
│   │   │   │   ├── persistence.h       # Atomic file write utilities
│   │   │   │   └── types.h             # Shared typedefs (slot_id_t, agent_id_t, etc.)
│   │   │   └── bootloader/
│   │   │       └── interface.h         # bootloader_interface_t function pointer table
│   │   └── src/
│   │       ├── error.c
│   │       ├── manifest.c
│   │       ├── protocol.c
│   │       ├── state.c
│   │       ├── version.c
│   │       ├── crypto_openssl.c        # OpenSSL backend (compiled on Linux)
│   │       ├── crypto_mbedtls.c        # mbedTLS backend (compiled on FreeRTOS)
│   │       └── persistence.c
│   ├── ota-master/                     # C++17 daemon
│   │   ├── CMakeLists.txt
│   │   ├── main.cpp
│   │   ├── config/
│   │   │   ├── config.hpp
│   │   │   ├── config.cpp
│   │   │   └── policy_engine.hpp       # OEM policy rules evaluation
│   │   ├── registry/
│   │   │   ├── agent_registry.hpp
│   │   │   └── agent_registry.cpp
│   │   ├── download/
│   │   │   ├── download_manager.hpp
│   │   │   ├── download_manager.cpp
│   │   │   ├── download_state.hpp      # Persistent download state
│   │   │   └── download_state.cpp
│   │   ├── verification/
│   │   │   ├── verification_pipeline.hpp
│   │   │   └── verification_pipeline.cpp
│   │   ├── compatibility/
│   │   │   ├── compatibility_engine.hpp
│   │   │   └── compatibility_engine.cpp
│   │   ├── transfer/
│   │   │   ├── package_transfer.hpp
│   │   │   └── package_transfer.cpp
│   │   ├── activation/
│   │   │   ├── activation_controller.hpp
│   │   │   └── activation_controller.cpp
│   │   ├── monitoring/
│   │   │   ├── health_aggregator.hpp
│   │   │   └── health_aggregator.cpp
│   │   ├── rollback/
│   │   │   ├── rollback_coordinator.hpp
│   │   │   └── rollback_coordinator.cpp
│   │   ├── campaign/
│   │   │   ├── campaign_manager.hpp
│   │   │   └── campaign_manager.cpp
│   │   ├── transport/
│   │   │   ├── transport_server.hpp    # Accepts Unix + TCP connections
│   │   │   ├── transport_server.cpp
│   │   │   ├── connection.hpp
│   │   │   └── connection.cpp
│   │   ├── cloud/
│   │   │   ├── cloud_client.hpp        # libcurl-based HTTP client
│   │   │   └── cloud_client.cpp
│   │   └── db/
│   │       ├── database.hpp            # SQLite wrapper
│   │       └── database.cpp
│   ├── ota-agent/                      # C++17 Linux daemon
│   │   ├── CMakeLists.txt
│   │   ├── main.cpp
│   │   ├── transport/
│   │   │   ├── transport_client.hpp
│   │   │   └── transport_client.cpp
│   │   ├── installer/
│   │   │   ├── sw_installer.hpp        # Software artifact install
│   │   │   ├── sw_installer.cpp
│   │   │   ├── fw_installer.hpp        # Firmware artifact install
│   │   │   └── fw_installer.cpp
│   │   ├── activator/
│   │   │   ├── sw_activator.hpp        # Symlink swap + service restart
│   │   │   ├── sw_activator.cpp
│   │   │   ├── fw_activator.hpp        # Bootloader prep + reboot trigger
│   │   │   └── fw_activator.cpp
│   │   ├── rollback/
│   │   │   ├── rollback_executor.hpp
│   │   │   └── rollback_executor.cpp
│   │   ├── health/
│   │   │   ├── health_monitor.hpp
│   │   │   └── health_monitor.cpp
│   │   └── bootloader/
│   │       ├── plugin_factory.hpp
│   │       ├── plugin_factory.cpp
│   │       ├── linux_generic_plugin.cpp
│   │       └── sim_plugin.cpp          # Simulated bootloader for testing
│   ├── ota-agent-freertos/             # C11 FreeRTOS agent (x86 sim build)
│   │   ├── CMakeLists.txt
│   │   ├── main.c                      # Entry point (simulates FreeRTOS task)
│   │   ├── agent_task.c               # Core agent loop
│   │   ├── agent_task.h
│   │   ├── transport_client.c
│   │   ├── transport_client.h
│   │   ├── fw_installer.c
│   │   ├── fw_installer.h
│   │   ├── fw_activator.c
│   │   ├── fw_activator.h
│   │   ├── health_monitor.c
│   │   ├── health_monitor.h
│   │   ├── hal/                        # Hardware abstraction layer
│   │   │   ├── hal.h                   # Flash, watchdog, reset abstractions
│   │   │   └── hal_sim.c              # x86 simulation implementation
│   │   └── bootloader/
│   │       ├── mcuboot_plugin.c
│   │       ├── mcuboot_plugin.h
│   │       └── mcuboot_sim.c           # Simulated MCUboot for testing
│   └── ota-pack/                       # C++17 CLI tool
│       ├── CMakeLists.txt
│       ├── main.cpp
│       ├── packager.hpp
│       ├── packager.cpp
│       ├── signer.hpp
│       └── signer.cpp
├── tests/
│   ├── unit/
│   │   ├── core/                       # libota-core unit tests
│   │   │   ├── test_manifest.cpp
│   │   │   ├── test_protocol.cpp
│   │   │   ├── test_state.cpp
│   │   │   ├── test_version.cpp
│   │   │   ├── test_crypto.cpp
│   │   │   └── test_error.cpp
│   │   ├── master/                     # ota-master unit tests
│   │   │   ├── test_config.cpp
│   │   │   ├── test_agent_registry.cpp
│   │   │   ├── test_download_state.cpp
│   │   │   ├── test_verification_pipeline.cpp
│   │   │   ├── test_compatibility_engine.cpp
│   │   │   ├── test_health_aggregator.cpp
│   │   │   └── test_campaign_manager.cpp
│   │   ├── agent/                      # ota-agent unit tests
│   │   │   ├── test_sw_installer.cpp
│   │   │   ├── test_sw_activator.cpp
│   │   │   ├── test_fw_installer.cpp
│   │   │   ├── test_rollback_executor.cpp
│   │   │   └── test_health_monitor.cpp
│   │   ├── agent_freertos/             # C unit tests for FreeRTOS agent
│   │   │   ├── test_agent_task.c
│   │   │   ├── test_fw_installer.c
│   │   │   └── test_hal_sim.c
│   │   └── bootloader/
│   │       ├── test_bootloader_interface.cpp
│   │       └── test_plugin_factory.cpp
│   ├── integration/
│   │   ├── test_master_agent_registration.cpp
│   │   ├── test_package_transfer.cpp
│   │   ├── test_sw_install_activate.cpp
│   │   ├── test_fw_install_activate.cpp
│   │   ├── test_rollback_flow.cpp
│   │   ├── test_download_resume.cpp
│   │   └── test_multi_agent_campaign.cpp
│   ├── e2e/
│   │   ├── test_full_sw_campaign.cpp
│   │   ├── test_full_fw_campaign.cpp
│   │   ├── test_error_recovery.cpp
│   │   └── test_version_supersede.cpp
│   └── fixtures/
│       ├── keys/
│       │   ├── test_private.pem        # ECDSA P-256 test private key
│       │   └── test_public.pem         # ECDSA P-256 test public key
│       ├── packages/
│       │   ├── valid_sw_package.ota     # Pre-built valid software package
│       │   ├── valid_fw_package.ota     # Pre-built valid firmware package
│       │   ├── corrupted_package.ota    # Tampered package (bad hash)
│       │   └── bad_signature.ota        # Valid content, wrong signature
│       ├── manifests/
│       │   ├── valid_sw_manifest.json
│       │   ├── valid_fw_manifest.json
│       │   └── incompatible_manifest.json
│       └── configs/
│           ├── test_master.json
│           └── test_agent.json
├── tools/
│   └── mock-cloud/                     # Python mock OTA cloud server
│       ├── requirements.txt
│       ├── server.py                   # FastAPI application
│       ├── config.py                   # Server configuration
│       ├── routes/
│       │   ├── updates.py              # /api/v1/updates endpoints
│       │   └── admin.py               # /api/v1/admin error injection
│       ├── storage/                    # Package file storage
│       └── scripts/
│           ├── generate_test_packages.py
│           └── generate_keys.py
├── config/
│   ├── ota-master.json.example
│   └── ota-agent.json.example
└── docs/                              # Original design documents
```

## 3. Data Model (SQLite — Master)

### Table: `agents`
| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `agent_id` | TEXT | PRIMARY KEY | UUID assigned on registration |
| `node_identifier` | TEXT | NOT NULL | Hardware node type identifier |
| `hardware_revision` | TEXT | NOT NULL | Hardware revision string |
| `architecture` | TEXT | NOT NULL | CPU architecture (aarch64, armv7, x86_64) |
| `os_type` | TEXT | NOT NULL | "linux" or "freertos" |
| `os_version` | TEXT | | OS version string |
| `sdk_version` | TEXT | | SDK/runtime version |
| `region` | TEXT | | Deployment region code |
| `ip_address` | TEXT | NOT NULL | Connection IP address |
| `port` | INTEGER | NOT NULL | Connection port |
| `transport_type` | TEXT | NOT NULL | "unix" or "tcp" |
| `status` | TEXT | NOT NULL DEFAULT 'registered' | registered, active, unreachable |
| `last_heartbeat` | INTEGER | NOT NULL | Unix timestamp of last heartbeat |
| `registered_at` | INTEGER | NOT NULL | Unix timestamp of registration |
| `capabilities_json` | TEXT | | JSON blob of agent capabilities |

### Table: `campaigns`
| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `campaign_id` | TEXT | PRIMARY KEY | UUID for the campaign |
| `package_id` | TEXT | NOT NULL | Package identifier from manifest |
| `version` | TEXT | NOT NULL | Package version string |
| `artifact_type` | TEXT | NOT NULL | "software" or "firmware" |
| `state` | TEXT | NOT NULL | See campaign states enum |
| `created_at` | INTEGER | NOT NULL | Unix timestamp |
| `updated_at` | INTEGER | NOT NULL | Unix timestamp |
| `manifest_json` | TEXT | NOT NULL | Full manifest JSON |

### Table: `campaign_targets`
| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `id` | INTEGER | PRIMARY KEY AUTOINCREMENT | Row ID |
| `campaign_id` | TEXT | NOT NULL FK(campaigns) | Parent campaign |
| `agent_id` | TEXT | NOT NULL FK(agents) | Target agent |
| `state` | TEXT | NOT NULL | Per-agent update state |
| `error_code` | INTEGER | | Last error code (0 = none) |
| `error_message` | TEXT | | Human-readable error |
| `retry_count` | INTEGER | NOT NULL DEFAULT 0 | Retry attempts |
| `healthy_cycles` | INTEGER | NOT NULL DEFAULT 0 | Consecutive healthy driving cycles |
| `updated_at` | INTEGER | NOT NULL | Unix timestamp |

### Table: `downloads`
| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `download_id` | TEXT | PRIMARY KEY | UUID for the download |
| `package_id` | TEXT | NOT NULL | Package identifier |
| `version` | TEXT | NOT NULL | Package version |
| `url` | TEXT | NOT NULL | Download URL |
| `total_size` | INTEGER | NOT NULL | Expected total bytes |
| `downloaded_bytes` | INTEGER | NOT NULL DEFAULT 0 | Bytes completed |
| `expected_crc32` | INTEGER | | CRC32 from cloud |
| `expected_sha256` | TEXT | | SHA-256 hex from cloud |
| `signature` | TEXT | | ECDSA signature (base64) |
| `state` | TEXT | NOT NULL | pending, in_progress, paused, completed, failed, superseded |
| `local_path` | TEXT | NOT NULL | Path to partial/complete file |
| `created_at` | INTEGER | NOT NULL | |
| `updated_at` | INTEGER | NOT NULL | |

### Table: `audit_log`
| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `id` | INTEGER | PRIMARY KEY AUTOINCREMENT | Row ID |
| `timestamp` | INTEGER | NOT NULL | Unix timestamp |
| `campaign_id` | TEXT | | Related campaign (nullable) |
| `agent_id` | TEXT | | Related agent (nullable) |
| `event_type` | TEXT | NOT NULL | See audit event types |
| `details_json` | TEXT | | JSON event details |

## 4. Wire Protocol Specification

### 4.1 Frame Format (all messages)

```
Offset  Size  Field           Description
0       2     magic           0x4F54 ("OT" — OTA)
2       1     version         Protocol version (0x01)
3       1     msg_type        Message type enum value
4       4     payload_length  Payload bytes (network byte order, big-endian)
8       4     sequence_num    Monotonically increasing per-connection
─────────────────────────────────────────────
12      N     payload         JSON or binary depending on msg_type
```

Total header: 12 bytes. Max payload: 16 MiB (16,777,216 bytes).

### 4.2 Message Type Enum

```c
typedef enum {
    OTA_MSG_REGISTER_REQ     = 0x01,
    OTA_MSG_REGISTER_ACK     = 0x02,
    OTA_MSG_HEARTBEAT        = 0x10,
    OTA_MSG_HEARTBEAT_ACK    = 0x11,
    OTA_MSG_TRANSFER_START   = 0x20,
    OTA_MSG_TRANSFER_DATA    = 0x21,
    OTA_MSG_TRANSFER_ACK     = 0x22,
    OTA_MSG_TRANSFER_NACK    = 0x23,
    OTA_MSG_TRANSFER_COMPLETE= 0x24,
    OTA_MSG_INSTALL_CMD      = 0x30,
    OTA_MSG_ACTIVATE_CMD     = 0x31,
    OTA_MSG_ROLLBACK_CMD     = 0x32,
    OTA_MSG_STATUS_REPORT    = 0x40,
    OTA_MSG_HEALTH_REPORT    = 0x41,
    OTA_MSG_CAMPAIGN_STATUS  = 0x50,
} ota_msg_type_t;
```

### 4.3 Key Message Payloads (JSON)

**REGISTER_REQ** (Agent → Master):
```json
{
    "node_identifier": "hpc-primary-001",
    "hardware_revision": "rev-B",
    "architecture": "x86_64",
    "os_type": "linux",
    "os_version": "5.15.0",
    "sdk_version": "1.0.0",
    "region": "EU",
    "installed_packages": [
        {"package_id": "nav-service", "version": "2.1.0", "type": "software"},
        {"package_id": "sensor-fw", "version": "1.3.2", "type": "firmware"}
    ]
}
```

**REGISTER_ACK** (Master → Agent):
```json
{
    "agent_id": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
    "status": "registered"
}
```

**TRANSFER_START** (Master → Agent):
```json
{
    "package_id": "nav-service",
    "version": "2.2.0",
    "artifact_type": "software",
    "total_size": 5242880,
    "sha256": "a3f2b8c1d4e5f67890abcdef1234567890abcdef1234567890abcdef12345678",
    "chunk_size": 65536
}
```

**TRANSFER_DATA** (Master → Agent): Binary payload. No JSON. Raw bytes of chunk.

**TRANSFER_ACK** (Agent → Master):
```json
{
    "package_id": "nav-service",
    "version": "2.2.0",
    "received_bytes": 131072,
    "status": "ok"
}
```

**INSTALL_CMD** (Master → Agent):
```json
{
    "package_id": "nav-service",
    "version": "2.2.0",
    "artifact_type": "software"
}
```

**ACTIVATE_CMD** (Master → Agent):
```json
{
    "package_id": "nav-service",
    "version": "2.2.0",
    "artifact_type": "software"
}
```

**ROLLBACK_CMD** (Master → Agent):
```json
{
    "package_id": "nav-service",
    "version": "2.2.0",
    "reason": "health_threshold_exceeded"
}
```

**STATUS_REPORT** (Agent → Master):
```json
{
    "agent_id": "a1b2c3d4-...",
    "package_id": "nav-service",
    "version": "2.2.0",
    "milestone": "install_completed",
    "error_code": 0,
    "error_message": "",
    "timestamp": 1708300000
}
```

**HEALTH_REPORT** (Agent → Master):
```json
{
    "agent_id": "a1b2c3d4-...",
    "driving_cycle": 42,
    "packages": [
        {
            "package_id": "nav-service",
            "version": "2.2.0",
            "error_count": 0,
            "crash_count": 0,
            "uptime_seconds": 3600,
            "status": "healthy"
        }
    ]
}
```

## 5. Mock Cloud REST API

Base URL: `http://localhost:8080/api/v1`

| Method | Path | Request | Response | Status |
|--------|------|---------|----------|--------|
| GET | `/updates` | query: `?node_identifier=X&region=Y` | `UpdateListResponse` | 200 |
| GET | `/updates/{package_id}/{version}/manifest` | — | `ManifestResponse` | 200, 404 |
| GET | `/updates/{package_id}/{version}/package` | Header: `Range: bytes=N-` | Binary stream | 200, 206, 404 |
| GET | `/updates/{package_id}/{version}/metadata` | — | `PackageMetadataResponse` | 200, 404 |
| POST | `/admin/inject-error` | `ErrorInjectionRequest` | 200 | 200 |
| POST | `/admin/add-package` | multipart: package file + manifest | 201 | 201, 400 |
| DELETE | `/admin/reset` | — | 204 | 204 |

### Response Types

**UpdateListResponse**:
```json
{
    "updates": [
        {
            "package_id": "nav-service",
            "version": "2.2.0",
            "artifact_type": "software",
            "size": 5242880,
            "released_at": "2026-02-20T00:00:00Z"
        }
    ]
}
```

**PackageMetadataResponse**:
```json
{
    "package_id": "nav-service",
    "version": "2.2.0",
    "size": 5242880,
    "crc32": 3045219847,
    "sha256": "a3f2b8c1...",
    "signature": "MEUCIQD...",
    "manifest": { ... }
}
```

**ErrorInjectionRequest**:
```json
{
    "type": "network_error",
    "target": "download",
    "config": {
        "fail_after_bytes": 1048576,
        "error": "connection_reset"
    }
}
```

## 6. Configuration Files

### Master Config (`/etc/ota/ota-master.json`)
```json
{
    "master": {
        "listen_unix": "/var/run/ota/master.sock",
        "listen_tcp": "0.0.0.0:9735",
        "db_path": "/var/lib/ota/master.db",
        "download_dir": "/var/lib/ota/downloads",
        "trusted_pubkey_path": "/etc/ota/keys/ota-cloud.pub.pem",
        "log_level": "info",
        "log_file": "/var/log/ota/master.log"
    },
    "cloud": {
        "base_url": "http://localhost:8080/api/v1",
        "timeout_seconds": 30,
        "retry_count": 3
    },
    "policy": {
        "check_on_ignition": true,
        "allowed_network_types": ["ethernet", "wifi"],
        "bandwidth_limit_kbps": 0,
        "require_safe_state_for_activation": true,
        "safe_states": ["parked", "short_parking"],
        "health_monitoring_cycles": 3,
        "error_threshold_percent": 10,
        "transfer_retry_count": 3,
        "transfer_chunk_size": 65536,
        "version_supersede_policy": "supersede"
    }
}
```

### Agent Config (`/etc/ota/ota-agent.json`)
```json
{
    "agent": {
        "node_identifier": "hpc-primary-001",
        "hardware_revision": "rev-B",
        "architecture": "x86_64",
        "os_type": "linux",
        "region": "EU",
        "install_base_dir": "/opt",
        "state_file": "/var/lib/ota/agent-state.json",
        "log_level": "info",
        "log_file": "/var/log/ota/agent.log"
    },
    "master": {
        "address": "/var/run/ota/master.sock",
        "transport": "unix"
    },
    "heartbeat": {
        "interval_seconds": 10,
        "timeout_seconds": 30
    },
    "bootloader": {
        "plugin": "sim",
        "config": {
            "slot_a_path": "/tmp/ota-sim/slot_a",
            "slot_b_path": "/tmp/ota-sim/slot_b"
        }
    }
}
```

## 7. Error Handling

### Error Code Ranges
```c
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
```

## 8. Update State Machine

### Campaign States (Master-side)
```
IDLE → DOWNLOADING → VALIDATING → TRANSFERRING → AWAITING_SAFE_STATE
→ INSTALLING → ACTIVATING → MONITORING → COMPLETED

Failure branches:
DOWNLOADING → DOWNLOAD_FAILED → IDLE (retry) or ABORTED
VALIDATING → VALIDATION_FAILED → ABORTED
TRANSFERRING → TRANSFER_FAILED → TRANSFERRING (retry) or ABORTED
INSTALLING → INSTALL_FAILED → ROLLED_BACK
ACTIVATING → ACTIVATE_FAILED → ROLLED_BACK
MONITORING → MONITORING_FAILED → ROLLED_BACK
```

### Per-Agent Target States
```
PENDING → TRANSFER_IN_PROGRESS → TRANSFERRED → INSTALLING → INSTALLED
→ ACTIVATING → ACTIVATED → MONITORING → COMPLETED

Failure: any → FAILED → ROLLING_BACK → ROLLED_BACK
```

### Milestone Enum
```c
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
```

## 9. Bootloader Plugin Interface

```c
/* Slot identifier */
typedef struct {
    uint8_t  slot_index;       /* 0 = A (primary), 1 = B (secondary) */
    char     label[32];        /* Human-readable: "slot_a", "slot_b" */
    bool     is_active;
    bool     is_valid;
    char     version[64];      /* Version currently in this slot */
} ota_slot_info_t;

/* Boot status */
typedef struct {
    ota_slot_info_t active_slot;
    ota_slot_info_t inactive_slot;
    uint32_t        boot_count;        /* Current boot attempt counter */
    uint32_t        boot_limit;        /* Max retries before fallback */
    bool            pending_activation;/* An image is staged and pending */
} ota_boot_status_t;

/* Plugin interface — implemented per platform */
typedef struct ota_bootloader_interface {
    /* Identify the inactive slot for staging */
    ota_error_t (*detect_inactive_slot)(void *ctx, ota_slot_info_t *out);

    /* Write firmware data to inactive slot (may be called in chunks) */
    ota_error_t (*stage_firmware)(void *ctx, const uint8_t *data,
                                  size_t len, size_t offset);

    /* Finalize staging — validate written data, mark slot as pending */
    ota_error_t (*prepare_activation)(void *ctx);

    /* Query current boot status */
    ota_error_t (*query_boot_status)(void *ctx, ota_boot_status_t *out);

    /* Confirm current image is good (post-activation) */
    ota_error_t (*mark_active_valid)(void *ctx);

    /* Mark a slot as invalid/bad */
    ota_error_t (*mark_slot_invalid)(void *ctx, uint8_t slot_index);

    /* Platform-specific context pointer */
    void *ctx;

    /* Plugin name for logging */
    const char *name;
} ota_bootloader_interface_t;

/* Factory function — returns appropriate plugin based on config */
ota_bootloader_interface_t *ota_bootloader_create(const char *plugin_name,
                                                   const char *config_json);
void ota_bootloader_destroy(ota_bootloader_interface_t *iface);
```

## 10. Package (.ota) File Format

A `.ota` file is a tar.gz archive with the following internal structure:

### Software Artifact Package
```
package.ota (tar.gz)
├── manifest.json          # Package manifest (REQUIRED)
├── signature.bin          # ECDSA P-256 signature of manifest.json (REQUIRED)
├── exe                    # Application executable (REQUIRED)
├── libs/                  # Shared libraries (OPTIONAL)
│   ├── libcustom.so
│   └── libvendor.so.2
└── res/                   # Resources (OPTIONAL)
    ├── config.json
    └── assets/
```

### Firmware Artifact Package
```
package.ota (tar.gz)
├── manifest.json          # Package manifest (REQUIRED)
├── signature.bin          # ECDSA P-256 signature (REQUIRED)
├── exe                    # Firmware binary image (REQUIRED)
└── rdinit                 # initramfs binary (OPTIONAL, Linux FW only)
```

### Manifest Schema
```json
{
    "package_identifier": "nav-service",
    "version": "2.2.0",
    "artifact_type": "software",
    "node_identifier": "hpc-primary",
    "minimum_os_version": "5.10.0",
    "maximum_os_version": "6.99.0",
    "minimum_sdk_version": "1.0.0",
    "maximum_sdk_version": "2.0.0",
    "targeted_region": "EU",
    "custom_oem_rules": {
        "variant": "sport",
        "feature_flags": ["premium_nav"]
    },
    "size": 5242880,
    "crc32": 3045219847,
    "sha256": "a3f2b8c1d4e5f67890abcdef1234567890abcdef1234567890abcdef12345678",
    "created_at": "2026-02-20T00:00:00Z"
}
```

## 11. Filesystem Layout (Runtime)

### Master
```
/etc/ota/ota-master.json                   # Configuration
/etc/ota/keys/ota-cloud.pub.pem           # Trusted public key
/var/lib/ota/master.db                     # SQLite database
/var/lib/ota/downloads/                    # In-progress and completed downloads
/var/lib/ota/downloads/{pkg_id}_{version}.ota.partial
/var/lib/ota/downloads/{pkg_id}_{version}.ota
/var/lib/ota/downloads/{pkg_id}_{version}.state.json  # Per-download state
/var/log/ota/master.log                    # Log file
/var/run/ota/master.sock                   # Unix domain socket
```

### Agent (Linux)
```
/etc/ota/ota-agent.json                    # Configuration
/var/lib/ota/agent-state.json              # Persistent agent state
/var/log/ota/agent.log                     # Log file
/opt/{package_id}/{version}/               # Installed version directory
/opt/{package_id}/{version}/exe            # Executable
/opt/{package_id}/{version}/libs/          # Libraries
/opt/{package_id}/{version}/res/           # Resources
/opt/{package_id}/newer   → {version}/     # Staged (not yet activated)
/opt/{package_id}/current → {version}/     # Active version
/opt/{package_id}/prev    → {version}/     # Previous (rollback target)
```

## 12. Build Commands

```bash
# Configure (debug with tests)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DOTA_BUILD_TESTS=ON -DOTA_BUILD_MOCK_CLOUD=ON

# Build all
cmake --build build -j$(nproc)

# Run all tests
cd build && ctest --output-on-failure

# Run unit tests only
cd build && ctest -R "^unit_" --output-on-failure

# Run integration tests only
cd build && ctest -R "^integ_" --output-on-failure

# Coverage
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DOTA_BUILD_TESTS=ON -DOTA_COVERAGE=ON
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
gcovr --root .. --html --html-details -o coverage.html

# Type checking / static analysis
cmake --build build --target clang-tidy

# Start mock cloud
cd tools/mock-cloud && pip install -r requirements.txt && uvicorn server:app --port 8080

# Build a test package
./build/ota-pack create --manifest tests/fixtures/manifests/valid_sw_manifest.json \
    --dir tests/fixtures/test-app/ --output /tmp/test.ota
./build/ota-pack sign --key tests/fixtures/keys/test_private.pem --package /tmp/test.ota
```
