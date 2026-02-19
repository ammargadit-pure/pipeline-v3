add_library(ota-core STATIC
    src/error.c
    src/manifest.c
    src/protocol.c
    src/state.c
    src/version.c
    src/crypto_openssl.c
    src/persistence.c
)

target_include_directories(ota-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${cjson_SOURCE_DIR}
)

target_link_libraries(ota-core
    PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        cjson
)
