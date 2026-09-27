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

static const board_detector_t* const esp32p4_detectors[] = {
  &corep4x_detector, &tab5_family_detector, nullptr
};
static const board_entry_t esp32p4_boards[] = {
  { &desc_corep4x, construct_corep4x, "board_M5CoreP4X", nullptr },
  { &desc_tab5, construct_tab5, "board_M5Tab5", nullptr },
  { &desc_tab5x, construct_tab5, "board_M5Tab5X", nullptr },
};
success_log_t success_log(const board_result_t& result) { return success_log(esp32p4_boards, result); }
}
}
}
