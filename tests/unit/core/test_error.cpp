#include <gtest/gtest.h>

extern "C" {
#include "ota/core/error.h"
}

TEST(Error, ErrorStrReturnsStringForAllCodes)
{
    const char *s = ota_error_str(OTA_OK);
    ASSERT_NE(s, nullptr);
    EXPECT_STREQ(s, "OTA_OK");
}

TEST(Error, ErrorStrReturnsUnknownForInvalid)
{
    const char *s = ota_error_str((ota_error_t)9999);
    ASSERT_NE(s, nullptr);
    EXPECT_STREQ(s, "unknown");
}
