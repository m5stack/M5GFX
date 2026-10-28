// Generated from extras/board_spec; do not edit by hand.
// Regenerate in extras/board_spec: node cli/spec.js emit-wiring
// Unspecified values use target-specific unknown or sentinel values.
// Precedence: board value > part default > panel-class default.
#pragma once

#include <cstdint>

#include "../../probe_types.hpp"

namespace m5gfx { namespace board_detect { namespace m5 { namespace wiring {

namespace cores3 {
  constexpr std::int8_t display_sclk = 36;
  constexpr std::int8_t display_mosi = 37;
  constexpr std::int8_t display_miso = 35;
  constexpr std::int8_t display_dc = 35;
  constexpr std::int8_t display_cs = 3;
  constexpr std::int8_t display_rst = -1;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t shared_sd_sclk = 36;
  constexpr std::int8_t shared_sd_mosi = 37;
  constexpr std::int8_t shared_sd_miso = 35;
  constexpr std::int8_t shared_sd_sd_cs = 4;
  constexpr std::int8_t shared_sd_other_cs = 3;
  constexpr std::int8_t internal_i2c_sda = 12;
  constexpr std::int8_t internal_i2c_scl = 11;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t touch_int = 21;
  constexpr std::int8_t camera_d4 = 15;
  constexpr std::int8_t camera_d5 = 16;
  constexpr std::int8_t camera_href = 38;
  constexpr std::int8_t camera_d0 = 39;
  constexpr std::int8_t camera_d1 = 40;
  constexpr std::int8_t camera_d2 = 41;
  constexpr std::int8_t camera_d3 = 42;
  constexpr std::int8_t camera_pclk = 45;
  constexpr std::int8_t camera_vsync = 46;
  constexpr std::int8_t camera_d7 = 47;
  constexpr std::int8_t camera_d6 = 48;
  constexpr std::int8_t hold[] = { 4, 3 };
  constexpr bool touches_opi_pins = true;
} // namespace cores3

namespace cores3se {
  constexpr std::int8_t display_sclk = 36;
  constexpr std::int8_t display_mosi = 37;
  constexpr std::int8_t display_miso = 35;
  constexpr std::int8_t display_dc = 35;
  constexpr std::int8_t display_cs = 3;
  constexpr std::int8_t display_rst = -1;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t shared_sd_sclk = 36;
  constexpr std::int8_t shared_sd_mosi = 37;
  constexpr std::int8_t shared_sd_miso = 35;
  constexpr std::int8_t shared_sd_sd_cs = 4;
  constexpr std::int8_t shared_sd_other_cs = 3;
  constexpr std::int8_t internal_i2c_sda = 12;
  constexpr std::int8_t internal_i2c_scl = 11;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t touch_int = 21;
  constexpr std::int8_t hold[] = { 4, 3 };
  constexpr bool touches_opi_pins = true;
} // namespace cores3se

namespace stackchan {
  constexpr std::int8_t display_sclk = 36;
  constexpr std::int8_t display_mosi = 37;
  constexpr std::int8_t display_miso = 35;
  constexpr std::int8_t display_dc = 35;
  constexpr std::int8_t display_cs = 3;
  constexpr std::int8_t display_rst = -1;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t shared_sd_sclk = 36;
  constexpr std::int8_t shared_sd_mosi = 37;
  constexpr std::int8_t shared_sd_miso = 35;
  constexpr std::int8_t shared_sd_sd_cs = 4;
  constexpr std::int8_t shared_sd_other_cs = 3;
  constexpr std::int8_t internal_i2c_sda = 12;
  constexpr std::int8_t internal_i2c_scl = 11;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t touch_int = 21;
  constexpr std::int8_t hold[] = { 4, 3 };
  constexpr bool touches_opi_pins = true;
} // namespace stackchan

namespace atoms3 {
  constexpr std::int8_t display_sclk = 17;
  constexpr std::int8_t display_mosi = 21;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 33;
  constexpr std::int8_t display_cs = 15;
  constexpr std::int8_t display_rst = 34;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t reset_gpio = 34;
  constexpr std::int8_t backlight_gpio = 16;
  constexpr std::int8_t hold[] = { 15 };
  constexpr bool touches_opi_pins = true;
} // namespace atoms3

namespace atoms3lite {
  constexpr bool touches_opi_pins = false;
} // namespace atoms3lite

namespace atoms3u {
  constexpr bool touches_opi_pins = false;
} // namespace atoms3u

namespace capsule {
  constexpr std::int8_t internal_i2c_sda = 8;
  constexpr std::int8_t internal_i2c_scl = 10;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t power_gpio = 46;
  constexpr bool touches_opi_pins = false;
} // namespace capsule

namespace powerhub {
  constexpr std::int8_t internal_i2c_sda = 45;
  constexpr std::int8_t internal_i2c_scl = 48;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr bool touches_opi_pins = false;
} // namespace powerhub

namespace dualkey {
  constexpr bool touches_opi_pins = false;
} // namespace dualkey

namespace stamps3 {
  constexpr bool touches_opi_pins = false;
} // namespace stamps3

namespace atoms3rcam {
  constexpr std::int8_t internal_i2c_sda = 45;
  constexpr std::int8_t internal_i2c_scl = 0;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr bool touches_opi_pins = false;
} // namespace atoms3rcam

namespace atoms3rext {
  constexpr std::int8_t internal_i2c_sda = 45;
  constexpr std::int8_t internal_i2c_scl = 0;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr bool touches_opi_pins = false;
} // namespace atoms3rext

namespace atomvoices3r {
  constexpr std::int8_t internal_i2c_sda = 45;
  constexpr std::int8_t internal_i2c_scl = 0;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr bool touches_opi_pins = false;
} // namespace atomvoices3r

namespace stamps3bat {
  constexpr std::int8_t internal_i2c_sda = 48;
  constexpr std::int8_t internal_i2c_scl = 47;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr bool touches_opi_pins = false;
} // namespace stamps3bat

namespace stamps3mini {
  constexpr std::int8_t internal_i2c_sda = 38;
  constexpr std::int8_t internal_i2c_scl = 39;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr bool touches_opi_pins = false;
} // namespace stamps3mini

namespace atoms3r {
  constexpr std::int8_t display_sclk = 15;
  constexpr std::int8_t display_mosi = 21;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 42;
  constexpr std::int8_t display_cs = 14;
  constexpr std::int8_t display_rst = 48;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 45;
  constexpr std::int8_t internal_i2c_scl = 0;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t reset_gpio = 48;
  constexpr std::int8_t hold[] = { 14 };
  constexpr bool touches_opi_pins = false;
} // namespace atoms3r

namespace dial {
  constexpr std::int8_t display_sclk = 6;
  constexpr std::int8_t display_mosi = 5;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 4;
  constexpr std::int8_t display_cs = 7;
  constexpr std::int8_t display_rst = 8;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 11;
  constexpr std::int8_t internal_i2c_scl = 12;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t reset_gpio = 8;
  constexpr std::int8_t power_gpio = 46;
  constexpr std::int8_t backlight_gpio = 9;
  constexpr std::int8_t touch_int = 14;
  constexpr std::int8_t hold[] = { 7 };
  constexpr bool touches_opi_pins = false;
} // namespace dial

namespace dinmeter {
  constexpr std::int8_t display_sclk = 6;
  constexpr std::int8_t display_mosi = 5;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 4;
  constexpr std::int8_t display_cs = 7;
  constexpr std::int8_t display_rst = 8;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 11;
  constexpr std::int8_t internal_i2c_scl = 12;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t reset_gpio = 8;
  constexpr std::int8_t power_gpio = 46;
  constexpr std::int8_t backlight_gpio = 9;
  constexpr std::int8_t hold[] = { 7 };
  constexpr bool touches_opi_pins = false;
} // namespace dinmeter

namespace airq {
  constexpr detect_class_expected_t detect_class_expected = { 0x4ULL, 0x0ULL, 0x0ULL, 0x4ULL, 0x0ULL };
  constexpr std::int8_t display_sclk = 5;
  constexpr std::int8_t display_mosi = 6;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 3;
  constexpr std::int8_t display_cs = 4;
  constexpr std::int8_t display_rst = 2;
  constexpr std::int8_t display_busy = 1;
  constexpr std::int8_t reset_gpio = 2;
  constexpr std::int8_t power_gpio = 46;
  constexpr std::int8_t hold[] = { 4 };
  constexpr bool touches_opi_pins = false;
} // namespace airq

namespace stamplc {
  constexpr std::int8_t display_sclk = 7;
  constexpr std::int8_t display_mosi = 8;
  constexpr std::int8_t display_miso = 9;
  constexpr std::int8_t display_dc = 6;
  constexpr std::int8_t display_cs = 12;
  constexpr std::int8_t display_rst = 3;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t shared_sd_sclk = 7;
  constexpr std::int8_t shared_sd_mosi = 8;
  constexpr std::int8_t shared_sd_miso = 9;
  constexpr std::int8_t shared_sd_sd_cs = 10;
  constexpr std::int8_t shared_sd_other_cs = 12;
  constexpr std::int8_t internal_i2c_sda = 13;
  constexpr std::int8_t internal_i2c_scl = 15;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t reset_gpio = 3;
  constexpr std::int8_t hold[] = { 10, 12 };
  constexpr bool touches_opi_pins = false;
} // namespace stamplc

namespace cardputer {
  constexpr std::int8_t display_sclk = 36;
  constexpr std::int8_t display_mosi = 35;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 34;
  constexpr std::int8_t display_cs = 37;
  constexpr std::int8_t display_rst = 33;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t reset_gpio = 33;
  constexpr std::int8_t backlight_gpio = 38;
  constexpr std::int8_t hold[] = { 37 };
  constexpr bool touches_opi_pins = true;
  namespace cardputer_subdivision {
    constexpr std::int8_t sense_pins[] = { 5, 6, 8, 9 };
    constexpr std::uint8_t vameter_i2c_addrs[] = { 0x40, 0x41 };
    constexpr std::int8_t vameter_i2c_sda = 5;
    constexpr std::int8_t vameter_i2c_scl = 6;
  } // namespace cardputer_subdivision
} // namespace cardputer

namespace cardputer_adv {
  constexpr std::int8_t display_sclk = 36;
  constexpr std::int8_t display_mosi = 35;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 34;
  constexpr std::int8_t display_cs = 37;
  constexpr std::int8_t display_rst = 33;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 8;
  constexpr std::int8_t internal_i2c_scl = 9;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t reset_gpio = 33;
  constexpr std::int8_t backlight_gpio = 38;
  constexpr std::int8_t hold[] = { 37 };
  constexpr bool touches_opi_pins = true;
} // namespace cardputer_adv

namespace vameter {
  constexpr std::int8_t display_sclk = 36;
  constexpr std::int8_t display_mosi = 35;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 34;
  constexpr std::int8_t display_cs = 37;
  constexpr std::int8_t display_rst = 33;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 5;
  constexpr std::int8_t internal_i2c_scl = 6;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t reset_gpio = 33;
  constexpr std::int8_t backlight_gpio = 38;
  constexpr std::int8_t hold[] = { 37 };
  constexpr bool touches_opi_pins = true;
} // namespace vameter

namespace sticks3 {
  constexpr std::int8_t display_sclk = 40;
  constexpr std::int8_t display_mosi = 39;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 45;
  constexpr std::int8_t display_cs = 41;
  constexpr std::int8_t display_rst = 21;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 47;
  constexpr std::int8_t internal_i2c_scl = 48;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t reset_gpio = 21;
  constexpr std::int8_t power_gpio = -1;
  constexpr std::int8_t backlight_gpio = 38;
  constexpr std::int8_t hold[] = { 41 };
  constexpr bool touches_opi_pins = false;
} // namespace sticks3

namespace stopwatch {
  constexpr std::int8_t display_sclk = 40;
  constexpr std::int8_t display_mosi = -1;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_io0 = 41;
  constexpr std::int8_t display_io1 = 42;
  constexpr std::int8_t display_io2 = 46;
  constexpr std::int8_t display_io3 = 45;
  constexpr std::int8_t display_dc = -1;
  constexpr std::int8_t display_cs = 39;
  constexpr std::int8_t display_rst = -1;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 47;
  constexpr std::int8_t internal_i2c_scl = 48;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t touch_int = 13;
  constexpr std::int8_t hold[] = { 39 };
  constexpr bool touches_opi_pins = false;
} // namespace stopwatch

namespace papermono {
  constexpr std::int8_t display_sclk = 15;
  constexpr std::int8_t display_mosi = 14;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 17;
  constexpr std::int8_t display_cs = 16;
  constexpr std::int8_t display_rst = -1;
  constexpr std::int8_t display_busy = 18;
  constexpr std::int8_t internal_i2c_sda = 47;
  constexpr std::int8_t internal_i2c_scl = 48;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t touch_int = 4;
  constexpr std::int8_t hold[] = { 16 };
  constexpr bool touches_opi_pins = false;
} // namespace papermono

namespace chaincaptain {
  constexpr std::int8_t display_sclk = 15;
  constexpr std::int8_t display_mosi = 16;
  constexpr std::int8_t display_miso = -1;
  constexpr std::int8_t display_dc = 46;
  constexpr std::int8_t display_cs = 45;
  constexpr std::int8_t display_rst = -1;
  constexpr std::int8_t display_busy = -1;
  constexpr std::int8_t internal_i2c_sda = 3;
  constexpr std::int8_t internal_i2c_scl = 2;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t hold[] = { 45 };
  constexpr bool touches_opi_pins = false;
} // namespace chaincaptain

namespace papercolor {
  constexpr std::int8_t display_sclk = 15;
  constexpr std::int8_t display_mosi = 13;
  constexpr std::int8_t display_miso = 14;
  constexpr std::int8_t display_dc = 43;
  constexpr std::int8_t display_cs = 44;
  constexpr std::int8_t display_rst = 12;
  constexpr std::int8_t display_busy = 11;
  constexpr std::int8_t shared_sd_sclk = 15;
  constexpr std::int8_t shared_sd_mosi = 13;
  constexpr std::int8_t shared_sd_miso = 14;
  constexpr std::int8_t shared_sd_sd_cs = 47;
  constexpr std::int8_t shared_sd_other_cs = 44;
  constexpr std::int8_t internal_i2c_sda = 3;
  constexpr std::int8_t internal_i2c_scl = 2;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t reset_gpio = 12;
  constexpr std::int8_t hold[] = { 47, 44 };
  constexpr bool touches_opi_pins = false;
} // namespace papercolor

namespace papers3 {
  constexpr std::int8_t display_data0 = 6;
  constexpr std::int8_t display_data1 = 14;
  constexpr std::int8_t display_data2 = 7;
  constexpr std::int8_t display_data3 = 12;
  constexpr std::int8_t display_data4 = 9;
  constexpr std::int8_t display_data5 = 11;
  constexpr std::int8_t display_data6 = 8;
  constexpr std::int8_t display_data7 = 10;
  constexpr std::int8_t display_pwr = 46;
  constexpr std::int8_t display_spv = 17;
  constexpr std::int8_t display_ckv = 18;
  constexpr std::int8_t display_sph = 13;
  constexpr std::int8_t display_oe = 45;
  constexpr std::int8_t display_le = 15;
  constexpr std::int8_t display_cl = 16;
  constexpr std::int8_t internal_i2c_sda = 41;
  constexpr std::int8_t internal_i2c_scl = 42;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr std::int8_t power_gpio = 44;
  constexpr std::int8_t touch_int = 48;
  constexpr bool touches_opi_pins = false;
} // namespace papers3

namespace paperdiy {
  constexpr std::int8_t display_data0 = 6;
  constexpr std::int8_t display_data1 = 14;
  constexpr std::int8_t display_data2 = 7;
  constexpr std::int8_t display_data3 = 12;
  constexpr std::int8_t display_data4 = 9;
  constexpr std::int8_t display_data5 = 11;
  constexpr std::int8_t display_data6 = 8;
  constexpr std::int8_t display_data7 = 10;
  constexpr std::int8_t display_pwr = 46;
  constexpr std::int8_t display_spv = 17;
  constexpr std::int8_t display_ckv = 18;
  constexpr std::int8_t display_sph = 13;
  constexpr std::int8_t display_oe = 45;
  constexpr std::int8_t display_le = 15;
  constexpr std::int8_t display_cl = 16;
  constexpr std::int8_t internal_i2c_sda = 41;
  constexpr std::int8_t internal_i2c_scl = 42;
  constexpr std::int8_t internal_i2c_port = 1;
  constexpr bool touches_opi_pins = false;
} // namespace paperdiy

namespace detection {
  constexpr std::int8_t unconditional_pins[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48 };
  constexpr std::int8_t opi_pins[] = { 33, 34, 35, 36, 37 };
} // namespace detection

} // namespace wiring

namespace generated_options {
namespace cores3 {
  constexpr std::uint32_t vbus_5v = 1u << 0;
  constexpr std::uint32_t internal_camera_confirmed = 1u << 1;
  constexpr std::uint32_t lcd_e = 1u << 2;
  static const char* const names[] = { "vbus_5v", "internal_camera_confirmed", "lcd_e" };
} // namespace cores3
namespace cores3se {
  constexpr std::uint32_t vbus_5v = 1u << 0;
  constexpr std::uint32_t reserved = 1u << 1;
  constexpr std::uint32_t lcd_e = 1u << 2;
  static const char* const names[] = { "vbus_5v", "reserved", "lcd_e" };
} // namespace cores3se
namespace stackchan {
  constexpr std::uint32_t vbus_5v = 1u << 0;
  constexpr std::uint32_t reserved = 1u << 1;
  constexpr std::uint32_t lcd_e = 1u << 2;
  static const char* const names[] = { "vbus_5v", "reserved", "lcd_e" };
} // namespace stackchan
namespace atoms3 {
  constexpr std::uint32_t gc9107 = 1u << 0;
  static const char* const names[] = { "gc9107" };
} // namespace atoms3
namespace atoms3r {
  constexpr std::uint32_t gc9107 = 1u << 0;
  static const char* const names[] = { "gc9107" };
} // namespace atoms3r
namespace airq {
  constexpr std::uint32_t m09 = 1u << 0;
  static const char* const names[] = { "m09" };
} // namespace airq
} // namespace generated_options

} } } // namespace m5gfx::board_detect::m5

