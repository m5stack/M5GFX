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
    // A board with no positive signature. Such members cannot promote their
    // detector through an NVS hint; a detector may expose them as candidates.
    def_flag_fallback = 1 << 0,
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
    // A weak, displayless suggestion returned only when no board is confirmed.
    const board_def_t* candidate = nullptr;
    detect_status_t status = detect_status_t::no_match;
    // Optional read-only member refinement after power preparation.
    refine_fn_t refine = nullptr;
    // Optional post-power observation for an already confirmed member. Unlike
    // refinement, failure of retained power may skip it without losing identity.
    refine_fn_t observe_after_power = nullptr;

    void assign(const board_desc_t* value);
  };

  enum class verdict_t : std::uint8_t { unknown, candidate, confirmed };
  enum class candidate_kind_t : std::uint8_t { none, weak, provisional };
  enum class fail_reason_t : std::uint8_t
  {
    none, no_signature, confirm_transient, confirm_definitive, excluded_by_set,
    family_unresolved, prepare_failed, construct_failed, adopt_failed,
  };

  struct detect_request_t
  {
    const board_id_t* target_set = nullptr;
    board_id_t hint = board_id_unknown;
    board_id_t preferred = board_id_unknown;
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
    // Identification and successful adoption remain separate: legacy init
    // retries a confirmed family when preparation or display adoption fails.
    bool setup_succeeded = false;
  };
}
}
