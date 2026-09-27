// Generated from extras/board_spec; do not edit by hand.
// Unspecified values use target-specific unknown or sentinel values.
// Precedence: board value > part default > panel-class default.
#pragma once

#include <cstdint>

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
  constexpr std::int8_t backlight_gpio = 9;
  constexpr std::int8_t hold[] = { 7 };
  constexpr bool touches_opi_pins = false;
} // namespace dinmeter

namespace airq {
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
  constexpr std::int8_t unconditional_pins[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48 };
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
