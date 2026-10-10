// Generated from extras/board_spec; do not edit by hand.
// Unspecified values use target-specific unknown or sentinel values.
// Precedence: board value > part default > panel-class default.
#pragma once

#include <cstdint>

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
