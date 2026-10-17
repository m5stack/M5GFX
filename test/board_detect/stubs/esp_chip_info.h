#pragma once
namespace host_chip {
static int revision;
}
struct esp_chip_info_t {
  int revision;
};
inline void esp_chip_info(esp_chip_info_t *info) { info->revision = host_chip::revision; }
