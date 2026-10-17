#pragma once
#include "board_detect/board_detect.hpp"
#include <functional>
#include <cassert>
#include <vector>
namespace host_probes {
static bool bus_present = true;
static int expected_sda = -1, expected_scl = -1;
static bool expected_zero_preamble;
static unsigned spi_calls;
static std::function<void(int, int, std::uint8_t)> expected_i2c;
inline void check_i2c(int sda, int scl, std::uint8_t addr) {
  if (expected_i2c) expected_i2c(sda, scl, addr);
  if (expected_sda >= 0) assert(sda == expected_sda);
  if (expected_scl >= 0) assert(scl == expected_scl);
}
static std::function<m5gfx::board_detect::pin_pull_result_t(std::uint64_t, std::uint32_t)> pulls;
static std::function<std::uint32_t(int, int, int, int, int, std::uint8_t)> spi;
static m5gfx::board_detect::dedicated_release_result_t dedicated;
static std::vector<std::uint32_t> spi_data;
static std::size_t cursor;
static unsigned reads, transfers, dedicated_calls, half_us;
static std::function<void()> on_read;
inline void reset() {
  bus_present = true;
  expected_sda = expected_scl = -1;
  expected_i2c = {};
  expected_zero_preamble = false;
  spi_calls = 0;
  pulls = {};
  spi = {};
  dedicated = {};
  spi_data.clear();
  cursor = reads = transfers = dedicated_calls = 0;
  on_read = {};
}
} // namespace host_probes
namespace m5gfx {
namespace board_detect {
bool probe_i2c_bus_present(probe_ctx_t &, int, int) { return host_probes::bus_present; }
bool probe_pin_pullup_low(probe_ctx_t &, int pin) { return !lgfx::gpio_in(pin); }
bool probe_i2c_ack(probe_ctx_t &, int sda, int scl, std::uint8_t addr) {
  host_probes::check_i2c(sda, scl, addr);
  std::uint8_t value = 0;
  return host_hardware::wire && host_hardware::wire(addr, -1, &value, 0, false);
}
bool probe_i2c_read(probe_ctx_t &, int sda, int scl, std::uint8_t addr, std::uint16_t reg,
                    std::uint8_t *data, std::size_t len, std::uint32_t, std::uint32_t, bool) {
  host_probes::check_i2c(sda, scl, addr);
  return host_hardware::wire && host_hardware::wire(addr, reg, data, len, false);
}
pin_pull_result_t probe_pin_pulls(probe_ctx_t &, std::uint64_t mask, std::uint32_t release) {
  if (!host_probes::pulls)
    std::abort();
  return host_probes::pulls(mask, release);
}
pin_pull_result_t recover_held_sda_and_resample(probe_ctx_t &, pin_pull_result_t value,
                                                std::uint64_t, int, int) {
  return value;
}
dedicated_release_result_t probe_dedicated_pin_release(probe_ctx_t &, const std::int8_t *,
                                                       std::uint8_t, std::uint16_t, std::uint8_t,
                                                       std::uint32_t) {
  ++host_probes::dedicated_calls;
  return host_probes::dedicated;
}
void soft_spi_t::init() {}
void soft_spi_t::beginTransaction() {}
void soft_spi_t::endTransaction() {}
void soft_spi_t::writeCommand(std::uint32_t, std::uint_fast8_t) {}
void soft_spi_t::writeData(std::uint32_t, std::uint_fast8_t) {}
std::uint32_t soft_spi_t::transferData(std::uint32_t, std::uint_fast8_t) {
  ++host_probes::transfers;
  return 0;
}
void soft_spi_t::beginRead(std::uint_fast8_t) {}
std::uint32_t soft_spi_t::readData(std::uint_fast8_t) {
  if (host_probes::on_read)
    host_probes::on_read();
  ++host_probes::reads;
  return host_probes::cursor < host_probes::spi_data.size()
             ? host_probes::spi_data[host_probes::cursor++]
             : 0;
}
void soft_spi_t::readBytes(std::uint8_t *data, std::size_t length) {
  for (std::size_t i = 0; i < length; ++i)
    data[i] = readData(8);
}
void soft_spi_t::endRead() {}
std::uint32_t soft_spi_read32(probe_ctx_t &, int sclk, int mosi, int miso, int dc, int cs,
                              std::uint8_t cmd, std::uint8_t, std::uint32_t half, bool zero) {
  ++host_probes::spi_calls;
  if (host_probes::expected_zero_preamble) assert(zero);
  host_probes::half_us = half;
  return host_probes::spi ? host_probes::spi(sclk, mosi, miso, dc, cs, cmd) : 0;
}
} // namespace board_detect
} // namespace m5gfx

namespace host_touch {
static std::uint16_t t4 = 100, t7 = 25;
static bool available = true;
static unsigned scans, clears;
} // namespace host_touch
