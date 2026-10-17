#define CONFIG_IDF_TARGET_ESP32P4 1
#define CONFIG_IDF_TARGET "esp32p4"
#include <cassert>
#include <initializer_list>
#include "support/runtime.hpp"
#include "support/probes.hpp"
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
construct_status_t setup_detected_board(const board_result_t &result, display_parts_t *parts) {
  return setup_board(esp32p4_boards, result, parts);
}
void destroy_display_parts(display_parts_t *parts) {
  assert(!parts->bus && !parts->panel && !parts->light && !parts->touch);
}
}
}
static unsigned get_pkg_ver() { return 0; }
static const char LIBRARY_NAME[] = "host";
static const int probe_i2c_port = -1;
using lgfx::board_t;
#include "board_detect/detection_package.inl"
#include "board_detect/detection_integration.inl"
}
using namespace m5gfx::board_detect;
using namespace m5gfx::board_detect::m5;
int main() {
  host_snapshot::check("p4", esp32p4_boards);
  unsigned cases = 0;
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
             !out.setup_succeeded && !should_persist_detection(out, 0));
      assert(out.attempts == (weak ? 5 : 1));
      ++cases;
    }
  // fixed_board dispatch selects this entry before entering the fixed runner.
  host_hardware::reset();
  const auto *fixed = m5gfx::select_fixed_board(lgfx::board_M5StampP4);
  assert(fixed && fixed->desc == &desc_stampp4 && fixed->construct == construct_displayless);
  unsigned adoptions = 0;
  const auto out = m5gfx::run_fixed_detection_session(*fixed, false,
      [&](display_parts_t &parts, lgfx::board_t board) {
        ++adoptions;
        assert(board == lgfx::board_M5StampP4);
        assert(!parts.bus && !parts.panel && !parts.light && !parts.touch);
        return true;
      });
  assert(out.setup_succeeded && out.attempts == 1 && out.verdict == verdict_t::confirmed &&
         out.result.desc == &desc_stampp4 && adoptions == 1);
  assert(host_hardware::events.empty() && host_hardware::ports.empty());
  for (auto detector = esp32p4_detectors; *detector; ++detector) assert(!(*detector)->has_member(desc_stampp4.def.id));
  ++cases;
  std::printf("P4 packages: %u cases passed\n", cases);
}
