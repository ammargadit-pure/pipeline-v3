#ifndef OTA_CORE_PERSISTENCE_H
#define OTA_CORE_PERSISTENCE_H

#include "ota/core/types.h"
#include "ota/core/error.h"

/* Atomic file write: write to temp file, fsync, rename to target path */
ota_error_t ota_atomic_write(const char *path, const uint8_t *data, size_t len);

/* Atomic JSON write: serialize JSON string, write atomically */
ota_error_t ota_atomic_write_json(const char *path, const char *json_str);

/* Read entire file into heap-allocated buffer. Caller frees *out_data. */
ota_error_t ota_read_file(const char *path, uint8_t **out_data, size_t *out_len);

/* Read file as null-terminated string. Caller frees returned pointer. */
char *ota_read_file_str(const char *path);

/* Check if file exists */
bool ota_file_exists(const char *path);

/* Create directory and parents (like mkdir -p) */
ota_error_t ota_mkdir_p(const char *path);

#endif /* OTA_CORE_PERSISTENCE_H */
