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
  static constexpr std::uint32_t pm1_i2c_freq = 100000;
  static constexpr std::uint8_t pm1_i2c_addr = 0x6E;
  static constexpr std::uint8_t ioe1_i2c_addr = 0x4F;
  static constexpr std::uint16_t pm1_device_id = 0x2050;
  static constexpr ops::i2c_device_t core2_devices[] = {
    { 400000, 0, 0x34, 0, 1, 0 },
  };
  static constexpr ops::i2c_device_t pm1_devices[] = {
    { pm1_i2c_freq, 0, pm1_i2c_addr, 0, 1, ops::i2c_device_retry_transient_nack },
  };
  static constexpr ops::i2c_device_t pm1_family_devices[] = {
    { pm1_i2c_freq, 0, pm1_i2c_addr, 0, 1, ops::i2c_device_retry_transient_nack },
    { pm1_i2c_freq, 0, ioe1_i2c_addr, 0, 1, ops::i2c_device_retry_transient_nack },
  };
  static constexpr ops::i2c_device_t cores3_devices[] = {
    { 400000, 0, 0x58, 0, 1, 0 },
    { 400000, 0, 0x34, 0, 1, 0 },
  };
  static constexpr ops::op_t cores3_vbus_off_power_on[] = {
    ops::i2c_bit_on(0, 0x02, 0x05), ops::i2c_bit_on(0, 0x03, 0x03),
    ops::i2c_write8(0, 0x04, 0x18), ops::i2c_write8(0, 0x05, 0x0C),
    ops::i2c_write8(0, 0x11, 0x10), ops::i2c_write8(0, 0x12, 0xFF),
    ops::i2c_write8(0, 0x13, 0xFF), ops::i2c_write8(1, 0x90, 0xBF),
    ops::i2c_write8(1, 0x94, 28), ops::i2c_write8(1, 0x95, 28),
  };
  static constexpr ops::op_t cores3_vbus_5v_power_on[] = {
    ops::i2c_bit_on(0, 0x02, 0x07), ops::i2c_bit_on(0, 0x03, 0x83),
    ops::i2c_write8(0, 0x04, 0x18), ops::i2c_write8(0, 0x05, 0x0C),
    ops::i2c_write8(0, 0x11, 0x10), ops::i2c_write8(0, 0x12, 0xFF),
    ops::i2c_write8(0, 0x13, 0xFF), ops::i2c_write8(1, 0x90, 0xBF),
    ops::i2c_write8(1, 0x94, 28), ops::i2c_write8(1, 0x95, 28),
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
  static constexpr ops::op_t stopwatch_power_on[] = {
    ops::i2c_write8(0, 0x09, 0x00), ops::i2c_write8(0, 0x0A, 0x00),
    ops::i2c_bit_on(0, 0x06, 0x17),
    ops::gpio_set_mode(39, ops::gpio_mode_t::output), ops::gpio_write_high(39),
    ops::i2c_write8(1, 0x23, 0x00), ops::i2c_bit_off(1, 0x13, 0x9D),
    ops::i2c_bit_on(1, 0x03, 0x9D), ops::i2c_bit_on(1, 0x05, 0x99),
    ops::delay_ms(10), ops::i2c_bit_off(1, 0x05, 0x18), ops::delay_ms(8),
    ops::i2c_bit_on(1, 0x05, 0x18), ops::delay_ms(2),
    ops::i2c_bit_off(1, 0x14, 0x02), ops::i2c_bit_on(1, 0x04, 0x02),
    ops::i2c_bit_off(1, 0x06, 0x02),
  };
  static constexpr ops::op_t toughc5_power_on[] = {
    ops::i2c_bit_on(1, 0x05, 0x18), ops::i2c_bit_off(1, 0x13, 0x18),
    ops::i2c_bit_on(1, 0x03, 0x18), ops::i2c_bit_on(0, 0x06, 0x04),
    ops::delay_ms(10), ops::i2c_write8(0, 0x09, 0x00),
    ops::i2c_write8(0, 0x0A, 0x00), ops::i2c_bit_on(0, 0x06, 0x17),
    ops::i2c_write8(1, 0x23, 0x00), ops::i2c_bit_off(1, 0x14, 0x02),
    ops::i2c_bit_on(1, 0x04, 0x02), ops::i2c_bit_on(0, 0x11, 0x04),
    ops::i2c_bit_off(0, 0x16, 0x30), ops::i2c_bit_on(0, 0x10, 0x04),
    ops::i2c_bit_off(0, 0x13, 0x04),
  };
  static constexpr ops::op_t corematrix_power_on[] = {
    ops::i2c_write8(0, 0x09, 0x00), ops::i2c_write8(0, 0x0A, 0x00),
    ops::i2c_write8(1, 0x23, 0x00), ops::i2c_bit_off(1, 0x13, 0x08),
    ops::i2c_bit_on(1, 0x03, 0x08), ops::i2c_bit_on(1, 0x05, 0x08),
    ops::delay_ms(20),
  };
  static constexpr ops::op_t toughc5_reset_assert[] = {
    ops::i2c_bit_off(1, 0x05, 0x08), ops::delay_ms(2),
    ops::i2c_bit_on(1, 0x05, 0x08), ops::delay_ms(10),
    ops::i2c_bit_off(0, 0x11, 0x04),
  };
  static constexpr ops::op_t toughc5_reset_release[] = {
    ops::i2c_bit_on(0, 0x11, 0x04),
  };
  static constexpr ops::op_t papermono_power_on[] = {
    ops::i2c_write8(0, 0x09, 0x00), ops::i2c_write8(0, 0x0A, 0x00),
    ops::i2c_bit_on(0, 0x06, 0x17),
    ops::gpio_set_mode(16, ops::gpio_mode_t::output), ops::gpio_write_high(16),
    ops::i2c_bit_on(1, 0x03, 0x34), ops::i2c_bit_on(1, 0x04, 0x30),
    ops::i2c_bit_on(1, 0x05, 0x04), ops::i2c_bit_on(1, 0x06, 0x30),
    ops::i2c_bit_off(1, 0x13, 0x34), ops::i2c_bit_off(1, 0x14, 0x34),
    ops::i2c_bit_off(1, 0x05, 0x30), ops::delay_ms(8),
    ops::i2c_bit_on(1, 0x05, 0x30), ops::delay_ms(2),
  };
  static constexpr ops::op_t papermono_no_display_power_on[] = {
    ops::i2c_write8(0, 0x09, 0x00), ops::i2c_write8(0, 0x0A, 0x00),
    ops::i2c_bit_on(0, 0x06, 0x17),
  };
  static constexpr ops::op_t chaincaptain_power_on[] = {
    ops::i2c_write8(0, 0x09, 0x00), ops::i2c_write8(0, 0x0A, 0x00),
    ops::i2c_write8(1, 0x23, 0x00),
    ops::i2c_bit_off(1, 0x14, 0x08), ops::i2c_bit_on(1, 0x04, 0x08),
    ops::i2c_bit_on(1, 0x06, 0x08), ops::i2c_bit_off(1, 0x13, 0x01),
    ops::i2c_bit_on(1, 0x03, 0x01),
  };
  static constexpr ops::op_t chaincaptain_reset_assert[] = {
    ops::i2c_bit_off(1, 0x05, 0x01),
  };
  static constexpr ops::op_t chaincaptain_reset_release[] = {
    ops::i2c_bit_on(1, 0x05, 0x01),
  };
  // PaperDIY: M5PM1 GPIO2 drives EPD_PWR.
  static constexpr ops::op_t paperdiy_power_on[] = {
    ops::i2c_write8(0, 0x09, 0x00), ops::i2c_write8(0, 0x0A, 0x00),
    ops::i2c_bit_off(0, 0x16, 0x30), ops::i2c_bit_on(0, 0x10, 0x04),
    ops::i2c_bit_off(0, 0x13, 0x04), ops::i2c_bit_on(0, 0x11, 0x04),
    ops::delay_ms(10),
  };
  static constexpr ops::op_t papercolor_power_on[] = {
    ops::i2c_write8(0, 0x0A, 0x00),
    ops::i2c_bit_off(0, 0x16, 0xC3), ops::i2c_bit_on(0, 0x10, 0x09),
    ops::i2c_bit_off(0, 0x13, 0x09), ops::i2c_bit_on(0, 0x11, 0x09),
    ops::i2c_write8(0, 0x09, 0x00), ops::delay_ms(100),
    ops::gpio_set_mode(44, ops::gpio_mode_t::output), ops::gpio_write_high(44),
  };
}
}
}
}
