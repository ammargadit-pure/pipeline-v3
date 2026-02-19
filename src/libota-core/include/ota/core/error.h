#ifndef OTA_CORE_ERROR_H
#define OTA_CORE_ERROR_H

#include <stdbool.h>

typedef enum {
    /* Success */
    OTA_OK                          = 0,

    /* General errors (1-99) */
    OTA_ERR_INVALID_ARGUMENT        = 1,
    OTA_ERR_OUT_OF_MEMORY           = 2,
    OTA_ERR_IO                      = 3,
    OTA_ERR_TIMEOUT                 = 4,
    OTA_ERR_NOT_FOUND               = 5,
    OTA_ERR_ALREADY_EXISTS          = 6,
    OTA_ERR_INVALID_STATE           = 7,

    /* Protocol errors (100-199) */
    OTA_ERR_PROTOCOL_MAGIC          = 100,
    OTA_ERR_PROTOCOL_VERSION        = 101,
    OTA_ERR_PROTOCOL_PAYLOAD_SIZE   = 102,
    OTA_ERR_PROTOCOL_PARSE          = 103,
    OTA_ERR_PROTOCOL_UNKNOWN_TYPE   = 104,

    /* Verification errors (200-299) */
    OTA_ERR_VERIFY_SIZE_MISMATCH    = 200,
    OTA_ERR_VERIFY_CRC32_MISMATCH   = 201,
    OTA_ERR_VERIFY_SHA256_MISMATCH  = 202,
    OTA_ERR_VERIFY_SIGNATURE_FAILED = 203,
    OTA_ERR_VERIFY_KEY_LOAD_FAILED  = 204,

    /* Download errors (300-399) */
    OTA_ERR_DOWNLOAD_NETWORK        = 300,
    OTA_ERR_DOWNLOAD_HTTP           = 301,
    OTA_ERR_DOWNLOAD_STORAGE        = 302,
    OTA_ERR_DOWNLOAD_SUPERSEDED     = 303,

    /* Transfer errors (400-499) */
    OTA_ERR_TRANSFER_REJECTED       = 400,
    OTA_ERR_TRANSFER_STORAGE        = 401,
    OTA_ERR_TRANSFER_CHECKSUM       = 402,
    OTA_ERR_TRANSFER_TIMEOUT        = 403,

    /* Installation errors (500-599) */
    OTA_ERR_INSTALL_EXTRACT         = 500,
    OTA_ERR_INSTALL_MANIFEST        = 501,
    OTA_ERR_INSTALL_STORAGE         = 502,
    OTA_ERR_INSTALL_SYMLINK         = 503,
    OTA_ERR_INSTALL_SLOT            = 504,

    /* Activation errors (600-699) */
    OTA_ERR_ACTIVATE_SERVICE_FAIL   = 600,
    OTA_ERR_ACTIVATE_HEALTH_CHECK   = 601,
    OTA_ERR_ACTIVATE_BOOT_FAIL      = 602,
    OTA_ERR_ACTIVATE_TIMEOUT        = 603,

    /* Compatibility errors (700-799) */
    OTA_ERR_COMPAT_NODE_MISMATCH    = 700,
    OTA_ERR_COMPAT_OS_VERSION       = 701,
    OTA_ERR_COMPAT_SDK_VERSION      = 702,
    OTA_ERR_COMPAT_REGION           = 703,
    OTA_ERR_COMPAT_OEM_RULE         = 704,

    /* Bootloader errors (800-899) */
    OTA_ERR_BOOTLOADER_SLOT         = 800,
    OTA_ERR_BOOTLOADER_STAGE        = 801,
    OTA_ERR_BOOTLOADER_ACTIVATE     = 802,
    OTA_ERR_BOOTLOADER_QUERY        = 803,
} ota_error_t;

/* Convert error code to string */
const char *ota_error_str(ota_error_t err);

/* Check if error code falls within a range */
static inline bool ota_error_is_protocol(ota_error_t e) { return e >= 100 && e < 200; }
static inline bool ota_error_is_verify(ota_error_t e)   { return e >= 200 && e < 300; }
static inline bool ota_error_is_download(ota_error_t e)  { return e >= 300 && e < 400; }
static inline bool ota_error_is_transfer(ota_error_t e)  { return e >= 400 && e < 500; }
static inline bool ota_error_is_install(ota_error_t e)   { return e >= 500 && e < 600; }
static inline bool ota_error_is_activate(ota_error_t e)  { return e >= 600 && e < 700; }
static inline bool ota_error_is_compat(ota_error_t e)    { return e >= 700 && e < 800; }
static inline bool ota_error_is_bootloader(ota_error_t e){ return e >= 800 && e < 900; }

#endif /* OTA_CORE_ERROR_H */
