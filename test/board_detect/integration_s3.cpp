#define CONFIG_IDF_TARGET_ESP32S3 1
#define CONFIG_IDF_TARGET "esp32s3"
#define main integration_common_main
#include "integration.cpp"
#undef main
int main() {
  assert(integration_common_main() == 0);
  for (bool unavailable : {false, true}) {
    reset();
    detector_t detector;
    const board_detector_t *list[] = {&detector, nullptr};
    on_signature = [&](const probe_ctx_t &ctx) {
      assert(ctx.conditional_pins_unavailable == unavailable);
      std::vector<int> captured, expected;
      for (const auto &event : host_hardware::events)
        if (event.kind == 5) captured.push_back(event.pin);
      for (auto pin : m5gfx::board_detect::m5::wiring::detection::unconditional_pins)
        expected.push_back(pin);
      if (!unavailable)
        for (auto pin : m5gfx::board_detect::m5::wiring::detection::opi_pins)
          expected.push_back(pin);
      assert(captured == expected);
    };
    detect_request_t request;
    request.conditional_pins_unavailable = unavailable;
    const auto out = m5gfx::run_detection_attempt(list, request, false,
        [](m5gfx::board_detect::m5::display_parts_t &, lgfx::board_t) { return true; });
    assert(out.setup_succeeded && host_hardware::ports.empty());
    // Fixed startup also refuses a GPIO currently owned by OPI PSRAM.
    reset();
    conditional_unavailable = unavailable;
    const std::int8_t reserved[] = {33};
    auto desc = member;
    desc.power = gpio_power(33);
    desc.hold_high_pins = pins(reserved);
    desc.op_gpio_pins = pins(reserved);
    const m5gfx::board_detect::m5::board_entry_t entry(&desc, construct, "OPI", nullptr);
    const auto fixed = m5gfx::run_fixed_detection(entry, false,
        [](m5gfx::board_detect::m5::display_parts_t &, lgfx::board_t) { return true; });
    assert(fixed.setup_succeeded == !unavailable && host_hardware::ports.empty());
    ++cases;
  }
  std::printf("S3 integration conditional pins: 2 cases passed\n");
}
