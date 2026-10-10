// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "ops.hpp"

namespace m5gfx
{
namespace board_detect
{
namespace ops
{
  struct lgfx_backend_context_t
  {
    const int* ports;
    std::size_t port_count;
  };

  inline int lgfx_port(void* context, const i2c_device_t& device)
  {
    auto* value = static_cast<lgfx_backend_context_t*>(context);
    return value != nullptr && device.bus_id < value->port_count ? value->ports[device.bus_id] : -1;
  }

  inline bool lgfx_i2c_read8(void* context, const i2c_device_t& device,
                             std::uint16_t reg, std::uint8_t* value)
  {
    const auto read = lgfx::i2c::readRegister8(lgfx_port(context, device), device.addr,
                                                static_cast<std::uint8_t>(reg), device.freq_hz);
    if (!read.has_value()) { return false; }
    *value = read.value();
    return true;
  }

  inline bool lgfx_i2c_write8(void* context, const i2c_device_t& device,
                              std::uint16_t reg, std::uint8_t value)
  {
    // mask 0 is lgfx's full-write path and deliberately performs no read.
    return lgfx::i2c::writeRegister8(lgfx_port(context, device), device.addr,
                                     static_cast<std::uint8_t>(reg), value, 0,
                                     device.freq_hz).has_value();
  }

  inline bool lgfx_i2c_write16le(void* context, const i2c_device_t& device,
                                 std::uint16_t reg, std::uint16_t value)
  {
    const std::uint8_t data[] = {
      static_cast<std::uint8_t>(reg), static_cast<std::uint8_t>(value),
      static_cast<std::uint8_t>(value >> 8),
    };
    return lgfx::i2c::transactionWrite(lgfx_port(context, device), device.addr,
                                        data, sizeof(data), device.freq_hz).has_value();
  }

  inline bool lgfx_i2c_ready(void* context, const i2c_device_t& device,
                             std::uint32_t timeout_ms)
  {
    // lgfx has no per-call timeout. Do not begin another fixed-time transaction
    // once the runner's deadline is reached. An in-flight attempt's overrun
    // depends on the lower I2C implementation: hardware is about 26 ms, while
    // software I2C may take tens of milliseconds with SCL stuck.
    if (timeout_ms == 0) { return false; }
    const auto port = lgfx_port(context, device);
    const bool began = lgfx::i2c::beginTransaction(port, device.addr,
                                                    device.freq_hz, false).has_value();
    const bool ended = lgfx::i2c::endTransaction(port).has_value();
    return began && ended;
  }

  inline void lgfx_wait_ready_finished(void*, const i2c_device_t& device,
                                       std::uint32_t elapsed_ms, bool ready)
  {
    ESP_LOGD("board_detect", "power wait_ready addr=0x%02x elapsed=%ums ready=%u",
             device.addr, static_cast<unsigned>(elapsed_ms), ready);
  }

  inline bool lgfx_gpio_set_mode(void*, std::uint16_t pin, gpio_mode_t mode)
  {
    lgfx::pin_mode_t native = lgfx::pin_mode_t::output;
    switch (mode)
    {
    case gpio_mode_t::input:          native = lgfx::pin_mode_t::input; break;
    case gpio_mode_t::input_pullup:   native = lgfx::pin_mode_t::input_pullup; break;
    case gpio_mode_t::input_pulldown: native = lgfx::pin_mode_t::input_pulldown; break;
    default: break;
    }
    lgfx::pinMode(pin, native);
    return true;
  }

  inline bool lgfx_gpio_write(void*, std::uint16_t pin, bool high)
  {
    if (high) { lgfx::gpio_hi(pin); }
    else      { lgfx::gpio_lo(pin); }
    return true;
  }

  inline void lgfx_delay_ms(void*, std::uint32_t milliseconds) { lgfx::delay(milliseconds); }
  inline std::uint32_t lgfx_millis(void*) { return lgfx::millis(); }

  inline backend_t lgfx_backend(lgfx_backend_context_t* context)
  {
    return { lgfx_i2c_read8, lgfx_i2c_write8, lgfx_i2c_write16le, lgfx_i2c_ready,
             lgfx_gpio_set_mode, lgfx_gpio_write, lgfx_delay_ms, lgfx_millis, context };
  }
}
}
}
