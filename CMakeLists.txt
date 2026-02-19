cmake_minimum_required(VERSION 3.20)
project(ota-system LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(OTA_BUILD_TESTS "Build test executables" OFF)
option(OTA_BUILD_MOCK_CLOUD "Build mock cloud server support" OFF)
option(OTA_COVERAGE "Enable code coverage" OFF)

# Include CMake modules
include(cmake/CompilerFlags.cmake)
include(cmake/Dependencies.cmake)

if(OTA_BUILD_TESTS)
    include(cmake/Testing.cmake)
endif()

# Source directories
add_subdirectory(src/libota-core)

# Tests
if(OTA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
