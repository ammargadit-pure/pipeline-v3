#include "ota/core/error.h"

const char *ota_error_str(ota_error_t err)
{
    switch (err) {
    /* Success */
    case OTA_OK:                          return "OTA_OK";

    /* General errors */
    case OTA_ERR_INVALID_ARGUMENT:        return "OTA_ERR_INVALID_ARGUMENT";
    case OTA_ERR_OUT_OF_MEMORY:           return "OTA_ERR_OUT_OF_MEMORY";
    case OTA_ERR_IO:                      return "OTA_ERR_IO";
    case OTA_ERR_TIMEOUT:                 return "OTA_ERR_TIMEOUT";
    case OTA_ERR_NOT_FOUND:               return "OTA_ERR_NOT_FOUND";
    case OTA_ERR_ALREADY_EXISTS:          return "OTA_ERR_ALREADY_EXISTS";
    case OTA_ERR_INVALID_STATE:           return "OTA_ERR_INVALID_STATE";

    /* Protocol errors */
    case OTA_ERR_PROTOCOL_MAGIC:          return "OTA_ERR_PROTOCOL_MAGIC";
    case OTA_ERR_PROTOCOL_VERSION:        return "OTA_ERR_PROTOCOL_VERSION";
    case OTA_ERR_PROTOCOL_PAYLOAD_SIZE:   return "OTA_ERR_PROTOCOL_PAYLOAD_SIZE";
    case OTA_ERR_PROTOCOL_PARSE:          return "OTA_ERR_PROTOCOL_PARSE";
    case OTA_ERR_PROTOCOL_UNKNOWN_TYPE:   return "OTA_ERR_PROTOCOL_UNKNOWN_TYPE";

    /* Verification errors */
    case OTA_ERR_VERIFY_SIZE_MISMATCH:    return "OTA_ERR_VERIFY_SIZE_MISMATCH";
    case OTA_ERR_VERIFY_CRC32_MISMATCH:   return "OTA_ERR_VERIFY_CRC32_MISMATCH";
    case OTA_ERR_VERIFY_SHA256_MISMATCH:  return "OTA_ERR_VERIFY_SHA256_MISMATCH";
    case OTA_ERR_VERIFY_SIGNATURE_FAILED: return "OTA_ERR_VERIFY_SIGNATURE_FAILED";
    case OTA_ERR_VERIFY_KEY_LOAD_FAILED:  return "OTA_ERR_VERIFY_KEY_LOAD_FAILED";

    /* Download errors */
    case OTA_ERR_DOWNLOAD_NETWORK:        return "OTA_ERR_DOWNLOAD_NETWORK";
    case OTA_ERR_DOWNLOAD_HTTP:           return "OTA_ERR_DOWNLOAD_HTTP";
    case OTA_ERR_DOWNLOAD_STORAGE:        return "OTA_ERR_DOWNLOAD_STORAGE";
    case OTA_ERR_DOWNLOAD_SUPERSEDED:     return "OTA_ERR_DOWNLOAD_SUPERSEDED";

    /* Transfer errors */
    case OTA_ERR_TRANSFER_REJECTED:       return "OTA_ERR_TRANSFER_REJECTED";
    case OTA_ERR_TRANSFER_STORAGE:        return "OTA_ERR_TRANSFER_STORAGE";
    case OTA_ERR_TRANSFER_CHECKSUM:       return "OTA_ERR_TRANSFER_CHECKSUM";
    case OTA_ERR_TRANSFER_TIMEOUT:        return "OTA_ERR_TRANSFER_TIMEOUT";

    /* Installation errors */
    case OTA_ERR_INSTALL_EXTRACT:         return "OTA_ERR_INSTALL_EXTRACT";
    case OTA_ERR_INSTALL_MANIFEST:        return "OTA_ERR_INSTALL_MANIFEST";
    case OTA_ERR_INSTALL_STORAGE:         return "OTA_ERR_INSTALL_STORAGE";
    case OTA_ERR_INSTALL_SYMLINK:         return "OTA_ERR_INSTALL_SYMLINK";
    case OTA_ERR_INSTALL_SLOT:            return "OTA_ERR_INSTALL_SLOT";

    /* Activation errors */
    case OTA_ERR_ACTIVATE_SERVICE_FAIL:   return "OTA_ERR_ACTIVATE_SERVICE_FAIL";
    case OTA_ERR_ACTIVATE_HEALTH_CHECK:   return "OTA_ERR_ACTIVATE_HEALTH_CHECK";
    case OTA_ERR_ACTIVATE_BOOT_FAIL:      return "OTA_ERR_ACTIVATE_BOOT_FAIL";
    case OTA_ERR_ACTIVATE_TIMEOUT:        return "OTA_ERR_ACTIVATE_TIMEOUT";

    /* Compatibility errors */
    case OTA_ERR_COMPAT_NODE_MISMATCH:    return "OTA_ERR_COMPAT_NODE_MISMATCH";
    case OTA_ERR_COMPAT_OS_VERSION:       return "OTA_ERR_COMPAT_OS_VERSION";
    case OTA_ERR_COMPAT_SDK_VERSION:      return "OTA_ERR_COMPAT_SDK_VERSION";
    case OTA_ERR_COMPAT_REGION:           return "OTA_ERR_COMPAT_REGION";
    case OTA_ERR_COMPAT_OEM_RULE:         return "OTA_ERR_COMPAT_OEM_RULE";

    /* Bootloader errors */
    case OTA_ERR_BOOTLOADER_SLOT:         return "OTA_ERR_BOOTLOADER_SLOT";
    case OTA_ERR_BOOTLOADER_STAGE:        return "OTA_ERR_BOOTLOADER_STAGE";
    case OTA_ERR_BOOTLOADER_ACTIVATE:     return "OTA_ERR_BOOTLOADER_ACTIVATE";
    case OTA_ERR_BOOTLOADER_QUERY:        return "OTA_ERR_BOOTLOADER_QUERY";
    }

    return "unknown";
}
