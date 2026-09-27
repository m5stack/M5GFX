// Generated from extras/board_spec; do not edit by hand.
// Unspecified values use target-specific unknown or sentinel values.
// Precedence: board value > part default > panel-class default.
#pragma once

#include <cstdint>
#include "../setup_sentinels.hpp"

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace stickc {
constexpr int bus_host = SPI2_HOST;
constexpr std::uint32_t bus_freq_write = 27000000;
constexpr std::uint32_t bus_freq_read = 14000000;
constexpr bool bus_three_wire = true;

namespace panel_st7735s {
  constexpr int width = 80;
  constexpr int height = 160;
  constexpr int memory_width = 132;
  constexpr int memory_height = 162;
  constexpr int offset_x = 26;
  constexpr int offset_y = 1;
  constexpr int rotation_offset = 2;
  constexpr int invert = 1;
  constexpr int readable = setup_sentinel::keep_i8;
} // namespace panel_st7735s

namespace backlight_i2c {
  constexpr std::uint8_t i2c_addr = 0x34;
} // namespace backlight_i2c

namespace pmic {
  constexpr std::uint8_t i2c_addr = 0x34;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr std::uint8_t id_reg = 0x3;
  constexpr std::uint8_t id_value = 0x3;
} // namespace pmic

} } } } } // namespace m5gfx::board_detect::m5::specs::stickc

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace stickcplus {
constexpr int bus_host = SPI2_HOST;
constexpr std::uint32_t bus_freq_write = 40000000;
constexpr std::uint32_t bus_freq_read = 15000000;
constexpr bool bus_three_wire = true;

namespace panel_st7789v2 {
  constexpr int width = 135;
  constexpr int height = 240;
  constexpr int memory_width = setup_sentinel::keep_dimension;
  constexpr int memory_height = setup_sentinel::keep_dimension;
  constexpr int offset_x = 52;
  constexpr int offset_y = 40;
  constexpr int rotation_offset = setup_sentinel::keep_u8;
  constexpr int invert = 1;
  constexpr int readable = setup_sentinel::keep_i8;
} // namespace panel_st7789v2

namespace probe_st7789v2 {
  constexpr std::uint8_t cmd = 0x4;
  constexpr std::uint32_t mask = 0xFB;
  constexpr std::uint32_t values[] = { 0x81 };
} // namespace probe_st7789v2

namespace backlight_i2c {
  constexpr std::uint8_t i2c_addr = 0x34;
} // namespace backlight_i2c

namespace pmic {
  constexpr std::uint8_t i2c_addr = 0x34;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr std::uint8_t id_reg = 0x3;
  constexpr std::uint8_t id_value = 0x3;
} // namespace pmic

} } } } } // namespace m5gfx::board_detect::m5::specs::stickcplus

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace coreink {
constexpr int bus_host = SPI3_HOST;
constexpr std::uint32_t bus_freq_write = 40000000;
constexpr std::uint32_t bus_freq_read = 16000000;
constexpr bool bus_three_wire = true;

namespace panel_gdew0154d67 {
  constexpr int width = 200;
  constexpr int height = 200;
  constexpr int memory_width = setup_sentinel::keep_dimension;
  constexpr int memory_height = setup_sentinel::keep_dimension;
  constexpr int offset_x = setup_sentinel::keep_offset;
  constexpr int offset_y = setup_sentinel::keep_offset;
  constexpr int rotation_offset = setup_sentinel::keep_u8;
  constexpr int invert = setup_sentinel::keep_i8;
  constexpr int readable = setup_sentinel::keep_i8;
} // namespace panel_gdew0154d67

namespace panel_gdew0154m09 {
  constexpr int width = 200;
  constexpr int height = 200;
  constexpr int memory_width = setup_sentinel::keep_dimension;
  constexpr int memory_height = setup_sentinel::keep_dimension;
  constexpr int offset_x = setup_sentinel::keep_offset;
  constexpr int offset_y = setup_sentinel::keep_offset;
  constexpr int rotation_offset = setup_sentinel::keep_u8;
  constexpr int invert = setup_sentinel::keep_i8;
  constexpr int readable = setup_sentinel::keep_i8;
} // namespace panel_gdew0154m09

namespace probe_gdew0154d67 {
  constexpr std::uint8_t cmd = 0x2F;
  constexpr std::uint8_t dummy_bits = 0;
  constexpr std::uint32_t mask = 0xFFFFFFFF;
  constexpr std::uint32_t values[] = { 0x10001 };
} // namespace probe_gdew0154d67

namespace probe_gdew0154m09 {
  constexpr std::uint8_t cmd = 0x70;
  constexpr std::uint8_t dummy_bits = 0;
  constexpr std::uint32_t mask = 0xFFFF00FF;
  constexpr std::uint32_t values[] = { 0xF00000 };
} // namespace probe_gdew0154m09

} } } } } // namespace m5gfx::board_detect::m5::specs::coreink

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace stickcplus2 {
constexpr int bus_host = SPI2_HOST;
constexpr std::uint32_t bus_freq_write = 40000000;
constexpr std::uint32_t bus_freq_read = 15000000;
constexpr bool bus_three_wire = true;

namespace panel_st7789v2 {
  constexpr int width = 135;
  constexpr int height = 240;
  constexpr int memory_width = setup_sentinel::keep_dimension;
  constexpr int memory_height = setup_sentinel::keep_dimension;
  constexpr int offset_x = 52;
  constexpr int offset_y = 40;
  constexpr int rotation_offset = setup_sentinel::keep_u8;
  constexpr int invert = 1;
  constexpr int readable = setup_sentinel::keep_i8;
} // namespace panel_st7789v2

namespace probe_st7789v2 {
  constexpr std::uint8_t cmd = 0x4;
  constexpr std::uint32_t mask = 0xFB;
  constexpr std::uint32_t values[] = { 0x81 };
} // namespace probe_st7789v2

namespace backlight {
  constexpr std::uint32_t freq = 256;
  constexpr std::uint8_t channel = 7;
  constexpr bool invert = false;
  constexpr std::uint8_t offset = 40;
} // namespace backlight

} } } } } // namespace m5gfx::board_detect::m5::specs::stickcplus2