#include "../setup_sentinels.hpp"

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace cores3 {
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
  constexpr int rotation_offset = setup_sentinel::keep_u8;
  constexpr int invert = setup_sentinel::keep_i8;
  constexpr int readable = setup_sentinel::keep_i8;
} // namespace panel_ili9342c

namespace probe_ili9342c {
  constexpr std::uint8_t cmd = 0x4;
  constexpr std::uint32_t mask = 0xFF;
  constexpr std::uint32_t values[] = { 0xE3 };
} // namespace probe_ili9342c

namespace pmic {
  constexpr std::uint8_t i2c_addr = 0x34;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr std::uint8_t id_reg = 0x3;
  constexpr std::uint8_t id_value = 0x4A;
} // namespace pmic

namespace touch {
  constexpr std::uint8_t i2c_addr = 0x38;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr int x_min = 0;
  constexpr int x_max = 319;
  constexpr int y_min = 0;
  constexpr int y_max = 239;
  constexpr int rotation_offset = setup_sentinel::keep_u8;
} // namespace touch

namespace i2c_io_expander {
  constexpr std::uint8_t i2c_addr = 0x58;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr std::uint8_t id_reg = 0x10;
  constexpr std::uint8_t id_value = 0x23;
} // namespace i2c_io_expander

namespace i2c_camera {
  constexpr std::uint8_t i2c_addr = 0x21;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr std::uint8_t id_reg = 0x0;
  constexpr std::uint8_t id_value = 0x9B;
} // namespace i2c_camera

