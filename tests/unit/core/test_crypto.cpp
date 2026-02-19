#include <gtest/gtest.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

extern "C" {
#include "ota/core/crypto.h"
}

/* Helper: path to test keys relative to project root */
static const char *test_pubkey_path()
{
    static char path[512];
    snprintf(path, sizeof(path), "%s/tests/fixtures/keys/test_public.pem",
             getenv("OTA_PROJECT_ROOT") ? getenv("OTA_PROJECT_ROOT") : ".");
    return path;
}

static const char *test_privkey_path()
{
    static char path[512];
    snprintf(path, sizeof(path), "%s/tests/fixtures/keys/test_private.pem",
             getenv("OTA_PROJECT_ROOT") ? getenv("OTA_PROJECT_ROOT") : ".");
    return path;
}

/* --- CRC32 tests --- */

TEST(Crypto, Crc32KnownValue)
{
    const uint8_t data[] = "123456789";
    ota_crc32_t crc = ota_crc32(data, 9);
    EXPECT_EQ(crc, 0xCBF43926u);
    EXPECT_NE(crc, 0u);
}

TEST(Crypto, Crc32Empty)
{
    ota_crc32_t crc = ota_crc32((const uint8_t *)"", 0);
    EXPECT_EQ(crc, 0x00000000u);
    EXPECT_NE(crc, 0xCBF43926u);
}

TEST(Crypto, Crc32Incremental)
{
    const uint8_t data[] = "123456789";
    ota_crc32_t single = ota_crc32(data, 9);

    ota_crc32_ctx_t ctx;
    ota_crc32_init(&ctx);
    ota_crc32_update(&ctx, data, 4);
    ota_crc32_update(&ctx, data + 4, 5);
    ota_crc32_t incremental = ota_crc32_finalize(&ctx);

    EXPECT_EQ(single, incremental);
    EXPECT_EQ(incremental, 0xCBF43926u);
}

/* --- SHA-256 tests --- */

