#define CONFIG_IDF_TARGET_ESP32S3 1
#define CONFIG_IDF_TARGET "esp32s3"
#include <cassert>
#include <initializer_list>
#include <vector>
#include "board_detect/board_detect.inl"
#include "board_detect/m5/esp32s3.inl"
#include "support/display_backend.hpp"
using namespace m5gfx::board_detect;
using namespace m5gfx::board_detect::m5;
static unsigned cases;
static void classifier() {
  const pull_class_t expected[] = {
      pull_class_t::down, pull_class_t::conflict, pull_class_t::weak_down, pull_class_t::up,
      pull_class_t::down, pull_class_t::conflict, pull_class_t::conflict,  pull_class_t::up,
      pull_class_t::down, pull_class_t::conflict, pull_class_t::floating,  pull_class_t::up,
      pull_class_t::down, pull_class_t::conflict, pull_class_t::weak_up,   pull_class_t::up};
  for (int pin : {0, 17, 63})
    for (unsigned sample = 0; sample < 16; ++sample)
      for (bool release : {false, true}) {
        auto bit = std::uint64_t(1) << pin;
        pin_pull_result_t r;
        r.release_sampled = release;
        r.pulldown_high = (sample & 1) ? ~std::uint64_t(0) : ~bit;
        r.pullup_high = (sample & 2) ? ~std::uint64_t(0) : ~bit;
        r.pulldown_release_high = (sample & 4) ? ~std::uint64_t(0) : ~bit;
        r.pullup_release_high = (sample & 8) ? ~std::uint64_t(0) : ~bit;
        auto value = classify_pin_pull(r, pin);
        assert(value ==
               (!release && (sample & 3) == 2 ? pull_class_t::pull_dependent : expected[sample]));
        if (!release)
          assert(value != pull_class_t::weak_up && value != pull_class_t::weak_down &&
                 value != pull_class_t::floating);
        ++cases;
      }
}
static void pull_sequence() {
  const std::int8_t watched[] = {3, 9};
  host_hardware::reset();
  detection_transaction_t tx(pins(watched), no_pins(), false);
  assert(tx.valid());
  probe_ctx_t ctx;
  ctx.transaction = &tx;
  std::vector<bool> samples = {false, true, true, false};
  std::size_t cursor = 0;
  host_hardware::events.clear();
  host_hardware::input = [&](int, int, bool) { return samples.at(cursor++); };
  auto legacy = probe_pin_pulls(ctx, (1ull << 3) | (1ull << 9));
  using host_hardware::event_t;
  using lgfx::pin_mode_t;
  std::vector<event_t> expected;
  for (int pin : {3, 9}) {
    expected.push_back({0, pin, pin_mode_t::input_pulldown});
    expected.push_back({1, -1, 10});
    expected.push_back({2, pin, unsigned(pin == 9)});
    expected.push_back({0, pin, pin_mode_t::input_pullup});
    expected.push_back({1, -1, 10});
    expected.push_back({2, pin, unsigned(pin == 3)});
    expected.push_back({3, pin, 0});
  }
  assert(host_hardware::events == expected && cursor == 4);
  assert(legacy.pulldown_high == (1ull << 9) && legacy.pullup_high == (1ull << 3));
  assert(!legacy.release_sampled && legacy.pulldown_release_high == 0 &&
         legacy.pullup_release_high == 0);
  host_hardware::events.clear();
  cursor = 0;
  probe_pin_pulls(ctx, (1ull << 3) | (1ull << 9), 0);
  assert(host_hardware::events == expected && cursor == 4);
  host_hardware::events.clear();
  cursor = 0;
  samples = {false, false, true, true, true, false, false, true};
  auto released = probe_pin_pulls(ctx, (1ull << 3) | (1ull << 9), 128);
  expected.clear();
  for (int pin : {3, 9}) {
    expected.push_back({0, pin, pin_mode_t::input_pulldown});
    expected.push_back({1, -1, 10});
    expected.push_back({2, pin, unsigned(pin == 9)});
    expected.push_back({0, pin, pin_mode_t::input});
    expected.push_back({1, -1, 128});
    expected.push_back({2, pin, 0});
    expected.push_back({0, pin, pin_mode_t::input_pullup});
    expected.push_back({1, -1, 10});
    expected.push_back({2, pin, unsigned(pin == 3)});
    expected.push_back({0, pin, pin_mode_t::input});
    expected.push_back({1, -1, 128});
    expected.push_back({2, pin, 1});
  }
  assert(host_hardware::events == expected && cursor == 8);
  assert(released.pulldown_high == (1ull << 9) && released.pullup_high == (1ull << 3));
  assert(released.pulldown_release_high == 0 &&
         released.pullup_release_high == ((1ull << 3) | (1ull << 9)) && released.release_sampled);
  assert(classify_pin_pull(released, 3) == pull_class_t::floating &&
         classify_pin_pull(released, 9) == pull_class_t::conflict);
  host_hardware::events.clear();
  cursor = 0;
  probe_pin_pulls(ctx, 0, 128);
  assert(host_hardware::events.empty() && cursor == 0);
  ++cases;
}
static void dualkey_table() {
  host_hardware::reset();
  const std::int8_t watched[] = {17, 21, 38, 39};
  detection_transaction_t tx(pins(watched), no_pins(), false);
  probe_ctx_t ctx;
  ctx.transaction = &tx;
#ifdef CONFIG_SPIRAM_MODE_OCT
  host_hardware::events.clear();
  assert(!dualkey_detector.signature(ctx) && host_hardware::events.empty() &&
         ctx.candidate == nullptr);
  ++cases;
#else
  for (unsigned a = 0; a < 16; ++a)
    for (unsigned b = 0; b < 16; ++b)
      for (bool key : {false, true}) {
        unsigned counts[64] = {};
        ctx.candidate = nullptr;
        host_hardware::events.clear();
        host_hardware::input = [&](int pin, int, bool) {
          if (pin == 21)
            return true;
          if (pin == 17)
            return !key;
          assert(pin == 38 || pin == 39);
          unsigned sample = pin == 38 ? a : b;
          const unsigned bits[] = {1, 4, 2, 8};
          return bool(sample & bits[counts[pin]++]);
        };
        bool floating = a == 10 && b == 10;
        assert(dualkey_detector.signature(ctx) == (floating && !key));
        assert((ctx.candidate != nullptr) == (floating && key));
        assert(counts[38] == 4 && counts[39] == 4);
        // Observe the actual DualKey call: two released samples on each socket pin.
        std::vector<host_hardware::event_t> release_waits;
        const auto &events = host_hardware::events;
        for (std::size_t i = 0; i + 2 < events.size(); ++i)
          if (events[i].kind == 0 && (events[i].pin == 38 || events[i].pin == 39) &&
              events[i].value == lgfx::input) {
            assert(events[i + 1].kind == 1 && events[i + 1].value == 128);
            assert(events[i + 2].kind == 2 && events[i + 2].pin == events[i].pin);
            release_waits.push_back({1, events[i].pin, events[i + 1].value});
          }
        const std::vector<host_hardware::event_t> expected_waits = {
            {1, 38, 128}, {1, 38, 128}, {1, 39, 128}, {1, 39, 128}};
        assert(release_waits == expected_waits);
        unsigned reads17 = 0;
        for (const auto &e : host_hardware::events)
          if (e.kind == 2 && e.pin == 17)
            ++reads17;
        assert(reads17 == (floating ? 2u : 0u));
        ++cases;
      }
#endif
}
int main() {
  classifier();
  pull_sequence();
  dualkey_table();
  std::printf("Pull/release: %u cases passed\n", cases);
}
