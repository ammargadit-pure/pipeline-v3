#ifndef OTA_AGENT_BOOTLOADER_LINUX_GENERIC_PLUGIN_HPP
#define OTA_AGENT_BOOTLOADER_LINUX_GENERIC_PLUGIN_HPP

#include "ota/bootloader/interface.h"

namespace ota::agent {

/* Create linux-generic bootloader plugin. config_json should contain:
 *   { "base_dir": "/path/to/metadata/storage" }
 * Uses JSON metadata files to simulate partition environment variables.
 * Caller must call ota_bootloader_destroy() to free. */
ota_bootloader_interface_t *create_linux_generic_plugin(const char *config_json);

/* Destroy linux-generic plugin and free its context. */
void destroy_linux_generic_plugin(ota_bootloader_interface_t *iface);

} // namespace ota::agent

#endif // OTA_AGENT_BOOTLOADER_LINUX_GENERIC_PLUGIN_HPP
