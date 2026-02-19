# Test Strategy: Automotive OTA Update System

## Test Infrastructure Report

### Available (ready to use)
- GCC 11.4 with C++17 and C11 support
- CMake 3.22
- OpenSSL 3.0.2 (libssl-dev)
- SQLite 3.37.2 (libsqlite3-dev)
- Python 3.10 + FastAPI + uvicorn + httpx + pytest
- curl, jq, git, make, docker
- x86_64 Linux (Ubuntu 22.04)
- GoogleTest / Unity — fetched via CMake FetchContent (no system install needed)
- nlohmann-json / cJSON / spdlog — fetched via CMake FetchContent

### Needs Installation Before Development
- **libcurl4-openssl-dev** — REQUIRED for Master (cloud client, download manager)
  - `sudo apt install libcurl4-openssl-dev`
- **libarchive-dev** — REQUIRED for ota-pack and agent installer (tar.gz extract)
  - `sudo apt install libarchive-dev`
- **gcovr** — REQUIRED for coverage reports
  - `pip3 install gcovr` or `sudo apt install gcovr`

### Optional (nice to have)
- **valgrind** — memory leak detection: `sudo apt install valgrind`
- **clang-tidy** — static analysis: `sudo apt install clang-tidy`
- **libmbedtls-dev** — only needed if building FreeRTOS agent with mbedTLS backend
  - For x86 simulation, we compile FreeRTOS agent with OpenSSL backend
  - `sudo apt install libmbedtls-dev` (optional, Phase 2 concern)

### Not Needed
- Docker — not required for any test tier (all tests run natively)
- SSH / staging — no remote deployment
- Browser / Playwright — no UI

### Disk Space Warning
- Filesystem at 94% capacity (28 GB free). Build artifacts + test fixtures should be fine but monitor.

---

## Testing Framework Decisions

| Component | Framework | Rationale |
|-----------|-----------|-----------|
| libota-core (C11) | GoogleTest (C++ wrapper) | Test C code from C++ test files. GTest's matchers + fixtures superior to Unity for complex assertions. Keep Unity only for FreeRTOS-specific tests. |
| ota-master (C++17) | GoogleTest + GoogleMock | MockCloudClient, MockTransportServer for unit isolation |
| ota-agent (C++17) | GoogleTest + GoogleMock | MockBootloaderPlugin, MockTransportClient |
| ota-agent-freertos (C11) | Unity + CMock | C-only tests matching FreeRTOS ecosystem. HAL mocked via CMock. |
| ota-pack (C++17) | GoogleTest | CLI tool, test packager/signer logic |
| mock-cloud (Python) | pytest + httpx | FastAPI TestClient for route testing |
| Integration tests | GoogleTest | Real Master + Agent processes, real IPC |
| E2E tests | GoogleTest + bash scripts | Full lifecycle with mock cloud |

---

## Module: libota-core

### Unit tests (developer writes, mocks OK within tests/unit/core/)

**test_error.cpp** — `TEST(Error, ...)`:
- `ErrorStrReturnsStringForAllCodes`: Every `ota_error_t` value maps to a non-null, non-empty string
- `ErrorStrReturnsUnknownForInvalid`: Out-of-range value returns "unknown"
- `RangeCheckProtocol`: `ota_error_is_protocol(100)` true, `ota_error_is_protocol(99)` false, `ota_error_is_protocol(200)` false
- `RangeCheckVerify`: `ota_error_is_verify(200)` true, `ota_error_is_verify(299)` true
- `RangeCheckAllCategories`: Each range check function correctly identifies its own range and rejects others

**test_manifest.cpp** — `TEST(Manifest, ...)`:
- `ParseValidSwManifest`: Parse test SW manifest JSON → all fields match expected values
- `ParseValidFwManifest`: Parse test FW manifest JSON → artifact_type == OTA_ARTIFACT_FIRMWARE
- `ParseMissingRequiredField`: JSON without `package_identifier` → returns `OTA_ERR_INVALID_ARGUMENT`
- `ParseEmptyJson`: Empty string → returns error
- `ParseNullJson`: NULL pointer → returns `OTA_ERR_INVALID_ARGUMENT`
- `ParseMalformedJson`: Invalid JSON → returns `OTA_ERR_PROTOCOL_PARSE`
- `ValidateCompleteManifest`: Valid manifest → `OTA_OK`
- `ValidateEmptyPackageId`: Empty package_identifier → error
- `ValidateEmptyVersion`: Empty version → error
- `SerializeRoundtrip`: Parse → serialize → parse → all fields identical
- `FreeDoubleFreeIsSafe`: Free, then free again → no crash

