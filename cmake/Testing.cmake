enable_testing()
include(CTest)

# Coverage flags
if(OTA_COVERAGE)
    add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
endif()
