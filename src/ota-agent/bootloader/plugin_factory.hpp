#ifndef OTA_AGENT_BOOTLOADER_PLUGIN_FACTORY_HPP
#define OTA_AGENT_BOOTLOADER_PLUGIN_FACTORY_HPP

#include "ota/bootloader/interface.h"

namespace ota::agent {

class PluginFactory {
public:
    /* Create bootloader plugin by name. Returns nullptr for unknown plugins.
     * Supported plugin names: "sim", "linux-generic"
     * config_json is plugin-specific configuration. */
    static ota_bootloader_interface_t *create(const char *plugin_name,
                                               const char *config_json);

    /* Destroy a previously created plugin. Safe to call with nullptr. */
    static void destroy(ota_bootloader_interface_t *iface);
};

} // namespace ota::agent

#endif // OTA_AGENT_BOOTLOADER_PLUGIN_FACTORY_HPP
