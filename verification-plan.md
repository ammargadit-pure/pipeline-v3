# Verification Plan

## Global Checks (run for EVERY task)

```bash
# Build everything (must succeed with zero warnings under -Werror)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DOTA_BUILD_TESTS=ON -DOTA_BUILD_MOCK_CLOUD=ON
cmake --build build -j$(nproc) 2>&1 | tee build.log
grep -c "warning:" build.log && echo "FAIL: compiler warnings found" && exit 1

# Run all tests
cd build && ctest --output-on-failure && cd ..

# Static analysis (when clang-tidy target exists)
# cmake --build build --target clang-tidy

# Mock contamination check — no mocks outside unit tests
grep -rn "MOCK_METHOD\|NiceMock\|StrictMock\|::testing::Mock" tests/integration/ tests/e2e/ 2>/dev/null && echo "FAIL: mocks in integration/e2e tests" && exit 1

# Skip test check — no skipped tests
grep -rn "GTEST_SKIP\|DISABLED_" tests/ 2>/dev/null && echo "FAIL: skipped/disabled tests found" && exit 1

# Unity skip check (C tests)
grep -rn "TEST_IGNORE\|UNITY_SKIP" tests/ 2>/dev/null && echo "FAIL: skipped Unity tests found" && exit 1
```

---

## Per-Module Verification

### Module: libota-core (Foundation)

**Build verification:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DOTA_BUILD_TESTS=ON
cmake --build build --target ota-core -j$(nproc)

# Library file must exist
test -f build/src/libota-core/libota-core.a || (echo "FAIL: libota-core.a not built" && exit 1)

