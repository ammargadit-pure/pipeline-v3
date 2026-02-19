#include <gtest/gtest.h>

extern "C" {
#include "ota/core/error.h"
}

TEST(Error, ErrorStrReturnsStringForAllCodes)
{
    const ota_error_t codes[] = {
        OTA_OK,
        OTA_ERR_INVALID_ARGUMENT, OTA_ERR_OUT_OF_MEMORY, OTA_ERR_IO,
        OTA_ERR_TIMEOUT, OTA_ERR_NOT_FOUND, OTA_ERR_ALREADY_EXISTS, OTA_ERR_INVALID_STATE,
        OTA_ERR_PROTOCOL_MAGIC, OTA_ERR_PROTOCOL_VERSION, OTA_ERR_PROTOCOL_PAYLOAD_SIZE,
        OTA_ERR_PROTOCOL_PARSE, OTA_ERR_PROTOCOL_UNKNOWN_TYPE,
        OTA_ERR_VERIFY_SIZE_MISMATCH, OTA_ERR_VERIFY_CRC32_MISMATCH,
        OTA_ERR_VERIFY_SHA256_MISMATCH, OTA_ERR_VERIFY_SIGNATURE_FAILED,
        OTA_ERR_VERIFY_KEY_LOAD_FAILED,
        OTA_ERR_DOWNLOAD_NETWORK, OTA_ERR_DOWNLOAD_HTTP, OTA_ERR_DOWNLOAD_STORAGE,
        OTA_ERR_DOWNLOAD_SUPERSEDED,
        OTA_ERR_TRANSFER_REJECTED, OTA_ERR_TRANSFER_STORAGE, OTA_ERR_TRANSFER_CHECKSUM,
        OTA_ERR_TRANSFER_TIMEOUT,
        OTA_ERR_INSTALL_EXTRACT, OTA_ERR_INSTALL_MANIFEST, OTA_ERR_INSTALL_STORAGE,
        OTA_ERR_INSTALL_SYMLINK, OTA_ERR_INSTALL_SLOT,
        OTA_ERR_ACTIVATE_SERVICE_FAIL, OTA_ERR_ACTIVATE_HEALTH_CHECK,
        OTA_ERR_ACTIVATE_BOOT_FAIL, OTA_ERR_ACTIVATE_TIMEOUT,
        OTA_ERR_COMPAT_NODE_MISMATCH, OTA_ERR_COMPAT_OS_VERSION,
        OTA_ERR_COMPAT_SDK_VERSION, OTA_ERR_COMPAT_REGION, OTA_ERR_COMPAT_OEM_RULE,
        OTA_ERR_BOOTLOADER_SLOT, OTA_ERR_BOOTLOADER_STAGE,
        OTA_ERR_BOOTLOADER_ACTIVATE, OTA_ERR_BOOTLOADER_QUERY,
    };

    for (size_t i = 0; i < sizeof(codes) / sizeof(codes[0]); i++) {
        const char *s = ota_error_str(codes[i]);
        ASSERT_NE(s, nullptr) << "ota_error_str returned null for code " << (int)codes[i];
        EXPECT_STRNE(s, "unknown") << "ota_error_str returned 'unknown' for code " << (int)codes[i];
    }

    EXPECT_STREQ(ota_error_str(OTA_OK), "OTA_OK");
    EXPECT_STREQ(ota_error_str(OTA_ERR_INVALID_ARGUMENT), "OTA_ERR_INVALID_ARGUMENT");
}

TEST(Error, ErrorStrReturnsUnknownForInvalid)
{
    const char *s = ota_error_str((ota_error_t)9999);
    ASSERT_NE(s, nullptr);
    EXPECT_STREQ(s, "unknown");

    const char *s2 = ota_error_str((ota_error_t)-1);
    ASSERT_NE(s2, nullptr);
    EXPECT_STREQ(s2, "unknown");
}

TEST(Error, RangeCheckProtocol)
{
    EXPECT_TRUE(ota_error_is_protocol((ota_error_t)100));
    EXPECT_TRUE(ota_error_is_protocol((ota_error_t)199));
    EXPECT_FALSE(ota_error_is_protocol((ota_error_t)99));
    EXPECT_FALSE(ota_error_is_protocol((ota_error_t)200));
}

TEST(Error, RangeCheckVerify)
{
    EXPECT_TRUE(ota_error_is_verify((ota_error_t)200));
    EXPECT_TRUE(ota_error_is_verify((ota_error_t)299));
    EXPECT_FALSE(ota_error_is_verify((ota_error_t)199));
    EXPECT_FALSE(ota_error_is_verify((ota_error_t)300));
}

TEST(Error, RangeCheckAllCategories)
{
    EXPECT_TRUE(ota_error_is_protocol(OTA_ERR_PROTOCOL_MAGIC));
    EXPECT_FALSE(ota_error_is_verify(OTA_ERR_PROTOCOL_MAGIC));

    EXPECT_TRUE(ota_error_is_verify(OTA_ERR_VERIFY_SIZE_MISMATCH));
    EXPECT_FALSE(ota_error_is_download(OTA_ERR_VERIFY_SIZE_MISMATCH));

    EXPECT_TRUE(ota_error_is_download(OTA_ERR_DOWNLOAD_NETWORK));
    EXPECT_FALSE(ota_error_is_transfer(OTA_ERR_DOWNLOAD_NETWORK));

    EXPECT_TRUE(ota_error_is_transfer(OTA_ERR_TRANSFER_REJECTED));
    EXPECT_FALSE(ota_error_is_install(OTA_ERR_TRANSFER_REJECTED));

    EXPECT_TRUE(ota_error_is_install(OTA_ERR_INSTALL_EXTRACT));
    EXPECT_FALSE(ota_error_is_activate(OTA_ERR_INSTALL_EXTRACT));

    EXPECT_TRUE(ota_error_is_activate(OTA_ERR_ACTIVATE_SERVICE_FAIL));
    EXPECT_FALSE(ota_error_is_compat(OTA_ERR_ACTIVATE_SERVICE_FAIL));

    EXPECT_TRUE(ota_error_is_compat(OTA_ERR_COMPAT_NODE_MISMATCH));
    EXPECT_FALSE(ota_error_is_bootloader(OTA_ERR_COMPAT_NODE_MISMATCH));

    EXPECT_TRUE(ota_error_is_bootloader(OTA_ERR_BOOTLOADER_SLOT));
    EXPECT_FALSE(ota_error_is_protocol(OTA_ERR_BOOTLOADER_SLOT));

    EXPECT_FALSE(ota_error_is_protocol(OTA_OK));
    EXPECT_FALSE(ota_error_is_verify(OTA_OK));
}
