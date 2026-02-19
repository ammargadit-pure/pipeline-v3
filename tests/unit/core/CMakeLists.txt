add_executable(unit_core_tests
    test_error.cpp
    test_manifest.cpp
    test_protocol.cpp
    test_state.cpp
    test_version.cpp
    test_crypto.cpp
    test_persistence.cpp
)

target_link_libraries(unit_core_tests
    PRIVATE
        ota-core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unit_core_tests
    TEST_PREFIX "unit_core_"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
