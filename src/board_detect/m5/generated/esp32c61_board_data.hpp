// Generated from extras/board_spec; do not edit by hand.
// Regenerate in extras/board_spec: node cli/spec.js emit-wiring
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

#include "../setup_sentinels.hpp"

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace corematrix {
constexpr int bus_port = 0;
constexpr std::uint32_t bus_freq_write = 400000;
constexpr std::uint32_t bus_freq_read = 400000;
constexpr std::uint8_t bus_i2c_addr = 0x72;
constexpr std::uint8_t bus_prefix_len = 0;

namespace panel_tm1680 {
  constexpr int width = 16;
  constexpr int height = 16;
  constexpr int memory_width = setup_sentinel::keep_dimension;
  constexpr int memory_height = setup_sentinel::keep_dimension;
  constexpr int offset_x = setup_sentinel::keep_offset;
  constexpr int offset_y = setup_sentinel::keep_offset;
  constexpr int rotation_offset = 0;
  constexpr int invert = setup_sentinel::keep_i8;
  constexpr int readable = setup_sentinel::keep_i8;
} // namespace panel_tm1680

namespace pmic {
  constexpr std::uint8_t i2c_addr = 0x6E;
  constexpr std::uint32_t i2c_freq = 100000;
  constexpr std::uint8_t id_reg = 0x0;
} // namespace pmic

} } } } } // namespace m5gfx::board_detect::m5::specs::corematrix