namespace release_probe {
  constexpr std::int8_t pins[] = { 38, 39, 40, 41, 42, 46, 47, 48 };
  constexpr std::uint16_t reads = 256;
  constexpr std::uint8_t samples = 9;
  constexpr std::uint32_t settle_us = 10;
  constexpr std::uint32_t short_max_ns = 260;
  constexpr std::uint32_t long_min_ns = 330;
} // namespace release_probe

} } } } } // namespace m5gfx::board_detect::m5::specs::cores3

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace cores3se {
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
  constexpr int rotation_offset = setup_sentinel::keep_u8;
  constexpr int invert = setup_sentinel::keep_i8;
  constexpr int readable = setup_sentinel::keep_i8;
} // namespace panel_ili9342c

namespace probe_ili9342c {
  constexpr std::uint8_t cmd = 0x4;
  constexpr std::uint32_t mask = 0xFF;
  constexpr std::uint32_t values[] = { 0xE3 };
} // namespace probe_ili9342c

namespace pmic {
  constexpr std::uint8_t i2c_addr = 0x34;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr std::uint8_t id_reg = 0x3;
  constexpr std::uint8_t id_value = 0x4A;
} // namespace pmic

namespace touch {
  constexpr std::uint8_t i2c_addr = 0x38;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr int x_min = 0;
  constexpr int x_max = 319;
  constexpr int y_min = 0;
  constexpr int y_max = 239;
  constexpr int rotation_offset = setup_sentinel::keep_u8;
} // namespace touch

