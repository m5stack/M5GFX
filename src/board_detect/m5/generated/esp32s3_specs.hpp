// Generated from extras/board_spec; do not edit by hand.
// Unspecified values use target-specific unknown or sentinel values.
// Precedence: board value > part default > panel-class default.
#pragma once

#include <cstdint>
#include "../setup_sentinels.hpp"

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

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace atoms3r {
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

namespace backlight_i2c {
  constexpr std::uint8_t i2c_addr = 0x30;
} // namespace backlight_i2c

} } } } } // namespace m5gfx::board_detect::m5::specs::atoms3r

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace dial {
constexpr int bus_host = SPI2_HOST;
constexpr std::uint32_t bus_freq_write = 80000000;
constexpr std::uint32_t bus_freq_read = 16000000;
constexpr bool bus_three_wire = true;

namespace panel_gc9a01 {
  constexpr int width = 240;
  constexpr int height = 240;
  constexpr int memory_width = setup_sentinel::keep_dimension;
  constexpr int memory_height = setup_sentinel::keep_dimension;
  constexpr int offset_x = setup_sentinel::keep_offset;
  constexpr int offset_y = setup_sentinel::keep_offset;
  constexpr int rotation_offset = setup_sentinel::keep_u8;
  constexpr int invert = 1;
  constexpr int readable = 0;
} // namespace panel_gc9a01

namespace probe_gc9a01 {
  constexpr std::uint8_t cmd = 0x4;
  constexpr std::uint32_t mask = 0xFFFFFF;
  constexpr std::uint32_t values[] = { 0x19A00 };
} // namespace probe_gc9a01

namespace backlight {
  constexpr std::uint32_t freq = 44100;
  constexpr std::uint8_t channel = 7;
  constexpr bool invert = false;
  constexpr std::uint8_t offset = 0;
} // namespace backlight

namespace touch {
  constexpr std::uint8_t i2c_addr = 0x38;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr int x_min = 0;
  constexpr int x_max = 239;
  constexpr int y_min = 0;
  constexpr int y_max = 239;
  constexpr int rotation_offset = 0;
} // namespace touch

} } } } } // namespace m5gfx::board_detect::m5::specs::dial

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace dinmeter {
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
  constexpr int rotation_offset = 3;
  constexpr int invert = 1;
  constexpr int readable = 1;
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
  constexpr std::uint8_t offset = 16;
} // namespace backlight

} } } } } // namespace m5gfx::board_detect::m5::specs::dinmeter

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace airq {
constexpr int bus_host = SPI2_HOST;
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

} } } } } // namespace m5gfx::board_detect::m5::specs::airq

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace stamplc {
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
  constexpr int rotation_offset = 1;
  constexpr int invert = 1;
  constexpr int readable = 1;
} // namespace panel_st7789v2

namespace probe_st7789v2 {
  constexpr std::uint8_t cmd = 0x4;
  constexpr std::uint32_t mask = 0xFB;
  constexpr std::uint32_t values[] = { 0x81 };
} // namespace probe_st7789v2