**test_protocol.cpp** — `TEST(Protocol, ...)`:
- `HeaderSerializeDeserializeRoundtrip`: Serialize → deserialize → all fields match
- `HeaderNetworkByteOrder`: Serialized bytes match expected big-endian values for known input
- `HeaderValidateBadMagic`: Wrong magic → `OTA_ERR_PROTOCOL_MAGIC`
- `HeaderValidateBadVersion`: Version 0xFF → `OTA_ERR_PROTOCOL_VERSION`
- `HeaderValidateOversizedPayload`: payload_length > 16 MiB → `OTA_ERR_PROTOCOL_PAYLOAD_SIZE`
- `HeaderValidateZeroPayload`: payload_length == 0 → `OTA_OK` (valid for heartbeat-ack)
- `MsgIsBinaryTransferData`: `ota_msg_is_binary(OTA_MSG_TRANSFER_DATA)` is true
- `MsgIsNotBinaryForOthers`: All non-TRANSFER_DATA types return false
- `MsgTypeStrReturnsNonNull`: Every `ota_msg_type_t` value → non-null string
- `BufferTooSmallForSerialize`: buf_len < 12 → error

**test_state.cpp** — `TEST(State, ...)`:
- `CampaignStateStrRoundtrip`: For every `ota_campaign_state_t`, `from_str(str(x)) == x`
- `TargetStateStrRoundtrip`: For every `ota_target_state_t`, `from_str(str(x)) == x`
- `MilestoneStrRoundtrip`: For every `ota_milestone_t`, `from_str(str(x)) == x`
- `DownloadStateStrRoundtrip`: For every `ota_download_state_t`, `from_str(str(x)) == x`
- `CampaignValidTransitions`: IDLE → DOWNLOADING (valid), DOWNLOADING → VALIDATING (valid)
- `CampaignInvalidTransitions`: COMPLETED → DOWNLOADING (invalid), IDLE → INSTALLING (invalid)
- `TargetValidTransitions`: PENDING → TRANSFER_IN_PROGRESS (valid), INSTALLED → ACTIVATING (valid)
- `TargetInvalidTransitions`: COMPLETED → PENDING (invalid)
- `FromStrInvalidReturnsDefault`: Unknown string → returns sensible default or error indicator

**test_version.cpp** — `TEST(Version, ...)`:
- `ParseValidVersion`: "2.2.0" → major=2, minor=2, patch=0
- `ParseMajorOnly`: "5" → expect parse error (require MAJOR.MINOR.PATCH)
- `ParseInvalid`: "abc" → error
- `ParseEmpty`: "" → error
- `CompareGreater`: "2.2.0" > "2.1.0" → positive result
- `CompareEqual`: "1.0.0" == "1.0.0" → zero
- `CompareLess`: "1.0.0" < "2.0.0" → negative result
- `ComparePatchLevel`: "1.0.1" > "1.0.0"
- `InRangeTrue`: "5.15.0" in ["5.10.0", "6.99.0"] → true
- `InRangeFalse`: "4.0.0" in ["5.10.0", "6.99.0"] → false
- `InRangeBoundaryMin`: "5.10.0" in ["5.10.0", "6.99.0"] → true (inclusive)
- `InRangeBoundaryMax`: "6.99.0" in ["5.10.0", "6.99.0"] → true (inclusive)
- `ToStrRoundtrip`: Parse → to_str → parse → same values

