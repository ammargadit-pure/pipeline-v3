#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "ota/core/manifest.h"
}

TEST(Manifest, ParseValidSwManifest)
{
    const char *json = R"({
        "package_identifier": "nav-service",
        "version": "2.2.0",
        "artifact_type": "software",
        "node_identifier": "hpc-primary"
    })";

    ota_manifest_t m;
    ota_error_t err = ota_manifest_parse(json, strlen(json), &m);
    ASSERT_EQ(err, OTA_OK);
    EXPECT_STREQ(m.package_id, "nav-service");
    ota_manifest_free(&m);
}

TEST(Manifest, ParseNullJson)
{
    ota_manifest_t m;
    ota_error_t err = ota_manifest_parse(nullptr, 0, &m);
    EXPECT_EQ(err, OTA_ERR_INVALID_ARGUMENT);
    EXPECT_NE(err, OTA_OK);
}
