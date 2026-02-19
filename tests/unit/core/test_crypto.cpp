#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "ota/core/crypto.h"
}

TEST(Crypto, Crc32KnownValue)
{
    const uint8_t data[] = "123456789";
    ota_crc32_t crc = ota_crc32(data, 9);
    EXPECT_EQ(crc, 0xCBF43926u);
    EXPECT_NE(crc, 0u);
}

TEST(Crypto, Sha256KnownValue)
{
    const uint8_t data[] = "";
    uint8_t digest[OTA_SHA256_DIGEST_LEN];
    ASSERT_EQ(ota_sha256(data, 0, digest), OTA_OK);

    char hex[OTA_MAX_SHA256_HEX_LEN];
    ota_sha256_to_hex(digest, hex);
    EXPECT_STREQ(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}
