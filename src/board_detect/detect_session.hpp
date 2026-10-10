// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once
#include "detect_types.hpp"

namespace m5gfx
{
namespace board_detect
{
  // Classify after preparation: refinement may retain an unresolved member.
  inline bool finalize_prepared_result(detect_outcome_t& outcome, board_id_t preferred)
  {
    outcome.verdict = outcome.result.provisional ? verdict_t::candidate : verdict_t::confirmed;
    outcome.candidate_kind = outcome.result.provisional ? candidate_kind_t::provisional : candidate_kind_t::none;
    if (outcome.result.provisional && preferred != board_id_unknown
     && outcome.result.def->id != preferred)
    {
      outcome.result.candidate = outcome.result.def;
      outcome.reason = fail_reason_t::family_unresolved;
      return false;
    }
    return true;
  }

  inline bool should_persist_detection(const detect_outcome_t& outcome, std::uint32_t previous)
  {
    return outcome.verdict == verdict_t::confirmed && outcome.setup_succeeded
        && previous != outcome.result.def->id;
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
      if (outcome.setup_succeeded) { return outcome; }
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
      // A mismatched preference retries until the final attempt. A retained
      // power failure skips member selection; a retry can run refinement once
      // power preparation succeeds and apply the observed member preference.
      if (final_attempt && outcome.reason == fail_reason_t::family_unresolved
       && outcome.candidate_kind == candidate_kind_t::provisional) { return outcome; }
      if (outcome.reason == fail_reason_t::no_signature) { return outcome; }
    }
    return outcome;
  }
}
}
