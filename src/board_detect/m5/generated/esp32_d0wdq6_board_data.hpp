// Generated from extras/board_spec; do not edit by hand.
// Regenerate in extras/board_spec: node cli/spec.js emit-wiring
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

namespace m5gfx { namespace board_detect { namespace m5 { namespace wiring {

namespace station {
  constexpr std::int8_t display_sclk = 18;
  constexpr std::int8_t display_mosi = 23;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 19;
  constexpr std::int8_t display_cs = 5;
  constexpr std::int8_t display_rst = 15;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 21;
  constexpr std::int8_t internal_i2c_scl = 22;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t reset_gpio = 15;
  constexpr std::int8_t hold[] = { 5 };
} // namespace station

namespace core2 {
  constexpr std::int8_t display_sclk = 18;
  constexpr std::int8_t display_mosi = 23;
  constexpr std::int8_t display_miso = 38;
  constexpr std::int8_t display_dc = 15;
  constexpr std::int8_t display_cs = 5;
  constexpr std::int8_t display_rst = -1;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t shared_sd_sclk = 18;
  constexpr std::int8_t shared_sd_mosi = 23;
  constexpr std::int8_t shared_sd_miso = 38;
  constexpr std::int8_t shared_sd_sd_cs = 4;
  constexpr std::int8_t shared_sd_other_cs = 5;
  constexpr std::int8_t internal_i2c_sda = 21;
  constexpr std::int8_t internal_i2c_scl = 22;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t hold[] = { 4, 5 };
} // namespace core2

namespace tough {
  constexpr std::int8_t display_sclk = 18;
  constexpr std::int8_t display_mosi = 23;
  constexpr std::int8_t display_miso = 38;
  constexpr std::int8_t display_dc = 15;
  constexpr std::int8_t display_cs = 5;
  constexpr std::int8_t display_rst = -1;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t shared_sd_sclk = 18;
  constexpr std::int8_t shared_sd_mosi = 23;
  constexpr std::int8_t shared_sd_miso = 38;
  constexpr std::int8_t shared_sd_sd_cs = 4;
  constexpr std::int8_t shared_sd_other_cs = 5;
  constexpr std::int8_t internal_i2c_sda = 21;
  constexpr std::int8_t internal_i2c_scl = 22;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t hold[] = { 4, 5 };
} // namespace tough

namespace stack {
  constexpr std::int8_t display_sclk = 18;
  constexpr std::int8_t display_mosi = 23;
  constexpr std::int8_t display_miso = 19;
  constexpr std::int8_t display_dc = 27;
  constexpr std::int8_t display_cs = 14;
  constexpr std::int8_t display_rst = 33;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t shared_sd_sclk = 18;
  constexpr std::int8_t shared_sd_mosi = 23;
  constexpr std::int8_t shared_sd_miso = 19;
  constexpr std::int8_t shared_sd_sd_cs = 4;
  constexpr std::int8_t shared_sd_other_cs = 14;
  constexpr std::int8_t reset_gpio = 33;
  constexpr std::int8_t hold[] = { 4, 14 };
} // namespace stack

namespace paper {
  constexpr std::int8_t display_sclk = 14;
  constexpr std::int8_t display_mosi = 12;
  constexpr std::int8_t display_miso = 13;
  constexpr std::int8_t display_dc = -1;
  constexpr std::int8_t display_cs = 15;
  constexpr std::int8_t display_rst = 23;
  constexpr std::int8_t display_busy = 27;
  constexpr std::int8_t shared_sd_sclk = 14;
  constexpr std::int8_t shared_sd_mosi = 12;
  constexpr std::int8_t shared_sd_miso = 13;
  constexpr std::int8_t shared_sd_sd_cs = 4;
  constexpr std::int8_t shared_sd_other_cs = 15;
  constexpr std::int8_t reset_gpio = 23;
  constexpr std::int8_t power_gpio = 2;
  constexpr std::int8_t hold[] = { 4, 15 };
} // namespace paper

namespace stickc {
  constexpr std::int8_t display_sclk = 13;
  constexpr std::int8_t display_mosi = 15;
  constexpr std::int8_t display_miso = 14;
  constexpr std::int8_t display_dc = 23;
  constexpr std::int8_t display_cs = 5;
  constexpr std::int8_t display_rst = 18;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 21;
  constexpr std::int8_t internal_i2c_scl = 22;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t reset_gpio = 18;
  constexpr std::int8_t hold[] = { 5 };
} // namespace stickc

namespace stickcplus {
  constexpr std::int8_t display_sclk = 13;
  constexpr std::int8_t display_mosi = 15;
  constexpr std::int8_t display_miso = 14;
  constexpr std::int8_t display_dc = 23;
  constexpr std::int8_t display_cs = 5;
  constexpr std::int8_t display_rst = 18;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 21;
  constexpr std::int8_t internal_i2c_scl = 22;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t reset_gpio = 18;
  constexpr std::int8_t hold[] = { 5 };
} // namespace stickcplus

namespace coreink {
  constexpr std::int8_t display_sclk = 18;
  constexpr std::int8_t display_mosi = 23;
  constexpr std::int8_t display_miso = 34;
  constexpr std::int8_t display_dc = 15;
  constexpr std::int8_t display_cs = 9;
  constexpr std::int8_t display_rst = 0;
  constexpr std::int8_t display_busy = 4;
  constexpr std::int8_t reset_gpio = 0;
  constexpr std::int8_t power_gpio = 12;
  constexpr std::int8_t hold[] = { 9 };
} // namespace coreink

namespace stickcplus2 {
  constexpr std::int8_t display_sclk = 13;
  constexpr std::int8_t display_mosi = 15;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 14;
  constexpr std::int8_t display_cs = 5;
  constexpr std::int8_t display_rst = 12;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t reset_gpio = 12;
  constexpr std::int8_t power_gpio = 4;
  constexpr std::int8_t backlight_gpio = 27;
  constexpr std::int8_t hold[] = { 5 };
} // namespace stickcplus2

namespace atomlite {
} // namespace atomlite

namespace atomu {
} // namespace atomu

namespace atommatrix {
} // namespace atommatrix

namespace atomvoice {
} // namespace atomvoice

namespace timercam {
  constexpr std::int8_t internal_i2c_sda = 12;
  constexpr std::int8_t internal_i2c_scl = 14;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t power_gpio = 33;
} // namespace timercam

namespace detection {
  constexpr std::int8_t unconditional_pins[] = { 0, 2, 4, 5, 9, 12, 13, 14, 15, 18, 19, 21, 22, 23, 25, 27, 32, 33, 34, 35, 37, 38 };
} // namespace detection

} // namespace wiring

namespace generated_options {
namespace core2 {
  constexpr std::uint32_t new_pmic = 1u << 0;
  constexpr std::uint32_t lcd_e = 1u << 1;
  static const char* const names[] = { "new_pmic", "lcd_e" };
} // namespace core2
namespace tough {
  constexpr std::uint32_t reserved = 1u << 0;
  constexpr std::uint32_t lcd_e = 1u << 1;
  static const char* const names[] = { "reserved", "lcd_e" };
} // namespace tough
namespace stack {
  constexpr std::uint32_t ips = 1u << 0;
  static const char* const names[] = { "ips" };
} // namespace stack
namespace coreink {
  constexpr std::uint32_t m09 = 1u << 0;
  static const char* const names[] = { "m09" };
} // namespace coreink
} // namespace generated_options

} } } // namespace m5gfx::board_detect::m5
