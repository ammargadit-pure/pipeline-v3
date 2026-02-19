# PRD: Automotive Over-The-Air (OTA) Update System

## 1. Product Overview

A vehicle-side OTA update system enabling secure, reliable, and resumable updates of software applications and ECU firmware during normal vehicle operation. The system coordinates updates across heterogeneous compute nodes (Linux-based HPCs and FreeRTOS-based ECUs) through a centralized Master orchestrator and distributed per-node Agents, with platform-abstracted bootloader integration for firmware activation and rollback.

## 2. Problem Statement

Vehicles contain multiple ECUs and HPCs running different software and firmware that must be updated in the field. Updates must:
- Download in background without disrupting running software
- Survive power loss, network interruption, and driving cycle boundaries
- Guarantee cryptographic authenticity and integrity before any activation
- Activate only in safe vehicle states with automatic rollback on failure
- Support both OS-based software (service restart) and bare-metal firmware (A/B slot swap with reboot)
- Provide full lifecycle visibility for monitoring, audit, and diagnostics

## 3. Users / Actors

| Actor | Description | Environment |
|-------|-------------|-------------|
| **OTA Master** | System-level orchestrator running on primary HPC. Single instance. Central authority for the entire OTA process. | Linux (HPC) |
| **OTA Agent (Linux)** | Node-local executor daemon on Linux-based ECUs/HPCs. Handles SW install via symlinks, FW install via bootloader plugin. | Linux (HPC/ECU) |
| **OTA Agent (FreeRTOS)** | Embedded module within FreeRTOS firmware on microcontroller ECUs. Handles FW install via flash slots. | FreeRTOS (MCU) |
| **Flash Bootloader** | Boot-time authority for firmware validation, slot selection, and automatic fallback. Not our code — we integrate via plugin interface. | Hardware-specific |
| **OTA Cloud Service** | Backend providing update packages, manifests, and signatures. Mocked for development. | Cloud (external) |
| **Vehicle Lifecycle** | Events (IGN ON/OFF, safe-state, parking) that gate download and activation decisions. Simulated for development. | Vehicle systems |

## 4. System Architecture Overview

```
                    +-----------------------+
                    |   OTA Cloud Service   |
                    |  (Mock Test Server)   |
                    +----------+------------+
                               | HTTPS (REST)
                               v
+--------------------------------------------------------------+
|                        Vehicle (HPC)                         |
|  +------------------+          +------------------+          |
|  |   OTA Master     |  <--->   |  OTA Agent       |          |
|  |  (Orchestrator)  |  Unix    |  (Linux Daemon)  |          |
|  |                  |  Socket  |                  |          |
|  | - Cloud client   |          | - SW install     |          |
|  | - Download mgr   |          | - FW staging     |          |
|  | - Verification   |          | - Activation     |          |
|  | - Compatibility  |          | - Rollback       |          |
|  | - Activation auth|          | - Health monitor |          |
|  | - Health monitor |          | - Bootloader IF  |          |
|  | - Campaign state |          +------------------+          |
|  +------------------+                                        |
|           |  TCP                                             |
|           v                                                  |
|  +------------------+     +------------------+               |
|  | OTA Agent (Linux)|     | OTA Agent        |               |
|  | (Remote ECU)     |     | (FreeRTOS ECU)   |               |
|  +------------------+     +------------------+               |
|           |                        |                         |
|           v                        v                         |
|  +------------------+     +------------------+               |
|  | Bootloader Plugin|     | Bootloader Plugin|               |
|  | (Linux/UBoot)    |     | (MCUboot)        |               |
|  +------------------+     +------------------+               |
+--------------------------------------------------------------+
```

## 5. Non-Negotiable Requirements (Phase 1)

