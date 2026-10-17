#define CONFIG_IDF_TARGET_ESP32S3 1
#define CONFIG_IDF_TARGET "esp32s3"
#include <cassert>
#include <initializer_list>
#include "support/runtime.hpp"
#include "support/probes.hpp"
#include "board_detect/m5/esp32s3.inl"
#include "support/display_backend.hpp"
using namespace m5gfx::board_detect;
using namespace m5gfx::board_detect::m5;
int main() {
  unsigned cases = 0;
  for (bool down_high : {false, true})
    for (bool up_high : {false, true}) {
      host_hardware::reset();
      host_probes::reset();
      unsigned reads = 0;
      host_probes::pulls = [&](std::uint64_t mask, std::uint32_t release) {
        ++reads;
        assert(mask == (1ull << 2) && release == 0);
        pin_pull_result_t result;
        result.pulldown_high = down_high ? 1ull << 2 : 0;
        result.pullup_high = up_high ? 1ull << 2 : 0;
        return result;
      };
      detection_transaction_t tx(pins(wiring::detection::unconditional_pins),
                                 pins(wiring::detection::opi_pins), true);
      probe_ctx_t ctx;
      ctx.transaction = &tx;
      assert(airq_detector.signature(ctx) == (!down_high && up_high));
      assert(reads == 1 && host_hardware::ports.empty());
      ++cases;
    }
  std::printf("AirQ generated gate: %u cases passed\n", cases);
}
