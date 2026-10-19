// Exercise production probe implementations, including GPIO changes hidden by the
// response-level doubles used in the family truth tables.
#pragma once
#include <cassert>
#include <initializer_list>
#include <set>
#include <map>
#include "board_detect/board_detect.inl"

#include COVERAGE_BOARD_HEADER
#include "display_backend.hpp"
using namespace m5gfx::board_detect;
using namespace m5gfx::board_detect::m5;
#include "../generated/detector_probe_pins.hpp"
static unsigned cases, families, member_mismatches;
static std::map<const board_detector_t*, std::set<int>> unexpected_family_pins;
static const expected_detector_pins_t& expected_pins(const board_detector_t& detector) {
  for (const auto& expected : expected_detector_pins)
    if (expected.detector == &detector) return expected;
  std::fprintf(stderr, "%s: missing generated family pin expectation\n", detector.members[0]->name);
  std::abort();
}
// Compare the ordered C++ membership with the catalog before exercising pins.
static void check_members(const board_detector_t& detector) {
  const auto& expected = expected_pins(detector);
  std::vector<board_id_t> actual;
  for (auto member = detector.members; *member; ++member) actual.push_back((*member)->id);
  if (actual == expected.members) return;
  ++member_mismatches;
  std::fprintf(stderr, "Family member mismatch (%s): actual:", expected.name);
  for (auto id : actual) std::fprintf(stderr, " %u", unsigned(id));
  std::fprintf(stderr, "; expected:");
  for (auto id : expected.members) std::fprintf(stderr, " %u", unsigned(id));
  std::fprintf(stderr, "\n");
}
struct confirmation_count_t {
  const board_detector_t* detector;
  unsigned calls, completed;
};
static std::vector<confirmation_count_t> confirmations;
static void record_confirmation(const board_detector_t& detector, bool completed) {
  for (auto& count : confirmations) if (count.detector == &detector) {
    ++count.calls;
    count.completed += completed;
    return;
  }
  confirmations.push_back({&detector, 1, unsigned(completed)});
}
static void check_events(const std::set<int>& captured, const board_detector_t& detector) {
  const auto& expected = expected_pins(detector);
  for (const auto& event : host_hardware::events) {
    // Delays and I2C-port release events do not name GPIO pads. Snapshot and
    // restore events do; checking them as well also checks the test setup.
    if (event.kind == 1 || event.kind == 6 || event.pin < 0) continue;
    if (!captured.count(event.pin)) {
      std::fprintf(stderr, "%s: GPIO %d (event %d) absent from snapshot\n",
                   detector.members[0]->name, event.pin, event.kind);
      std::abort();
    }
    // Probe operations, rather than transaction-wide snapshot/restore events,
    // must belong to this family's own catalog-derived union.
    if ((event.kind == 0 || event.kind == 2 || event.kind == 4 || event.kind == 7)
     && !expected.pins.count(event.pin))
      unexpected_family_pins[&detector].insert(event.pin);
  }
}
// These families read eFuse/package facts and only nominate a weak candidate;
// neither their signature nor their always-false confirmation uses a GPIO pad.
static bool gpio_free_family(const board_detector_t& detector) {
  const auto id = detector.members[0]->id;
  return id == static_cast<board_id_t>(lgfx::board_M5StampC5)
      || id == static_cast<board_id_t>(lgfx::board_M5StampC6);
}
static void cover(const board_detector_t* const* detectors) {
  for (auto entry = detectors; *entry; ++entry) {
    const auto& detector = **entry;
    ++families;
    check_members(detector);
    std::size_t touched = 0;
    // No fixed hint, then every member hint, exercises shared SPI-ID families.
    for (unsigned member = 0; ; ++member) {
      const auto hint = member ? detector.members[member - 1] : nullptr;
      if (member && !hint) break;
      for (unsigned pattern = 0; pattern != 4; ++pattern)
        for (unsigned sample : {0u, 1u, 2u, 0x20u, 0x54u, 0x7cu, 0x81u, 0x85u, 0xa5u, 0xffu}) {
          host_hardware::reset();
          host_hardware::flash_capacity = sample & 7;
          host_hardware::psram_capacity = (sample >> 3) & 3;
          host_hardware::wire = [=](int, int, std::uint8_t* data, std::size_t length, bool) {
            for (std::size_t i = 0; i < length; ++i) data[i] = sample;
            return sample != 0;
          };
          host_hardware::input = [=](int pin, int mode, bool) {
            // Real GPIO reads consume time even in a polling loop without delay.
            ++host_hardware::now_us;
            if (pattern == 0) return true;
            if (pattern == 1) return mode == lgfx::input_pullup;
            if (pattern == 2) return pin % 2 == 0;
            // Atom: G2 high, G18 floating, G12 low, G34 follows G23.
            if (pin == 2) return true;
            if (pin == 12) return false;
            if (pin == 34) return host_hardware::gpio[23].mode == lgfx::input_pullup;
            return mode == lgfx::input_pullup;
          };
          {
            detection_transaction_t tx(pins(wiring::detection::unconditional_pins),
                                       COVERAGE_CONDITIONAL_PINS, true);
            assert(tx.valid());
            std::set<int> captured;
            for (const auto& event : host_hardware::events)
              if (event.kind == 5) captured.insert(event.pin);
            host_hardware::events.clear();
            probe_ctx_t ctx;
            ctx.transaction = &tx;
            ctx.i2c_port_probe = -1;
            ctx.allow_reset = true;
            ctx.final_attempt = true;
            ctx.hint = hint ? hint->id : board_id_unknown;
            detector.signature(ctx);
            // Confirm is checked independently of the signature verdict: a
            // failed signature must not hide the GPIO paths inside confirm.
            board_result_t result;
            record_confirmation(detector, detector.confirm(ctx, &result));
            check_events(captured, detector);
            for (const auto& event : host_hardware::events)
              if (event.kind == 0 || event.kind == 2 || event.kind == 4 || event.kind == 7)
                ++touched;
          }
          assert(host_hardware::ports.empty());
          ++cases;
        }
    }
    std::printf("GPIO/I2C events (%s): %zu%s\n", detector.members[0]->name, touched,
                gpio_free_family(detector) ? " (eFuse-only candidate)" : "");
    if (!gpio_free_family(detector) && touched == 0)
      std::fprintf(stderr, "%s: no GPIO/I2C events observed\n", detector.members[0]->name);
    assert(gpio_free_family(detector) || touched > 0);
  }
}
int main() {
  COVERAGE_FAMILIES
#ifdef COVERAGE_EXTRA
  COVERAGE_EXTRA
#endif
  for (const auto& count : confirmations)
    std::printf("confirm completion (%s): %u/%u\n", count.detector->members[0]->name,
                count.completed, count.calls);
  for (const auto& mismatch : unexpected_family_pins) {
    const auto& expected = expected_pins(*mismatch.first);
    std::fprintf(stderr, "Family pin mismatch (%s): observed outside family:", expected.name);
    for (int pin : mismatch.second) std::fprintf(stderr, " %d", pin);
    std::fprintf(stderr, "; expected:");
    for (int pin : expected.pins) std::fprintf(stderr, " %d", pin);
    std::fprintf(stderr, "\n");
  }
  std::printf("GPIO snapshot coverage (%s): %u families, %u cases passed\n",
              CONFIG_IDF_TARGET, families, cases);
  std::printf("Family catalog pin coverage (%s): %u families, %zu mismatched\n",
              CONFIG_IDF_TARGET, families, unexpected_family_pins.size());
  std::printf("Family catalog member coverage (%s): %u families, %u mismatched\n",
              CONFIG_IDF_TARGET, families, member_mismatches);
  return unexpected_family_pins.empty() && member_mismatches == 0 ? 0 : 1;
}
