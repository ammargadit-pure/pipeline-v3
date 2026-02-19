#include <gtest/gtest.h>

extern "C" {
#include "ota/core/state.h"
}

TEST(State, CampaignStateStrRoundtrip)
{
    const char *s = ota_campaign_state_str(OTA_CAMPAIGN_IDLE);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(ota_campaign_state_from_str(s), OTA_CAMPAIGN_IDLE);
}

TEST(State, CampaignValidTransitions)
{
    EXPECT_TRUE(ota_campaign_can_transition(OTA_CAMPAIGN_IDLE, OTA_CAMPAIGN_DOWNLOADING));
    EXPECT_FALSE(ota_campaign_can_transition(OTA_CAMPAIGN_COMPLETED, OTA_CAMPAIGN_DOWNLOADING));
}
