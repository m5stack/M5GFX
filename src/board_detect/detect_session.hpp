// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "detect_types.hpp"

namespace m5gfx
{
namespace board_detect
{
  // Preserve the chip-specific hint and last-attempt policies until S1 aligns
  // them. The gate is a zero-terminated allow-list; an unknown hint is allowed.
  struct detect_session_policy_t
  {
    const board_id_t* hint_gate = nullptr;
    bool keep_initial_hint = false;
    bool suppress_final_attempt = false;
    // Keep a legacy gate rejection before a wider NVS hint is narrowed.
    bool reject_initial_hint = false;
  };

  __attribute__((always_inline)) inline bool session_hint_allowed(const detect_session_policy_t& policy, board_id_t hint)
  {
    if (hint == board_id_unknown || policy.hint_gate == nullptr) { return true; }
    for (auto p = policy.hint_gate; *p != board_id_unknown; ++p)
    {
      if (*p == hint) { return true; }
    }
    return false;
  }

  // RunAttempt owns each GPIO/bus transaction and the prepare/construct/adopt
  // steps. Session state keeps the first weak candidate across failed attempts.
  // Keep constant chip policies visible to the caller under size optimization.
  template <typename RunAttempt>
  __attribute__((always_inline)) inline detect_outcome_t run_detection_session(detect_request_t request,
                                         const detect_session_policy_t& policy,
                                         RunAttempt run_attempt)
  {
    detect_outcome_t outcome;
    const auto initial_hint = request.hint;
    const board_def_t* candidate = nullptr;
    std::uint8_t attempts = 0;
    for (unsigned attempt = request.attempt; attempt < request.max_attempts; ++attempt)
    {
      request.attempt = static_cast<std::uint8_t>(attempt);
      // The legacy loop promotes reset permission on its penultimate attempt.
      if (attempt + 2 == request.max_attempts) { request.allow_reset = true; }
      request.hint = policy.keep_initial_hint ? initial_hint : request.hint;
      const bool final_attempt = !policy.suppress_final_attempt
                              && attempt + 1 == request.max_attempts;
      outcome = {};
      if ((!policy.reject_initial_hint || attempts != 0)
       && session_hint_allowed(policy, request.hint))
      {
        outcome = run_attempt(request, final_attempt);
      }
      outcome.attempts = ++attempts;
      if (candidate == nullptr) { candidate = outcome.result.candidate; }
      if (outcome.setup_succeeded) { return outcome; }
      outcome.result.candidate = candidate;
      if (outcome.verdict == verdict_t::unknown && candidate != nullptr)
      { outcome.verdict = verdict_t::candidate; }
      if (outcome.reason == fail_reason_t::no_signature) { return outcome; }
      // Non-ESP32 legacy callers feed the previous unknown board into retries.
      request.hint = board_id_unknown;
    }
    return outcome;
  }
}
}
