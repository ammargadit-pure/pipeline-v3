#include "ota/core/persistence.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>

ota_error_t ota_atomic_write(const char *path, const uint8_t *data, size_t len)
{
    if (!path || (!data && len > 0)) {
        return OTA_ERR_INVALID_ARGUMENT;
    }

    /* Create temp file path */
    char tmp_path[OTA_MAX_PATH_LEN + 8];
    int n = snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
    if (n < 0 || (size_t)n >= sizeof(tmp_path)) {
        return OTA_ERR_INVALID_ARGUMENT;
    }

    FILE *f = fopen(tmp_path, "wb");
    if (!f) return OTA_ERR_IO;

    if (len > 0) {
        size_t written = fwrite(data, 1, len, f);
        if (written != len) {
            fclose(f);
            unlink(tmp_path);
            return OTA_ERR_IO;
        }
    }

    /* fsync to ensure data is on disk */
    if (fflush(f) != 0 || fsync(fileno(f)) != 0) {
        fclose(f);
        unlink(tmp_path);
        return OTA_ERR_IO;
    }

    fclose(f);

    /* Atomic rename */
    if (rename(tmp_path, path) != 0) {
        unlink(tmp_path);
        return OTA_ERR_IO;
    }

    return OTA_OK;
}

ota_error_t ota_atomic_write_json(const char *path, const char *json_str)
{
    if (!path || !json_str) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    return ota_atomic_write(path, (const uint8_t *)json_str, strlen(json_str));
}

ota_error_t ota_read_file(const char *path, uint8_t **out_data, size_t *out_len)
{
    if (!path || !out_data || !out_len) {
        return OTA_ERR_INVALID_ARGUMENT;
    }

    *out_data = NULL;
    *out_len = 0;

    FILE *f = fopen(path, "rb");
    if (!f) return OTA_ERR_NOT_FOUND;

    /* Get file size */
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return OTA_ERR_IO;
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return OTA_ERR_IO;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return OTA_ERR_IO;
    }

    uint8_t *buf = (uint8_t *)malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        return OTA_ERR_OUT_OF_MEMORY;
    }

    if (size > 0) {
        size_t read_bytes = fread(buf, 1, (size_t)size, f);
        if (read_bytes != (size_t)size) {
            free(buf);
            fclose(f);
            return OTA_ERR_IO;
        }
    }
    buf[size] = '\0';

    fclose(f);

    *out_data = buf;
    *out_len = (size_t)size;
    return OTA_OK;
}

char *ota_read_file_str(const char *path)
{
    uint8_t *data = NULL;
    size_t len = 0;
    if (ota_read_file(path, &data, &len) != OTA_OK) {
        return NULL;
    }
    return (char *)data;
}

bool ota_file_exists(const char *path)
{
    if (!path) return false;
    struct stat st;
    return stat(path, &st) == 0;
}

ota_error_t ota_mkdir_p(const char *path)
{
    if (!path || path[0] == '\0') {
        return OTA_ERR_INVALID_ARGUMENT;
    }

    char tmp[OTA_MAX_PATH_LEN];
    size_t len = strlen(path);
    if (len >= sizeof(tmp)) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    memcpy(tmp, path, len + 1);

    /* Remove trailing slash */
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                return OTA_ERR_IO;
            }
            *p = '/';
        }
    }

    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        return OTA_ERR_IO;
    }

    return OTA_OK;
}
