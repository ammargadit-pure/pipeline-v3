#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include <sys/stat.h>

#include "ota/bootloader/interface.h"
#include "ota/core/error.h"

static const char *TEST_FACTORY_BASE_DIR = "/tmp/ota-test/bootloader-factory-test";

class PluginFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::string rm_cmd = std::string("rm -rf ") + TEST_FACTORY_BASE_DIR;
        system(rm_cmd.c_str());
        mkdir(TEST_FACTORY_BASE_DIR, 0755);
    }

    void TearDown() override {
        std::string rm_cmd = std::string("rm -rf ") + TEST_FACTORY_BASE_DIR;
        system(rm_cmd.c_str());
    }
};

TEST_F(PluginFactoryTest, CreateSimPlugin) {
    std::string config = std::string(R"({"base_dir": ")") + TEST_FACTORY_BASE_DIR + "/sim\"}";
    ota_bootloader_interface_t *iface = ota_bootloader_create(OTA_BOOTLOADER_PLUGIN_SIM,
                                                                config.c_str());
    ASSERT_NE(iface, nullptr);
    EXPECT_STREQ(iface->name, OTA_BOOTLOADER_PLUGIN_SIM);

    /* Verify the plugin is functional */
    ota_boot_status_t status;
    ota_error_t err = iface->query_boot_status(iface->ctx, &status);
    EXPECT_EQ(err, OTA_OK);
    EXPECT_EQ(status.active_slot.slot_index, 0);

    ota_bootloader_destroy(iface);
}

TEST_F(PluginFactoryTest, CreateLinuxGenericPlugin) {
    std::string config = std::string(R"({"base_dir": ")") + TEST_FACTORY_BASE_DIR + "/lg\"}";
    ota_bootloader_interface_t *iface = ota_bootloader_create(OTA_BOOTLOADER_PLUGIN_LINUX_GENERIC,
                                                                config.c_str());
    ASSERT_NE(iface, nullptr);
    EXPECT_STREQ(iface->name, OTA_BOOTLOADER_PLUGIN_LINUX_GENERIC);

    /* Verify the plugin is functional */
    ota_boot_status_t status;
    ota_error_t err = iface->query_boot_status(iface->ctx, &status);
    EXPECT_EQ(err, OTA_OK);
    EXPECT_EQ(status.active_slot.slot_index, 0);

    ota_bootloader_destroy(iface);
}

TEST_F(PluginFactoryTest, CreateUnknownPlugin) {
    ota_bootloader_interface_t *iface = ota_bootloader_create("unknown", "{}");
    EXPECT_EQ(iface, nullptr);

    /* Also test with a known-ish but unsupported name */
    iface = ota_bootloader_create(OTA_BOOTLOADER_PLUGIN_MCUBOOT, "{}");
    EXPECT_EQ(iface, nullptr);
}

TEST_F(PluginFactoryTest, CreateNullPluginName) {
    ota_bootloader_interface_t *iface = ota_bootloader_create(nullptr, "{}");
    EXPECT_EQ(iface, nullptr);

    /* Empty string also not a valid plugin name */
    iface = ota_bootloader_create("", "{}");
    EXPECT_EQ(iface, nullptr);
}

TEST_F(PluginFactoryTest, DestroyNullIsSafe) {
    /* Should not crash */
    ota_bootloader_destroy(nullptr);

    /* Verify we can still create after destroying null */
    std::string config = std::string(R"({"base_dir": ")") + TEST_FACTORY_BASE_DIR + "/safe\"}";
    ota_bootloader_interface_t *iface = ota_bootloader_create(OTA_BOOTLOADER_PLUGIN_SIM,
                                                                config.c_str());
    ASSERT_NE(iface, nullptr);
    EXPECT_STREQ(iface->name, OTA_BOOTLOADER_PLUGIN_SIM);
    ota_bootloader_destroy(iface);
}

TEST_F(PluginFactoryTest, CreateWithNullConfig) {
    /* NULL config should use defaults */
    ota_bootloader_interface_t *iface = ota_bootloader_create(OTA_BOOTLOADER_PLUGIN_SIM, nullptr);
    ASSERT_NE(iface, nullptr);
    EXPECT_STREQ(iface->name, OTA_BOOTLOADER_PLUGIN_SIM);
    ota_bootloader_destroy(iface);
}

TEST_F(PluginFactoryTest, CreateWithEmptyConfig) {
    /* Empty config should use defaults */
    ota_bootloader_interface_t *iface = ota_bootloader_create(OTA_BOOTLOADER_PLUGIN_SIM, "");
    ASSERT_NE(iface, nullptr);
    EXPECT_STREQ(iface->name, OTA_BOOTLOADER_PLUGIN_SIM);
    ota_bootloader_destroy(iface);
}

TEST_F(PluginFactoryTest, AllPluginFunctionPointersSet) {
    std::string config = std::string(R"({"base_dir": ")") + TEST_FACTORY_BASE_DIR + "/ptrs\"}";

    /* Sim plugin */
    ota_bootloader_interface_t *sim = ota_bootloader_create(OTA_BOOTLOADER_PLUGIN_SIM,
                                                              config.c_str());
    ASSERT_NE(sim, nullptr);
    EXPECT_NE(sim->detect_inactive_slot, nullptr);
    EXPECT_NE(sim->stage_firmware, nullptr);
    EXPECT_NE(sim->prepare_activation, nullptr);
    EXPECT_NE(sim->query_boot_status, nullptr);
    EXPECT_NE(sim->mark_active_valid, nullptr);
    EXPECT_NE(sim->mark_slot_invalid, nullptr);
    EXPECT_NE(sim->ctx, nullptr);
    ota_bootloader_destroy(sim);

    /* Linux-generic plugin */
    std::string lg_config = std::string(R"({"base_dir": ")") + TEST_FACTORY_BASE_DIR + "/ptrs-lg\"}";
    ota_bootloader_interface_t *lg = ota_bootloader_create(OTA_BOOTLOADER_PLUGIN_LINUX_GENERIC,
                                                             lg_config.c_str());
    ASSERT_NE(lg, nullptr);
    EXPECT_NE(lg->detect_inactive_slot, nullptr);
    EXPECT_NE(lg->stage_firmware, nullptr);
    EXPECT_NE(lg->prepare_activation, nullptr);
    EXPECT_NE(lg->query_boot_status, nullptr);
    EXPECT_NE(lg->mark_active_valid, nullptr);
    EXPECT_NE(lg->mark_slot_invalid, nullptr);
    EXPECT_NE(lg->ctx, nullptr);
    ota_bootloader_destroy(lg);
}