**test_crypto.cpp** — `TEST(Crypto, ...)`:
- `Crc32KnownValue`: CRC32("123456789") == 0xCBF43926
- `Crc32Empty`: CRC32("") == 0x00000000
- `Crc32Incremental`: Init → update chunks → finalize == single-shot result
- `Sha256KnownValue`: SHA-256("abc") matches known hex digest
- `Sha256Empty`: SHA-256("") matches known empty-string digest
- `Sha256Incremental`: Init → update chunks → finalize == single-shot
- `Sha256FileMatchesBuffer`: sha256_file(path) == sha256(file_contents)
- `Sha256ToHex`: Digest bytes → hex string matches expected
- `Sha256HexMatches`: hex_matches(correct_hex, digest) → true
- `Sha256HexMismatch`: hex_matches(wrong_hex, digest) → false
- `EcdsaSignVerifyRoundtrip`: Sign with test private key → verify with test public key → success
- `EcdsaVerifyBadSignature`: Tampered signature → `OTA_ERR_VERIFY_SIGNATURE_FAILED`
- `EcdsaVerifyWrongKey`: Sign with key A, verify with key B → failure
- `EcdsaVerifyNonexistentKey`: Bad key path → `OTA_ERR_VERIFY_KEY_LOAD_FAILED`

**test_persistence.cpp** (if written as separate tests):
- `AtomicWriteReadRoundtrip`: Write data → read back → identical
- `AtomicWriteCreatesParentDir`: Write to nonexistent dir → succeeds (mkdir -p)
- `ReadNonexistentFile`: → error
- `FileExistsTrue`: After write → true
- `FileExistsFalse`: Before write → false
- `MkdirPCreatesNested`: Creates multiple levels

### Infrastructure needed
- Test keys: `tests/fixtures/keys/test_private.pem` + `test_public.pem` — ECDSA P-256
  - Generated by: `openssl ecparam -name prime256v1 -genkey -noout -out test_private.pem && openssl ec -in test_private.pem -pubout -o test_public.pem`
  - These MUST be generated as part of the foundation build task (CMake custom command or script)
- Temp directory: `/tmp/ota-test/` — created by test fixtures, cleaned in teardown
- NO external services needed

---

## Module: ota-master

### Unit tests (developer writes, GoogleMock for external dependencies)

Mocking allowed for: `CloudClient`, `TransportServer`, `Database` (to isolate business logic).
Mocking NOT allowed for: SQLite (use real in-memory DB), filesystem (use real temp files).

**test_config.cpp** — `TEST(Config, ...)`:
- `LoadValidConfig`: Load `tests/fixtures/configs/test_master.json` → all fields populated
- `LoadMissingFile`: Nonexistent path → error
- `LoadMissingRequiredField`: JSON without `master.listen_unix` → error
- `LoadInvalidJson`: Malformed JSON → error
- `DefaultValues`: Missing optional fields use defaults (e.g. `bandwidth_limit_kbps=0`)

**test_agent_registry.cpp** — `TEST(AgentRegistry, ...)`:
- `RegisterNewAgent`: Registration → assigns UUID, status "registered", stored in DB
- `RegisterSameNodeIdempotent`: Same node_identifier twice → same agent_id returned
- `UpdateHeartbeat`: Heartbeat → `last_heartbeat` updated
- `MarkUnreachableAfterTimeout`: No heartbeat for `timeout_seconds` → status "unreachable"
- `ListAgents`: Register 3 agents → list returns all 3
- `GetAgentById`: Registered agent → returned. Non-existent → error.
- `GetAgentsByNodeIdentifier`: Filter by node_identifier → correct subset

**test_download_state.cpp** — `TEST(DownloadState, ...)`:
- `CreateNewDownload`: → state "pending", downloaded_bytes=0
- `UpdateProgress`: Set downloaded_bytes=1000 → persisted correctly
- `PersistAndReload`: Write state JSON → read back → identical
- `MarkCompleted`: → state "completed"
- `MarkSuperseded`: → state "superseded"
- `MarkFailed`: → state "failed" with error message

