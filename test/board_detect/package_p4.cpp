#define CONFIG_IDF_TARGET_ESP32P4 1
#define CONFIG_SPIRAM 1
#define CONFIG_IDF_TARGET "esp32p4"
#include <cassert>
#include <initializer_list>
#include "support/runtime.hpp"
#include "support/probes.hpp"
#include "support/dsi_devices.hpp"
#include "board_detect/m5/esp32p4.inl"
#include "support/display_backend.hpp"
#include "support/snapshot.hpp"
namespace lgfx {
struct IBus;
struct Panel_Device;
struct ILight;
struct ITouch;
}
namespace m5gfx {
namespace board_detect {
namespace m5 {
#include "board_detect/m5/display_parts.inl"
construct_status_t construct_detected_board(const board_result_t &result, display_parts_t *parts) {
  return construct_board(esp32p4_boards, result, parts);
}
void destroy_display_parts(display_parts_t *parts) {
  assert(!parts->bus && !parts->panel && !parts->light && !parts->touch);
}
}
}
static unsigned get_pkg_ver() { return 0; }
static const char LIBRARY_NAME[] = "host";
static const int detection_i2c_port = -1;
using lgfx::board_t;
#include "board_detect/detection_package.inl"
#include "board_detect/detection_integration.inl"
}
using namespace m5gfx::board_detect;
using namespace m5gfx::board_detect::m5;
static unsigned tab5_observations() {
  struct scenario_t {
    int version;       // -1 means no ST response; 7 is an unknown version.
    unsigned ready;    // Number of unsuccessful or unknown first samples.
    bool gt911;
    std::uint32_t flags;
    unsigned samples, waits;
  };
  const scenario_t scenarios[] = {
    {1, 0, false, tab5_detail::read_st_touch_fw | tab5_detail::touch_st7121, 1, 0},
    {3, 0, false, tab5_detail::read_st_touch_fw | tab5_detail::touch_st7123, 1, 0},
    {1, 3, false, tab5_detail::read_st_touch_fw | tab5_detail::touch_st7121, 4, 3},
    {7, 0, true, tab5_detail::read_st_touch_fw, 60, 60},
    {-1, 0, false, 0, 60, 60},
    {-1, 0, true, tab5_detail::touch_gt911, 1, 0},
    {-1, 1, true, tab5_detail::read_st_touch_fw | tab5_detail::touch_gt911, 2, 1},
    {3, 1, true, tab5_detail::read_st_touch_fw | tab5_detail::touch_st7123, 2, 1},
  };
  unsigned cases = 0;
  for (const auto *desc : {&desc_tab5, &desc_tab5x})
    for (bool fixed : {false, true})
      for (bool already_open : {false, true})
        for (const auto &scenario : scenarios) {
          host_hardware::reset();
          host_probes::reset();
          host_chip::revision = desc == &desc_tab5x ? 300 : 0;
          if (already_open) lgfx::i2c::init(1, 7, 8);
          const auto caller_ports = host_hardware::ports;
          unsigned samples = 0, gt911_probes = 0, writes = 0;
          host_hardware::wire = [&](int addr, int reg, std::uint8_t *data,
                                    std::size_t len, bool write) {
            if (addr == 0x43 || addr == 0x44) {
              if (write) ++writes;
              else if (len) *data = 0xA0;
              return true;
            }
            assert(host_hardware::ports.count(-1) == 1);
            assert(host_hardware::ports.at(-1) == std::make_pair(31, 32));
            assert(writes > 0 && host_hardware::now_us >= 110000);
            assert(host_hardware::gpio[23].high);
            if (addr == 0x55) {
              assert(reg == 0 && len == 1 && !write);
              ++samples;
              if (samples <= scenario.ready) {
                if (scenario.ready == 3) return false;
                *data = 7;
                return true;
              }
              if (scenario.version < 0) return false;
              *data = scenario.version;
              return true;
            }
            assert(addr == 0x14 && reg == -1 && len == 0 && !write);
            ++gt911_probes;
            return scenario.gt911;
          };
          const std::int8_t watched[] = {31, 32, 23};
          detection_transaction_t transaction(pins(watched), no_pins(), false);
          probe_ctx_t ctx;
          ctx.transaction = &transaction;
          ctx.probe_port = -1;
          board_result_t result;
          result.option = 0xA5;
          if (fixed) {
            result.assign(desc);
            const auto *entry = find_board(esp32p4_boards, desc->def.id);
            assert(entry && entry->fixed_prepare == tab5_detail::fixed_prepare);
            assert(entry->fixed_prepare(result, ctx));
          } else {
            assert(tab5_family_detector.confirm(ctx, &result));
            assert(result.observe_after_power == tab5_detail::observe_touch);
            assert(prepare(*result.desc, result, ctx));
          }
          assert(result.desc == desc && result.option == 0xA5);
          assert((result.prepared & 0xFFFF0000u) == scenario.flags);
          assert(result.prepared & prepared_observation);
          assert(samples == scenario.samples);
          const auto expected_gt911 = scenario.version < 0
            ? (scenario.gt911 ? 1u : 60u)
            : (scenario.ready == 3 ? 3u : 0u);
          assert(gt911_probes == expected_gt911);
          assert(host_hardware::now_us == 110000 + 10000 * scenario.waits);
          assert(host_hardware::ports == caller_ports);
          const auto before = host_hardware::now_us;
          assert(prepare(*desc, result, ctx));
          assert(host_hardware::now_us == before && samples == scenario.samples);
          transaction.rollback();
          assert(host_hardware::ports == caller_ports);
          ++cases;
        }
  return cases;
}
int main() {
  host_snapshot::check("p4", esp32p4_boards);
  unsigned cases = tab5_observations();
  for (bool weak : {false, true})
    for (int revision : {0, 300}) {
      host_hardware::reset();
      host_probes::reset();
      host_chip::revision = revision;
      host_probes::bus_present = weak;
      host_hardware::wire = [](int, int, std::uint8_t *, std::size_t, bool) { return false; };
      host_probes::pulls = [](std::uint64_t mask, std::uint32_t) {
        pin_pull_result_t p;
        p.pulldown_high = wiring::tab5::detect_class_expected.up;
        p.pullup_high = wiring::tab5::detect_class_expected.up | wiring::tab5::detect_class_expected.floating;
        p.pulldown_high &= mask;
        p.pullup_high &= mask;
        return p;
      };
      const auto *expected = weak ? (revision >= 300 ? &desc_tab5x.def : &desc_tab5.def) : nullptr;
      detect_request_t req;
      req.max_attempts = 5;
      auto out = run_detection_session(req, [&](const detect_request_t &, bool) {
        detection_transaction_t transaction(no_pins(), no_pins(), false);
        probe_ctx_t ctx;
        ctx.transaction = &transaction;
        detect_outcome_t out;
        out.result = detect_board(esp32p4_detectors, 0, ctx);
        if (!ctx.confirm_attempted)
          out.reason = fail_reason_t::no_signature;
        if (out.result.candidate) {
          out.verdict = verdict_t::candidate;
          out.candidate_kind = candidate_kind_t::weak;
        }
        return out;
      });
      assert(out.result.candidate == expected &&
             out.verdict == (weak ? verdict_t::candidate : verdict_t::unknown) &&
             !out.adopted && !should_persist_detection(out, 0));
      assert(out.attempts == (weak ? 5 : 1));
      ++cases;
    }
  // fixed_board dispatch selects this entry before entering the fixed runner.
  host_hardware::reset();
  const auto *fixed = m5gfx::select_fixed_board(lgfx::board_M5StampP4);
  assert(fixed && fixed->desc == &desc_stampp4 && fixed->construct == construct_displayless);
  unsigned adoptions = 0;
  const auto out = m5gfx::run_fixed_startup_session(*fixed, false,
      [&](display_parts_t &parts, lgfx::board_t board) {
        ++adoptions;
        assert(board == lgfx::board_M5StampP4);
        assert(!parts.bus && !parts.panel && !parts.light && !parts.touch);
        return true;
      });
  assert(out.adopted && out.attempts == 1 && out.verdict == verdict_t::confirmed &&
         out.result.desc == &desc_stampp4 && adoptions == 1);
  assert(host_hardware::events.empty() && host_hardware::ports.empty());
  for (auto detector = esp32p4_detectors; *detector; ++detector) assert(!(*detector)->has_member(desc_stampp4.def.id));
  ++cases;
  std::printf("P4 packages: %u cases passed\n", cases);
}
