// Generated from extras/board_spec; do not edit by hand.
// Regenerate in extras/board_spec: node cli/spec.js emit-wiring
// Unspecified values use target-specific unknown or sentinel values.
// Precedence: board value > part default > panel-class default.
#pragma once

#include <cstdint>

#include "../../probe_types.hpp"

namespace m5gfx { namespace board_detect { namespace m5 { namespace wiring {

namespace corep4x {
  constexpr std::int8_t internal_i2c_sda = 11;
  constexpr std::int8_t internal_i2c_scl = 9;
  constexpr std::int8_t internal_i2c_port = 1;
} // namespace corep4x

namespace unitpoep4 {
  constexpr detect_class_expected_t detect_class_expected = { 0x10000180000000ULL, 0x10000080000000ULL, 0x0ULL, 0x100000000ULL, 0x0ULL };
} // namespace unitpoep4

namespace tab5 {
  constexpr detect_class_expected_t detect_class_expected = { 0x400000ULL, 0x0ULL, 0x400000ULL, 0x0ULL, 0x0ULL };
  constexpr std::int8_t internal_i2c_sda = 31;
  constexpr std::int8_t internal_i2c_scl = 32;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t touch_int = 23;
} // namespace tab5

namespace tab5x {
  constexpr detect_class_expected_t detect_class_expected = { 0x400000ULL, 0x0ULL, 0x400000ULL, 0x0ULL, 0x0ULL };
  constexpr std::int8_t internal_i2c_sda = 31;
  constexpr std::int8_t internal_i2c_scl = 32;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t touch_int = 23;
} // namespace tab5x

namespace detection {
  constexpr std::int8_t unconditional_pins[] = { 9, 11, 22, 23, 31, 32, 52 };
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

#include "../setup_sentinels.hpp"

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace corep4x {
constexpr int bus_id = 0;
constexpr std::uint8_t bus_lane_num = 2;
constexpr std::uint16_t bus_lane_mbps = 600;
constexpr std::uint8_t bus_ldo_chan_id = 3;
constexpr std::uint16_t bus_ldo_voltage_mv = 2500;

namespace panel_st7102 {
  constexpr int width = 480;
  constexpr int height = 480;
  constexpr int memory_width = 480;
  constexpr int memory_height = 480;
  constexpr int offset_x = setup_sentinel::keep_offset;
  constexpr int offset_y = setup_sentinel::keep_offset;
  constexpr int rotation_offset = 2;
  constexpr int invert = setup_sentinel::keep_i8;
  constexpr int readable = 0;
  constexpr int rgb_order = 1;
  constexpr std::uint8_t dpi_freq_mhz = 24;
  constexpr std::uint16_t hsync_back_porch = 40;
  constexpr std::uint16_t hsync_pulse_width = 2;
  constexpr std::uint16_t hsync_front_porch = 40;
  constexpr std::uint16_t vsync_back_porch = 8;
  constexpr std::uint16_t vsync_pulse_width = 4;
  constexpr std::uint16_t vsync_front_porch = 200;
} // namespace panel_st7102

namespace pmic {
  constexpr std::uint8_t i2c_addr = 0x6E;
  constexpr std::uint32_t i2c_freq = 100000;
  constexpr std::uint8_t id_reg = 0x0;
} // namespace pmic

namespace touch {
  constexpr int int_pin = 1;
  constexpr std::uint8_t i2c_addr = 0x58;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr int x_min = 0;
  constexpr int x_max = 479;
  constexpr int y_min = 0;
  constexpr int y_max = 479;
  constexpr int rotation_offset = 2;
} // namespace touch

} } } } } // namespace m5gfx::board_detect::m5::specs::corep4x

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace tab5 {
constexpr int bus_id = 0;
constexpr std::uint8_t bus_lane_num = 2;
constexpr std::uint16_t bus_lane_mbps = 1040;
constexpr std::uint8_t bus_ldo_chan_id = 3;
constexpr std::uint16_t bus_ldo_voltage_mv = 2500;

namespace panel_ili9881c {
  constexpr int width = 720;
  constexpr int height = 1280;
  constexpr int memory_width = 720;
  constexpr int memory_height = 1280;
  constexpr int offset_x = setup_sentinel::keep_offset;
  constexpr int offset_y = setup_sentinel::keep_offset;
  constexpr int rotation_offset = 0;
  constexpr int invert = setup_sentinel::keep_i8;
  constexpr int readable = 1;
  constexpr int rgb_order = 1;
  constexpr std::uint8_t dpi_freq_mhz = 80;
  constexpr std::uint16_t hsync_back_porch = 140;
  constexpr std::uint16_t hsync_pulse_width = 40;
  constexpr std::uint16_t hsync_front_porch = 40;
  constexpr std::uint16_t vsync_back_porch = 20;
  constexpr std::uint16_t vsync_pulse_width = 4;
  constexpr std::uint16_t vsync_front_porch = 20;
} // namespace panel_ili9881c

namespace panel_st7121 {
  constexpr int width = 720;
  constexpr int height = 1280;
  constexpr int memory_width = 720;
  constexpr int memory_height = 1280;
  constexpr int offset_x = setup_sentinel::keep_offset;
  constexpr int offset_y = setup_sentinel::keep_offset;
  constexpr int rotation_offset = 0;
  constexpr int invert = setup_sentinel::keep_i8;
  constexpr int readable = 1;
  constexpr int rgb_order = 1;
  constexpr std::uint8_t dpi_freq_mhz = 70;
  constexpr std::uint16_t hsync_back_porch = 40;
  constexpr std::uint16_t hsync_pulse_width = 2;
  constexpr std::uint16_t hsync_front_porch = 40;
  constexpr std::uint16_t vsync_back_porch = 24;
  constexpr std::uint16_t vsync_pulse_width = 20;
  constexpr std::uint16_t vsync_front_porch = 200;
} // namespace panel_st7121

namespace panel_st7123 {
  constexpr int width = 720;
  constexpr int height = 1280;
  constexpr int memory_width = 720;
  constexpr int memory_height = 1280;
  constexpr int offset_x = setup_sentinel::keep_offset;
  constexpr int offset_y = setup_sentinel::keep_offset;
  constexpr int rotation_offset = 0;
  constexpr int invert = setup_sentinel::keep_i8;
  constexpr int readable = 1;
  constexpr int rgb_order = 1;
  constexpr std::uint8_t dpi_freq_mhz = 80;
  constexpr std::uint16_t hsync_back_porch = 40;
  constexpr std::uint16_t hsync_pulse_width = 2;
  constexpr std::uint16_t hsync_front_porch = 40;
  constexpr std::uint16_t vsync_back_porch = 8;
  constexpr std::uint16_t vsync_pulse_width = 2;
  constexpr std::uint16_t vsync_front_porch = 220;
} // namespace panel_st7123

namespace backlight {
  constexpr int pin = 22;
  constexpr std::uint32_t freq = 44100;
  constexpr std::uint8_t channel = 7;
  constexpr bool invert = false;
  constexpr std::uint8_t offset = 0;
} // namespace backlight

namespace touch {
  constexpr int int_pin = 23;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr int x_min = 0;
  constexpr int x_max = 719;
  constexpr int y_min = 0;
  constexpr int y_max = 1279;
  constexpr int rotation_offset = 0;
} // namespace touch

} } } } } // namespace m5gfx::board_detect::m5::specs::tab5