**test_verification_pipeline.cpp** — `TEST(VerificationPipeline, ...)`:
- `ValidPackagePassesAll`: Good package → `OTA_OK`
- `SizeMismatchFailsFast`: Wrong expected size → `OTA_ERR_VERIFY_SIZE_MISMATCH` (no CRC/SHA/sig attempted)
- `Crc32MismatchFails`: Correct size, wrong CRC → `OTA_ERR_VERIFY_CRC32_MISMATCH`
- `Sha256MismatchFails`: Correct size+CRC, wrong SHA → `OTA_ERR_VERIFY_SHA256_MISMATCH`
- `SignatureFailureFails`: Correct size+CRC+SHA, bad signature → `OTA_ERR_VERIFY_SIGNATURE_FAILED`
- `PipelineStopsOnFirstFailure`: Verify each stage short-circuits remaining checks

**test_compatibility_engine.cpp** — `TEST(CompatibilityEngine, ...)`:
- `CompatibleAgentEligible`: Matching node, OS range, region → eligible
- `NodeMismatchRejected`: Different node_identifier → `OTA_ERR_COMPAT_NODE_MISMATCH`
- `OsVersionBelowMinRejected`: OS < min_os_version → `OTA_ERR_COMPAT_OS_VERSION`
- `OsVersionAboveMaxRejected`: OS > max_os_version → `OTA_ERR_COMPAT_OS_VERSION`
- `RegionMismatchRejected`: Different region → `OTA_ERR_COMPAT_REGION`
- `MultipleAgentsDifferentEligibility`: 3 agents, 1 eligible → filter returns only eligible

**test_health_aggregator.cpp** — `TEST(HealthAggregator, ...)`:
- `HealthyCyclesAccumulate`: N healthy reports → healthy_cycles == N
- `ErrorExceedsThreshold`: Error rate > `error_threshold_percent` → returns "rollback_needed"
- `HealthyCyclesResetOnError`: Healthy → error → healthy_cycles reset to 0
- `CompletedAfterRequiredCycles`: `health_monitoring_cycles` healthy reports → "completed"
- `MultiplePackagesTrackedSeparately`: Different packages have independent health tracking

**test_campaign_manager.cpp** — `TEST(CampaignManager, ...)`:
- `CreateCampaign`: → campaign in DB with state "idle"
- `ValidStateTransition`: idle→downloading → succeeds
- `InvalidStateTransition`: completed→downloading → error
- `AllTargetsCompletedTransitionsCampaign`: All targets "completed" → campaign "completed"
- `AnyTargetFailedTransitionsCampaign`: One target "failed" → appropriate campaign state
- `AuditLogCreated`: State change → audit_log entry exists

**test_database.cpp** (if separated):
- `SchemaInitialized`: Open fresh DB → all tables exist
- `AgentCRUD`: Insert, read, update, delete
- `CampaignCRUD`: Insert, read, update
- `TransactionRollback`: Begin → error → rollback → data unchanged

### Integration tests (developer writes, NO mocks, real IPC + DB)

**test_master_agent_registration.cpp** — `TEST(MasterAgentInteg, ...)`:
- `AgentRegistersViaTcp`: Start master, start agent (TCP) → agent gets UUID → master has agent in registry
- `AgentRegistersViaUnixSocket`: Same but Unix socket transport
- `HeartbeatKeepsAlive`: Agent sends heartbeats → master tracks last_heartbeat
- `HeartbeatTimeoutMarksUnreachable`: Stop agent → wait timeout → master marks unreachable
- `MultipleAgentsRegister`: Start master + 3 agents → all register successfully

### Infrastructure needed
- Real SQLite database (temp file, NOT in-memory, for testing WAL/persistence)
- Temp directory: `/tmp/ota-test/master/`
- Test fixture configs
- NO mocks for integration tests

---

## Module: ota-agent (Linux)

### Unit tests (developer writes, mocks OK within tests/unit/agent/)

Mocking allowed for: `TransportClient`, `BootloaderInterface` (via function pointer substitution).
Mocking NOT allowed for: filesystem operations (use real temp directories).

