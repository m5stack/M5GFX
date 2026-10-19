#define CONFIG_IDF_TARGET_ESP32P4 1
#define CONFIG_IDF_TARGET "esp32p4"
#define CONFIG_SPIRAM 1
#define CONFIG_SPIRAM_SPEED 200
#include <cassert>
#include <initializer_list>
#include "support/runtime.hpp"
#include "support/probes.hpp"
#include "support/dsi_devices.hpp"
#include "board_detect/m5/esp32p4.inl"
namespace m5gfx {
static const char LIBRARY_NAME[] = "host";
#include "board_detect/m5/esp32p4/tab5_setup.inl"
namespace board_detect {
namespace m5 {
construct_status_t construct_corep4x(const board_result_t &, display_parts_t *) { std::abort(); }
}
}
}
using namespace m5gfx::board_detect;
using namespace m5gfx::board_detect::m5;
int main() {
  struct scenario_t { int fw; bool gt911, dsi; int panel; unsigned waits, id_reads; };
  const scenario_t scenarios[] = {
    {1, false, false, 2, 0, 0},
    {3, false, false, 3, 0, 0},
    {7, true, true, 0, 600, 0}, // An unknown but answering ST must not become GT911/ILI9881C.
    {-1, true, false, 1, 0, 0},
    {-1, false, true, 1, 680, 3},
    {-1, false, false, 0, 680, 9},
  };
  unsigned cases = 0;
  for (const auto *desc : {&desc_tab5, &desc_tab5x})
    for (bool fixed : {false, true})
      for (bool power_failure : {false, true})
        for (const auto &scenario : scenarios) {
          host_hardware::reset();
          host_probes::reset();
          host_dsi::reset(scenario.dsi);
          host_chip::revision = desc == &desc_tab5x ? 300 : 0;
          unsigned touch_reads = 0;
          host_hardware::wire = [&](int addr, int reg, std::uint8_t *data,
                                    std::size_t len, bool write) {
            if (addr == 0x43 || addr == 0x44) {
              if (!write && len) *data = 0xA0;
              return !(power_failure && write && addr == 0x43 && reg == 5 && *data == 0x76);
            }
            assert(host_hardware::ports.at(-1) == std::make_pair(31, 32));
            if (addr == 0x55) {
              ++touch_reads;
              if (scenario.fw < 0) return false;
              *data = scenario.fw;
              return true;
            }
            assert(addr == 0x14 && reg == -1);
            return scenario.gt911;
          };
          const std::int8_t watched[] = {31, 32, 23};
          detection_transaction_t transaction(pins(watched), no_pins(), false);
          probe_ctx_t ctx;
          ctx.transaction = &transaction;
          board_result_t result;
          result.option = 0xA5;
          if (fixed) {
            result.assign(desc);
            const auto *entry = find_board(esp32p4_boards, desc->def.id);
            assert(entry->fixed_start(result, ctx));
          } else {
            assert(tab5_family_detector.confirm(ctx, &result));
            assert(prepare(*result.desc, result, ctx));
          }
          assert(bool(result.prepared & prepared_power_failed) == power_failure);
          assert(result.observe_on_power_failure && touch_reads > 0);
          assert(host_hardware::ports.empty());
          const auto reads_before_construction = touch_reads;
          display_parts_t parts;
          const auto status = construct_tab5(result, &parts);
          assert(status == (scenario.panel ? construct_status_t::ok : construct_status_t::no_display));
          assert(touch_reads == reads_before_construction && result.option == 0xA5);
          assert(host_dsi::id_reads == scenario.id_reads);
          assert(host_hardware::now_us == ((power_failure ? 10 : 110) + scenario.waits) * 1000);
          if (scenario.panel) {
            assert(parts.panel && parts.bus && parts.touch && parts.light);
            assert(parts.panel->kind() == scenario.panel);
            assert(parts.touch->kind() == (scenario.panel == 1 ? 3 : 4));
            const auto cfg = static_cast<lgfx::Bus_DSI *>(parts.bus)->config();
            assert(cfg.lane_mbps == (scenario.panel == 2 ? 900 : 1040));
            assert(parts.touch->config().i2c_port == 1);
            assert(parts.touch->config().pin_sda == 31 && parts.touch->config().pin_scl == 32);
            destroy_display_parts(&parts);
          } else {
            assert(!parts.bus && !parts.panel && !parts.touch && !parts.light);
          }
          ++cases;
        }
  std::printf("P4 construction/observation: %u cases passed\n", cases);
}
