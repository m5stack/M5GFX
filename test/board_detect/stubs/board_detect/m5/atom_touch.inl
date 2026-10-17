// Touch peripheral and WS2812 transport only; the Atom detector remains production code.
namespace atom_touch {
static bool measure(std::uint16_t *a, std::uint16_t *b) {
  ++::host_touch::scans;
  *a = ::host_touch::t4;
  *b = ::host_touch::t7;
  return ::host_touch::available;
}
static void clear_led() { ++::host_touch::clears; }
} // namespace atom_touch
