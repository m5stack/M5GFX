// Generated from extras/board_spec; do not edit by hand.
// Unspecified values use target-specific unknown or sentinel values.
// Precedence: board value > part default > panel-class default.
#pragma once

#include <cstdint>
#include "../setup_sentinels.hpp"

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace toughc5 {
constexpr int bus_host = SPI2_HOST;
constexpr std::uint32_t bus_freq_write = 40000000;
constexpr std::uint32_t bus_freq_read = 16000000;
constexpr bool bus_three_wire = true;

namespace panel_ili9342c {
  constexpr int width = 320;
  constexpr int height = 240;
  constexpr int memory_width = setup_sentinel::keep_dimension;
  constexpr int memory_height = setup_sentinel::keep_dimension;
  constexpr int offset_x = setup_sentinel::keep_offset;
  constexpr int offset_y = setup_sentinel::keep_offset;
  constexpr int rotation_offset = 0;
  constexpr int invert = 1;
  constexpr int readable = 1;
} // namespace panel_ili9342c

namespace probe_ili9342c {
  constexpr std::uint8_t cmd = 0x4;
  constexpr std::uint32_t mask = 0xFF;
  constexpr std::uint32_t values[] = { 0xE3 };
} // namespace probe_ili9342c

namespace pmic {
  constexpr std::uint8_t i2c_addr = 0x6E;
  constexpr std::uint32_t i2c_freq = 100000;
  constexpr std::uint8_t id_reg = 0x0;
} // namespace pmic

namespace touch {
  constexpr std::uint8_t i2c_addr = 0x2E;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr int x_min = 0;
  constexpr int x_max = 319;
  constexpr int y_min = 0;
  constexpr int y_max = 239;
  constexpr int rotation_offset = setup_sentinel::keep_u8;
} // namespace touch

} } } } } // namespace m5gfx::board_detect::m5::specs::toughc5
