// Generated from extras/board_spec; do not edit by hand.
// Unspecified values use target-specific unknown or sentinel values.
// Precedence: board value > part default > panel-class default.
#pragma once

#include <cstdint>

namespace m5gfx { namespace board_detect { namespace m5 { namespace wiring {

namespace corep4x {
  constexpr std::int8_t internal_i2c_sda = 11;
  constexpr std::int8_t internal_i2c_scl = 9;
  constexpr std::int8_t internal_i2c_port = 1;
} // namespace corep4x

namespace tab5 {
  constexpr std::int8_t internal_i2c_sda = 31;
  constexpr std::int8_t internal_i2c_scl = 32;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t touch_int = 23;
} // namespace tab5

namespace tab5x {
  constexpr std::int8_t internal_i2c_sda = 31;
  constexpr std::int8_t internal_i2c_scl = 32;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t touch_int = 23;
} // namespace tab5x

namespace detection {
  constexpr std::int8_t unconditional_pins[] = { 9, 11, 23, 31, 32 };
} // namespace detection

} // namespace wiring

namespace generated_options {
namespace tab5 {
  constexpr std::uint32_t st7121 = 1u << 0;
  constexpr std::uint32_t st7123 = 1u << 1;
  static const char* const names[] = { "st7121", "st7123" };
} // namespace tab5
namespace tab5x {
  constexpr std::uint32_t st7121 = 1u << 0;
  constexpr std::uint32_t st7123 = 1u << 1;
  static const char* const names[] = { "st7121", "st7123" };
} // namespace tab5x
} // namespace generated_options

} } } // namespace m5gfx::board_detect::m5