# Headers must be accessible
echo '#include "ota/core/error.h"
#include "ota/core/manifest.h"
#include "ota/core/protocol.h"
#include "ota/core/state.h"
#include "ota/core/version.h"
#include "ota/core/crypto.h"
#include "ota/core/persistence.h"
#include "ota/core/types.h"
#include "ota/bootloader/interface.h"
int main() { return 0; }' > /tmp/test_headers.c
gcc -c /tmp/test_headers.c -I src/libota-core/include -o /dev/null
```

**Unit tests:**
```bash
cd build && ctest -R "^unit_core_" --output-on-failure
```

**Specific assertions (within test code):**
- `test_error.cpp`: `ota_error_str(OTA_OK)` returns `"OTA_OK"` (non-null, non-empty)
- `test_error.cpp`: Range checks — `ota_error_is_protocol(100)` is true, `ota_error_is_protocol(99)` is false
- `test_manifest.cpp`: Parse valid JSON → all fields populated, `ota_manifest_validate()` returns `OTA_OK`
- `test_manifest.cpp`: Parse empty JSON → returns `OTA_ERR_INVALID_ARGUMENT`
- `test_manifest.cpp`: Missing required field → returns error
- `test_manifest.cpp`: Roundtrip: parse → serialize → parse → compare
- `test_protocol.cpp`: Serialize header → deserialize → fields match
- `test_protocol.cpp`: Bad magic → `OTA_ERR_PROTOCOL_MAGIC`
- `test_protocol.cpp`: Oversized payload → `OTA_ERR_PROTOCOL_PAYLOAD_SIZE`
- `test_protocol.cpp`: `ota_msg_is_binary(OTA_MSG_TRANSFER_DATA)` is true, all others false
- `test_state.cpp`: All `*_str()` functions return non-null for all enum values
- `test_state.cpp`: All `*_from_str()` functions roundtrip
- `test_state.cpp`: `ota_campaign_can_transition(IDLE, DOWNLOADING)` is true
- `test_state.cpp`: `ota_campaign_can_transition(COMPLETED, DOWNLOADING)` is false
- `test_version.cpp`: `ota_version_compare("2.2.0", "2.1.0")` > 0
- `test_version.cpp`: `ota_version_compare("1.0.0", "1.0.0")` == 0
- `test_version.cpp`: `ota_version_in_range("5.15.0", "5.10.0", "6.99.0")` is true
- `test_version.cpp`: `ota_version_in_range("4.0.0", "5.10.0", "6.99.0")` is false
- `test_crypto.cpp`: CRC32 of known data matches expected value
- `test_crypto.cpp`: SHA-256 of known data matches expected hex
- `test_crypto.cpp`: ECDSA sign → verify roundtrip succeeds
- `test_crypto.cpp`: ECDSA verify with wrong key → `OTA_ERR_VERIFY_SIGNATURE_FAILED`

### Module: ota-master

**Build verification:**
```bash
cmake --build build --target ota-master -j$(nproc)
test -f build/src/ota-master/ota-master || (echo "FAIL: ota-master binary not built" && exit 1)
```

**Unit tests:**
```bash
cd build && ctest -R "^unit_master_" --output-on-failure
```

**Specific assertions:**

*Config (`test_config.cpp`):*
- Load valid test config → all fields populated correctly
- Load config with missing required field → error
- Load nonexistent file → `OTA_ERR_NOT_FOUND`

*Agent Registry (`test_agent_registry.cpp`):*
- Register agent → assigned UUID, status "registered"
- Register same node_identifier twice → returns same agent_id (idempotent)
- Heartbeat update → `last_heartbeat` increases
- Mark unreachable after timeout → status changes to "unreachable"
- List agents → returns all registered agents

*Download State (`test_download_state.cpp`):*
- Create download → state is "pending"
- Update progress → `downloaded_bytes` increases
- Persist state → read back → identical
- Mark superseded → state is "superseded"

*Verification Pipeline (`test_verification_pipeline.cpp`):*
- Valid package → all checks pass → `OTA_OK`
- Size mismatch → `OTA_ERR_VERIFY_SIZE_MISMATCH` (fails fast, skips hash/sig)
- CRC32 mismatch → `OTA_ERR_VERIFY_CRC32_MISMATCH`
- SHA-256 mismatch → `OTA_ERR_VERIFY_SHA256_MISMATCH`
- Bad ECDSA signature → `OTA_ERR_VERIFY_SIGNATURE_FAILED`

*Compatibility Engine (`test_compatibility_engine.cpp`):*
- Compatible agent + manifest → eligible
- Node identifier mismatch → `OTA_ERR_COMPAT_NODE_MISMATCH`
- OS version out of range → `OTA_ERR_COMPAT_OS_VERSION`
- Region mismatch → `OTA_ERR_COMPAT_REGION`

*Health Aggregator (`test_health_aggregator.cpp`):*
- Healthy reports for N cycles → returns "healthy"
- Error count exceeds threshold → returns "rollback_needed"
- Mixed healthy/unhealthy → resets healthy cycle counter

*Campaign Manager (`test_campaign_manager.cpp`):*
- Create campaign → state is "idle"
- State transitions follow valid DAG → succeed
- Invalid state transition → error
- Campaign with all targets completed → campaign state "completed"

### Module: ota-agent (Linux)

**Build verification:**
```bash
cmake --build build --target ota-agent -j$(nproc)
test -f build/src/ota-agent/ota-agent || (echo "FAIL: ota-agent binary not built" && exit 1)
```

**Unit tests:**
```bash
cd build && ctest -R "^unit_agent_" --output-on-failure
```

**Specific assertions:**

*SW Installer (`test_sw_installer.cpp`):*
- Install valid .ota → creates `/opt/{pkg_id}/{version}/exe`
- Install → `newer` symlink points to version directory
- Install with libs → `/opt/{pkg_id}/{version}/libs/` populated
- Install to missing directory → creates it
- Double install same version → succeeds (idempotent)

*SW Activator (`test_sw_activator.cpp`):*
- Activate → `current` symlink points to version dir, `prev` points to old current
- Activate with no `newer` → error
- `newer` symlink removed after activation

*FW Installer (`test_fw_installer.cpp`):*
- Stage firmware → calls `detect_inactive_slot()` + `stage_firmware()` on plugin
- Stage to full slot → `OTA_ERR_INSTALL_SLOT`

*Rollback Executor (`test_rollback_executor.cpp`):*
- SW rollback → `current` symlink reverts to `prev`
- FW rollback → calls `mark_slot_invalid()` on active slot

*Health Monitor (`test_health_monitor.cpp`):*
- Record healthy cycle → healthy_cycles increments
- Record error → error_count increments
- Generate health report → correct JSON fields

### Module: ota-agent-freertos

**Build verification:**
```bash
cmake --build build --target ota-agent-freertos -j$(nproc)
test -f build/src/ota-agent-freertos/ota-agent-freertos || (echo "FAIL: ota-agent-freertos binary not built" && exit 1)
```

**Unit tests (Unity):**
```bash
cd build && ctest -R "^unit_agent_freertos_" --output-on-failure
```

**Specific assertions:**
- `test_agent_task.c`: Initial state is "idle", transitions to "registering" on start
- `test_fw_installer.c`: Stage firmware → calls HAL flash_write with correct addr/len
- `test_hal_sim.c`: Flash write → flash read → data matches

### Module: Bootloader Plugins

**Unit tests:**
```bash
cd build && ctest -R "^unit_bootloader_" --output-on-failure
```

**Specific assertions:**
- `test_bootloader_interface.cpp`: Sim plugin — `detect_inactive_slot()` returns slot B when A is active
- `test_bootloader_interface.cpp`: Sim plugin — `stage_firmware()` + `prepare_activation()` → `pending_activation` is true
- `test_bootloader_interface.cpp`: Sim plugin — `mark_active_valid()` → clears pending
- `test_plugin_factory.cpp`: `ota_bootloader_create("sim", ...)` returns non-null
- `test_plugin_factory.cpp`: `ota_bootloader_create("unknown", ...)` returns null

### Module: ota-pack (CLI)

**Build verification:**
```bash
cmake --build build --target ota-pack -j$(nproc)
test -f build/src/ota-pack/ota-pack || (echo "FAIL: ota-pack binary not built" && exit 1)
```

**Functional verification:**
```bash
# Create a test package
mkdir -p /tmp/ota-test-app && echo '#!/bin/sh' > /tmp/ota-test-app/exe && chmod +x /tmp/ota-test-app/exe
./build/src/ota-pack/ota-pack create \
  --manifest tests/fixtures/manifests/valid_sw_manifest.json \
  --dir /tmp/ota-test-app/ \
  --output /tmp/test-pack.ota

