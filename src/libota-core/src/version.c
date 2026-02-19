#include "ota/core/version.h"
#include <stdio.h>
#include <string.h>

ota_error_t ota_version_parse(const char *str, ota_version_t *out)
{
    if (!str || !out) {
        return OTA_ERR_INVALID_ARGUMENT;
    }

    int major = 0, minor = 0, patch = 0;
    char trailing = '\0';
    int matched = sscanf(str, "%d.%d.%d%c", &major, &minor, &patch, &trailing);

    if (matched < 3 || trailing != '\0') {
        return OTA_ERR_INVALID_ARGUMENT;
    }

    if (major < 0 || minor < 0 || patch < 0) {
        return OTA_ERR_INVALID_ARGUMENT;
    }

    out->major = major;
    out->minor = minor;
    out->patch = patch;
    return OTA_OK;
}

int ota_version_compare(const ota_version_t *a, const ota_version_t *b)
{
    if (a->major != b->major) return a->major - b->major;
    if (a->minor != b->minor) return a->minor - b->minor;
    return a->patch - b->patch;
}

bool ota_version_in_range(const ota_version_t *ver,
                          const ota_version_t *min,
                          const ota_version_t *max)
{
    return ota_version_compare(ver, min) >= 0 &&
           ota_version_compare(ver, max) <= 0;
}

ota_error_t ota_version_to_str(const ota_version_t *ver, char *buf, size_t buf_len)
{
    if (!ver || !buf || buf_len == 0) {
        return OTA_ERR_INVALID_ARGUMENT;
    }

    int written = snprintf(buf, buf_len, "%d.%d.%d", ver->major, ver->minor, ver->patch);
    if (written < 0 || (size_t)written >= buf_len) {
        return OTA_ERR_INVALID_ARGUMENT;
    }

    return OTA_OK;
}
