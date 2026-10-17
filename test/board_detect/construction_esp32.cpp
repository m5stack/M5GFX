#define CONFIG_IDF_TARGET_ESP32 1
#define CONFIG_IDF_TARGET "esp32"
#include <cassert>
#include <initializer_list>
#include "support/runtime.hpp"
#include "support/probes.hpp"
#include "board_detect/m5/esp32_d0wdq6.inl"
#include "support/display_devices.hpp"
namespace m5gfx {
#include "board_detect/m5/esp32_d0wdq6_setup.inl"
}
using namespace m5gfx::board_detect;
using namespace m5gfx::board_detect::m5;
int main() {
  unsigned cases = 0;
  for (bool new_pmic : {false, true})
    for (bool panel_e : {false, true}) {
      board_result_t r;
      r.assign(&desc_tough);
      r.option = (new_pmic ? generated_options::core2::new_pmic : 0u) |
                 (panel_e ? generated_options::tough::lcd_e : 0u);
      display_parts_t parts;
      assert(construct_tough(r, &parts) == construct_status_t::ok);
      assert(parts.light->kind() == (new_pmic ? 1 : 2));
      assert(parts.touch->kind() == 1);
      assert(parts.panel->kind() == (panel_e ? 2 : 1));
      assert(parts.panel->reset_reg == (new_pmic ? 0x90 : 0x96));
      assert(parts.touch->config().i2c_addr == 0x2e);
      assert(parts.bus->config().freq_write == 40000000 &&
             parts.bus->config().freq_read == 16000000);
      destroy_display_parts(&parts);
      ++cases;
    }
  for (const auto &entry : esp32_d0wdq6_boards) {
    board_result_t r;
    r.assign(entry.desc);
    display_parts_t parts;
    auto status = setup_detected_board(r, &parts);
    assert(status != construct_status_t::failed);
    if (status == construct_status_t::ok) {
      assert(parts.panel && parts.bus);
      destroy_display_parts(&parts);
    }
    ++cases;
  }
  std::printf("ESP32 construction: %u cases passed\n", cases);
}