**test_sw_installer.cpp** — `TEST(SwInstaller, ...)`:
- `InstallValidPackage`: Extract .ota → creates `/tmp/ota-test/opt/{pkg_id}/{version}/exe`
- `InstallCreatesNewerSymlink`: After install → `newer` symlink points to version dir
- `InstallWithLibs`: Package with libs/ → extracted to version dir
- `InstallWithResources`: Package with res/ → extracted to version dir
- `InstallCreatesParentDirs`: Fresh install → all directories created
- `InstallSameVersionIdempotent`: Install twice → succeeds, no errors
- `InstallInvalidPackage`: Corrupted tar.gz → `OTA_ERR_INSTALL_EXTRACT`

**test_sw_activator.cpp** — `TEST(SwActivator, ...)`:
- `ActivateSwapsSymlinks`: `newer`→`current`, old `current`→`prev`
- `ActivateRemovesNewer`: After activation → `newer` symlink gone
- `ActivateNoNewerFails`: No `newer` symlink → error
- `ActivatePreservesExePermissions`: Executable permissions intact after activation

**test_fw_installer.cpp** — `TEST(FwInstaller, ...)`:
- `StageFirmwareCallsPlugin`: Calls `detect_inactive_slot` + `stage_firmware` on mock bootloader
- `StageFirmwareSlotError`: Plugin returns error → `OTA_ERR_INSTALL_SLOT`
- `StageFirmwareDataWritten`: Data passed through to plugin correctly

**test_rollback_executor.cpp** — `TEST(RollbackExecutor, ...)`:
- `SwRollbackRevertsSymlinks`: `current`→`prev` version
- `SwRollbackNoPrevFails`: No `prev` symlink → error
- `FwRollbackMarksSlotInvalid`: Calls `mark_slot_invalid` on active slot

**test_health_monitor.cpp** — `TEST(HealthMonitor, ...)`:
- `RecordHealthyCycle`: healthy_cycles increments
- `RecordError`: error_count increments
- `RecordCrash`: crash_count increments
- `GenerateHealthReport`: Produces correct JSON with all fields
- `ResetCounters`: After rollback → counters reset

### Integration tests (developer writes, NO mocks)

**test_sw_install_activate.cpp** — `TEST(AgentSwInteg, ...)`:
- `FullSwInstallActivateFlow`: Receive package → install → activate → symlinks correct → STATUS_REPORTs generated
- `SwRollbackAfterActivate`: Activate → rollback → reverts to previous

**test_fw_install_activate.cpp** — `TEST(AgentFwInteg, ...)`:
- `FullFwInstallActivateFlow`: Receive FW → stage via sim plugin → activate → boot status correct

### Infrastructure needed
- Temp directory: `/tmp/ota-test/opt/` for software installation
- Sim bootloader plugin (`sim_plugin.cpp`) — simulates A/B slots in temp files
- Pre-built test packages in `tests/fixtures/packages/`

---

## Module: ota-agent-freertos (x86 simulation)

### Unit tests (C, Unity + CMock)

**test_agent_task.c**:
- `test_initial_state_is_idle`: Agent task starts in IDLE state
- `test_transition_to_registering`: Start → state becomes REGISTERING
- `test_heartbeat_interval`: Heartbeat sent at configured interval
- `test_command_dispatch_install`: Received INSTALL_CMD → routes to fw_installer
- `test_command_dispatch_activate`: Received ACTIVATE_CMD → routes to fw_activator
- `test_command_dispatch_rollback`: Received ROLLBACK_CMD → routes to rollback handler

**test_fw_installer.c**:
- `test_stage_calls_hal_flash_write`: Stage firmware → calls `ota_hal_flash_write` with correct addr/len
- `test_stage_writes_to_inactive_slot`: Writes to slot B when slot A is active
- `test_stage_reports_milestone`: After staging → STATUS_REPORT with `install_completed`

**test_hal_sim.c**:
- `test_flash_write_read_roundtrip`: Write data → read back → matches
- `test_flash_erase_zeros_region`: Erase → read → all 0xFF
- `test_flash_write_boundary`: Write at end of region → succeeds
- `test_flash_write_past_end`: Write beyond region → error

### Infrastructure needed
- NO FreeRTOS headers needed — simulation builds as plain C with POSIX threads
- HAL simulation provides file-backed "flash" in `/tmp/ota-test/freertos/`

---

## Module: Bootloader Plugins

### Unit tests (developer writes)

