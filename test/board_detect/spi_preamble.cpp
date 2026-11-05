#define CONFIG_IDF_TARGET_ESP32 1
#define CONFIG_IDF_TARGET "esp32"
#include <cassert>
#include <initializer_list>
#include "board_detect/board_detect.inl"
using namespace m5gfx::board_detect;
int main() {
  for (bool zero : {false, true}) {
    host_hardware::reset();
    const std::int8_t watched[] = {13, 15, 23, 5};
    detection_transaction_t transaction(pins(watched), no_pins(), false);
    probe_ctx_t ctx;
    ctx.transaction = &transaction;
    host_hardware::input = [](int pin, int, bool) { assert(pin == 15); return true; };
    host_hardware::events.clear();
    assert(soft_spi_read32(ctx, 13, 15, 15, 23, 5, 4, 1, 1, zero) == 0xffffffffu);
    bool selected = false, mosi_high = false;
    unsigned preamble_clocks = 0, post_clocks = 0;
    std::uint32_t preamble_us = 0;
    for (const auto &event : host_hardware::events) {
      if (event.kind == 4 && event.pin == 5 && event.value == 0) {
        assert(!selected);
        selected = true;
      }
      if (event.kind == 4 && event.pin == 15) mosi_high = event.value;
      if (event.kind == 1 && !selected) preamble_us += event.value;
      if (event.kind == 4 && event.pin == 13 && event.value == 1) {
        if (!selected) { assert(!mosi_high); ++preamble_clocks; }
        else ++post_clocks;
      }
    }
    assert(selected && preamble_clocks == (zero ? 8u : 0u));
    assert(preamble_us == (zero ? 24u : 0u));
    assert(post_clocks == 41); // command byte, dummy bit, and 32 response bits
  }
  std::printf("Soft SPI preamble: 2 cases passed\n");
}
