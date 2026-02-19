#ifndef OTA_CORE_CRYPTO_H
#define OTA_CORE_CRYPTO_H

#include "ota/core/types.h"
#include "ota/core/error.h"

/* CRC32 — compute over a buffer (ITU-T / zlib polynomial) */
ota_crc32_t ota_crc32(const uint8_t *data, size_t len);

/* CRC32 — incremental: init, update, finalize */
typedef struct { uint32_t state; } ota_crc32_ctx_t;
void        ota_crc32_init(ota_crc32_ctx_t *ctx);
void        ota_crc32_update(ota_crc32_ctx_t *ctx, const uint8_t *data, size_t len);
ota_crc32_t ota_crc32_finalize(ota_crc32_ctx_t *ctx);

/* SHA-256 — compute digest over buffer */
ota_error_t ota_sha256(const uint8_t *data, size_t len, uint8_t digest[OTA_SHA256_DIGEST_LEN]);

/* SHA-256 — incremental */
typedef struct { uint8_t opaque[128]; } ota_sha256_ctx_t;
ota_error_t ota_sha256_init(ota_sha256_ctx_t *ctx);
ota_error_t ota_sha256_update(ota_sha256_ctx_t *ctx, const uint8_t *data, size_t len);
ota_error_t ota_sha256_finalize(ota_sha256_ctx_t *ctx, uint8_t digest[OTA_SHA256_DIGEST_LEN]);

/* SHA-256 — compute over file */
ota_error_t ota_sha256_file(const char *path, uint8_t digest[OTA_SHA256_DIGEST_LEN]);

/* Convert SHA-256 digest to hex string (must be >= OTA_MAX_SHA256_HEX_LEN) */
void ota_sha256_to_hex(const uint8_t digest[OTA_SHA256_DIGEST_LEN], char *hex_out);

/* Compare hex string against digest */
bool ota_sha256_hex_matches(const char *hex, const uint8_t digest[OTA_SHA256_DIGEST_LEN]);

/* ECDSA P-256 — verify signature of data using PEM public key file */
ota_error_t ota_ecdsa_verify(const uint8_t *data, size_t data_len,
                              const uint8_t *signature, size_t sig_len,
                              const char *pubkey_pem_path);

/* ECDSA P-256 — sign data using PEM private key file (for packaging tool) */
ota_error_t ota_ecdsa_sign(const uint8_t *data, size_t data_len,
                            const char *privkey_pem_path,
                            uint8_t *sig_out, size_t *sig_len);

#endif /* OTA_CORE_CRYPTO_H */