test -f /tmp/test-pack.ota || (echo "FAIL: package not created" && exit 1)

# Sign the package
./build/src/ota-pack/ota-pack sign \
  --key tests/fixtures/keys/test_private.pem \
  --package /tmp/test-pack.ota

# Verify the package
./build/src/ota-pack/ota-pack verify \
  --pubkey tests/fixtures/keys/test_public.pem \
  --package /tmp/test-pack.ota
echo "ota-pack verify exit code: $?"
test $? -eq 0 || (echo "FAIL: package verification failed" && exit 1)

# Inspect the package
./build/src/ota-pack/ota-pack inspect --package /tmp/test-pack.ota | grep -q "nav-service"
test $? -eq 0 || (echo "FAIL: inspect does not show package_id" && exit 1)

# Cleanup
rm -rf /tmp/ota-test-app /tmp/test-pack.ota
```

### Module: mock-cloud

**Startup verification:**
```bash
cd tools/mock-cloud
pip install -r requirements.txt -q
uvicorn server:app --port 18080 &
CLOUD_PID=$!
sleep 2

# Health check (server responds)
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:18080/api/v1/updates?node_identifier=test)
test "$HTTP_CODE" = "200" || (echo "FAIL: mock cloud not responding" && kill $CLOUD_PID && exit 1)

# Upload a test package
curl -s -X POST http://127.0.0.1:18080/api/v1/admin/add-package \
  -F "manifest=@tests/fixtures/manifests/valid_sw_manifest.json" \
  -F "package=@tests/fixtures/packages/valid_sw_package.ota"
test $? -eq 0 || (echo "FAIL: package upload failed" && kill $CLOUD_PID && exit 1)

# List updates
curl -s http://127.0.0.1:18080/api/v1/updates?node_identifier=hpc-primary | grep -q "nav-service"
test $? -eq 0 || (echo "FAIL: uploaded package not listed" && kill $CLOUD_PID && exit 1)

# Get manifest
curl -s http://127.0.0.1:18080/api/v1/updates/nav-service/2.2.0/manifest | grep -q "package_identifier"
test $? -eq 0 || (echo "FAIL: manifest not returned" && kill $CLOUD_PID && exit 1)

# Download package (range request support)
curl -s -H "Range: bytes=0-99" -o /dev/null -w "%{http_code}" \
  http://127.0.0.1:18080/api/v1/updates/nav-service/2.2.0/package | grep -q "206"
test $? -eq 0 || (echo "FAIL: range request not supported" && kill $CLOUD_PID && exit 1)

# Error injection
curl -s -X POST http://127.0.0.1:18080/api/v1/admin/inject-error \
  -H "Content-Type: application/json" \
  -d '{"type":"network_error","target":"download","config":{"error":"connection_reset"}}' \
  -o /dev/null -w "%{http_code}" | grep -q "200"

