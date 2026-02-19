# Compiler warning flags
add_compile_options(
    -Wall
    -Wextra
    -Werror
    -Wpedantic
)

# Sanitizers for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
endif()