**test_bootloader_interface.cpp** — `TEST(BootloaderPlugin, ...)`:
- `SimDetectInactiveSlotB`: Slot A active → detect returns slot B
- `SimDetectInactiveSlotA`: Slot B active → detect returns slot A
- `SimStageFirmware`: Write data → prepare_activation → pending_activation true
- `SimMarkActiveValid`: Clears pending_activation
- `SimMarkSlotInvalid`: Slot marked invalid → is_valid false
- `SimQueryBootStatus`: Returns both slots with correct states
- `SimBootCountTracking`: Boot count increments correctly

**test_plugin_factory.cpp** — `TEST(PluginFactory, ...)`:
- `CreateSimPlugin`: `ota_bootloader_create("sim", ...)` → non-null, name == "sim"
- `CreateLinuxGenericPlugin`: `ota_bootloader_create("linux-generic", ...)` → non-null
- `CreateUnknownPlugin`: `ota_bootloader_create("unknown", ...)` → null
- `DestroyNullIsSafe`: `ota_bootloader_destroy(NULL)` → no crash
- `DestroyFreesResources`: Create → destroy → no leaks (if valgrind available)

### Infrastructure needed
- Sim plugin uses temp files in `/tmp/ota-test/bootloader/`
- linux-generic plugin uses temp files simulating partition metadata

---

## Module: ota-pack (CLI tool)

### Unit tests (developer writes)

**test_packager.cpp** — `TEST(Packager, ...)`:
- `CreateSwPackage`: Source dir + manifest → valid .ota file
- `CreateFwPackage`: Firmware source + manifest → valid .ota file
- `CreateWithLibs`: Source with libs/ → libs included in archive
- `CreateWithResources`: Source with res/ → res included in archive
- `CreateMissingExe`: No exe file in dir → error
- `CreateMissingManifest`: No manifest → error
- `InspectSwPackage`: Dump → shows package_identifier, version, artifact_type
- `InspectFwPackage`: Dump → shows firmware fields

**test_signer.cpp** — `TEST(Signer, ...)`:
- `SignProducesSignatureFile`: Sign → signature.bin exists in archive
- `VerifySignedPackageSucceeds`: Sign → verify → exit 0
- `VerifyTamperedPackageFails`: Sign → modify archive → verify → exit non-zero
- `VerifyUnsignedPackageFails`: Package without signature.bin → error
- `SignWithInvalidKeyFails`: Bad key file → error

### Functional tests (shell commands)
```bash
# Build test package from scratch
mkdir -p /tmp/ota-test-build/exe && echo '#!/bin/sh' > /tmp/ota-test-build/exe
./build/src/ota-pack/ota-pack create --manifest tests/fixtures/manifests/valid_sw_manifest.json \
    --dir /tmp/ota-test-build/ --output /tmp/ota-test-build/test.ota
./build/src/ota-pack/ota-pack sign --key tests/fixtures/keys/test_private.pem \
    --package /tmp/ota-test-build/test.ota
./build/src/ota-pack/ota-pack verify --pubkey tests/fixtures/keys/test_public.pem \
    --package /tmp/ota-test-build/test.ota
./build/src/ota-pack/ota-pack inspect --package /tmp/ota-test-build/test.ota
rm -rf /tmp/ota-test-build
```

### Infrastructure needed
- Test keys (same as libota-core)
- Temp directory for package creation

---

## Module: mock-cloud (Python/FastAPI)

### Unit tests (pytest)

**test_routes_updates.py**:
- `test_list_updates_empty`: No packages → empty list
- `test_list_updates_with_package`: Add package → appears in list
- `test_list_updates_filter_by_node`: Filter by node_identifier → correct subset
- `test_get_manifest_exists`: → 200 + JSON manifest
- `test_get_manifest_not_found`: Unknown package → 404
- `test_download_package_full`: → 200 + binary data
- `test_download_package_range`: Range header → 206 + partial content
- `test_download_package_not_found`: Unknown package → 404
- `test_get_metadata`: → 200 + size, crc32, sha256, signature