### 5.1 OTA Master
- **M-01**: Load configuration from file on startup (`/etc/ota/ota-master.conf`)
- **M-02**: Agent registration — broadcast registration event, handle late-joining agents
- **M-03**: Driving cycle awareness — respond to IGN ON/OFF events, gate operations on vehicle state
- **M-04**: Resumable downloads — persistent state at `/var/lib/ota/downloads/`, HTTP range resume, survive power cycles
- **M-05**: New version handling — detect superseding versions, archive/discard partial downloads
- **M-06**: Post-download validation pipeline: size check → CRC32 → SHA-256 → ECDSA P-256 signature verification
- **M-07**: Manifest-based compatibility — per-agent eligibility (hardware, software version, OS version, region, OEM rules)
- **M-08**: Package transfer to eligible agents with acknowledgement, retry, and progress tracking
- **M-09**: Safe-state gating — only authorize activation when vehicle is in approved state
- **M-10**: Post-activation health monitoring — aggregate health signals across driving cycles, threshold-based rollback decision
- **M-11**: Master-initiated rollback coordination — send rollback command, handle timeout, update campaign state
- **M-12**: Campaign status aggregation, per-node progress tracking, persistent audit trail

### 5.2 OTA Agent (Shared Core)
- **A-01**: Agent startup, registration with Master, periodic heartbeat, command reception
- **A-02**: Status reporting at milestones: install_started, install_completed, activate_started, activate_completed, failure(reason)
- **A-03**: Artifact type dispatch — route to SW or FW installation path based on manifest
- **A-04**: Local rollback execution on install/activate failure — immediate, no Master instruction needed
- **A-05**: Post-activation local health monitoring — crash detection, error tracking, periodic health reports

### 5.3 OTA Agent (Linux-Specific)
- **AL-01**: Software installation — extract to `/opt/{package_id}/{version}/`, create `newer` symlink
- **AL-02**: Software activation — atomic symlink swap (`newer`→`current`, `current`→`prev`), service restart
- **AL-03**: Software rollback — revert `current`→`prev`, restart service
- **AL-04**: Firmware staging — write to inactive slot/partition via bootloader plugin
- **AL-05**: Firmware activation — update boot metadata, initiate controlled reboot
- **AL-06**: Handle `/libs`, `/res`, `/exe` package structure with library isolation per application

### 5.4 OTA Agent (FreeRTOS-Specific)
- **AF-01**: Embedded within firmware image as a task/module (not a standalone daemon)
- **AF-02**: Firmware staging — write received image to inactive flash slot via bootloader plugin
- **AF-03**: Firmware activation — mark staged image as pending, trigger controlled reset
- **AF-04**: Firmware rollback — coordinated via bootloader (automatic on boot failure)
- **AF-05**: Communication with Master over TCP/IP (lwIP stack) using same protocol as Linux agent

### 5.5 Bootloader Plugin Layer
- **B-01**: Abstract `BootloaderInterface` class: `detectInactiveSlot()`, `stageFirmware()`, `prepareActivation()`, `queryBootStatus()`
- **B-02**: Linux generic plugin — for Linux ECUs with UBoot-style env vars or generic A/B partitions
- **B-03**: MCUboot plugin — for FreeRTOS ECUs using MCUboot slot management
- **B-04**: Plugin factory — select correct plugin based on configuration or node identifier
- **B-05**: Agent integration — call interface methods during firmware install/activate/rollback flows

### 5.6 Artifact Packaging & Tooling
- **P-01**: Define manifest schema (package_identifier, version, node_identifier, min/max OS version, min/max SDK version, targeted_region, custom_oem_rules)
- **P-02**: Package structure: `/exe` (mandatory), `/libs` (SW only), `/res` (optional), `/rdinit` (FW only)
- **P-03**: CLI packaging tool — create archive from source directory, embed manifest
- **P-04**: Offline signing tool — sign package with ECDSA P-256, embed signature
- **P-05**: Offline validation tool — verify package integrity and signature without Master

### 5.7 Mock Cloud Server & Test Harness
- **T-01**: REST API endpoints: list available updates, get manifest, download package (with range support)
- **T-02**: Package hosting — serve pre-built, pre-signed packages
- **T-03**: Configurable error injection — simulate partial downloads, network failures, corrupted packages, version superseding
- **T-04**: Multiple concurrent package versions for testing upgrade and supersede scenarios

## 6. Deferred to Phase 2

