// Generated from extras/board_spec; do not edit by hand.
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

} } } } // namespace m5gfx::board_detect::m5::wiring
