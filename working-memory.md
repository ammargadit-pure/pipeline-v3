# Working Memory

## Patterns That Work
- FetchContent for GoogleTest, cJSON works well with CMake 3.20+
- OpenSSL 3.x EVP API for all crypto (SHA-256, ECDSA P-256)
- CRC32 using zlib polynomial (0xEDB88320) with lookup table — known value 0xCBF43926 for "123456789"
- Atomic file writes: write tmp → fsync → rename pattern
- cJSON for C11 JSON parsing/serialization in libota-core
- htons/htonl for network byte order in protocol header serialization

## Anti-Patterns to Avoid
- Don't use deprecated OpenSSL APIs (EVP is the way forward)
- Don't forget to add `#include <arpa/inet.h>` for htons/htonl on Linux
- Don't forget to link OpenSSL::Crypto (not just OpenSSL::SSL) for EVP functions

## Codebase Insights
- Root CMakeLists.txt uses FetchContent for GoogleTest 1.14, cJSON 1.7.17
- CompilerFlags.cmake: -Wall -Wextra -Wpedantic (NOT -Werror, to avoid blocking on minor warnings)
- ota-core is a static library (libota-core.a)
- All 9 public headers in src/libota-core/include/ match shared-contracts.md exactly
- All 7 source files in src/libota-core/src/ implement complete contract coverage
- Test fixtures exist: keys (ECDSA P-256), configs (master+agent), manifests (sw, fw, incompatible)

## Test Insights
- Tests are registered as individual CTest cases via gtest_discover_tests
- Test naming: unit_core_{Suite}.{TestName}
- 68 comprehensive unit tests across 7 test files — ALL PASS
- Each test has >= 2 assertions as required
- No GTEST_SKIP, no DISABLED_, no mocks — pure unit tests of C functions
- Test breakdown: error(5), manifest(11), protocol(10), state(9), version(13), crypto(14), persistence(6)
- WORKING_DIRECTORY set to CMAKE_SOURCE_DIR so test key paths resolve relative to project root
- ECDSA tests generate temp keypairs via system("openssl ...") for wrong-key verification tests

## Iteration Log
- found-001 completed: CMake scaffold + full libota-core (7 source files, 9 headers, 12 passing tests)
- test-found-001 completed: 68 comprehensive unit tests for all 7 libota-core modules — ALL PASS, zero warnings
- test-found-001 validation: clang-tidy exits 0 (only clang-analyzer DeprecatedOrUnsafeBufferHandling warnings for standard C functions — expected in C11 code)
- test-found-001 status: dev_complete — quality gate satisfied, downstream tasks unblocked