| Item | Reason |
|------|--------|
| **Self-Update & Master Failover** (WBS #25) | Highest complexity. Core OTA flow must work first. |
| **Platform-specific bootloader plugins** (Orin nvbootctrl, NXP-specific UBoot) | Abstract interface + generic plugins first. Platform-specific plugins when hardware is available. |
| **Boot-time image validation & automatic fallback** (WBS #22) | This is bootloader firmware code, not application code. Depends on actual hardware. |
| **HPC firmware updates** | Explicitly out of scope per scope document |
| **Key rotation** | Mentioned as optional in tech spec |
| **DBUS diagnostic interface** | Observability via file/log first, DBUS later |

## 7. Technical Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Language (Master, Linux Agent)** | C++17 | Modern features (std::optional, std::filesystem, structured bindings), wide embedded Linux compiler support |
| **Language (FreeRTOS Agent, Bootloader plugins)** | C11 | Maximum portability to MCU toolchains, compatible with FreeRTOS ecosystem |
| **Build System** | CMake 3.20+ | Industry standard for C/C++ cross-platform builds |
| **IPC (same-node)** | Unix domain sockets | Zero network overhead, reliable, simple |
| **IPC (cross-node)** | TCP sockets | FreeRTOS ECUs have Ethernet/IP (lwIP). Same protocol for Linux and FreeRTOS agents. |
| **Protocol format** | Custom binary + JSON control messages | Binary for file transfer efficiency, JSON for control/status messages (human-readable, debuggable) |
| **Crypto (Linux)** | OpenSSL 3.x | SHA-256, ECDSA P-256, CRC32. Battle-tested, widely available. |
| **Crypto (FreeRTOS)** | mbedTLS | Lightweight, designed for embedded. Same algorithms (SHA-256, ECDSA P-256). |
| **HTTP client** | libcurl | Resumable downloads, range requests, TLS, proven reliability |
| **JSON parsing** | nlohmann-json (C++), cJSON (C) | Best-in-class for each language |
| **Database (Master)** | SQLite | Persistent state, ACID transactions, single-file, no server. Perfect for embedded. |
| **Config format** | JSON | Consistent with manifest format, same parser |
| **Logging** | spdlog (C++), custom lightweight logger (C) | Fast, configurable levels, file rotation |
| **Testing framework** | GoogleTest + GoogleMock (C++), Unity (C) | Industry standard for each ecosystem |
| **Mock cloud server** | Python (Flask/FastAPI) | Rapid development, easy to make configurable, not production code |
| **Package archive format** | tar.gz | Standard, well-supported, streamable |

## 8. Component Breakdown

### 8.1 Shared Core Library (`libota-core`)
**Language**: C (for maximum sharing between C++ and C codebases)

Contains:
- Manifest parsing and validation
- Package verification utilities (CRC32, SHA-256 hash computation)
- Update state machine (shared states and transitions)
- Error codes and error domain definitions
- Atomic file/flash persistence utilities
- Protocol message definitions (serialize/deserialize)
- Version comparison utilities

This library is linked by both the C++ Master/Linux-Agent and the C FreeRTOS-Agent.

### 8.2 OTA Master (`ota-master`)
**Language**: C++17
**Runs as**: Linux daemon (systemd service)

Modules:
- `config/` — Configuration loader and policy engine
- `registry/` — Agent registration, discovery, heartbeat tracking
- `download/` — Resumable download manager with persistent state
- `verification/` — Validation pipeline (size, CRC32, SHA-256, ECDSA P-256)
- `compatibility/` — Manifest parser, per-agent eligibility engine
- `transfer/` — Package distribution to agents, progress tracking, retry
- `activation/` — Safe-state evaluation, activation authorization, command dispatch
- `monitoring/` — Post-activation health aggregation, threshold engine
- `rollback/` — Rollback coordination, campaign state transitions
- `campaign/` — Campaign lifecycle, status aggregation, audit logging
- `transport/` — IPC server (Unix socket + TCP listener)
- `db/` — SQLite database layer for persistent state
- `cloud/` — HTTP client for OTA cloud service interaction

### 8.3 OTA Agent — Linux (`ota-agent`)
**Language**: C++17
**Runs as**: Linux daemon (systemd service)

Modules:
- `core/` — Links to `libota-core` for shared logic
- `transport/` — IPC client (Unix socket or TCP to Master)
- `installer/` — Software artifact installation (extract, symlink, libs/res handling)
- `activator/` — Software activation (symlink swap, service restart) and firmware activation (bootloader plugin call + reboot)
- `rollback/` — Local rollback execution (SW symlink revert, FW slot revert)
- `health/` — Post-activation health monitoring, crash/error detection
- `bootloader/` — Bootloader plugin loader and interface calls
- `supervisor/` — systemd integration for service management

### 8.4 OTA Agent — FreeRTOS (`ota-agent-freertos`)
**Language**: C11
**Runs as**: FreeRTOS task within firmware image

Modules:
- `core/` — Links to `libota-core` for shared logic
- `transport/` — TCP client over lwIP
- `installer/` — Firmware staging to inactive flash slot
- `activator/` — Mark pending image, trigger reset
- `rollback/` — Report boot status, coordinate with bootloader
- `health/` — Runtime health monitoring, error counters
- `bootloader/` — MCUboot plugin (or platform-specific plugin)
- `hal/` — Hardware abstraction (flash read/write, watchdog, reset control)

### 8.5 Bootloader Plugin Interface
**Language**: C11 (header-only interface, implementations per platform)

```c
// BootloaderInterface — pure virtual in C via function pointers
typedef struct {
    int (*detect_inactive_slot)(void *ctx, slot_info_t *out);
    int (*stage_firmware)(void *ctx, const uint8_t *data, size_t len, const slot_info_t *slot);
    int (*prepare_activation)(void *ctx, const slot_info_t *slot);
    int (*query_boot_status)(void *ctx, boot_status_t *out);
    int (*mark_slot_valid)(void *ctx, const slot_info_t *slot);
    int (*mark_slot_invalid)(void *ctx, const slot_info_t *slot);
    int (*get_active_slot)(void *ctx, slot_info_t *out);
    void *ctx; // platform-specific context
} bootloader_interface_t;
```

Plugin implementations:
- **Generic Linux plugin**: Reads/writes partition metadata, handles A/B switch via env vars or config files
- **MCUboot plugin**: Uses MCUboot image trailer API for slot management
- **Plugin factory**: Selects correct plugin at startup based on config `node_identifier`

### 8.6 Artifact Packaging Tooling (`ota-pack`)
**Language**: Python or C++17 CLI

Commands:
- `ota-pack create --manifest manifest.json --dir ./build-output/ --output package.ota`
- `ota-pack sign --key private.pem --package package.ota`
- `ota-pack verify --pubkey public.pem --package package.ota`
- `ota-pack inspect --package package.ota` (dump manifest + structure)

### 8.7 Mock Cloud Server (`ota-cloud-mock`)
**Language**: Python (FastAPI)

Features:
- REST API: `GET /updates`, `GET /updates/{id}/manifest`, `GET /updates/{id}/package`
- HTTP range request support for resumable downloads
- Package hosting from local directory
- Pre-signed packages served with correct metadata (size, CRC32, SHA-256, signature)
- Configurable behaviors via API or config file:
  - Inject network errors (timeout, connection reset, slow response)
  - Serve corrupted packages (bad CRC, bad signature, truncated)
  - Simulate version superseding mid-download
  - Throttle bandwidth
  - Multiple package versions simultaneously

## 9. Communication Protocol

### 9.1 Message Types (Master ↔ Agent)

| Category | Message | Direction | Description |
|----------|---------|-----------|-------------|
| Registration | `REGISTER_REQ` | Agent→Master | Agent identity, capabilities, current SW/FW versions |
| Registration | `REGISTER_ACK` | Master→Agent | Registration confirmed, agent ID assigned |
| Heartbeat | `HEARTBEAT` | Agent→Master | Periodic alive signal with basic status |
| Transfer | `TRANSFER_START` | Master→Agent | Begin package transfer (package_id, version, size, hash) |
| Transfer | `TRANSFER_DATA` | Master→Agent | Binary chunk of package data |
| Transfer | `TRANSFER_ACK` | Agent→Master | Chunk received + cumulative progress |
| Transfer | `TRANSFER_NACK` | Agent→Master | Transfer failure (reason code) |
| Transfer | `TRANSFER_COMPLETE` | Master→Agent | All chunks sent, finalize |
| Command | `INSTALL_CMD` | Master→Agent | Install specified package (package_id, version, type) |
| Command | `ACTIVATE_CMD` | Master→Agent | Activate specified package |
| Command | `ROLLBACK_CMD` | Master→Agent | Roll back specified package |
| Status | `STATUS_REPORT` | Agent→Master | Milestone report (install_started, install_completed, activate_started, activate_completed, failure) |
| Health | `HEALTH_REPORT` | Agent→Master | Periodic health data (error counts, crash events, resource usage) |
| Campaign | `CAMPAIGN_STATUS` | Master→Agent | Campaign-wide status broadcast |

### 9.2 Wire Format
- **Header** (fixed 12 bytes): magic (2B) + version (1B) + type (1B) + payload_length (4B) + sequence (4B)
- **Payload**: JSON for control messages, raw binary for file transfer chunks
- **Transport**: TCP with length-prefixed framing. Unix domain socket for same-node.

## 10. Security Model

- **Package signing**: ECDSA P-256 with SHA-256 digest
- **Verification pipeline**: Size → CRC32 → SHA-256 → ECDSA signature (all must pass before any installation)
- **Trust anchor**: EC public key provisioned on vehicle (file-based for dev, secure storage for production)
- **No activation without verification**: Hard gate enforced by Master
- **Agent-side verification**: Agent re-verifies package hash after transfer before installation
- **Transport security**: TLS for cloud communication. Internal vehicle transport is plaintext for Phase 1 (TLS between Master-Agent is a Phase 2 enhancement)

## 11. Update Lifecycle State Machine

```
                    IDLE
                     │
            [Cloud has update]
                     │
                     v
               DOWNLOADING ──────> DOWNLOAD_FAILED
                     │                    │
              [Complete]          [Retry / Abort]
                     │
                     v
               VALIDATING ──────> VALIDATION_FAILED
                     │                    │
              [All checks pass]   [Reject package]
                     │
                     v
              TRANSFERRING ──────> TRANSFER_FAILED
                     │                    │
              [Agent ACKs]         [Retry / Abort]
                     │
                     v
            AWAITING_SAFE_STATE
                     │
            [Vehicle in safe state]
                     │
                     v
              INSTALLING ──────> INSTALL_FAILED ──> ROLLED_BACK
                     │
              [Success]
                     │
                     v
              ACTIVATING ──────> ACTIVATE_FAILED ──> ROLLED_BACK
                     │
              [Success]
                     │
                     v
              MONITORING ──────> MONITORING_FAILED ──> ROLLED_BACK
                     │
           [Healthy across N cycles]
                     │
                     v
               COMPLETED
```

## 12. Constraints and Assumptions

1. **Development environment**: x86_64 Linux. All agents simulated as separate processes on the same host.
2. **No real hardware initially**: Bootloader plugins tested against mock/simulated flash.
3. **FreeRTOS agent**: Compiled as x86 binary for testing (simulates FreeRTOS behavior with POSIX threads). Real FreeRTOS cross-compilation is a deployment concern, not a development concern.
4. **Vehicle events**: Driving cycle events (IGN ON/OFF) and safe-state signals are simulated via the test harness or manual triggers.
5. **Single Master instance**: No HA/failover in Phase 1.
6. **Ethernet/TCP for all agents**: Both Linux and FreeRTOS agents communicate with Master over TCP/IP. No CAN bus support in Phase 1.
7. **Package size**: Assumed to fit on target storage. No partial/streaming installation.
8. **Database**: SQLite on Master. Flat files (JSON) for Agent local state on Linux. Flash-backed struct on FreeRTOS.
9. **Time**: No NTP or time sync assumed. Driving cycle counters used instead of wall-clock time for monitoring.

## 13. WBS Mapping to Implementation Phases

### Phase 1A: Foundation (WBS #1)
- CMake project structure (monorepo with multiple targets)
- `libota-core` shared library
- Logging, config parsing, error domain
- Atomic persistence utilities
- Protocol message definitions

### Phase 1B: Master Core (WBS #2, #3, #4, #5, #6)
- Master startup and configuration
- Agent registration (in-memory registry)
- Driving cycle event handling
- Resumable download manager
- Version superseding logic
- Full verification pipeline (CRC32, SHA-256, ECDSA)
- Manifest compatibility engine

### Phase 1C: Master ↔ Agent Communication (WBS #7, #8, #11)
- Transport layer (Unix socket server + TCP server)
- Package transfer protocol with ACK/NACK/retry
- Agent command dispatch (install, activate, rollback)
- Status reporting reception and aggregation

### Phase 1D: Linux Agent Core (WBS #12, #14, #16)
- Agent daemon with registration and heartbeat
- Software artifact installation (extract + symlink)
- Software artifact activation (symlink swap + service restart)
- Software rollback (symlink revert + service restart)

### Phase 1E: Bootloader Interface + Linux FW Flow (WBS #18, #13, #15, #21)
- Abstract bootloader interface definition
- Generic Linux bootloader plugin
- Firmware staging via plugin
- Firmware activation via controlled reboot
- Plugin factory and agent integration

### Phase 1F: FreeRTOS Agent (WBS #11 partial, #13 partial, #23)
- FreeRTOS agent task (x86 simulation build)
- TCP transport client (POSIX sockets simulating lwIP)
- Firmware staging via MCUboot plugin (simulated)
- Agent ↔ bootloader communication protocol

### Phase 1G: Monitoring & Rollback (WBS #9, #10, #17)
- Post-activation health monitoring (Master-side aggregation)
- Threshold-based rollback decision engine
- Master-initiated rollback coordination
- Agent-side local health monitoring and reporting

### Phase 1H: Packaging & Tooling (WBS #24)
- Manifest schema definition
- CLI packaging tool
- Offline signing and verification tool

### Phase 1I: Mock Cloud & Testing (WBS #27)
- Mock cloud server (FastAPI)
- Configurable test harness with error injection
- Integration tests: full download→verify→transfer→install→activate→monitor flow
- Rollback scenario tests
- Multi-agent campaign tests

### Phase 1J: Observability (WBS #26)
- Campaign status aggregation
- Per-node progress reporting
- Persistent audit trail (file-based logs)

### Phase 2 (Deferred)
- Self-update & Master failover (WBS #25)
- Platform-specific bootloader plugins — Orin, NXP (WBS #19, #20)
- Boot-time image validation & automatic fallback (WBS #22)
- Internal TLS for Master↔Agent
- DBUS diagnostic interface
- Key rotation mechanism

## 14. Acceptance Criteria (Phase 1 Complete)

1. Master downloads a package from mock cloud, survives a simulated power interruption, resumes, and completes download.
2. Master validates a package through the full pipeline (size, CRC32, SHA-256, ECDSA) and rejects tampered packages.
3. Master identifies eligible agents from manifest and transfers package with acknowledgement.
4. Linux Agent installs a software artifact, activates it via symlink swap, and reports success to Master.
5. Linux Agent detects a failed activation and automatically rolls back to previous version.
6. Linux Agent stages firmware via bootloader plugin and activates via simulated reboot.
7. FreeRTOS Agent (x86 sim) receives firmware, stages to simulated flash slot, and activates.
8. Master monitors post-activation health across simulated driving cycles and triggers rollback when threshold exceeded.
9. Full campaign lifecycle runs: download → verify → transfer → install → activate → monitor → complete.
10. Mock cloud server can inject errors (corrupted package, network failure, version supersede) and system handles them correctly.
11. Multiple agents (Linux + FreeRTOS) can be updated as part of a single campaign.
12. All operations are logged with persistent audit trail.
