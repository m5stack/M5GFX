// Generated from extras/board_spec; do not edit by hand.
// Unspecified values use target-specific unknown or sentinel values.
// Precedence: board value > part default > panel-class default.
#pragma once

#include <cstdint>
#include "../setup_sentinels.hpp"

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace unitc6l {
constexpr int bus_host = SPI2_HOST;
constexpr std::uint32_t bus_freq_write = 40000000;
constexpr std::uint32_t bus_freq_read = 10000000;
constexpr bool bus_three_wire = true;

namespace panel_ssd1306 {
  constexpr int width = 64;
  constexpr int height = 48;
  constexpr int memory_width = setup_sentinel::keep_dimension;
  constexpr int memory_height = setup_sentinel::keep_dimension;
  constexpr int offset_x = 32;
  constexpr int offset_y = 0;
  constexpr int rotation_offset = 0;
  constexpr int invert = setup_sentinel::keep_i8;
  constexpr int readable = 1;
} // namespace panel_ssd1306

namespace power_hold {
  constexpr bool active_low = false;
} // namespace power_hold

namespace i2c_ioe {
  constexpr std::uint8_t i2c_addr = 0x43;
  constexpr std::uint32_t i2c_freq = 100000;
  constexpr std::uint8_t id_reg = 0x1;
} // namespace i2c_ioe

} } } } } // namespace m5gfx::board_detect::m5::specs::unitc6l

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace nesson1 {
constexpr int bus_host = SPI2_HOST;
constexpr std::uint32_t bus_freq_write = 40000000;
constexpr std::uint32_t bus_freq_read = 16000000;
constexpr bool bus_three_wire = true;

namespace panel_st7789v2 {
  constexpr int width = 135;
  constexpr int height = 240;
  constexpr int memory_width = setup_sentinel::keep_dimension;
  constexpr int memory_height = setup_sentinel::keep_dimension;
  constexpr int offset_x = 52;
  constexpr int offset_y = 40;
  constexpr int rotation_offset = 0;
  constexpr int invert = 1;
  constexpr int readable = 1;
} // namespace panel_st7789v2

namespace probe_st7789v2 {
  constexpr std::uint8_t cmd = 0x4;
  constexpr std::uint32_t mask = 0xFB;
  constexpr std::uint32_t values[] = { 0x81 };
} // namespace probe_st7789v2

namespace backlight_i2c {
  constexpr std::uint8_t i2c_addr = 0x44;
  constexpr std::uint32_t i2c_freq = 400000;
} // namespace backlight_i2c

namespace touch {
  constexpr std::uint8_t i2c_addr = 0x38;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr int x_min = 0;
  constexpr int x_max = 134;
  constexpr int y_min = 0;
  constexpr int y_max = 239;
  constexpr int rotation_offset = 0;
} // namespace touch

namespace i2c_pi4io2 {
  constexpr std::uint8_t i2c_addr = 0x44;
  constexpr std::uint32_t i2c_freq = 100000;
  constexpr std::uint8_t id_reg = 0x1;
} // namespace i2c_pi4io2

namespace i2c_pi4io1 {
  constexpr std::uint8_t i2c_addr = 0x43;
  constexpr std::uint32_t i2c_freq = 100000;
  constexpr std::uint8_t id_reg = 0x1;
} // namespace i2c_pi4io1

} } } } } // namespace m5gfx::board_detect::m5::specs::nesson1
