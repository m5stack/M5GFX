// Copyright (c) M5Stack. All rights reserved.
#pragma once

#include "../board_detect.hpp"
#include "board_registry.inl"
#include "generated/esp32p4_wiring.hpp"
#include "generated/esp32p4_specs.hpp"
#include "pmic_ops.hpp"

namespace m5gfx
{
namespace board_detect
{
namespace m5
{
#include "esp32p4/corep4x.inl"
#include "esp32p4/tab5.inl"
#include "esp32p4/unitpoep4.inl"

// No detector: selectable only through fixed_board.
static constexpr board_desc_t desc_stampp4 = {
  { static_cast<board_id_t>(lgfx::board_M5StampP4), "M5StampP4", 0 },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), no_pins(),
};

// Module revision alone supplies no board evidence; leave defaults to M5Unified.
#include "generated/esp32p4_detector_order.hpp"
static const board_entry_t esp32p4_boards[] = {
  { &desc_corep4x, construct_corep4x, "board_M5CoreP4X", nullptr },
  { &desc_tab5, construct_tab5, "board_M5Tab5", nullptr, tab5_detail::fixed_prepare },
  { &desc_tab5x, construct_tab5, "board_M5Tab5X", nullptr, tab5_detail::fixed_prepare },
  { &desc_unitpoep4, construct_displayless, "board_M5UnitPoEP4", nullptr },
  { &desc_stampp4, construct_displayless, "board_M5StampP4", nullptr },
};
success_log_t success_log(const board_result_t& result) { return success_log(esp32p4_boards, result); }
}
}
}
