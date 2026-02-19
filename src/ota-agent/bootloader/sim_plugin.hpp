#ifndef OTA_AGENT_BOOTLOADER_SIM_PLUGIN_HPP
#define OTA_AGENT_BOOTLOADER_SIM_PLUGIN_HPP

#include "ota/bootloader/interface.h"

namespace ota::agent {

/* Create sim bootloader plugin. config_json should contain:
 *   { "base_dir": "/path/to/sim/storage" }
 * Caller must call ota_bootloader_destroy() to free. */
ota_bootloader_interface_t *create_sim_plugin(const char *config_json);

/* Destroy sim plugin and free its context. */
void destroy_sim_plugin(ota_bootloader_interface_t *iface);

} // namespace ota::agent

#endif // OTA_AGENT_BOOTLOADER_SIM_PLUGIN_HPP