namespace backlight_i2c {
  constexpr std::uint8_t i2c_addr = 0x43;
} // namespace backlight_i2c

} } } } } // namespace m5gfx::board_detect::m5::specs::stamplc

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace cardputer {
constexpr int bus_host = SPI3_HOST;
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
  constexpr int rotation_offset = 1;
  constexpr int invert = 1;
  constexpr int readable = 1;
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
  constexpr std::uint8_t offset = 16;
} // namespace backlight

} } } } } // namespace m5gfx::board_detect::m5::specs::cardputer

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace cardputer_adv {
constexpr int bus_host = SPI3_HOST;
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
  constexpr int rotation_offset = 1;
  constexpr int invert = 1;
  constexpr int readable = 1;
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
  constexpr std::uint8_t offset = 16;
} // namespace backlight

} } } } } // namespace m5gfx::board_detect::m5::specs::cardputer_adv

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace vameter {
constexpr int bus_host = SPI3_HOST;
constexpr std::uint32_t bus_freq_write = 40000000;
constexpr std::uint32_t bus_freq_read = 16000000;
constexpr bool bus_three_wire = true;

namespace panel_st7789v2 {
  constexpr int width = 240;
  constexpr int height = 240;
  constexpr int memory_width = setup_sentinel::keep_dimension;
  constexpr int memory_height = setup_sentinel::keep_dimension;
  constexpr int offset_x = 0;
  constexpr int offset_y = 0;
  constexpr int rotation_offset = 0;
  constexpr int invert = 1;
  constexpr int readable = 1;
} // namespace panel_st7789v2

namespace probe_st7789v2 {
  constexpr std::uint8_t cmd = 0x4;
  constexpr std::uint32_t mask = 0xFB;
  constexpr std::uint32_t values[] = { 0x81 };
} // namespace probe_st7789v2

namespace backlight {
  constexpr std::uint32_t freq = 512;
  constexpr std::uint8_t channel = 7;
  constexpr bool invert = false;
  constexpr std::uint8_t offset = 64;
} // namespace backlight

} } } } } // namespace m5gfx::board_detect::m5::specs::vameter

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace sticks3 {
constexpr int bus_host = SPI3_HOST;
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

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace stopwatch {
constexpr int bus_host = SPI2_HOST;
constexpr std::uint32_t bus_freq_write = 80000000;
constexpr std::uint32_t bus_freq_read = 1000000;
constexpr bool bus_three_wire = true;

namespace panel_co5300 {
  constexpr int width = 468;
  constexpr int height = 468;
  constexpr int memory_width = setup_sentinel::keep_dimension;
  constexpr int memory_height = setup_sentinel::keep_dimension;
  constexpr int offset_x = 6;
  constexpr int offset_y = 0;
  constexpr int rotation_offset = 0;
  constexpr int invert = 0;
  constexpr int readable = 0;
} // namespace panel_co5300

namespace pmic {
  constexpr std::uint8_t i2c_addr = 0x6E;
  constexpr std::uint32_t i2c_freq = 100000;
  constexpr std::uint8_t id_reg = 0x0;
} // namespace pmic

namespace touch {
  constexpr std::uint8_t i2c_addr = 0x15;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr int x_min = 0;
  constexpr int x_max = 233;
  constexpr int y_min = 0;
  constexpr int y_max = 233;
  constexpr int rotation_offset = 0;
} // namespace touch

} } } } } // namespace m5gfx::board_detect::m5::specs::stopwatch

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace papermono {
constexpr int bus_host = SPI2_HOST;
constexpr std::uint32_t bus_freq_write = 40000000;
constexpr std::uint32_t bus_freq_read = 10000000;
constexpr bool bus_three_wire = true;

namespace panel_ssd1677 {
  constexpr int width = 800;
  constexpr int height = 480;
  constexpr int memory_width = setup_sentinel::keep_dimension;
  constexpr int memory_height = setup_sentinel::keep_dimension;
  constexpr int offset_x = 0;
  constexpr int offset_y = 0;
  constexpr int rotation_offset = 3;
  constexpr int invert = 0;
  constexpr int readable = 0;
} // namespace panel_ssd1677

namespace backlight_i2c {
  constexpr std::uint8_t i2c_addr = 0x6E;
} // namespace backlight_i2c

namespace pmic {
  constexpr std::uint8_t i2c_addr = 0x6E;
  constexpr std::uint32_t i2c_freq = 100000;
  constexpr std::uint8_t id_reg = 0x0;
} // namespace pmic

namespace touch {
  constexpr std::uint8_t i2c_addr = 0x38;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr int x_min = 0;
  constexpr int x_max = 479;
  constexpr int y_min = 0;
  constexpr int y_max = 799;
  constexpr int rotation_offset = 0;
} // namespace touch

} } } } } // namespace m5gfx::board_detect::m5::specs::papermono

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace chaincaptain {
constexpr int bus_host = SPI2_HOST;
constexpr std::uint32_t bus_freq_write = 40000000;
constexpr std::uint32_t bus_freq_read = 16000000;
constexpr bool bus_three_wire = true;

namespace panel_jd9853 {
  constexpr int width = 240;
  constexpr int height = 240;
  constexpr int memory_width = 240;
  constexpr int memory_height = 320;
  constexpr int offset_x = 0;
  constexpr int offset_y = 0;
  constexpr int rotation_offset = 2;
  constexpr int invert = 0;
  constexpr int readable = 0;
  constexpr int rgb_order = 1;
} // namespace panel_jd9853

namespace backlight_i2c {
  constexpr std::uint8_t i2c_addr = 0x4F;
} // namespace backlight_i2c

namespace pmic {
  constexpr std::uint8_t i2c_addr = 0x6E;
  constexpr std::uint32_t i2c_freq = 100000;
  constexpr std::uint8_t id_reg = 0x0;
} // namespace pmic

} } } } } // namespace m5gfx::board_detect::m5::specs::chaincaptain

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace papercolor {
constexpr int bus_host = SPI2_HOST;
constexpr std::uint32_t bus_freq_write = 4000000;
constexpr std::uint32_t bus_freq_read = 1000000;
constexpr bool bus_three_wire = true;

namespace panel_ed2208 {
  constexpr int width = 400;
  constexpr int height = 600;
  constexpr int memory_width = setup_sentinel::keep_dimension;
  constexpr int memory_height = setup_sentinel::keep_dimension;
  constexpr int offset_x = 0;
  constexpr int offset_y = 0;
  constexpr int rotation_offset = 0;
  constexpr int invert = 0;
  constexpr int readable = 0;
} // namespace panel_ed2208

namespace pmic {
  constexpr std::uint8_t i2c_addr = 0x6E;
  constexpr std::uint32_t i2c_freq = 100000;
  constexpr std::uint8_t id_reg = 0x0;
} // namespace pmic

} } } } } // namespace m5gfx::board_detect::m5::specs::papercolor

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace papers3 {
constexpr std::uint32_t bus_speed = 16000000;
constexpr std::uint8_t bus_width = 8;

namespace panel_ed047tc1 {
  constexpr int width = 960;
  constexpr int height = 540;
  constexpr int memory_width = 960;
  constexpr int memory_height = 540;
  constexpr int offset_x = 0;
  constexpr int offset_y = 0;
  constexpr int rotation_offset = 3;
  constexpr int invert = setup_sentinel::keep_i8;
  constexpr int readable = setup_sentinel::keep_i8;
  constexpr std::uint8_t line_padding = 8;
} // namespace panel_ed047tc1

namespace power_hold {
  constexpr bool active_low = true;
} // namespace power_hold

namespace touch {
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr int x_min = 0;
  constexpr int x_max = 539;
  constexpr int y_min = 0;
  constexpr int y_max = 959;
  constexpr int rotation_offset = 1;
} // namespace touch

} } } } } // namespace m5gfx::board_detect::m5::specs::papers3

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace paperdiy {
constexpr std::uint32_t bus_speed = 16000000;
constexpr std::uint8_t bus_width = 8;

namespace panel_ed047tc1 {
  constexpr int width = 960;
  constexpr int height = 540;
  constexpr int memory_width = 960;
  constexpr int memory_height = 540;
  constexpr int offset_x = 0;
  constexpr int offset_y = 0;
  constexpr int rotation_offset = 3;
  constexpr int invert = setup_sentinel::keep_i8;
  constexpr int readable = setup_sentinel::keep_i8;
  constexpr std::uint8_t line_padding = 8;
} // namespace panel_ed047tc1

namespace pmic {
  constexpr std::uint8_t i2c_addr = 0x6E;
  constexpr std::uint32_t i2c_freq = 100000;
  constexpr std::uint8_t id_reg = 0x0;
} // namespace pmic

} } } } } // namespace m5gfx::board_detect::m5::specs::paperdiy
