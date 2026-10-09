// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "../board_detect.hpp"
#include "board_registry.inl"
#include "generated/esp32c6_wiring.hpp"
#include "generated/esp32c6_specs.hpp"
#include "pmic_ops.hpp"

namespace m5gfx
{
namespace board_detect
{
namespace m5
{
#include "esp32c6/c6_displayless.inl"
#include "esp32c6/c6_display.inl"
}
}
}