TEST(Crypto, Sha256KnownValue)
{
    /* SHA-256("abc") */
    const uint8_t data[] = "abc";
    uint8_t digest[OTA_SHA256_DIGEST_LEN];
    ASSERT_EQ(ota_sha256(data, 3, digest), OTA_OK);

    char hex[OTA_MAX_SHA256_HEX_LEN];
    ota_sha256_to_hex(digest, hex);
    EXPECT_STREQ(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(Crypto, Sha256Empty)
{
    const uint8_t data[] = "";
    uint8_t digest[OTA_SHA256_DIGEST_LEN];
    ASSERT_EQ(ota_sha256(data, 0, digest), OTA_OK);

    char hex[OTA_MAX_SHA256_HEX_LEN];
    ota_sha256_to_hex(digest, hex);
    EXPECT_STREQ(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(Crypto, Sha256Incremental)
{
    const uint8_t data[] = "abcdef";
    uint8_t single_digest[OTA_SHA256_DIGEST_LEN];
    ASSERT_EQ(ota_sha256(data, 6, single_digest), OTA_OK);

    ota_sha256_ctx_t ctx;
    ASSERT_EQ(ota_sha256_init(&ctx), OTA_OK);
    ASSERT_EQ(ota_sha256_update(&ctx, data, 3), OTA_OK);
    ASSERT_EQ(ota_sha256_update(&ctx, data + 3, 3), OTA_OK);
    uint8_t inc_digest[OTA_SHA256_DIGEST_LEN];
    ASSERT_EQ(ota_sha256_finalize(&ctx, inc_digest), OTA_OK);

    EXPECT_EQ(memcmp(single_digest, inc_digest, OTA_SHA256_DIGEST_LEN), 0);

    char hex[OTA_MAX_SHA256_HEX_LEN];
    ota_sha256_to_hex(inc_digest, hex);
    EXPECT_NE(strlen(hex), 0u);
}

TEST(Crypto, Sha256FileMatchesBuffer)
{
    /* Write known data to a temp file, compute digest both ways */
    const char *tmp_path = "/tmp/ota-test-sha256-file.bin";
    const uint8_t data[] = "hello world for sha256 test";
    size_t data_len = strlen((const char *)data);

    FILE *f = fopen(tmp_path, "wb");
    ASSERT_NE(f, nullptr);
    fwrite(data, 1, data_len, f);
    fclose(f);

    uint8_t buf_digest[OTA_SHA256_DIGEST_LEN];
    ASSERT_EQ(ota_sha256(data, data_len, buf_digest), OTA_OK);

    uint8_t file_digest[OTA_SHA256_DIGEST_LEN];
    ASSERT_EQ(ota_sha256_file(tmp_path, file_digest), OTA_OK);

    EXPECT_EQ(memcmp(buf_digest, file_digest, OTA_SHA256_DIGEST_LEN), 0);

    unlink(tmp_path);
}

TEST(Crypto, Sha256ToHex)
{
    uint8_t digest[OTA_SHA256_DIGEST_LEN];
    memset(digest, 0, sizeof(digest));
    digest[0] = 0xAB;
    digest[1] = 0xCD;
    digest[31] = 0xEF;

    char hex[OTA_MAX_SHA256_HEX_LEN];
    ota_sha256_to_hex(digest, hex);

    EXPECT_EQ(strlen(hex), 64u);
    EXPECT_EQ(hex[0], 'a');
    EXPECT_EQ(hex[1], 'b');
    EXPECT_EQ(hex[2], 'c');
    EXPECT_EQ(hex[3], 'd');
}

TEST(Crypto, Sha256HexMatches)
{
    const uint8_t data[] = "";
    uint8_t digest[OTA_SHA256_DIGEST_LEN];
    ASSERT_EQ(ota_sha256(data, 0, digest), OTA_OK);

    EXPECT_TRUE(ota_sha256_hex_matches(
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", digest));
    EXPECT_FALSE(ota_sha256_hex_matches(
        "0000000000000000000000000000000000000000000000000000000000000000", digest));
}

TEST(Crypto, Sha256HexMismatch)
{
    const uint8_t data[] = "abc";
    uint8_t digest[OTA_SHA256_DIGEST_LEN];
    ASSERT_EQ(ota_sha256(data, 3, digest), OTA_OK);

    EXPECT_FALSE(ota_sha256_hex_matches("wrong_hex_value_that_does_not_match_at_all_padding0000000000", digest));
    EXPECT_TRUE(ota_sha256_hex_matches(
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", digest));
}

/* --- ECDSA tests --- */

TEST(Crypto, EcdsaSignVerifyRoundtrip)
{
    const uint8_t data[] = "test data for signing";
    size_t data_len = strlen((const char *)data);

    uint8_t sig[256];
    size_t sig_len = sizeof(sig);

    ota_error_t sign_err = ota_ecdsa_sign(data, data_len, test_privkey_path(), sig, &sig_len);
    ASSERT_EQ(sign_err, OTA_OK);
    EXPECT_GT(sig_len, 0u);

    ota_error_t verify_err = ota_ecdsa_verify(data, data_len, sig, sig_len, test_pubkey_path());
    EXPECT_EQ(verify_err, OTA_OK);
}

TEST(Crypto, EcdsaVerifyBadSignature)
{
    const uint8_t data[] = "test data for signing";
    size_t data_len = strlen((const char *)data);

    uint8_t sig[256];
    size_t sig_len = sizeof(sig);
    ASSERT_EQ(ota_ecdsa_sign(data, data_len, test_privkey_path(), sig, &sig_len), OTA_OK);

    /* Tamper with signature */
    sig[0] ^= 0xFF;

    ota_error_t err = ota_ecdsa_verify(data, data_len, sig, sig_len, test_pubkey_path());
    EXPECT_EQ(err, OTA_ERR_VERIFY_SIGNATURE_FAILED);
    EXPECT_NE(err, OTA_OK);
}

TEST(Crypto, EcdsaVerifyWrongKey)
{
    /* Generate a temporary second keypair */
    const char *tmp_priv = "/tmp/ota-test-wrong-key-priv.pem";
    const char *tmp_pub = "/tmp/ota-test-wrong-key-pub.pem";

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "openssl ecparam -name prime256v1 -genkey -noout -out %s 2>/dev/null && "
             "openssl ec -in %s -pubout -out %s 2>/dev/null",
             tmp_priv, tmp_priv, tmp_pub);
    int rc = system(cmd);
    ASSERT_EQ(rc, 0) << "Failed to generate temp keypair";

    const uint8_t data[] = "test data for wrong key";
    size_t data_len = strlen((const char *)data);

    /* Sign with test key */
    uint8_t sig[256];
    size_t sig_len = sizeof(sig);
    ASSERT_EQ(ota_ecdsa_sign(data, data_len, test_privkey_path(), sig, &sig_len), OTA_OK);

    /* Verify with wrong key should fail */
    ota_error_t err = ota_ecdsa_verify(data, data_len, sig, sig_len, tmp_pub);
    EXPECT_EQ(err, OTA_ERR_VERIFY_SIGNATURE_FAILED);
    EXPECT_NE(err, OTA_OK);

    unlink(tmp_priv);
    unlink(tmp_pub);
}

TEST(Crypto, EcdsaVerifyNonexistentKey)
{
    const uint8_t data[] = "test";
    uint8_t sig[64] = {0};

    ota_error_t err = ota_ecdsa_verify(data, 4, sig, sizeof(sig), "/tmp/nonexistent_key.pem");
    EXPECT_EQ(err, OTA_ERR_VERIFY_KEY_LOAD_FAILED);
    EXPECT_NE(err, OTA_OK);
}
