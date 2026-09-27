// Generated from extras/board_spec; do not edit by hand.
#pragma once

#include <cstdint>

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace atoms3 {
constexpr int bus_host = SPI3_HOST;
constexpr std::uint32_t bus_freq_write = 40000000;
constexpr std::uint32_t bus_freq_read = 16000000;
constexpr bool bus_three_wire = true;

namespace panel_st7735s {
  constexpr int width = 128;
  constexpr int height = 128;
  constexpr int memory_width = 132;
  constexpr int memory_height = 132;
  constexpr int offset_x = 2;
  constexpr int offset_y = 1;
  constexpr int rotation_offset = 2;
  constexpr int invert = 1;
  constexpr int readable = 1;
} // namespace panel_st7735s

namespace panel_gc9107 {
  constexpr int width = 128;
  constexpr int height = 128;
  constexpr int memory_width = 128;
  constexpr int memory_height = 160;
  constexpr int offset_x = 0;
  constexpr int offset_y = 32;
  constexpr int rotation_offset = 0;
  constexpr int invert = 0;
  constexpr int readable = 0;
} // namespace panel_gc9107

namespace probe_st7735s {
  constexpr std::uint8_t cmd = 0x4;
  constexpr std::uint32_t mask = 0xFFFF;
  constexpr std::uint32_t values[] = { 0x7683, 0x897C };
} // namespace probe_st7735s

namespace probe_gc9107 {
  constexpr std::uint8_t cmd = 0x4;
  constexpr std::uint32_t mask = 0xFFFFFF;
  constexpr std::uint32_t values[] = { 0x79100 };
} // namespace probe_gc9107

namespace backlight {
  constexpr std::uint32_t freq = 256;
  constexpr std::uint8_t channel = 7;
  constexpr bool invert = false;
  constexpr std::uint8_t offset = 48;
} // namespace backlight

} } } } } // namespace m5gfx::board_detect::m5::specs::atoms3

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace sticks3 {
constexpr int bus_host = SPI3_HOST;
constexpr std::uint32_t bus_freq_write = 40000000;
constexpr std::uint32_t bus_freq_read = 16000000;
constexpr bool bus_three_wire = true;

namespace panel_st7789v2 {
  constexpr int width = 135;
  constexpr int height = 240;
  constexpr int memory_width = 0;
  constexpr int memory_height = 0;
  constexpr int offset_x = 52;
  constexpr int offset_y = 40;
  constexpr int rotation_offset = 0;
  constexpr int invert = 1;
  constexpr int readable = 1;
} // namespace panel_st7789v2

namespace backlight {
  constexpr std::uint32_t freq = 256;
  constexpr std::uint8_t channel = 7;
  constexpr bool invert = false;
  constexpr std::uint8_t offset = 16;
} // namespace backlight

namespace pmic {
  constexpr std::uint8_t i2c_addr = 0x6E;
  constexpr std::uint32_t i2c_freq = 100000;
  constexpr std::uint8_t id_reg = 0x0;
} // namespace pmic

} } } } } // namespace m5gfx::board_detect::m5::specs::sticks3
