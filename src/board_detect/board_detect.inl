// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "board_detect.hpp"
#include "i2c_bus_probe.hpp"
#include "ops.inl"
#include "m5/generated/gpio_power_hold_board_ids.hpp"
#include "m5/generated/detector_order_constraints.hpp"

#include <cstdio>
#include <cstring>

namespace m5gfx
{
namespace board_detect
{
  // Call after family-specific evidence gates; a hint is eligible only while
  // that member remains possible. Defaults may be absent for open families.
  // Only some chips' detectors use it.
#include "member_selection.inl"

#include "transaction.inl"

#include "detector_runner.inl"

#include "probe_hardware.inl"

#include "startup.inl"

#include "spi_detector.inl"

#include "prepare.inl"
}
}
