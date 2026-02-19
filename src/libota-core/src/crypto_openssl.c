#include "ota/core/crypto.h"
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* --- CRC32 (zlib polynomial 0xEDB88320) --- */

static uint32_t crc32_table[256];
static bool crc32_table_init = false;

static void crc32_generate_table(void)
{
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320u;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_table_init = true;
}

static void ensure_crc32_table(void)
{
    if (!crc32_table_init) {
        crc32_generate_table();
    }
}

ota_crc32_t ota_crc32(const uint8_t *data, size_t len)
{
    ensure_crc32_table();
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

void ota_crc32_init(ota_crc32_ctx_t *ctx)
{
    ensure_crc32_table();
    if (ctx) {
        ctx->state = 0xFFFFFFFFu;
    }
}

void ota_crc32_update(ota_crc32_ctx_t *ctx, const uint8_t *data, size_t len)
{
    if (!ctx || !data) return;
    uint32_t crc = ctx->state;
    for (size_t i = 0; i < len; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    ctx->state = crc;
}

ota_crc32_t ota_crc32_finalize(ota_crc32_ctx_t *ctx)
{
    if (!ctx) return 0;
    return ctx->state ^ 0xFFFFFFFFu;
}

/* --- SHA-256 --- */

ota_error_t ota_sha256(const uint8_t *data, size_t len, uint8_t digest[OTA_SHA256_DIGEST_LEN])
{
    if (!data || !digest) return OTA_ERR_INVALID_ARGUMENT;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) return OTA_ERR_OUT_OF_MEMORY;

    unsigned int md_len = 0;
    int ok = EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) &&
             EVP_DigestUpdate(ctx, data, len) &&
             EVP_DigestFinal_ex(ctx, digest, &md_len);

    EVP_MD_CTX_free(ctx);
    return ok ? OTA_OK : OTA_ERR_IO;
}

ota_error_t ota_sha256_init(ota_sha256_ctx_t *ctx)
{
    if (!ctx) return OTA_ERR_INVALID_ARGUMENT;
    memset(ctx, 0, sizeof(*ctx));

    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) return OTA_ERR_OUT_OF_MEMORY;

    if (!EVP_DigestInit_ex(md_ctx, EVP_sha256(), NULL)) {
        EVP_MD_CTX_free(md_ctx);
        return OTA_ERR_IO;
    }

    /* Store pointer in opaque buffer */
    memcpy(ctx->opaque, &md_ctx, sizeof(md_ctx));
    return OTA_OK;
}

static EVP_MD_CTX *get_md_ctx(ota_sha256_ctx_t *ctx)
{
    EVP_MD_CTX *md_ctx = NULL;
    memcpy(&md_ctx, ctx->opaque, sizeof(md_ctx));
    return md_ctx;
}

ota_error_t ota_sha256_update(ota_sha256_ctx_t *ctx, const uint8_t *data, size_t len)
{
    if (!ctx || !data) return OTA_ERR_INVALID_ARGUMENT;

    EVP_MD_CTX *md_ctx = get_md_ctx(ctx);
    if (!md_ctx) return OTA_ERR_INVALID_STATE;

    if (!EVP_DigestUpdate(md_ctx, data, len)) {
        return OTA_ERR_IO;
    }
    return OTA_OK;
}

ota_error_t ota_sha256_finalize(ota_sha256_ctx_t *ctx, uint8_t digest[OTA_SHA256_DIGEST_LEN])
{
    if (!ctx || !digest) return OTA_ERR_INVALID_ARGUMENT;

    EVP_MD_CTX *md_ctx = get_md_ctx(ctx);
    if (!md_ctx) return OTA_ERR_INVALID_STATE;

    unsigned int md_len = 0;
    int ok = EVP_DigestFinal_ex(md_ctx, digest, &md_len);
    EVP_MD_CTX_free(md_ctx);
    memset(ctx->opaque, 0, sizeof(ctx->opaque));

    return ok ? OTA_OK : OTA_ERR_IO;
}

