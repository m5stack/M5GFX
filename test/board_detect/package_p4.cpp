#define CONFIG_IDF_TARGET_ESP32P4 1
#define CONFIG_IDF_TARGET "esp32p4"
#include <cassert>
#include <initializer_list>
#include "support/runtime.hpp"
#include "support/probes.hpp"
#include "board_detect/m5/esp32p4.inl"
#include "support/display_backend.hpp"
#include "support/snapshot.hpp"
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
        p.pulldown_high = wiring::tab5::detect_class_up;
        p.pullup_high = wiring::tab5::detect_class_up | wiring::tab5::detect_class_floating;
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
  std::printf("P4 packages: %u cases passed\n", cases);
}
