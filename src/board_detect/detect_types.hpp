// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include <cstdint>

namespace m5gfx
{
namespace board_detect
{
  using board_id_t = std::uint16_t;
  static constexpr board_id_t board_id_unknown = 0;

  enum board_def_flag_t : std::uint8_t
  {
    // No positive signature; returned only as a weak candidate.
    // An NVS hint cannot promote a family containing only these members.
    def_flag_candidate_only = 1 << 0,
  };

  struct board_def_t
  {
    board_id_t id;
    const char* name;
    std::uint8_t flags;
  };

#if __cplusplus >= 201703L
  inline const board_def_t board_def_unknown = { board_id_unknown, "unknown", 0 };
#else
  // Arduino-ESP32 2.x still compiles as C++11. Internal linkage provides the
  // same ODR safety there; C++17 and newer use the single inline definition.
  static const board_def_t board_def_unknown = { board_id_unknown, "unknown", 0 };
#endif

  enum prepared_state_t : std::uint32_t
  {
    // Bits 0..15 are manufacturer-independent completed operations or probe
    // side effects. Bits 16..31 are reserved for board-specific state.
    prepared_power  = 1u << 0,
    prepared_reset  = 1u << 1,
    prepared_sd_spi = 1u << 2,
    // The GPIO reset line has been driven inactive, but a reset pulse may not
    // have been allowed. Keep this distinct from prepared_reset so a later
    // reset-enabled prepare can still pulse the panel reset.
    prepared_reset_line = 1u << 4,
    // A confirmed family's post-power refinement has been resolved (run, or
    // deliberately skipped after a failed retained power sequence).
    prepared_refine = 1u << 5,
    // The confirmed board was retained after its power operation list stopped.
    prepared_power_failed = 1u << 6,
    prepared_observation = 1u << 7,
  };

  enum class detect_status_t : std::uint8_t
  {
    no_match,
    matched,
    excluded,
  };

  struct board_desc_t;
  struct prepare_ctx_t;
  class detection_transaction_t;
  struct board_result_t;
  using refine_fn_t = bool (*)(board_result_t&, const prepare_ctx_t&);

  struct board_result_t
  {
    // A matched/direct result owns no description; this points at the static
    // canonical description and def aliases &desc->def. Unknown results have
    // desc == nullptr and retain board_def_unknown for compatibility.
    const board_desc_t* desc = nullptr;
    const board_def_t* def = &board_def_unknown;
    std::uint32_t option = 0;
    std::uint32_t prepared = 0;
    // Family evidence permits setup, but its member is not confirmed.
    bool provisional = false;
    // Copied from probe_ctx_t::candidate when a scan ends without a match.
    // Provisional members are also retained; the session carries the strongest
    // candidate across attempts, replacing a weak candidate with a provisional one.
    const board_def_t* candidate = nullptr;
    detect_status_t status = detect_status_t::no_match;
    // Optional read-only member refinement after power preparation.
    refine_fn_t refine = nullptr;
    // Detector-private state passed to refine; construction and NVS ignore it.
    std::uint8_t refine_state = 0;
    // Optional post-power observation for an already confirmed member. Unlike
    // refinement, failure of retained power may skip it without losing identity.
    refine_fn_t observe_after_power = nullptr;
    // Some controllers still answer after a partial power sequence; preserve
    // their observations without changing the default skip policy.
    bool observe_on_power_failure = false;

    void assign(const board_desc_t* value);
  };

  enum class verdict_t : std::uint8_t { unknown, candidate, confirmed };
  enum class candidate_kind_t : std::uint8_t { none, weak, provisional };
  enum class fail_reason_t : std::uint8_t
  {
    none, no_signature, excluded_by_set,
    family_unresolved, prepare_failed, construct_failed, adopt_failed,
  };

  struct detect_request_t
  {
    const board_id_t* target_set = nullptr;
    board_id_t hint = board_id_unknown;
    board_id_t fallback = board_id_unknown;
    bool allow_reset = false;
    std::uint8_t attempt = 0;
    std::uint8_t max_attempts = 1;
    bool conditional_pins_unavailable = false;
  };

  struct detect_outcome_t
  {
    verdict_t verdict = verdict_t::unknown;
    candidate_kind_t candidate_kind = candidate_kind_t::none;
    board_result_t result;
    fail_reason_t reason = fail_reason_t::none;
    std::uint8_t attempts = 0;
    // Identification and successful adoption remain separate: the session
    // retries a confirmed family when preparation or display adoption fails.
    bool adopted = false;
  };
}
}


namespace m5gfx
{
namespace board_detect
{
  // Classify identification even after preparation fails; unresolved members stay provisional.
  // This classification does not imply the result was adopted.
  inline bool finalize_prepared_result(detect_outcome_t& outcome, board_id_t fallback)
  {
    outcome.verdict = outcome.result.provisional ? verdict_t::candidate : verdict_t::confirmed;
    outcome.candidate_kind = outcome.result.provisional ? candidate_kind_t::provisional : candidate_kind_t::none;
    if (outcome.result.provisional && fallback != board_id_unknown
     && outcome.result.def->id != fallback)
    {
      outcome.result.candidate = outcome.result.def;
      outcome.reason = fail_reason_t::family_unresolved;
      return false;
    }
    return true;
  }

  // Persist only after confirmed adoption, when the board differs from the
  // hint (the saved NVS value, or the build-time board if no value was saved).
  inline bool should_persist_detection(const detect_outcome_t& outcome, std::uint32_t previous_hint)
  {
    return outcome.verdict == verdict_t::confirmed && outcome.adopted
        && previous_hint != outcome.result.def->id;
  }

  template <typename RunAttempt>
  __attribute__((always_inline)) inline detect_outcome_t run_detection_session(
    detect_request_t request, RunAttempt run_attempt)
  {
    detect_outcome_t outcome;
    const board_def_t* candidate = nullptr;
    candidate_kind_t kind = candidate_kind_t::none;
    std::uint8_t attempts = 0;
    for (unsigned attempt = request.attempt; attempt < request.max_attempts; ++attempt)
    {
      request.attempt = static_cast<std::uint8_t>(attempt);
      if (attempt + 2 == request.max_attempts) { request.allow_reset = true; }
      const bool final_attempt = attempt + 1 == request.max_attempts;
      outcome = run_attempt(request, final_attempt);
      outcome.attempts = ++attempts;
      if (outcome.adopted) { return outcome; }
      // Failed setup still leaves stronger family evidence than a weak hint.
      if (outcome.candidate_kind == candidate_kind_t::provisional
       && outcome.result.candidate == nullptr)
      { outcome.result.candidate = outcome.result.def; }
      // Later provisional evidence can refine an earlier family representative.
      // Weak suggestions cannot replace it; weak-only candidates keep the first.
      if (outcome.result.candidate != nullptr
       && (candidate == nullptr || outcome.candidate_kind == candidate_kind_t::provisional))
      {
        candidate = outcome.result.candidate;
        kind = outcome.candidate_kind;
      }
      outcome.result.candidate = candidate;
      // Candidate metadata follows its pointer even after confirmed setup fails.
      outcome.candidate_kind = kind;
      if (outcome.verdict != verdict_t::confirmed && candidate != nullptr)
      { outcome.verdict = verdict_t::candidate; }
      // This reason ends the session without another attempt.
      if (outcome.reason == fail_reason_t::no_signature) { return outcome; }
    }
    return outcome;
  }
}
}
