#ifndef OTA_CORE_VERSION_H
#define OTA_CORE_VERSION_H

#include "ota/core/types.h"
#include "ota/core/error.h"

/* Parsed semantic version */
typedef struct {
    int major;
    int minor;
    int patch;
} ota_version_t;

/* Parse "MAJOR.MINOR.PATCH" string */
ota_error_t ota_version_parse(const char *str, ota_version_t *out);

/* Compare two versions. Returns: <0 if a<b, 0 if a==b, >0 if a>b */
int ota_version_compare(const ota_version_t *a, const ota_version_t *b);

/* Check if version is within [min, max] range (inclusive) */
bool ota_version_in_range(const ota_version_t *ver,
                          const ota_version_t *min,
                          const ota_version_t *max);

/* Format version to string buffer (must be >= OTA_MAX_VERSION_LEN) */
ota_error_t ota_version_to_str(const ota_version_t *ver, char *buf, size_t buf_len);

#endif /* OTA_CORE_VERSION_H */
