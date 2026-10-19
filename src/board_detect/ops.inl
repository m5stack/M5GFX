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
  struct io_backend_context_t
  {
    const int* ports;
    std::size_t port_count;
  };

  inline int io_port(void* context, const i2c_device_t& device)
  {
    auto* value = static_cast<io_backend_context_t*>(context);
    return value != nullptr && device.bus_id < value->port_count ? value->ports[device.bus_id] : -1;
  }

  inline bool io_i2c_read8(void* context, const i2c_device_t& device,
                             std::uint16_t reg, std::uint8_t* value)
  {
    return io::i2c_read_reg8(io_port(context, device), device.addr,
                              static_cast<std::uint8_t>(reg), value, device.freq_hz);
  }

  inline bool io_i2c_write8(void* context, const i2c_device_t& device,
                              std::uint16_t reg, std::uint8_t value)
  {
    // Write the full register byte without reading it first.
    return io::i2c_write_reg8(io_port(context, device), device.addr, static_cast<std::uint8_t>(reg), value, device.freq_hz);
  }

  inline bool io_i2c_write16le(void* context, const i2c_device_t& device,
                                 std::uint16_t reg, std::uint16_t value)
  {
    const std::uint8_t data[] = {
      static_cast<std::uint8_t>(reg), static_cast<std::uint8_t>(value),
      static_cast<std::uint8_t>(value >> 8),
    };
    return io::i2c_write(io_port(context, device), device.addr,
                                        data, sizeof(data), device.freq_hz);
  }

  inline bool io_i2c_ready(void* context, const i2c_device_t& device,
                             std::uint32_t timeout_ms)
  {
    // The backend has no per-call timeout. Do not begin another fixed-time transaction
    // once the runner's deadline is reached. An in-flight attempt's overrun
    // depends on the lower I2C implementation: hardware is about 26 ms, while
    // software I2C may take tens of milliseconds with SCL stuck.
    if (timeout_ms == 0) { return false; }
    const auto port = io_port(context, device);
    return io::i2c_ack(port, device.addr, device.freq_hz, true);
  }

  inline void io_wait_ready_finished(void*, const i2c_device_t& device,
                                       std::uint32_t elapsed_ms, bool ready)
  {
    ESP_LOGD("board_detect", "power wait_ready addr=0x%02x elapsed=%ums ready=%u",
             device.addr, static_cast<unsigned>(elapsed_ms), ready);
  }

  inline bool io_gpio_set_mode(void*, std::uint16_t pin, gpio_mode_t mode)
  {
    io::pin_mode_t native = io::pin_mode_t::output;
    switch (mode)
    {
    case gpio_mode_t::input:          native = io::pin_mode_t::input; break;
    case gpio_mode_t::input_pullup:   native = io::pin_mode_t::input_pullup; break;
    case gpio_mode_t::input_pulldown: native = io::pin_mode_t::input_pulldown; break;
    default: break;
    }
    io::pin_mode(pin, native);
    return true;
  }

  inline bool io_gpio_write(void*, std::uint16_t pin, bool high)
  {
    if (high) { io::pin_high(pin); }
    else      { io::pin_low(pin); }
    return true;
  }

  inline void io_delay_ms(void*, std::uint32_t milliseconds) { io::delay_ms(milliseconds); }
  inline std::uint32_t io_millis(void*) { return io::millis(); }

  inline backend_t io_backend(io_backend_context_t* context)
  {
    return { io_i2c_read8, io_i2c_write8, io_i2c_write16le, io_i2c_ready,
             io_gpio_set_mode, io_gpio_write, io_delay_ms, io_millis, context };
  }
}
}
}