ota_error_t ota_sha256_file(const char *path, uint8_t digest[OTA_SHA256_DIGEST_LEN])
{
    if (!path || !digest) return OTA_ERR_INVALID_ARGUMENT;

    FILE *f = fopen(path, "rb");
    if (!f) return OTA_ERR_NOT_FOUND;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        fclose(f);
        return OTA_ERR_OUT_OF_MEMORY;
    }

    if (!EVP_DigestInit_ex(ctx, EVP_sha256(), NULL)) {
        EVP_MD_CTX_free(ctx);
        fclose(f);
        return OTA_ERR_IO;
    }

    uint8_t buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (!EVP_DigestUpdate(ctx, buf, n)) {
            EVP_MD_CTX_free(ctx);
            fclose(f);
            return OTA_ERR_IO;
        }
    }

    if (ferror(f)) {
        EVP_MD_CTX_free(ctx);
        fclose(f);
        return OTA_ERR_IO;
    }

    fclose(f);

    unsigned int md_len = 0;
    int ok = EVP_DigestFinal_ex(ctx, digest, &md_len);
    EVP_MD_CTX_free(ctx);

    return ok ? OTA_OK : OTA_ERR_IO;
}

void ota_sha256_to_hex(const uint8_t digest[OTA_SHA256_DIGEST_LEN], char *hex_out)
{
    static const char hex_chars[] = "0123456789abcdef";
    for (int i = 0; i < OTA_SHA256_DIGEST_LEN; i++) {
        hex_out[i * 2]     = hex_chars[(digest[i] >> 4) & 0x0F];
        hex_out[i * 2 + 1] = hex_chars[digest[i] & 0x0F];
    }
    hex_out[OTA_SHA256_DIGEST_LEN * 2] = '\0';
}

bool ota_sha256_hex_matches(const char *hex, const uint8_t digest[OTA_SHA256_DIGEST_LEN])
{
    if (!hex || !digest) return false;

    char computed[OTA_MAX_SHA256_HEX_LEN];
    ota_sha256_to_hex(digest, computed);
    return strcmp(hex, computed) == 0;
}

/* --- ECDSA P-256 --- */

ota_error_t ota_ecdsa_verify(const uint8_t *data, size_t data_len,
                              const uint8_t *signature, size_t sig_len,
                              const char *pubkey_pem_path)
{
    if (!data || !signature || !pubkey_pem_path) {
        return OTA_ERR_INVALID_ARGUMENT;
    }

    FILE *f = fopen(pubkey_pem_path, "r");
    if (!f) return OTA_ERR_VERIFY_KEY_LOAD_FAILED;

    EVP_PKEY *pkey = PEM_read_PUBKEY(f, NULL, NULL, NULL);
    fclose(f);
    if (!pkey) return OTA_ERR_VERIFY_KEY_LOAD_FAILED;

    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        EVP_PKEY_free(pkey);
        return OTA_ERR_OUT_OF_MEMORY;
    }

    ota_error_t result = OTA_ERR_VERIFY_SIGNATURE_FAILED;

    if (EVP_DigestVerifyInit(md_ctx, NULL, EVP_sha256(), NULL, pkey) == 1 &&
        EVP_DigestVerifyUpdate(md_ctx, data, data_len) == 1) {
        int rc = EVP_DigestVerifyFinal(md_ctx, signature, sig_len);
        if (rc == 1) {
            result = OTA_OK;
        }
    }

    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);
    return result;
}

ota_error_t ota_ecdsa_sign(const uint8_t *data, size_t data_len,
                            const char *privkey_pem_path,
                            uint8_t *sig_out, size_t *sig_len)
{
    if (!data || !privkey_pem_path || !sig_out || !sig_len) {
        return OTA_ERR_INVALID_ARGUMENT;
    }

    FILE *f = fopen(privkey_pem_path, "r");
    if (!f) return OTA_ERR_VERIFY_KEY_LOAD_FAILED;

    EVP_PKEY *pkey = PEM_read_PrivateKey(f, NULL, NULL, NULL);
    fclose(f);
    if (!pkey) return OTA_ERR_VERIFY_KEY_LOAD_FAILED;

    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        EVP_PKEY_free(pkey);
        return OTA_ERR_OUT_OF_MEMORY;
    }

    ota_error_t result = OTA_ERR_IO;

    if (EVP_DigestSignInit(md_ctx, NULL, EVP_sha256(), NULL, pkey) == 1 &&
        EVP_DigestSignUpdate(md_ctx, data, data_len) == 1) {
        /* First call to get required sig length */
        size_t required_len = 0;
        if (EVP_DigestSignFinal(md_ctx, NULL, &required_len) == 1) {
            if (required_len <= *sig_len) {
                if (EVP_DigestSignFinal(md_ctx, sig_out, &required_len) == 1) {
                    *sig_len = required_len;
                    result = OTA_OK;
                }
            }
        }
    }

    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);
    return result;
}
