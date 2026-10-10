#include "board_detect/detect_session.hpp"
#include <cassert>
#include <cstdio>
#include <initializer_list>
using namespace m5gfx::board_detect;
static const board_def_t member = { 1, "member", 0 }, other = { 2, "other", 0 }, weak = { 3, "weak", 0 };

int main()
{
  unsigned cases = 0;
  // All selection-table rows, including displayless adoption and setup failure.
  for (int verdict = 0; verdict != 4; ++verdict)
  for (board_id_t preferred = 0; preferred != 4; ++preferred)
  for (int adopted = 0; adopted != 2; ++adopted)
  {
    detect_request_t request; request.preferred = preferred; request.max_attempts = 5;
    unsigned calls = 0, construct = 0, adopt = 0, saves = 0;
    auto outcome = run_detection_session(request, [&](const detect_request_t& r, bool) {
      ++calls; detect_outcome_t out;
      if (verdict < 2) {
        out.reason = fail_reason_t::no_signature;
        if (verdict == 1) { out.verdict = verdict_t::candidate; out.candidate_kind = candidate_kind_t::weak; out.result.candidate = &weak; }
        return out;
      }
      out.result.def = &member; out.result.provisional = verdict == 2;
      if (!finalize_prepared_result(out, r.preferred)) { return out; }
      ++construct; ++adopt;
      out.setup_succeeded = adopted;
      out.reason = adopted ? fail_reason_t::none : fail_reason_t::adopt_failed;
      return out;
    });
    const bool yields = verdict == 2 && preferred != 0 && preferred != member.id;
    assert(calls == unsigned(verdict < 2 || (adopted && !yields) ? 1 : 5));
    assert(construct == (verdict < 2 || yields ? 0 : calls)); assert(adopt == construct);
    const auto board = outcome.setup_succeeded ? outcome.result.def->id : board_id_unknown;
    const auto candidate = board ? board_id_unknown : outcome.result.candidate ? outcome.result.candidate->id : board_id_unknown;
    assert(board == (verdict >= 2 && !yields && adopted ? member.id : board_id_unknown));
    assert(candidate == (yields ? member.id : verdict == 1 ? weak.id : verdict == 2 && !adopted ? member.id : board_id_unknown));
    assert(outcome.verdict == (verdict == 3 ? verdict_t::confirmed : verdict ? verdict_t::candidate : verdict_t::unknown));
    assert(outcome.candidate_kind == (verdict == 2 ? candidate_kind_t::provisional : verdict == 1 ? candidate_kind_t::weak : candidate_kind_t::none));
    if (should_persist_detection(outcome, 0)) { ++saves; }
    assert(saves == unsigned(verdict == 3 && adopted));
    assert(!should_persist_detection(outcome, member.id));
    ++cases;
  }
  // A weak candidate survives unknown attempts; provisional replaces it through the final retry.
  detect_request_t request; request.max_attempts = 5; request.hint = other.id; request.preferred = other.id;
  auto out = run_detection_session(request, [&](const detect_request_t& r, bool final) {
    assert(r.hint == other.id); assert(r.preferred == other.id); assert(final == (r.attempt == 4));
    assert(r.allow_reset == (r.attempt >= 3));
    detect_outcome_t result;
    if (r.attempt == 0) { result.verdict = verdict_t::candidate; result.candidate_kind = candidate_kind_t::weak; result.result.candidate = &weak; }
    if (r.attempt == 3) { result.result.def = &member; result.result.provisional = true; assert(!finalize_prepared_result(result, r.preferred)); }
    return result;
  });
  assert(out.attempts == 5 && out.result.candidate == &member && out.candidate_kind == candidate_kind_t::provisional);
  assert(!should_persist_detection(out, 0));
  // A provisional setup failure cannot be downgraded by weak/unknown retries.
  request.preferred = 0;
  out = run_detection_session(request, [&](const detect_request_t& r, bool) {
    detect_outcome_t result;
    if (r.attempt == 0) { result.result.def = &member; result.result.provisional = true; finalize_prepared_result(result, 0); result.reason = fail_reason_t::construct_failed; }
    else if (r.attempt == 1) { result.verdict = verdict_t::candidate; result.candidate_kind = candidate_kind_t::weak; result.result.candidate = &weak; }
    return result;
  });
  assert(out.attempts == 5 && out.result.candidate == &member && out.candidate_kind == candidate_kind_t::provisional);
  assert(out.verdict == verdict_t::candidate);
  // Retained candidate kind survives every confirmed-but-failed setup reason.
  for (auto kind : {candidate_kind_t::weak, candidate_kind_t::provisional})
  for (auto failure : {fail_reason_t::prepare_failed, fail_reason_t::construct_failed, fail_reason_t::adopt_failed}) {
    out = run_detection_session(request, [&](const detect_request_t& r, bool) {
      detect_outcome_t result;
      if (r.attempt == 0) {
        result.verdict = verdict_t::candidate; result.candidate_kind = kind;
        result.result.candidate = &weak;
      } else {
        result.result.def = &member; finalize_prepared_result(result, 0);
        result.reason = failure;
      }
      return result;
    });
    assert(out.attempts == 5 && out.verdict == verdict_t::confirmed && !out.setup_succeeded);
    assert(out.result.candidate == &weak && out.candidate_kind == kind && !should_persist_detection(out, 0));
  }
  // An early mismatch may refine successfully on the next attempt.
  request.preferred = other.id;
  out = run_detection_session(request, [&](const detect_request_t& r, bool) {
    detect_outcome_t result; result.result.def = r.attempt == 0 ? &member : &other;
    result.result.provisional = r.attempt == 0;
    if (finalize_prepared_result(result, r.preferred)) { result.setup_succeeded = true; }
    return result;
  });
  assert(out.attempts == 2 && out.setup_succeeded && out.result.def == &other);
  request.preferred = 0;
  // Retained candidates follow confidence first, and provisional recency second.
  for (auto first : {candidate_kind_t::weak, candidate_kind_t::provisional})
  for (auto next : {candidate_kind_t::weak, candidate_kind_t::provisional}) {
    out = run_detection_session(request, [&](const detect_request_t& r, bool) {
      detect_outcome_t result;
      if (r.attempt < 2) {
        result.verdict = verdict_t::candidate;
        result.result.candidate = r.attempt == 0 ? &member : &other;
        result.candidate_kind = r.attempt == 0 ? first : next;
      }
      return result;
    });
    assert(out.attempts == 5 && out.verdict == verdict_t::candidate);
    assert(out.result.candidate == (next == candidate_kind_t::provisional ? &other : &member));
    assert(out.candidate_kind == (next == candidate_kind_t::provisional ? next : first));
  }
  // Initial power failure retains Core2; final refinement selects Tough while yielding to Paper.
  request.hint = other.id; request.preferred = weak.id;
  out = run_detection_session(request, [&](const detect_request_t& r, bool final) {
    detect_outcome_t result;
    if (r.attempt == 0 || final) {
      result.result.def = final ? &other : &member;
      result.result.provisional = true;
      assert(!finalize_prepared_result(result, r.preferred));
    }
    return result;
  });
  assert(out.attempts == 5 && !out.setup_succeeded && out.result.candidate == &other);
  assert(out.verdict == verdict_t::candidate && out.candidate_kind == candidate_kind_t::provisional);
  assert(!should_persist_detection(out, 0));
  request.preferred = 0;
  // Reset escalation and immutable hint remain true through the final retry.
  out = run_detection_session(request, [&](const detect_request_t& r, bool final) {
    assert(r.hint == other.id); assert(final == (r.attempt == 4)); assert(r.allow_reset == (r.attempt >= 3));
    return detect_outcome_t{};
  }); assert(out.attempts == 5);
  for (const auto reason : {fail_reason_t::prepare_failed, fail_reason_t::construct_failed, fail_reason_t::adopt_failed}) {
    out = {}; out.result.def = &member; finalize_prepared_result(out, 0); out.reason = reason;
    assert(!should_persist_detection(out, 0));
  }
  std::printf("selection cases=%u\n", cases);
}
