#define CONFIG_IDF_TARGET_ESP32C61 1
#define CONFIG_IDF_TARGET "esp32c61"
#include <cassert>
#include <initializer_list>
#include "support/runtime.hpp"
#include "support/probes.hpp"
#include "board_detect/m5/esp32c61.inl"
#include "support/display_devices.hpp"
namespace lgfx { struct Panel_TM1680 : Panel_Device {}; }
namespace m5gfx {
static const char LIBRARY_NAME[] = "host";
#include "board_detect/m5/esp32c61/corematrix_setup.inl"
}
using namespace m5gfx::board_detect;
using namespace m5gfx::board_detect::m5;
int main() {
  unsigned cases = 0;
  for (bool opened : {false, true})
    for (bool ack : {false, true})
      for (bool accepts_parts : {false, true}) {
        host_hardware::reset();
        host_hardware::i2c_open_ok = opened;
        unsigned probes = 0;
        host_hardware::wire = [&](int addr, int reg, std::uint8_t *, std::size_t len, bool write) {
          ++probes;
          assert(addr == 0x72 && reg == -1 && len == 0 && !write);
          assert(lgfx::i2c::isInitialized(0) == opened);
          if (opened) assert(host_hardware::ports.at(0) == std::make_pair(0, 1));
          return opened && ack;
        };
        board_result_t result;
        result.assign(&desc_corematrix);
        display_parts_t parts;
        const auto status = construct_corematrix(result, accepts_parts ? &parts : nullptr);
        assert(status == (accepts_parts ? construct_status_t::ok : construct_status_t::failed));
        assert(probes == 1 && host_hardware::ports.empty());
        if (accepts_parts) {
          assert(parts.bus && parts.panel);
          assert(parts.bus->config().i2c_port == 0);
          assert(parts.bus->config().pin_sda == 0 && parts.bus->config().pin_scl == 1);
          destroy_display_parts(&parts);
        }
        assert(host_hardware::ports.empty());
        ++cases;
      }
  std::printf("C61 construction/I2C ownership: %u cases passed\n", cases);
}
