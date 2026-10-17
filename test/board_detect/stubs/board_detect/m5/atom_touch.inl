// Touch peripheral and WS2812 transport only; the Atom detector remains production code.
namespace atom_touch {
static bool measure(std::uint16_t *a, std::uint16_t *b) {
  ++::host_touch::scans;
  for (const auto& pad : {t4_pad, t7_pad})
    ::host_hardware::events.push_back({0, pad.gpio, 0});
  *a = ::host_touch::t4;
  *b = ::host_touch::t7;
  return ::host_touch::available;
}
static void clear_led() {
  ++::host_touch::clears;
  ::host_hardware::events.push_back({0, led_gpio, unsigned(lgfx::output)});
  ::host_hardware::events.push_back({4, led_gpio, 0});
}
} // namespace atom_touch
