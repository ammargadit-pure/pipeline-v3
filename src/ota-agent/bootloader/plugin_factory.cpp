#include "plugin_factory.hpp"
#include "sim_plugin.hpp"
#include "linux_generic_plugin.hpp"

#include <cstring>

namespace ota::agent {

ota_bootloader_interface_t *PluginFactory::create(const char *plugin_name,
                                                    const char *config_json) {
    if (!plugin_name) {
        return nullptr;
    }

    if (std::strcmp(plugin_name, OTA_BOOTLOADER_PLUGIN_SIM) == 0) {
        return create_sim_plugin(config_json);
    }

    if (std::strcmp(plugin_name, OTA_BOOTLOADER_PLUGIN_LINUX_GENERIC) == 0) {
        return create_linux_generic_plugin(config_json);
    }

    return nullptr;
}

void PluginFactory::destroy(ota_bootloader_interface_t *iface) {
    if (!iface) {
        return;
    }

    /* Free the context based on plugin type */
    if (iface->ctx) {
        if (iface->name && std::strcmp(iface->name, OTA_BOOTLOADER_PLUGIN_SIM) == 0) {
            delete static_cast<struct SimPluginCtx *>(iface->ctx);
        } else if (iface->name && std::strcmp(iface->name, OTA_BOOTLOADER_PLUGIN_LINUX_GENERIC) == 0) {
            delete static_cast<struct LinuxGenericCtx *>(iface->ctx);
        }
    }

    delete iface;
}

} // namespace ota::agent

/* C-linkage factory functions declared in ota/bootloader/interface.h */
extern "C" {

ota_bootloader_interface_t *ota_bootloader_create(const char *plugin_name,
                                                    const char *config_json) {
    return ota::agent::PluginFactory::create(plugin_name, config_json);
}

void ota_bootloader_destroy(ota_bootloader_interface_t *iface) {
    ota::agent::PluginFactory::destroy(iface);
}

} /* extern "C" */
