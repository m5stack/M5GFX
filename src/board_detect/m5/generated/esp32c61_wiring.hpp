// Generated from extras/board_spec; do not edit by hand.
// Unspecified values use target-specific unknown or sentinel values.
// Precedence: board value > part default > panel-class default.
#pragma once

#include <cstdint>

namespace m5gfx { namespace board_detect { namespace m5 { namespace wiring {

namespace corematrix {
  constexpr std::int8_t internal_i2c_sda = 0;
  constexpr std::int8_t internal_i2c_scl = 1;
  constexpr std::int8_t internal_i2c_port = 0;
} // namespace corematrix

namespace detection {
  constexpr std::int8_t unconditional_pins[] = { 0, 1 };
} // namespace detection

} // namespace wiring

} } } // namespace m5gfx::board_detect::m5