**test_routes_admin.py**:
- `test_add_package`: Upload .ota + manifest → 201
- `test_add_package_invalid`: Missing manifest → 400
- `test_inject_network_error`: Set error → next download fails with connection reset
- `test_inject_corrupted_download`: Set error → download returns corrupted data
- `test_inject_truncated_download`: Set error → download cuts off after N bytes
- `test_inject_slow_response`: Set error → response delayed
- `test_inject_version_supersede`: Set error → new version appears mid-download
- `test_reset_clears_errors`: Inject error → reset → error cleared
- `test_reset_clears_packages`: Add package → reset → package gone

### Verification commands
```bash
cd tools/mock-cloud && python3 -m pytest -v
```

### Infrastructure needed
- NO external services
- httpx TestClient (included with FastAPI)

---

## Integration Test Plan

### test_master_agent_registration.cpp
**Tier**: 2 (needs foundation + master + agent transport)
**Processes**: Master (background) + Agent (foreground)
**Protocol**: Real Unix socket / TCP messages
**Assertions**:
- Agent sends REGISTER_REQ → Master sends REGISTER_ACK with UUID
- Master DB has agent record with correct node_identifier
- Heartbeats maintain connection
- Timeout → agent marked unreachable

### test_package_transfer.cpp
**Tier**: 2 (needs foundation + master transport + agent transport)
**Processes**: Master + Agent
**Protocol**: Real wire protocol
**Assertions**:
- TRANSFER_START with package metadata → Agent ACKs
- TRANSFER_DATA chunks → Agent ACKs each with received_bytes
- TRANSFER_COMPLETE → Agent has full file
- SHA-256 of received file matches expected

### test_sw_install_activate.cpp
**Tier**: 2 (needs foundation + master + agent + sw_installer + sw_activator)
**Processes**: Master + Agent
**Assertions**:
- INSTALL_CMD → Agent creates version dir + newer symlink
- ACTIVATE_CMD → Agent swaps symlinks (newer→current→prev)
- STATUS_REPORT milestones: install_started, install_completed, activate_started, activate_completed
- File system state correct after each step

### test_fw_install_activate.cpp
**Tier**: 2 (needs foundation + master + agent + bootloader)
**Processes**: Master + Agent
**Assertions**:
- INSTALL_CMD (firmware) → Agent stages via sim bootloader plugin
- ACTIVATE_CMD → Agent calls prepare_activation on plugin
- Boot status shows pending_activation=true
- STATUS_REPORT milestones received

### test_rollback_flow.cpp
**Tier**: 2 (needs all agent install/activate + rollback)
**Processes**: Master + Agent
**Assertions**:
- ROLLBACK_CMD → Agent reverts SW symlinks or FW slot
- STATUS_REPORT: rollback_started, rollback_completed
- File system state matches pre-activation state

### test_download_resume.cpp
**Tier**: 2 (needs master + mock cloud)
**Processes**: Master + Mock Cloud
**Assertions**:
- Start download → interrupt (kill download) → restart
- Downloaded bytes resume from last position (Range header)
- Complete download → verification passes
- Download state JSON persisted across interruption

### test_multi_agent_campaign.cpp
**Tier**: 3 (needs ALL modules built)
**Processes**: Master + 2 Linux Agents + 1 FreeRTOS Agent
**Assertions**:
- 3 agents register
- Campaign created targeting all 3
- All agents receive packages
- All agents install and activate
- Campaign state progresses to COMPLETED
- Per-target states all COMPLETED

---

## E2E Test Plan

### test_full_sw_campaign.cpp
**Tier**: 3 (needs ALL modules + mock cloud)
**Processes**: Mock Cloud (background) + Master + Agent
**Flow**: Cloud has update → Master polls → download → verify → transfer → install → activate → monitor → complete
**Assertions**:
- Campaign starts in IDLE
- Download completes (checked via download state)
- Verification passes all 4 stages
- Agent installs and activates successfully
- Health monitoring passes for N cycles
- Campaign ends in COMPLETED
- Audit log contains full lifecycle events

### test_full_fw_campaign.cpp
**Tier**: 3
Same as SW campaign but with firmware artifact, bootloader plugin interactions.

