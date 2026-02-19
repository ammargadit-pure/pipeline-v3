#include <gtest/gtest.h>

extern "C" {
#include "ota/core/version.h"
}

TEST(Version, ParseValidVersion)
{
    ota_version_t v;
    ASSERT_EQ(ota_version_parse("2.2.0", &v), OTA_OK);
    EXPECT_EQ(v.major, 2);
    EXPECT_EQ(v.minor, 2);
}

TEST(Version, CompareGreater)
{
    ota_version_t a = {2, 2, 0};
    ota_version_t b = {2, 1, 0};
    EXPECT_GT(ota_version_compare(&a, &b), 0);
    EXPECT_LT(ota_version_compare(&b, &a), 0);
}
