// Generated from extras/board_spec; do not edit by hand.
// Unspecified values use target-specific unknown or sentinel values.
// Precedence: board value > part default > panel-class default.
#pragma once

#include <cstdint>

namespace m5gfx { namespace board_detect { namespace m5 { namespace wiring {

namespace toughc5 {
  constexpr std::int8_t display_sclk = 9;
  constexpr std::int8_t display_mosi = 7;
  constexpr std::int8_t display_miso = 8;
  constexpr std::int8_t display_dc = 26;
  constexpr std::int8_t display_cs = 25;
  constexpr std::int8_t display_rst = -1;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 2;
  constexpr std::int8_t internal_i2c_scl = 3;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t touch_int = -1;
} // namespace toughc5

namespace detection {
  constexpr std::int8_t unconditional_pins[] = { 2, 3, 7, 8, 9, 25, 26 };
} // namespace detection

} // namespace wiring

} } } // namespace m5gfx::board_detect::m5
