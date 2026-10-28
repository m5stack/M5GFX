// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "board_detect.hpp"
#include "probe_types.hpp"
#include "ops.inl"
#include "m5/generated/gpio_power_hold_board_ids.hpp"
#include "m5/generated/detector_order_constraints.hpp"

#include <cstdio>
#include <cstring>

namespace m5gfx
{
namespace board_detect
{

#include "transaction.inl"

#include "detector_runner.inl"

#include "probe_hardware.inl"

#include "prepare.inl"
}
}
