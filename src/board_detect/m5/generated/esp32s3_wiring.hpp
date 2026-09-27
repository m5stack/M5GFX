// Generated from extras/board_spec; do not edit by hand.
// Unspecified values use target-specific unknown or sentinel values.
// Precedence: board value > part default > panel-class default.
#pragma once

#include <cstdint>

namespace m5gfx { namespace board_detect { namespace m5 { namespace wiring {

namespace atoms3 {
  constexpr std::int8_t display_sclk = 17;
  constexpr std::int8_t display_mosi = 21;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 33;
  constexpr std::int8_t display_cs = 15;
  constexpr std::int8_t display_rst = 34;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t reset_gpio = 34;
  constexpr std::int8_t backlight_gpio = 16;
  constexpr std::int8_t hold[] = { 15 };
  constexpr bool touches_opi_pins = true;
} // namespace atoms3

namespace dial {
  constexpr std::int8_t display_sclk = 6;
  constexpr std::int8_t display_mosi = 5;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 4;
  constexpr std::int8_t display_cs = 7;
  constexpr std::int8_t display_rst = 8;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 11;
  constexpr std::int8_t internal_i2c_scl = 12;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t reset_gpio = 8;
  constexpr std::int8_t backlight_gpio = 9;
  constexpr std::int8_t touch_int = 14;
  constexpr std::int8_t hold[] = { 7 };
  constexpr bool touches_opi_pins = false;
} // namespace dial

namespace dinmeter {
  constexpr std::int8_t display_sclk = 6;
  constexpr std::int8_t display_mosi = 5;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 4;
  constexpr std::int8_t display_cs = 7;
  constexpr std::int8_t display_rst = 8;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 11;
  constexpr std::int8_t internal_i2c_scl = 12;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t reset_gpio = 8;
  constexpr std::int8_t backlight_gpio = 9;
  constexpr std::int8_t hold[] = { 7 };
  constexpr bool touches_opi_pins = false;
} // namespace dinmeter

namespace sticks3 {
  constexpr std::int8_t display_sclk = 40;
  constexpr std::int8_t display_mosi = 39;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 45;
  constexpr std::int8_t display_cs = 41;
  constexpr std::int8_t display_rst = 21;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 47;
  constexpr std::int8_t internal_i2c_scl = 48;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t reset_gpio = 21;
  constexpr std::int8_t power_gpio = -1;
  constexpr std::int8_t backlight_gpio = 38;
  constexpr std::int8_t hold[] = { 41 };
  constexpr bool touches_opi_pins = false;
} // namespace sticks3

namespace detection {
  constexpr std::int8_t unconditional_pins[] = { 4, 5, 6, 7, 8, 9, 11, 12, 14, 15, 16, 17, 21, 38, 39, 40, 41, 45, 47, 48 };
  constexpr std::int8_t opi_pins[] = { 33, 34 };
} // namespace detection

} // namespace wiring

namespace generated_options {
namespace atoms3 {
  constexpr std::uint32_t gc9107 = 1u << 0;
  static const char* const names[] = { "gc9107" };
} // namespace atoms3
} // namespace generated_options

} } } // namespace m5gfx::board_detect::m5