### test_error_recovery.cpp
**Tier**: 3 (needs mock cloud error injection)
**Scenarios**:
- Corrupted package → verification rejects → campaign ABORTED
- Network failure during download → resume → complete
- Agent install failure → agent auto-rollback → target ROLLED_BACK
- Activate failure → Master-initiated rollback

### test_version_supersede.cpp
**Tier**: 3 (needs mock cloud + download manager)
**Flow**: Start downloading v2.2.0 → cloud adds v2.3.0 → Master detects → marks v2.2.0 SUPERSEDED → downloads v2.3.0
**Assertions**:
- Old download state = "superseded"
- New download starts and completes
- Campaign targets the new version

---

## Test Dependencies Map

### Tier 1: Can test immediately after build (no extra infrastructure)
- Unit tests for `libota-core` (test_error, test_manifest, test_protocol, test_state, test_version, test_persistence)
- Unit tests for `ota-pack` (test_packager, test_signer)
- Unit tests for `ota-agent-freertos` (test_agent_task, test_hal_sim)
- Unit tests for bootloader plugins (test_bootloader_interface, test_plugin_factory)
- Mock cloud pytest tests

**Requires**: Test keys generated, test fixture packages built, temp directories

### Tier 2: After foundation + module build complete
- `test_crypto.cpp` — requires test ECDSA keys to exist
- Unit tests for `ota-master` (all test_*.cpp in tests/unit/master/)
- Unit tests for `ota-agent` (all test_*.cpp in tests/unit/agent/)
- Integration tests: registration, transfer, install/activate, rollback, download resume

**Requires**: Working libota-core, real IPC sockets, real SQLite, sim bootloader

### Tier 3: After ALL modules built and unit/integration tests pass
- E2E tests: full SW campaign, full FW campaign, error recovery, version supersede
- Multi-agent campaign integration test

**Requires**: Mock cloud server running, all Master + Agent binaries, test packages

### Tier 4: Infrastructure tasks (must be in DAG)
- **infra-test-keys**: Generate ECDSA P-256 test keypair → `tests/fixtures/keys/`
  - Command: `openssl ecparam -name prime256v1 -genkey -noout -out tests/fixtures/keys/test_private.pem && openssl ec -in tests/fixtures/keys/test_private.pem -pubout -out tests/fixtures/keys/test_public.pem`
  - Blocker for: ALL crypto tests, ALL verification tests, ALL signing tests
- **infra-test-packages**: Build test .ota packages → `tests/fixtures/packages/`
  - Blocker for: transfer tests, install tests, e2e tests
  - Blocked by: ota-pack build + infra-test-keys
- **infra-test-configs**: Write test config JSON files → `tests/fixtures/configs/`
  - Blocker for: master config tests, integration tests
- **infra-apt-packages**: Install `libcurl4-openssl-dev` and `libarchive-dev`
  - Blocker for: Master build (cloud_client), ota-pack build

### Tier 5: Blocked until human provides
- None. All tests can run on x86 Linux development environment without external dependencies.

---

## Test File Naming Convention

| Test Type | Directory | File Pattern | CTest Label |
|-----------|-----------|-------------|-------------|
| Unit (C++ / GTest) | `tests/unit/{module}/` | `test_*.cpp` | `unit_{module}_{name}` |
| Unit (C / Unity) | `tests/unit/{module}/` | `test_*.c` | `unit_{module}_{name}` |
| Integration (GTest) | `tests/integration/` | `test_*.cpp` | `integ_{name}` |
| E2E (GTest) | `tests/e2e/` | `test_*.cpp` | `e2e_{name}` |
| Mock cloud (pytest) | `tools/mock-cloud/` | `test_*.py` | `mock_cloud_{name}` |

CTest regex patterns:
- `^unit_` — all unit tests
- `^unit_core_` — libota-core unit tests
- `^unit_master_` — master unit tests
- `^unit_agent_` — agent unit tests (Linux)
- `^unit_agent_freertos_` — FreeRTOS agent unit tests
- `^unit_bootloader_` — bootloader plugin unit tests
- `^integ_` — all integration tests
- `^e2e_` — all E2E tests
