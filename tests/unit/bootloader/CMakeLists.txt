add_executable(unit_bootloader_tests
    test_bootloader_interface.cpp
    test_plugin_factory.cpp
    test_linux_generic_plugin.cpp
)

target_link_libraries(unit_bootloader_tests
    PRIVATE
        ota-agent-bootloader
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unit_bootloader_tests
    TEST_PREFIX "unit_bootloader_"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