# Reset
curl -s -X DELETE http://127.0.0.1:18080/api/v1/admin/reset -o /dev/null -w "%{http_code}" | grep -q "204"

kill $CLOUD_PID
echo "mock-cloud verification: PASS"
```

---

## Integration Test Verification

### Master ↔ Agent Registration Flow
```bash
cd build && ctest -R "^integ_.*registration" --output-on-failure
```
**Expected**: Agent connects → sends REGISTER_REQ → Master responds with REGISTER_ACK → Agent appears in registry → Heartbeats work.

### Package Transfer Flow
```bash
cd build && ctest -R "^integ_.*transfer" --output-on-failure
```
**Expected**: Master sends TRANSFER_START → chunks via TRANSFER_DATA → Agent ACKs → TRANSFER_COMPLETE → Agent has full file with matching SHA-256.

### Software Install + Activate Flow
```bash
cd build && ctest -R "^integ_.*sw_install" --output-on-failure
```
**Expected**: Master sends INSTALL_CMD → Agent extracts to `/opt/{pkg}/{ver}/` → `newer` symlink → Master sends ACTIVATE_CMD → `current` updated → `prev` points to old version → STATUS_REPORT milestones received.

### Firmware Install + Activate Flow
```bash
cd build && ctest -R "^integ_.*fw_install" --output-on-failure
```
**Expected**: Master sends INSTALL_CMD (firmware) → Agent stages via bootloader plugin → Master sends ACTIVATE_CMD → Agent prepares activation → simulated reboot → STATUS_REPORT milestones received.

### Rollback Flow
```bash
cd build && ctest -R "^integ_.*rollback" --output-on-failure
```
**Expected**: Agent receives ROLLBACK_CMD → SW: reverts symlinks → FW: marks slot invalid → STATUS_REPORT with rollback milestones.

### Download Resume
```bash
cd build && ctest -R "^integ_.*resume" --output-on-failure
```
**Expected**: Start download → simulate interruption → resume from last byte → complete → verification passes.

### Multi-Agent Campaign
```bash
cd build && ctest -R "^integ_.*multi_agent" --output-on-failure
```
**Expected**: Multiple agents register → Campaign targets all eligible → Parallel transfer → All install → All activate → Campaign state reaches "completed".

---

## End-to-End Test Verification

### Full Software Campaign
```bash
cd build && ctest -R "^e2e_.*sw_campaign" --output-on-failure
```
**Expected**: Cloud has update → Master downloads → Verifies → Transfers to agent → Agent installs → Agent activates → Health monitoring passes → Campaign completes.

### Full Firmware Campaign
```bash
cd build && ctest -R "^e2e_.*fw_campaign" --output-on-failure
```
**Expected**: Same flow as SW campaign but with firmware artifact type and bootloader plugin interactions.

### Error Recovery
```bash
cd build && ctest -R "^e2e_.*error_recovery" --output-on-failure
```
**Expected**: Inject errors (corrupted package, network failure) → System detects → Retries or aborts gracefully → Audit log captures events.

### Version Supersede
```bash
cd build && ctest -R "^e2e_.*supersede" --output-on-failure
```
**Expected**: Start downloading v2.2.0 → Cloud adds v2.3.0 → Master detects supersede → Discards v2.2.0 partial → Downloads v2.3.0 → Completes normally.

---

## Coverage Verification

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DOTA_BUILD_TESTS=ON -DOTA_COVERAGE=ON
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure && cd ..

# Generate coverage report
gcovr --root . --filter 'src/' --print-summary --html --html-details -o build/coverage.html

# Check minimum coverage thresholds
COVERAGE=$(gcovr --root . --filter 'src/' --print-summary 2>&1 | grep "lines:" | awk '{print $2}' | tr -d '%')
echo "Line coverage: ${COVERAGE}%"
# Target: >80% line coverage for libota-core, >70% for master/agent
```

---

## Human Checkpoints

- **After Foundation (libota-core) builds and tests pass**: STOP. Ask human to review core API design.
- **After Master + Agent unit tests pass**: STOP. Ask human to review module boundaries.
- **After integration tests pass**: STOP. Ask human to run manual smoke tests.
- **After all builds complete**: STOP. Ask human to verify full system.
- **After judge evaluation**: STOP. Show quality report to human.
