#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "ota/core/state.h"
}

TEST(State, CampaignStateStrRoundtrip)
{
    const ota_campaign_state_t all[] = {
        OTA_CAMPAIGN_IDLE, OTA_CAMPAIGN_DOWNLOADING, OTA_CAMPAIGN_DOWNLOAD_FAILED,
        OTA_CAMPAIGN_VALIDATING, OTA_CAMPAIGN_VALIDATION_FAILED,
        OTA_CAMPAIGN_TRANSFERRING, OTA_CAMPAIGN_TRANSFER_FAILED,
        OTA_CAMPAIGN_AWAITING_SAFE, OTA_CAMPAIGN_INSTALLING, OTA_CAMPAIGN_INSTALL_FAILED,
        OTA_CAMPAIGN_ACTIVATING, OTA_CAMPAIGN_ACTIVATE_FAILED,
        OTA_CAMPAIGN_MONITORING, OTA_CAMPAIGN_MONITORING_FAILED,
        OTA_CAMPAIGN_COMPLETED, OTA_CAMPAIGN_ROLLED_BACK, OTA_CAMPAIGN_ABORTED,
    };

    for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); i++) {
        const char *s = ota_campaign_state_str(all[i]);
        ASSERT_NE(s, nullptr);
        EXPECT_EQ(ota_campaign_state_from_str(s), all[i])
            << "Roundtrip failed for campaign state " << (int)all[i];
    }
}

TEST(State, TargetStateStrRoundtrip)
{
    const ota_target_state_t all[] = {
        OTA_TARGET_PENDING, OTA_TARGET_TRANSFER_IN_PROGRESS, OTA_TARGET_TRANSFERRED,
        OTA_TARGET_INSTALLING, OTA_TARGET_INSTALLED,
        OTA_TARGET_ACTIVATING, OTA_TARGET_ACTIVATED,
        OTA_TARGET_MONITORING, OTA_TARGET_COMPLETED,
        OTA_TARGET_FAILED, OTA_TARGET_ROLLING_BACK, OTA_TARGET_ROLLED_BACK,
    };

    for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); i++) {
        const char *s = ota_target_state_str(all[i]);
        ASSERT_NE(s, nullptr);
        EXPECT_EQ(ota_target_state_from_str(s), all[i])
            << "Roundtrip failed for target state " << (int)all[i];
    }
}

TEST(State, MilestoneStrRoundtrip)
{
    const ota_milestone_t all[] = {
        OTA_MILESTONE_TRANSFER_STARTED, OTA_MILESTONE_TRANSFER_COMPLETED,
        OTA_MILESTONE_INSTALL_STARTED, OTA_MILESTONE_INSTALL_COMPLETED,
        OTA_MILESTONE_ACTIVATE_STARTED, OTA_MILESTONE_ACTIVATE_COMPLETED,
        OTA_MILESTONE_ROLLBACK_STARTED, OTA_MILESTONE_ROLLBACK_COMPLETED,
        OTA_MILESTONE_FAILURE,
    };

    for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); i++) {
        const char *s = ota_milestone_str(all[i]);
        ASSERT_NE(s, nullptr);
        EXPECT_EQ(ota_milestone_from_str(s), all[i])
            << "Roundtrip failed for milestone " << (int)all[i];
    }
}

TEST(State, DownloadStateStrRoundtrip)
{
    const ota_download_state_t all[] = {
        OTA_DOWNLOAD_PENDING, OTA_DOWNLOAD_IN_PROGRESS, OTA_DOWNLOAD_PAUSED,
        OTA_DOWNLOAD_COMPLETED, OTA_DOWNLOAD_FAILED, OTA_DOWNLOAD_SUPERSEDED,
    };

    for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); i++) {
        const char *s = ota_download_state_str(all[i]);
        ASSERT_NE(s, nullptr);
        EXPECT_EQ(ota_download_state_from_str(s), all[i])
            << "Roundtrip failed for download state " << (int)all[i];
    }
}

