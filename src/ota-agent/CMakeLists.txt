# Bootloader plugin library (sub-component of ota-agent)
# Built as a static library so tests can link against it independently.
add_library(ota-agent-bootloader STATIC
    bootloader/plugin_factory.cpp
    bootloader/sim_plugin.cpp
    bootloader/linux_generic_plugin.cpp
)

target_include_directories(ota-agent-bootloader
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${cjson_SOURCE_DIR}
)

target_link_libraries(ota-agent-bootloader
    PUBLIC
        ota-core
    PRIVATE
        cjson
)
