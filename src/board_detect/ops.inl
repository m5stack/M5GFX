// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "ops.hpp"
#include "detect_io.hpp"

namespace m5gfx
{
namespace board_detect
{
namespace ops
{
  inline int io_port(const int* ports, std::size_t port_count, const i2c_device_t& device)
  {
    return ports != nullptr && device.bus_id < port_count ? ports[device.bus_id] : -1;
  }

  inline void io_wait_ready_finished(const i2c_device_t& device,
                                      std::uint32_t elapsed_ms, bool ready)
  {
    ESP_LOGD("board_detect", "power wait_ready addr=0x%02x elapsed=%ums ready=%u",
             device.addr, static_cast<unsigned>(elapsed_ms), ready);
  }

  inline bool retry_available(const retry_policy_t* policy)
  {
    if (policy == nullptr) { return false; }
    return static_cast<std::int32_t>(io::millis() - policy->deadline_ms) < 0;
  }

  inline const retry_policy_t* operation_retry_policy(
    const i2c_device_t& device,
    const retry_policy_t* policy, retry_policy_t& device_policy)
  {
    if (!(device.flags & i2c_device_retry_transient_nack)) { return policy; }
    // MCU-based PM1/IOE1 slaves can NACK briefly while processing a write.
    device_policy = { io::millis() + transient_nack_retry_ms,
                      transient_nack_retry_interval_ms };
    if (policy != nullptr)
    {
      if (static_cast<std::int32_t>(policy->deadline_ms - device_policy.deadline_ms) > 0)
      { device_policy.deadline_ms = policy->deadline_ms; }
      if (policy->interval_ms < device_policy.interval_ms)
      { device_policy.interval_ms = policy->interval_ms; }
    }
    return &device_policy;
  }

  inline void retry_delay(const retry_policy_t& policy)
  {
    io::delay_ms(policy.interval_ms);
  }

  inline run_result_t run_ops(const int* ports, std::size_t port_count,
                              const i2c_device_t* devices, std::size_t device_count,
                              const op_t* operations, std::size_t count,
                              const gpio_scope_t& gpio_scope,
                              const retry_policy_t* policy = nullptr,
                              void (*wait_ready_finished)(const i2c_device_t&,
                                                          std::uint32_t, bool) = nullptr)
  {
    auto checked = validate_ops(devices, device_count, operations, count, gpio_scope);
    if (checked.status != op_status_t::ok) { return checked; }
    if (policy != nullptr && policy->interval_ms == 0)
    { return result(op_status_t::unsupported, 0); }
    for (std::size_t i = 0; i < count; ++i)
    {
      const auto& operation = operations[i];
      const auto kind = operation.kind();
      if (kind == op_kind_t::delay_ms)
      {
        io::delay_ms(op_access_t::milliseconds(operation));
      }
      else if (kind == op_kind_t::gpio_set_mode)
      {
        io::pin_mode_t mode = io::pin_mode_t::output;
        switch (op_access_t::mode(operation))
        {
        case gpio_mode_t::input:          mode = io::pin_mode_t::input; break;
        case gpio_mode_t::input_pullup:   mode = io::pin_mode_t::input_pullup; break;
        case gpio_mode_t::input_pulldown: mode = io::pin_mode_t::input_pulldown; break;
        default: break;
        }
        io::pin_mode(op_access_t::pin(operation), mode);
      }
      else if (kind == op_kind_t::gpio_write_high || kind == op_kind_t::gpio_write_low)
      {
        const auto pin = op_access_t::pin(operation);
        if (kind == op_kind_t::gpio_write_high) { io::pin_high(pin); }
        else { io::pin_low(pin); }
      }
      else if (kind == op_kind_t::i2c_wait_ready)
      {
        const auto& device = devices[op_access_t::dev(operation)];
        const auto started = io::millis();
        const auto deadline = started + op_access_t::timeout(operation);
        for (;;)
        {
          const auto now = io::millis();
          const auto remaining = static_cast<std::int32_t>(deadline - now) > 0
                               ? deadline - now : 0;
          const auto wire_timeout = device.wire_timeout_ms == 0
                                  ? remaining
                                  : (device.wire_timeout_ms < remaining
                                     ? device.wire_timeout_ms : remaining);
          // Do not start another fixed-time I2C transaction at the deadline.
          // An in-flight call can overrun: about 26 ms on hardware, or tens
          // of milliseconds with SCL stuck in software I2C.
          if (wire_timeout != 0
           && io::i2c_ack(io_port(ports, port_count, device), device.addr, device.freq_hz, true))
          {
            if (wait_ready_finished)
            { wait_ready_finished(device, io::millis() - started, true); }
            break;
          }
          if (static_cast<std::int32_t>(io::millis() - deadline) >= 0)
          {
            if (wait_ready_finished)
            { wait_ready_finished(device, io::millis() - started, false); }
            return result(op_status_t::timeout, i);
          }
          const auto left = deadline - io::millis();
          const auto interval = op_access_t::interval(operation);
          io::delay_ms(interval < left ? interval : left);
        }
      }
      else
      {
        if (kind == op_kind_t::i2c_write16le)
        {
          const auto write = op_access_t::reg16(operation);
          const auto& device = devices[write.dev];
          retry_policy_t device_retry_policy {};
          const auto* operation_policy = operation_retry_policy(
            device, policy, device_retry_policy);
          const std::uint8_t data[] = { static_cast<std::uint8_t>(write.reg),
                                        static_cast<std::uint8_t>(write.value),
                                        static_cast<std::uint8_t>(write.value >> 8) };
          while (!io::i2c_write(io_port(ports, port_count, device), device.addr,
                               data, sizeof(data), device.freq_hz))
          {
            if (!retry_available(operation_policy))
            {
              return result(operation_policy == nullptr ? op_status_t::i2c_nack
                                                         : op_status_t::timeout, i);
            }
            retry_delay(*operation_policy);
          }
          continue;
        }
        const auto write = op_access_t::reg8(operation);
        const auto& device = devices[write.dev];
        retry_policy_t device_retry_policy {};
        const auto* operation_policy = operation_retry_policy(
          device, policy, device_retry_policy);
        for (;;)
        {
          bool ok = false;
          if (kind == op_kind_t::i2c_write8 || write.mask == 0xFF)
          {
            ok = io::i2c_write_reg8(io_port(ports, port_count, device), device.addr,
                                    static_cast<std::uint8_t>(write.reg), write.value, device.freq_hz);
          }
          else
          {
            std::uint8_t old_value = 0;
            if (io::i2c_read_reg8(io_port(ports, port_count, device), device.addr,
                                  static_cast<std::uint8_t>(write.reg), &old_value, device.freq_hz))
            {
              const auto new_value = static_cast<std::uint8_t>(
                (old_value & static_cast<std::uint8_t>(~write.mask))
                | (write.value & write.mask));
              ok = io::i2c_write_reg8(io_port(ports, port_count, device), device.addr,
                                      static_cast<std::uint8_t>(write.reg), new_value, device.freq_hz);
            }
          }
          if (ok) { break; }
          if (!retry_available(operation_policy))
          {
            return result(operation_policy == nullptr ? op_status_t::i2c_nack
                                                       : op_status_t::timeout, i);
          }
          retry_delay(*operation_policy);
        }
      }
    }
    return result(op_status_t::ok, count);
  }
}
}
}
