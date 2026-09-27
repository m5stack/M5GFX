// Generated from extras/board_spec; do not edit by hand.
// Unspecified values use target-specific unknown or sentinel values.
// Precedence: board value > part default > panel-class default.
#pragma once

#include <cstdint>

namespace m5gfx { namespace board_detect { namespace m5 { namespace wiring {

namespace unitc6l {
  constexpr std::int8_t display_sclk = 20;
  constexpr std::int8_t display_mosi = 21;
  constexpr std::int8_t display_miso = 22;
  constexpr std::int8_t display_dc = 18;
  constexpr std::int8_t display_cs = 6;
  constexpr std::int8_t display_rst = 15;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 10;
  constexpr std::int8_t internal_i2c_scl = 8;
  constexpr std::int8_t internal_i2c_port = 0;
  constexpr std::int8_t reset_gpio = 15;
  constexpr std::int8_t hold[] = { 6 };
} // namespace unitc6l

namespace nesson1 {
  constexpr std::int8_t display_sclk = 20;
  constexpr std::int8_t display_mosi = 21;
  constexpr std::int8_t display_miso = 22;
  constexpr std::int8_t display_dc = 16;
  constexpr std::int8_t display_cs = 17;
  constexpr std::int8_t display_rst = -1;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 10;
  constexpr std::int8_t internal_i2c_scl = 8;
  constexpr std::int8_t internal_i2c_port = 0;
  constexpr std::int8_t touch_int = 3;
  constexpr std::int8_t hold[] = { 17 };
} // namespace nesson1

namespace detection {
  constexpr std::int8_t unconditional_pins[] = { 3, 6, 8, 10, 15, 16, 17, 18, 20, 21, 22 };
} // namespace detection

} // namespace wiring

} } } // namespace m5gfx::board_detect::m5
