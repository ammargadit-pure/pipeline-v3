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

static const char *TEST_SIM_BASE_DIR = "/tmp/ota-test/bootloader-sim-test";

class SimPluginTest : public ::testing::Test {
protected:
    ota_bootloader_interface_t *iface = nullptr;

    void SetUp() override {
        /* Clean up and create test directory (with parents) */
        std::string rm_cmd = std::string("rm -rf ") + TEST_SIM_BASE_DIR;
        system(rm_cmd.c_str());
        std::string mk_cmd = std::string("mkdir -p ") + TEST_SIM_BASE_DIR;
        system(mk_cmd.c_str());

        std::string config = std::string(R"({"base_dir": ")") + TEST_SIM_BASE_DIR + R"("})";
        iface = ota_bootloader_create(OTA_BOOTLOADER_PLUGIN_SIM, config.c_str());
    }

    void TearDown() override {
        ota_bootloader_destroy(iface);
        iface = nullptr;
        std::string rm_cmd = std::string("rm -rf ") + TEST_SIM_BASE_DIR;
        system(rm_cmd.c_str());
    }
};

TEST_F(SimPluginTest, SimDetectInactiveSlotB) {
    /* Slot A is active by default, so inactive should be slot B */
    ASSERT_NE(iface, nullptr);
    ota_slot_info_t slot;
    ota_error_t err = iface->detect_inactive_slot(iface->ctx, &slot);
    EXPECT_EQ(err, OTA_OK);
    EXPECT_EQ(slot.slot_index, 1);
    EXPECT_STREQ(slot.label, "slot_b");
    EXPECT_FALSE(slot.is_active);
}

TEST_F(SimPluginTest, SimDetectInactiveSlotA) {
    /* After prepare_activation, slots swap. Now slot B is active, so inactive = slot A */
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

TEST_F(SimPluginTest, SimStageFirmware) {
    ASSERT_NE(iface, nullptr);
    const uint8_t firmware_data[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};

    /* Stage firmware to inactive slot */
    ota_error_t err = iface->stage_firmware(iface->ctx, firmware_data, sizeof(firmware_data), 0);
    EXPECT_EQ(err, OTA_OK);

    /* Prepare activation to confirm staging works */
    err = iface->prepare_activation(iface->ctx);
    EXPECT_EQ(err, OTA_OK);

    /* After activation, pending_activation should be true */
    ota_boot_status_t status;
    err = iface->query_boot_status(iface->ctx, &status);
    EXPECT_EQ(err, OTA_OK);
    EXPECT_TRUE(status.pending_activation);
}

TEST_F(SimPluginTest, SimStageFirmwareWritesData) {
    ASSERT_NE(iface, nullptr);
    const uint8_t data1[] = {0x01, 0x02, 0x03, 0x04};
    const uint8_t data2[] = {0x05, 0x06, 0x07, 0x08};

    /* Write first chunk at offset 0 */
    ota_error_t err = iface->stage_firmware(iface->ctx, data1, sizeof(data1), 0);
    EXPECT_EQ(err, OTA_OK);

    /* Write second chunk at offset 4 */
    err = iface->stage_firmware(iface->ctx, data2, sizeof(data2), 4);
    EXPECT_EQ(err, OTA_OK);

    /* Verify the file contents */
    std::string slot_b_path = std::string(TEST_SIM_BASE_DIR) + "/slot_b.bin";
    FILE *fp = fopen(slot_b_path.c_str(), "rb");
    ASSERT_NE(fp, nullptr);
    uint8_t buf[8];
    size_t nread = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    EXPECT_EQ(nread, 8u);
    EXPECT_EQ(buf[0], 0x01);
    EXPECT_EQ(buf[4], 0x05);
}

TEST_F(SimPluginTest, SimMarkActiveValid) {
    ASSERT_NE(iface, nullptr);

    /* Activate to set pending */
    ota_error_t err = iface->prepare_activation(iface->ctx);
    ASSERT_EQ(err, OTA_OK);

    ota_boot_status_t status;
    err = iface->query_boot_status(iface->ctx, &status);
    ASSERT_EQ(err, OTA_OK);
    EXPECT_TRUE(status.pending_activation);

    /* Mark active valid — clears pending */
    err = iface->mark_active_valid(iface->ctx);
    EXPECT_EQ(err, OTA_OK);

    err = iface->query_boot_status(iface->ctx, &status);
    EXPECT_EQ(err, OTA_OK);
    EXPECT_FALSE(status.pending_activation);
}

TEST_F(SimPluginTest, SimMarkSlotInvalid) {
    ASSERT_NE(iface, nullptr);

    /* Mark slot B invalid */
    ota_error_t err = iface->mark_slot_invalid(iface->ctx, 1);
    EXPECT_EQ(err, OTA_OK);

    /* Verify slot B is invalid */
    ota_boot_status_t status;
    err = iface->query_boot_status(iface->ctx, &status);
    EXPECT_EQ(err, OTA_OK);
    EXPECT_FALSE(status.inactive_slot.is_valid);
    EXPECT_TRUE(status.active_slot.is_valid);
}

TEST_F(SimPluginTest, SimMarkSlotAInvalid) {
    ASSERT_NE(iface, nullptr);

    /* Mark slot A (active) invalid */
    ota_error_t err = iface->mark_slot_invalid(iface->ctx, 0);
    EXPECT_EQ(err, OTA_OK);

    /* Verify slot A is invalid */
    ota_boot_status_t status;
    err = iface->query_boot_status(iface->ctx, &status);
    EXPECT_EQ(err, OTA_OK);
    EXPECT_FALSE(status.active_slot.is_valid);
}

TEST_F(SimPluginTest, SimMarkInvalidSlotIndex) {
    ASSERT_NE(iface, nullptr);

    /* Invalid slot index should return error */
    ota_error_t err = iface->mark_slot_invalid(iface->ctx, 2);
    EXPECT_EQ(err, OTA_ERR_BOOTLOADER_SLOT);

    err = iface->mark_slot_invalid(iface->ctx, 255);
    EXPECT_EQ(err, OTA_ERR_BOOTLOADER_SLOT);
}

TEST_F(SimPluginTest, SimQueryBootStatus) {
    ASSERT_NE(iface, nullptr);

    ota_boot_status_t status;
    ota_error_t err = iface->query_boot_status(iface->ctx, &status);
    EXPECT_EQ(err, OTA_OK);

    /* Default: slot A active, slot B inactive */
    EXPECT_EQ(status.active_slot.slot_index, 0);
    EXPECT_STREQ(status.active_slot.label, "slot_a");
    EXPECT_TRUE(status.active_slot.is_active);
    EXPECT_TRUE(status.active_slot.is_valid);

    EXPECT_EQ(status.inactive_slot.slot_index, 1);
    EXPECT_STREQ(status.inactive_slot.label, "slot_b");
    EXPECT_FALSE(status.inactive_slot.is_active);
    EXPECT_TRUE(status.inactive_slot.is_valid);

    EXPECT_EQ(status.boot_count, 0u);
    EXPECT_FALSE(status.pending_activation);
}

TEST_F(SimPluginTest, SimBootCountTracking) {
    ASSERT_NE(iface, nullptr);

    ota_boot_status_t status;
    ota_error_t err = iface->query_boot_status(iface->ctx, &status);
    ASSERT_EQ(err, OTA_OK);
    EXPECT_EQ(status.boot_count, 0u);

    /* Each prepare_activation increments boot_count */
    err = iface->prepare_activation(iface->ctx);
    ASSERT_EQ(err, OTA_OK);
    err = iface->query_boot_status(iface->ctx, &status);
    ASSERT_EQ(err, OTA_OK);
    EXPECT_EQ(status.boot_count, 1u);

    /* Second activation */
    err = iface->prepare_activation(iface->ctx);
    ASSERT_EQ(err, OTA_OK);
    err = iface->query_boot_status(iface->ctx, &status);
    EXPECT_EQ(err, OTA_OK);
    EXPECT_EQ(status.boot_count, 2u);
}

TEST_F(SimPluginTest, SimNullArgumentsHandled) {
    ASSERT_NE(iface, nullptr);

    /* NULL ctx */
    EXPECT_EQ(iface->detect_inactive_slot(nullptr, nullptr), OTA_ERR_INVALID_ARGUMENT);
    EXPECT_EQ(iface->stage_firmware(nullptr, nullptr, 0, 0), OTA_ERR_INVALID_ARGUMENT);
    EXPECT_EQ(iface->prepare_activation(nullptr), OTA_ERR_INVALID_ARGUMENT);
    EXPECT_EQ(iface->query_boot_status(nullptr, nullptr), OTA_ERR_INVALID_ARGUMENT);

    /* NULL output pointers */
    EXPECT_EQ(iface->detect_inactive_slot(iface->ctx, nullptr), OTA_ERR_INVALID_ARGUMENT);
    EXPECT_EQ(iface->query_boot_status(iface->ctx, nullptr), OTA_ERR_INVALID_ARGUMENT);
}
