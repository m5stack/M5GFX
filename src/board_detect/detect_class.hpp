// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once
#include <cstdint>

namespace m5gfx { namespace board_detect {
  struct pin_pull_result_t
  {
    std::uint64_t pulldown_high = 0;
    std::uint64_t pullup_high = 0;
  };

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
