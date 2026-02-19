# Project Context

## What This Is
Automotive Over-The-Air (OTA) Update System — a vehicle-side platform for secure, resumable, multi-node software and firmware updates. Components: OTA Master (C++17 orchestrator), OTA Agent Linux (C++17 daemon), OTA Agent FreeRTOS (C11 embedded task, x86 simulation), libota-core (C11 shared library), Bootloader Plugin Layer (C11), ota-pack CLI tool (C++17), Mock Cloud Server (Python/FastAPI).

## Tech Stack
- C++17 (Master, Linux Agent, ota-pack) — GCC 11+ / Clang 14+
- C11 (libota-core, FreeRTOS Agent, Bootloader plugins) — GCC 11+ / Clang 14+
- CMake 3.20+ (build system)
- OpenSSL 3.x (crypto on Linux — SHA-256, ECDSA P-256, CRC32)
- mbedTLS 3.x (crypto on FreeRTOS)
- libcurl 7.80+ (HTTP client, resumable downloads)
- nlohmann-json 3.11+ (C++ JSON)
- cJSON 1.7+ (C JSON)
- SQLite 3.39+ (Master database)
- spdlog 1.11+ (C++ logging)
- libarchive 3.6+ (tar.gz packaging)
- GoogleTest 1.13+ / GoogleMock (C++ tests)
- Unity + CMock (C tests)
- Python 3.10+ / FastAPI 0.100+ (Mock cloud server)

## How Agents Work

### Context Loading (EVERY agent does this FIRST)
1. Read this file (CLAUDE.md)
2. Read shared-contracts.md — naming, types, response shapes, exact identifiers
3. Read test-strategy.md — testing approach, infrastructure, dependencies
4. Read working-memory.md — learnings from past iterations
5. Read phase-plan.json — find YOUR task
6. Run: git log --oneline -10
7. Run: ls src/ tests/

### CRITICAL RULES (apply to ALL agents)
1. Follow shared-contracts.md EXACTLY. Do NOT invent new names, types, or identifiers.
2. Do NOT mark a task complete unless validation passes with exit code 0.
3. Do NOT use GTEST_SKIP() or DISABLED_ prefixes. Skipped tests are treated as failures.
4. Do NOT use mocks outside tests/unit/. Zero tolerance.
5. Every test must have at least 2 meaningful assertions.
6. Commit messages: feat(module): description OR test(module): description OR fix(module): description
7. All C functions use `ota_` prefix. All C types use `ota_` prefix + `_t` suffix.
8. All C++ classes live in `ota::master`, `ota::agent`, or `ota::pack` namespaces.
9. Public headers are included via `ota/core/...` or `ota/bootloader/...` paths.
10. Wire protocol uses 12-byte binary header (magic 0x4F54) + JSON payload (except TRANSFER_DATA which is binary).

## Commands

```bash
# Configure (debug + tests + mock cloud)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DOTA_BUILD_TESTS=ON -DOTA_BUILD_MOCK_CLOUD=ON

# Build all targets
cmake --build build -j$(nproc)

# Run ALL tests
cd build && ctest --output-on-failure

# Run unit tests only
cd build && ctest -R "^unit_" --output-on-failure

# Run integration tests only
cd build && ctest -R "^integ_" --output-on-failure

# Run e2e tests only
cd build && ctest -R "^e2e_" --output-on-failure

# Coverage build + report
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DOTA_BUILD_TESTS=ON -DOTA_COVERAGE=ON
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
gcovr --root .. --filter 'src/' --html --html-details -o coverage.html

# Static analysis
cmake --build build --target clang-tidy

# Start mock cloud server (for manual testing)
cd tools/mock-cloud && pip install -r requirements.txt && uvicorn server:app --port 8080

# Build + sign a test package
./build/src/ota-pack/ota-pack create --manifest tests/fixtures/manifests/valid_sw_manifest.json --dir tests/fixtures/test-app/ --output /tmp/test.ota
./build/src/ota-pack/ota-pack sign --key tests/fixtures/keys/test_private.pem --package /tmp/test.ota
```

### Command Shortcuts
- `$CMD_TEST` = `cd build && ctest --output-on-failure`
- `$CMD_TEST_UNIT` = `cd build && ctest -R "^unit_" --output-on-failure`
- `$CMD_TEST_INTEG` = `cd build && ctest -R "^integ_" --output-on-failure`
- `$CMD_COVERAGE` = (see coverage build above)
- `$CMD_TYPECHECK` = `cmake --build build --target clang-tidy`
- `$CMD_DEV` = `cd tools/mock-cloud && uvicorn server:app --port 8080`

## Key Documentation
- `PRD.md` — Product requirements (14 sections, acceptance criteria)
- `tech-spec.md` — Technical specification (12 sections, data model, protocol, APIs)
- `shared-contracts.md` — Exact identifiers, types, enums, SQL schema, test data
- `verification-plan.md` — Per-module verification commands and assertions
- `docs/` — Original design documents (PDFs): scope, Master spec, Agent/Bootloader spec, WBS
