#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "ota/core/manifest.h"
}

TEST(Manifest, ParseValidSwManifest)
{
    const char *json = R"({
        "package_identifier": "nav-service",
        "version": "2.2.0",
        "artifact_type": "software",
        "node_identifier": "hpc-primary",
        "minimum_os_version": "5.10.0",
        "maximum_os_version": "6.0.0",
        "minimum_sdk_version": "1.0.0",
        "maximum_sdk_version": "2.0.0",
        "targeted_region": "EU",
        "size": 5242880,
        "crc32": 12345,
        "sha256": "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789",
        "created_at": "2026-01-15T10:00:00Z"
    })";

    ota_manifest_t m;
    ota_error_t err = ota_manifest_parse(json, strlen(json), &m);
    ASSERT_EQ(err, OTA_OK);
    EXPECT_STREQ(m.package_id, "nav-service");
    EXPECT_STREQ(m.version, "2.2.0");
    EXPECT_EQ(m.artifact_type, OTA_ARTIFACT_SOFTWARE);
    EXPECT_STREQ(m.node_identifier, "hpc-primary");
    EXPECT_STREQ(m.min_os_version, "5.10.0");
    EXPECT_STREQ(m.max_os_version, "6.0.0");
    EXPECT_STREQ(m.targeted_region, "EU");
    EXPECT_EQ(m.size, 5242880u);
    EXPECT_EQ(m.crc32, 12345u);
    ota_manifest_free(&m);
}

TEST(Manifest, ParseValidFwManifest)
{
    const char *json = R"({
        "package_identifier": "sensor-fw",
        "version": "1.4.0",
        "artifact_type": "firmware",
        "node_identifier": "mcu-sensor"
    })";

    ota_manifest_t m;
    ota_error_t err = ota_manifest_parse(json, strlen(json), &m);
    ASSERT_EQ(err, OTA_OK);
    EXPECT_EQ(m.artifact_type, OTA_ARTIFACT_FIRMWARE);
    EXPECT_STREQ(m.package_id, "sensor-fw");
    ota_manifest_free(&m);
}

TEST(Manifest, ParseMissingRequiredField)
{
    const char *json = R"({
        "version": "2.2.0",
        "artifact_type": "software",
        "node_identifier": "hpc-primary"
    })";

    ota_manifest_t m;
    ota_error_t err = ota_manifest_parse(json, strlen(json), &m);
    EXPECT_EQ(err, OTA_ERR_INVALID_ARGUMENT);
    EXPECT_NE(err, OTA_OK);
}

TEST(Manifest, ParseEmptyJson)
{
    const char *json = "{}";
    ota_manifest_t m;
    ota_error_t err = ota_manifest_parse(json, strlen(json), &m);
    EXPECT_EQ(err, OTA_ERR_INVALID_ARGUMENT);
    EXPECT_NE(err, OTA_OK);
}

TEST(Manifest, ParseNullJson)
{
    ota_manifest_t m;
    ota_error_t err = ota_manifest_parse(nullptr, 0, &m);
    EXPECT_EQ(err, OTA_ERR_INVALID_ARGUMENT);
    EXPECT_NE(err, OTA_OK);
}

TEST(Manifest, ParseMalformedJson)
{
    const char *json = "{ this is not valid json ";
    ota_manifest_t m;
    ota_error_t err = ota_manifest_parse(json, strlen(json), &m);
    EXPECT_EQ(err, OTA_ERR_PROTOCOL_PARSE);
    EXPECT_NE(err, OTA_OK);
}

TEST(Manifest, ValidateCompleteManifest)
{
    ota_manifest_t m;
    memset(&m, 0, sizeof(m));
    strncpy(m.package_id, "nav-service", sizeof(m.package_id) - 1);
    strncpy(m.version, "2.2.0", sizeof(m.version) - 1);
    strncpy(m.node_identifier, "hpc-primary", sizeof(m.node_identifier) - 1);

    EXPECT_EQ(ota_manifest_validate(&m), OTA_OK);
    EXPECT_NE(m.package_id[0], '\0');
}

TEST(Manifest, ValidateEmptyPackageId)
{
    ota_manifest_t m;
    memset(&m, 0, sizeof(m));
    strncpy(m.version, "2.2.0", sizeof(m.version) - 1);
    strncpy(m.node_identifier, "hpc-primary", sizeof(m.node_identifier) - 1);

    EXPECT_EQ(ota_manifest_validate(&m), OTA_ERR_INVALID_ARGUMENT);
    EXPECT_NE(ota_manifest_validate(&m), OTA_OK);
}

TEST(Manifest, ValidateEmptyVersion)
{
    ota_manifest_t m;
    memset(&m, 0, sizeof(m));
    strncpy(m.package_id, "nav-service", sizeof(m.package_id) - 1);
    strncpy(m.node_identifier, "hpc-primary", sizeof(m.node_identifier) - 1);

    EXPECT_EQ(ota_manifest_validate(&m), OTA_ERR_INVALID_ARGUMENT);
    EXPECT_NE(ota_manifest_validate(&m), OTA_OK);
}

TEST(Manifest, SerializeRoundtrip)
{
    const char *json = R"({
        "package_identifier": "nav-service",
        "version": "2.2.0",
        "artifact_type": "software",
        "node_identifier": "hpc-primary",
        "minimum_os_version": "5.10.0",
        "maximum_os_version": "6.0.0",
        "targeted_region": "EU",
        "size": 5242880,
        "crc32": 12345
    })";

    ota_manifest_t m1;
    ASSERT_EQ(ota_manifest_parse(json, strlen(json), &m1), OTA_OK);

    char *serialized = ota_manifest_serialize(&m1);
    ASSERT_NE(serialized, nullptr);

    ota_manifest_t m2;
    ASSERT_EQ(ota_manifest_parse(serialized, strlen(serialized), &m2), OTA_OK);

    EXPECT_STREQ(m1.package_id, m2.package_id);
    EXPECT_STREQ(m1.version, m2.version);
    EXPECT_EQ(m1.artifact_type, m2.artifact_type);
    EXPECT_STREQ(m1.node_identifier, m2.node_identifier);
    EXPECT_STREQ(m1.targeted_region, m2.targeted_region);

    free(serialized);
    ota_manifest_free(&m1);
    ota_manifest_free(&m2);
}

TEST(Manifest, FreeDoubleFreeIsSafe)
{
    const char *json = R"({
        "package_identifier": "nav-service",
        "version": "2.2.0",
        "artifact_type": "software",
        "node_identifier": "hpc-primary",
        "custom_oem_rules": {"key": "value"}
    })";

    ota_manifest_t m;
    ASSERT_EQ(ota_manifest_parse(json, strlen(json), &m), OTA_OK);
    EXPECT_NE(m.custom_oem_rules_json, nullptr);

    ota_manifest_free(&m);
    EXPECT_EQ(m.custom_oem_rules_json, nullptr);

    /* Second free should not crash */
    ota_manifest_free(&m);
    EXPECT_EQ(m.custom_oem_rules_json, nullptr);
}
