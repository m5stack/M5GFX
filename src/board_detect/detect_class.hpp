// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once
#include <cstdint>

namespace m5gfx { namespace board_detect {
  struct pin_pull_result_t
  {
    std::uint64_t pulldown_high = 0;
    std::uint64_t pullup_high = 0;
    std::uint64_t pulldown_release_high = 0;
    std::uint64_t pullup_release_high = 0;
    bool release_sampled = false;
  };

  // Catalog detect_class floating follows internal pulls after 10 us only.
  // This floating class additionally requires both biased levels to survive release.
  enum class pull_class_t : std::uint8_t
  {
    up, down, weak_up, weak_down, floating, conflict, pull_dependent,
  };

  // Requires a measured pin in 0..63. Pull-dependent pins without release
  // samples cannot be distinguished as floating or weakly biased.
  inline pull_class_t classify_pin_pull(const pin_pull_result_t& measured, int pin)
  {
    const auto bit = std::uint64_t(1) << pin;
    const bool pd = measured.pulldown_high & bit;
    const bool pu = measured.pullup_high & bit;
    if (pd && pu) { return pull_class_t::up; }
    if (!pd && !pu) { return pull_class_t::down; }
    if (pd && !pu) { return pull_class_t::conflict; }
    if (!measured.release_sampled) { return pull_class_t::pull_dependent; }
    const bool pd_release = measured.pulldown_release_high & bit;
    const bool pu_release = measured.pullup_release_high & bit;
    if (!pd_release && pu_release) { return pull_class_t::floating; }
    if (!pd_release && !pu_release) { return pull_class_t::weak_down; }
    if (pd_release && pu_release) { return pull_class_t::weak_up; }
    return pull_class_t::conflict;
  }

  struct detect_class_expected_t
  {
    std::uint64_t mask;
    std::uint64_t up;
    std::uint64_t down;
    std::uint64_t floating;
    std::uint64_t fixed;
  };

  // Catalog detect_class only on pins whose classification is invariant across
  // cold boots, software resets, wake from sleep and interrupted detection.
  // Driven pins may use fixed only when the driver is always powered. Do not
  // use PMIC-switched pulls or pins without internal pulls. This is a gate
  // before driven probes: board pins exclude candidates; choice soc_pins may
  // select revisions (revision selection is not implemented yet).
  inline bool match_detect_class(const detect_class_expected_t& expected,
                                 const pin_pull_result_t& measured)
  {
    const auto up = measured.pulldown_high & measured.pullup_high;
    const auto down = ~(measured.pulldown_high | measured.pullup_high);
    const auto floating = measured.pullup_high & ~measured.pulldown_high;
    const auto matched = (expected.up & up) | (expected.down & down)
                       | (expected.floating & floating) | (expected.fixed & (up | down));
    return (matched & expected.mask) == expected.mask;
  }
} }
