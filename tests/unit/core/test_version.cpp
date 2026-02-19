#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "ota/core/version.h"
}

TEST(Version, ParseValidVersion)
{
    ota_version_t v;
    ASSERT_EQ(ota_version_parse("2.2.0", &v), OTA_OK);
    EXPECT_EQ(v.major, 2);
    EXPECT_EQ(v.minor, 2);
    EXPECT_EQ(v.patch, 0);
}

TEST(Version, ParseMajorOnly)
{
    ota_version_t v;
    ota_error_t err = ota_version_parse("5", &v);
    EXPECT_EQ(err, OTA_ERR_INVALID_ARGUMENT);
    EXPECT_NE(err, OTA_OK);
}

TEST(Version, ParseInvalid)
{
    ota_version_t v;
    ota_error_t err = ota_version_parse("abc", &v);
    EXPECT_EQ(err, OTA_ERR_INVALID_ARGUMENT);
    EXPECT_NE(err, OTA_OK);
}

TEST(Version, ParseEmpty)
{
    ota_version_t v;
    ota_error_t err = ota_version_parse("", &v);
    EXPECT_EQ(err, OTA_ERR_INVALID_ARGUMENT);
    EXPECT_NE(err, OTA_OK);
}

TEST(Version, CompareGreater)
{
    ota_version_t a = {2, 2, 0};
    ota_version_t b = {2, 1, 0};
    EXPECT_GT(ota_version_compare(&a, &b), 0);
    EXPECT_LT(ota_version_compare(&b, &a), 0);
}

TEST(Version, CompareEqual)
{
    ota_version_t a = {1, 0, 0};
    ota_version_t b = {1, 0, 0};
    EXPECT_EQ(ota_version_compare(&a, &b), 0);
    EXPECT_EQ(ota_version_compare(&b, &a), 0);
}

TEST(Version, CompareLess)
{
    ota_version_t a = {1, 0, 0};
    ota_version_t b = {2, 0, 0};
    EXPECT_LT(ota_version_compare(&a, &b), 0);
    EXPECT_GT(ota_version_compare(&b, &a), 0);
}

TEST(Version, ComparePatchLevel)
{
    ota_version_t a = {1, 0, 1};
    ota_version_t b = {1, 0, 0};
    EXPECT_GT(ota_version_compare(&a, &b), 0);
    EXPECT_LT(ota_version_compare(&b, &a), 0);
}

TEST(Version, InRangeTrue)
{
    ota_version_t ver = {5, 15, 0};
    ota_version_t min = {5, 10, 0};
    ota_version_t max = {6, 99, 0};
    EXPECT_TRUE(ota_version_in_range(&ver, &min, &max));
    EXPECT_GT(ota_version_compare(&ver, &min), 0);
}

TEST(Version, InRangeFalse)
{
    ota_version_t ver = {4, 0, 0};
    ota_version_t min = {5, 10, 0};
    ota_version_t max = {6, 99, 0};
    EXPECT_FALSE(ota_version_in_range(&ver, &min, &max));
    EXPECT_LT(ota_version_compare(&ver, &min), 0);
}

TEST(Version, InRangeBoundaryMin)
{
    ota_version_t ver = {5, 10, 0};
    ota_version_t min = {5, 10, 0};
    ota_version_t max = {6, 99, 0};
    EXPECT_TRUE(ota_version_in_range(&ver, &min, &max));
    EXPECT_EQ(ota_version_compare(&ver, &min), 0);
}

TEST(Version, InRangeBoundaryMax)
{
    ota_version_t ver = {6, 99, 0};
    ota_version_t min = {5, 10, 0};
    ota_version_t max = {6, 99, 0};
    EXPECT_TRUE(ota_version_in_range(&ver, &min, &max));
    EXPECT_EQ(ota_version_compare(&ver, &max), 0);
}

TEST(Version, ToStrRoundtrip)
{
    ota_version_t v1;
    ASSERT_EQ(ota_version_parse("3.14.159", &v1), OTA_OK);

    char buf[OTA_MAX_VERSION_LEN];
    ASSERT_EQ(ota_version_to_str(&v1, buf, sizeof(buf)), OTA_OK);

    ota_version_t v2;
    ASSERT_EQ(ota_version_parse(buf, &v2), OTA_OK);

    EXPECT_EQ(v1.major, v2.major);
    EXPECT_EQ(v1.minor, v2.minor);
    EXPECT_EQ(v1.patch, v2.patch);
}