TEST(State, CampaignValidTransitions)
{
    EXPECT_TRUE(ota_campaign_can_transition(OTA_CAMPAIGN_IDLE, OTA_CAMPAIGN_DOWNLOADING));
    EXPECT_TRUE(ota_campaign_can_transition(OTA_CAMPAIGN_DOWNLOADING, OTA_CAMPAIGN_VALIDATING));
    EXPECT_TRUE(ota_campaign_can_transition(OTA_CAMPAIGN_DOWNLOADING, OTA_CAMPAIGN_DOWNLOAD_FAILED));
    EXPECT_TRUE(ota_campaign_can_transition(OTA_CAMPAIGN_VALIDATING, OTA_CAMPAIGN_TRANSFERRING));
    EXPECT_TRUE(ota_campaign_can_transition(OTA_CAMPAIGN_MONITORING, OTA_CAMPAIGN_COMPLETED));
}

TEST(State, CampaignInvalidTransitions)
{
    EXPECT_FALSE(ota_campaign_can_transition(OTA_CAMPAIGN_COMPLETED, OTA_CAMPAIGN_DOWNLOADING));
    EXPECT_FALSE(ota_campaign_can_transition(OTA_CAMPAIGN_IDLE, OTA_CAMPAIGN_INSTALLING));
    EXPECT_FALSE(ota_campaign_can_transition(OTA_CAMPAIGN_COMPLETED, OTA_CAMPAIGN_IDLE));
    EXPECT_FALSE(ota_campaign_can_transition(OTA_CAMPAIGN_ABORTED, OTA_CAMPAIGN_IDLE));
}

TEST(State, TargetValidTransitions)
{
    EXPECT_TRUE(ota_target_can_transition(OTA_TARGET_PENDING, OTA_TARGET_TRANSFER_IN_PROGRESS));
    EXPECT_TRUE(ota_target_can_transition(OTA_TARGET_INSTALLED, OTA_TARGET_ACTIVATING));
    EXPECT_TRUE(ota_target_can_transition(OTA_TARGET_MONITORING, OTA_TARGET_COMPLETED));
    EXPECT_TRUE(ota_target_can_transition(OTA_TARGET_FAILED, OTA_TARGET_ROLLING_BACK));
}

TEST(State, TargetInvalidTransitions)
{
    EXPECT_FALSE(ota_target_can_transition(OTA_TARGET_COMPLETED, OTA_TARGET_PENDING));
    EXPECT_FALSE(ota_target_can_transition(OTA_TARGET_ROLLED_BACK, OTA_TARGET_PENDING));
    EXPECT_FALSE(ota_target_can_transition(OTA_TARGET_PENDING, OTA_TARGET_INSTALLED));
    EXPECT_FALSE(ota_target_can_transition(OTA_TARGET_COMPLETED, OTA_TARGET_ACTIVATING));
}

TEST(State, FromStrInvalidReturnsDefault)
{
    /* Campaign: unknown string returns IDLE (default) */
    EXPECT_EQ(ota_campaign_state_from_str("nonexistent"), OTA_CAMPAIGN_IDLE);
    EXPECT_EQ(ota_campaign_state_from_str(nullptr), OTA_CAMPAIGN_IDLE);

    /* Target: unknown string returns PENDING (default) */
    EXPECT_EQ(ota_target_state_from_str("nonexistent"), OTA_TARGET_PENDING);
    EXPECT_EQ(ota_target_state_from_str(nullptr), OTA_TARGET_PENDING);

    /* Milestone: unknown string returns FAILURE (default) */
    EXPECT_EQ(ota_milestone_from_str("nonexistent"), OTA_MILESTONE_FAILURE);
    EXPECT_EQ(ota_milestone_from_str(nullptr), OTA_MILESTONE_FAILURE);

    /* Download: unknown string returns PENDING (default) */
    EXPECT_EQ(ota_download_state_from_str("nonexistent"), OTA_DOWNLOAD_PENDING);
    EXPECT_EQ(ota_download_state_from_str(nullptr), OTA_DOWNLOAD_PENDING);
}
