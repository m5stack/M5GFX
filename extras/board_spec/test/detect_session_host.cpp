#include "board_detect/detect_session.hpp"
#include <array>
#include <cassert>
#include <iostream>
using namespace m5gfx::board_detect;

static const board_def_t proven = { 11, "proven", 0 };
static const board_def_t suggested = { 12, "suggested", 0 };
static const board_def_t alternate = { 13, "alternate", 0 };
struct trace_t {
  unsigned attempt;
  board_id_t hint;
  bool reset, final;
  bool operator==(const trace_t& rhs) const {
    return attempt == rhs.attempt && hint == rhs.hint && reset == rhs.reset && final == rhs.final;
  }
};
struct observed_t {
  board_id_t board = 0, candidate = 0;
  bool transient = false;
  unsigned attempts = 0, calls = 0;
  std::array<trace_t, 5> trace;
};
static detect_outcome_t injected(unsigned kind) {
  detect_outcome_t out;
  if (kind == 0 || kind == 1) { out.reason = fail_reason_t::no_signature; }
  if (kind == 1 || kind == 7) {
    out.result.candidate = kind == 1 ? &suggested : &alternate;
    out.verdict = verdict_t::candidate;
  }
  if (kind == 3 || kind == 5 || kind == 6 || kind == 8 || kind == 9) {
    out.verdict = verdict_t::confirmed;
    out.result.status = detect_status_t::matched;
    out.result.def = &proven;
    out.result.transient_fallback = kind == 3 || kind == 6;
    out.setup_succeeded = kind == 5 || kind == 6;
    if (kind == 3) { out.reason = fail_reason_t::prepare_failed; }
    if (kind == 8) { out.reason = fail_reason_t::construct_failed; }
    if (kind == 9) { out.reason = fail_reason_t::adopt_failed; }
  }
  if (kind == 4) { out.reason = fail_reason_t::excluded_by_set; }
  return out;
}
// Independent transcription of the pre-session loop: retry=4..0, ESP32's
// original NVS hint versus the other chips' previous board, and hint gates.
static observed_t legacy(unsigned sequence, const detect_session_policy_t& policy,
                          board_id_t initial_hint, bool reset, bool has_list) {
  observed_t obs;
  board_id_t board = initial_hint;
  int retry = 4;
  do {
    ++obs.attempts;
    if (retry == 1) { reset = true; }
    const auto hint = policy.keep_initial_hint ? initial_hint : board;
    bool allowed = hint == 0 || policy.hint_gate == nullptr;
    if (!allowed) {
      for (auto p = policy.hint_gate; *p; ++p) { if (*p == hint) { allowed = true; } }
    }
    detect_outcome_t out;
    if (has_list && allowed) {
      obs.trace[obs.calls++] = { unsigned(4 - retry), hint, reset,
                                retry == 0 && !policy.suppress_final_attempt };
      unsigned digits = sequence;
      for (int i = 0; i < 4 - retry; ++i) { digits /= 10; }
      out = injected(digits % 10);
    }
    if (!obs.candidate && out.result.candidate) { obs.candidate = out.result.candidate->id; }
    if (out.setup_succeeded) {
      board = out.result.def->id;
      obs.transient = out.result.transient_fallback;
      break;
    }
    board = 0;
    if (out.reason == fail_reason_t::no_signature) { obs.transient = true; break; }
  } while (board == 0 && --retry >= 0);
  obs.board = board;
  if (board) { obs.candidate = 0; }
  return obs;
}
static observed_t session(unsigned sequence, const detect_session_policy_t& policy,
                           board_id_t initial_hint, bool reset, bool has_list) {
  observed_t obs;
  detect_request_t request;
  request.hint = initial_hint; request.allow_reset = reset; request.max_attempts = 5;
  const auto outcome = run_detection_session(request, policy,
    [&](const detect_request_t& attempt, bool final) {
      assert(attempt.target_set == nullptr);
      if (!has_list) { return detect_outcome_t{}; }
      obs.trace[obs.calls++] = { attempt.attempt, attempt.hint, attempt.allow_reset, final };
      unsigned digits = sequence;
      for (unsigned i = 0; i < attempt.attempt; ++i) { digits /= 10; }
      return injected(digits % 10);
    });
  obs.attempts = outcome.attempts;
  obs.board = outcome.setup_succeeded ? outcome.result.def->id : 0;
  obs.candidate = !obs.board && outcome.result.candidate ? outcome.result.candidate->id : 0;
  obs.transient = outcome.reason == fail_reason_t::no_signature
               || (outcome.setup_succeeded && outcome.result.transient_fallback);
  return obs;
}
int main() {
  static const board_id_t gate[] = { 11, 0 };
  unsigned cases = 0;
  for (unsigned flags = 0; flags < 8; ++flags) {
    detect_session_policy_t policy;
    policy.hint_gate = flags & 1 ? gate : nullptr;
    policy.keep_initial_hint = flags & 2;
    policy.suppress_final_attempt = flags & 4;
    for (board_id_t hint : { board_id_t(0), board_id_t(11), board_id_t(99) }) {
      for (bool reset : { false, true }) {
        for (unsigned sequence = 0; sequence < 100000; ++sequence) {
          const auto before = legacy(sequence, policy, hint, reset, true);
          const auto after = session(sequence, policy, hint, reset, true);
          assert(before.board == after.board && before.candidate == after.candidate);
          assert(before.transient == after.transient && before.attempts == after.attempts);
          assert(before.calls == after.calls);
          for (unsigned i = 0; i < before.calls; ++i) { assert(before.trace[i] == after.trace[i]); }
          // The unchanged NVS expression also retains unknown=0 writes.
          assert((!before.transient && hint != before.board) == (!after.transient && hint != after.board));
          ++cases;
        }
        const auto before = legacy(0, policy, hint, reset, false);
        const auto after = session(0, policy, hint, reset, false);
        assert(before.board == after.board && before.transient == after.transient);
        assert(before.attempts == after.attempts && after.calls == 0);
      }
    }
  }
  // A wide NVS value ending in an allowed board ID still fails the old gate
  // before narrowing. Only the following unknown-hint attempt may probe.
  detect_session_policy_t narrowed;
  narrowed.hint_gate = gate;
  narrowed.reject_initial_hint = true;
  detect_request_t wide;
  wide.hint = 11; wide.max_attempts = 5;
  unsigned calls = 0;
  const auto after_wide = run_detection_session(wide, narrowed,
    [&](const detect_request_t& request, bool) {
      ++calls; assert(request.attempt == 1 && request.hint == 0); return injected(5);
    });
  assert(calls == 1 && after_wide.attempts == 2 && after_wide.setup_succeeded);
  // The protected compatibility entry is a single attempt and must not promote
  // reset simply because max_attempts is one.
  detect_request_t single;
  detect_session_policy_t policy;
  policy.suppress_final_attempt = true;
  const auto one = run_detection_session(single, policy,
    [](const detect_request_t& request, bool final) {
      assert(!request.allow_reset && !final); return injected(3);
    });
  assert(one.attempts == 1 && one.verdict == verdict_t::confirmed && !one.setup_succeeded);
  std::cout << "legacy/session equivalence cases=" << cases << '\n';
}
