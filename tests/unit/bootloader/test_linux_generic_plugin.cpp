#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

#include "ota/bootloader/interface.h"
#include "ota/core/error.h"
#include "ota/core/types.h"

static const char *TEST_LG_BASE_DIR = "/tmp/ota-test/bootloader-lg-test";

class LinuxGenericPluginTest : public ::testing::Test {
protected:
    ota_bootloader_interface_t *iface = nullptr;

    void SetUp() override {
        std::string rm_cmd = std::string("rm -rf ") + TEST_LG_BASE_DIR;
        system(rm_cmd.c_str());
        mkdir(TEST_LG_BASE_DIR, 0755);

        std::string config = std::string(R"({"base_dir": ")") + TEST_LG_BASE_DIR + R"("})";
        iface = ota_bootloader_create(OTA_BOOTLOADER_PLUGIN_LINUX_GENERIC, config.c_str());
    }

    void TearDown() override {
        ota_bootloader_destroy(iface);
        iface = nullptr;
        std::string rm_cmd = std::string("rm -rf ") + TEST_LG_BASE_DIR;
        system(rm_cmd.c_str());
    }
};

TEST_F(LinuxGenericPluginTest, DetectInactiveSlotB) {
    ASSERT_NE(iface, nullptr);
    ota_slot_info_t slot;
    ota_error_t err = iface->detect_inactive_slot(iface->ctx, &slot);
    EXPECT_EQ(err, OTA_OK);
    EXPECT_EQ(slot.slot_index, 1);
    EXPECT_STREQ(slot.label, "slot_b");
    EXPECT_FALSE(slot.is_active);
}

TEST_F(LinuxGenericPluginTest, DetectInactiveSlotAAfterActivation) {
    ASSERT_NE(iface, nullptr);
    ota_error_t err = iface->prepare_activation(iface->ctx);
    ASSERT_EQ(err, OTA_OK);

    ota_slot_info_t slot;
    err = iface->detect_inactive_slot(iface->ctx, &slot);
    EXPECT_EQ(err, OTA_OK);
    EXPECT_EQ(slot.slot_index, 0);
    EXPECT_STREQ(slot.label, "slot_a");
    EXPECT_FALSE(slot.is_active);
}

TEST_F(LinuxGenericPluginTest, StageFirmwareWritesFile) {
    ASSERT_NE(iface, nullptr);
    const uint8_t data[] = {0xAA, 0xBB, 0xCC, 0xDD};

    ota_error_t err = iface->stage_firmware(iface->ctx, data, sizeof(data), 0);
    EXPECT_EQ(err, OTA_OK);

    /* Verify slot_b.bin was written (slot B is inactive by default) */
    std::string slot_b = std::string(TEST_LG_BASE_DIR) + "/slot_b.bin";
    FILE *fp = fopen(slot_b.c_str(), "rb");
    ASSERT_NE(fp, nullptr);
    uint8_t buf[4];
    size_t n = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    EXPECT_EQ(n, 4u);
    EXPECT_EQ(buf[0], 0xAA);
    EXPECT_EQ(buf[3], 0xDD);
}

TEST_F(LinuxGenericPluginTest, StageFirmwareAtOffset) {
    ASSERT_NE(iface, nullptr);
    const uint8_t data1[] = {0x01, 0x02};
    const uint8_t data2[] = {0x03, 0x04};

    ota_error_t err = iface->stage_firmware(iface->ctx, data1, sizeof(data1), 0);
    ASSERT_EQ(err, OTA_OK);

    err = iface->stage_firmware(iface->ctx, data2, sizeof(data2), 2);
    EXPECT_EQ(err, OTA_OK);

    /* Verify combined data */
    std::string slot_b = std::string(TEST_LG_BASE_DIR) + "/slot_b.bin";
    FILE *fp = fopen(slot_b.c_str(), "rb");
    ASSERT_NE(fp, nullptr);
    uint8_t buf[4];
    size_t n = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    EXPECT_EQ(n, 4u);
    EXPECT_EQ(buf[0], 0x01);
    EXPECT_EQ(buf[2], 0x03);
}

TEST_F(LinuxGenericPluginTest, PrepareActivationSwapsSlots) {
    ASSERT_NE(iface, nullptr);

    ota_boot_status_t before;
    ota_error_t err = iface->query_boot_status(iface->ctx, &before);
    ASSERT_EQ(err, OTA_OK);
    EXPECT_EQ(before.active_slot.slot_index, 0);

    err = iface->prepare_activation(iface->ctx);
    ASSERT_EQ(err, OTA_OK);

    ota_boot_status_t after;
    err = iface->query_boot_status(iface->ctx, &after);
    EXPECT_EQ(err, OTA_OK);
    EXPECT_EQ(after.active_slot.slot_index, 1);
    EXPECT_TRUE(after.pending_activation);
}

TEST_F(LinuxGenericPluginTest, MarkActiveValidClearsPending) {
    ASSERT_NE(iface, nullptr);

    /* Activate first to set pending */
    ota_error_t err = iface->prepare_activation(iface->ctx);
    ASSERT_EQ(err, OTA_OK);

    ota_boot_status_t status;
    err = iface->query_boot_status(iface->ctx, &status);
    ASSERT_EQ(err, OTA_OK);
    EXPECT_TRUE(status.pending_activation);

    /* Mark active valid */
    err = iface->mark_active_valid(iface->ctx);
    EXPECT_EQ(err, OTA_OK);

    err = iface->query_boot_status(iface->ctx, &status);
    EXPECT_EQ(err, OTA_OK);
    EXPECT_FALSE(status.pending_activation);
}

TEST_F(LinuxGenericPluginTest, MarkSlotInvalid) {
    ASSERT_NE(iface, nullptr);

    ota_error_t err = iface->mark_slot_invalid(iface->ctx, 1);
    EXPECT_EQ(err, OTA_OK);

    ota_boot_status_t status;
    err = iface->query_boot_status(iface->ctx, &status);
    EXPECT_EQ(err, OTA_OK);
    EXPECT_FALSE(status.inactive_slot.is_valid);
    EXPECT_TRUE(status.active_slot.is_valid);
}

TEST_F(LinuxGenericPluginTest, MarkInvalidSlotIndex) {
    ASSERT_NE(iface, nullptr);

    ota_error_t err = iface->mark_slot_invalid(iface->ctx, 2);
    EXPECT_EQ(err, OTA_ERR_BOOTLOADER_SLOT);

    err = iface->mark_slot_invalid(iface->ctx, 255);
    EXPECT_EQ(err, OTA_ERR_BOOTLOADER_SLOT);
}

TEST_F(LinuxGenericPluginTest, BootCountIncrements) {
    ASSERT_NE(iface, nullptr);

    ota_boot_status_t status;
    ota_error_t err = iface->query_boot_status(iface->ctx, &status);
    ASSERT_EQ(err, OTA_OK);
    EXPECT_EQ(status.boot_count, 0u);

    err = iface->prepare_activation(iface->ctx);
    ASSERT_EQ(err, OTA_OK);

    err = iface->query_boot_status(iface->ctx, &status);
    EXPECT_EQ(err, OTA_OK);
    EXPECT_EQ(status.boot_count, 1u);
}

TEST_F(LinuxGenericPluginTest, MetadataPersistsAcrossReads) {
    ASSERT_NE(iface, nullptr);

    /* Stage firmware and activate */
    const uint8_t data[] = {0x11, 0x22};
    ota_error_t err = iface->stage_firmware(iface->ctx, data, sizeof(data), 0);
    ASSERT_EQ(err, OTA_OK);

    err = iface->prepare_activation(iface->ctx);
    ASSERT_EQ(err, OTA_OK);

    /* Read boot status — metadata should have been written to JSON file */
    ota_boot_status_t status;
    err = iface->query_boot_status(iface->ctx, &status);
    EXPECT_EQ(err, OTA_OK);
    EXPECT_EQ(status.active_slot.slot_index, 1);
    EXPECT_EQ(status.boot_count, 1u);
    EXPECT_TRUE(status.pending_activation);
}

TEST_F(LinuxGenericPluginTest, QueryBootStatusDefaults) {
    ASSERT_NE(iface, nullptr);

    ota_boot_status_t status;
    ota_error_t err = iface->query_boot_status(iface->ctx, &status);
    EXPECT_EQ(err, OTA_OK);

    EXPECT_EQ(status.active_slot.slot_index, 0);
    EXPECT_STREQ(status.active_slot.label, "slot_a");
    EXPECT_TRUE(status.active_slot.is_active);
    EXPECT_TRUE(status.active_slot.is_valid);

    EXPECT_EQ(status.inactive_slot.slot_index, 1);
    EXPECT_STREQ(status.inactive_slot.label, "slot_b");
    EXPECT_FALSE(status.inactive_slot.is_active);
    EXPECT_TRUE(status.inactive_slot.is_valid);

    EXPECT_EQ(status.boot_count, 0u);
    EXPECT_EQ(status.boot_limit, 3u);
    EXPECT_FALSE(status.pending_activation);
}