namespace i2c_io_expander {
  constexpr std::uint8_t i2c_addr = 0x58;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr std::uint8_t id_reg = 0x10;
  constexpr std::uint8_t id_value = 0x23;
} // namespace i2c_io_expander

} } } } } // namespace m5gfx::board_detect::m5::specs::cores3se

namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace stackchan {
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
  constexpr int rotation_offset = setup_sentinel::keep_u8;
  constexpr int invert = setup_sentinel::keep_i8;
  constexpr int readable = setup_sentinel::keep_i8;
} // namespace panel_ili9342c

namespace probe_ili9342c {
  constexpr std::uint8_t cmd = 0x4;
  constexpr std::uint32_t mask = 0xFF;
  constexpr std::uint32_t values[] = { 0xE3 };
} // namespace probe_ili9342c

namespace pmic {
  constexpr std::uint8_t i2c_addr = 0x34;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr std::uint8_t id_reg = 0x3;
  constexpr std::uint8_t id_value = 0x4A;
} // namespace pmic

namespace touch {
  constexpr std::uint8_t i2c_addr = 0x38;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr int x_min = 0;
  constexpr int x_max = 319;
  constexpr int y_min = 0;
  constexpr int y_max = 239;
  constexpr int rotation_offset = setup_sentinel::keep_u8;
} // namespace touch

namespace i2c_io_expander {
  constexpr std::uint8_t i2c_addr = 0x58;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr std::uint8_t id_reg = 0x10;
  constexpr std::uint8_t id_value = 0x23;
} // namespace i2c_io_expander

namespace i2c_camera {
  constexpr std::uint8_t i2c_addr = 0x21;
  constexpr std::uint32_t i2c_freq = 400000;
  constexpr std::uint8_t id_reg = 0x0;
  constexpr std::uint8_t id_value = 0x9B;
} // namespace i2c_camera

namespace i2c_stackchan_ioe {
  constexpr std::uint8_t i2c_addr = 0x6F;
  constexpr std::uint32_t i2c_freq = 100000;
  constexpr std::uint8_t id_reg = 0x0;
  constexpr std::uint8_t firmware_reg = 0x2;
  constexpr std::uint8_t firmware_min = 0x4;
} // namespace i2c_stackchan_ioe

} } } } } // namespace m5gfx::board_detect::m5::specs::stackchan

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
