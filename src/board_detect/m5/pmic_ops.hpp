// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "../ops.hpp"

namespace m5gfx
{
namespace board_detect
{
namespace m5
{
namespace pmic_ops
{
  static constexpr ops::i2c_device_t core2_devices[] = {
    { 400000, 0, 0x34, 0, 1, 0 },
  };
  static constexpr ops::i2c_device_t sticks3_devices[] = {
    { 100000, 0, 0x6E, 0, 1, 0 },
  };

  static constexpr ops::op_t power192[] = {
    ops::i2c_masked8(0, 0x95, 0x84, 0x8D), ops::i2c_bit_on(0, 0x28, 0xF0),
    ops::i2c_bit_on(0, 0x12, 0x04), ops::i2c_bit_off(0, 0x92, 0x07),
    ops::i2c_bit_on(0, 0x96, 0x02), ops::i2c_bit_on(0, 0x94, 0x02),
  };
  static constexpr ops::op_t power2101[] = {
    ops::i2c_bit_on(0, 0x90, 0x08), ops::i2c_bit_on(0, 0x80, 0x05),
    ops::i2c_write8(0, 0x82, 0x12), ops::i2c_write8(0, 0x84, 0x6A),
    ops::i2c_bit_on(0, 0x90, 0x02),
  };
  static constexpr ops::op_t reset192[] = {
    ops::i2c_bit_off(0, 0x96, 0x02), ops::i2c_bit_off(0, 0x94, 0x02),
  };
  static constexpr ops::op_t release192[] = {
    ops::i2c_bit_on(0, 0x96, 0x02), ops::i2c_bit_on(0, 0x94, 0x02),
  };
  static constexpr ops::op_t reset2101[] = {
    ops::i2c_bit_off(0, 0x90, 0x02),
  };
  static constexpr ops::op_t release2101[] = {
    ops::i2c_bit_on(0, 0x90, 0x02),
  };
  static constexpr ops::op_t sticks3_power_on[] = {
    ops::i2c_write8(0, 0x09, 0x00),
    ops::i2c_bit_off(0, 0x16, 0x04),
    ops::i2c_bit_on(0, 0x10, 0x04),
    ops::i2c_bit_off(0, 0x13, 0x04),
    ops::i2c_bit_on(0, 0x11, 0x04),
  };
}
}
}
}
