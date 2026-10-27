#include <cassert>
#include <initializer_list>
#include "support/runtime.hpp"
#include "support/probes.hpp"
#ifdef TEST_C5
#include "board_detect/m5/esp32c5.inl"
#else
#include "board_detect/m5/esp32c6.inl"
#endif
#include "support/display_backend.hpp"
#include "support/snapshot.hpp"
using namespace m5gfx::board_detect;
using namespace m5gfx::board_detect::m5;
static const board_def_t earlier = {65533, "earlier candidate", def_flag_candidate_only};
struct spy_t : board_detector_t {
  mutable unsigned calls = 0;
  bool seed;
  spy_t(const board_def_t *const *members_, bool value = false)
      : board_detector_t(members_), seed(value) {}
  bool signature(probe_ctx_t &ctx) const override {
    ++calls;
    if (seed)
      ctx.candidate = &earlier;
    return false;
  }
  bool confirm(probe_ctx_t &, board_result_t *) const override {
    assert(false);
    return false;
  }
};
int main() {
#ifdef TEST_C5
  host_snapshot::check("c5", esp32c5_boards);
  const auto &desc = desc_stampc5;
  const auto &detector = stampc5_detector;
  const auto &next = desc_toughc5.def;
#else
  host_snapshot::check("c6", esp32c6_boards);
  const auto &desc = desc_stampc6;
  const auto &detector = stampc6_detector;
  const auto &next = desc_nanoc6.def;
#endif
  const board_def_t *later_members[] = {&next, nullptr};
  const board_def_t *earlier_members[] = {&earlier, nullptr};
  unsigned cases = 0;
  for (int flash = 0; flash < 4; ++flash)
    for (int psram = 0; psram < 4; ++psram)
      for (bool seeded : {false, true})
        for (unsigned start : {0u, 4u})
          for (board_id_t pref : {board_id_unknown, board_id_t(65535)})
            for (board_id_t hint : {board_id_unknown, desc.def.id, next.id}) {
              host_hardware::reset();
              host_probes::reset();
              host_hardware::flash_capacity = flash;
              host_hardware::psram_capacity = psram;
              host_hardware::wire = [](int, int, std::uint8_t *, std::size_t, bool) -> bool {
                assert(false);
                return false;
              };
              spy_t later(later_members), seed(earlier_members, true);
              const board_detector_t *normal[] = {&detector, &later, nullptr};
              const board_detector_t *with_seed[] = {&seed, &detector, &later, nullptr};
              detect_request_t req;
              req.attempt = start;
              req.max_attempts = 5;
              req.hint = hint;
              req.fallback = pref;
              auto out = run_detection_session(req, [&](const detect_request_t &request, bool) {
                detection_transaction_t transaction(no_pins(), no_pins(), false);
                probe_ctx_t ctx;
                ctx.transaction = &transaction;
                ctx.fallback = request.fallback;
                detect_outcome_t out;
                out.result = detect_board(seeded ? with_seed : normal, request.hint, ctx);
                out.reason = fail_reason_t::no_signature;
                if (out.result.candidate) {
                  out.verdict = verdict_t::candidate;
                  out.candidate_kind = candidate_kind_t::weak;
                }
                return out;
              });
#ifdef TEST_C5
              bool matches = flash == 1 && psram == 0;
#else
              bool matches = flash == 2;
#endif
              assert(out.attempts == 1 && !out.adopted &&
                     later.calls == unsigned(!matches));
              assert(host_hardware::events.empty() && !should_persist_detection(out, 0));
              const auto *expected = seeded ? &earlier : matches ? &desc.def : nullptr;
              assert(out.result.candidate == expected &&
                     out.verdict == (expected ? verdict_t::candidate : verdict_t::unknown) &&
                     out.candidate_kind ==
                         (expected ? candidate_kind_t::weak : candidate_kind_t::none));
              ++cases;
            }
  std::printf("Package module: %u cases passed\n", cases);
}
