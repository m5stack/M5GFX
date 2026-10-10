// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include <cstddef>
#include <cstdint>

namespace m5gfx
{
namespace board_detect
{
namespace ops
{
  enum class gpio_mode_t : std::uint8_t
  {
    output,
    input,
    input_pullup,
    input_pulldown,
  };

  struct i2c_device_t
  {
    std::uint32_t freq_hz;
    std::uint32_t wire_timeout_ms;
    std::uint16_t addr;
    std::uint8_t bus_id;
    std::uint8_t register_address_bytes;
    std::uint8_t flags;
  };

  enum i2c_device_flag_t : std::uint8_t
  {
    i2c_device_retry_transient_nack = 1u << 0,
  };

  static constexpr std::uint8_t i2c_device_known_flags = i2c_device_retry_transient_nack;
  static constexpr std::uint16_t transient_nack_retry_ms = 20;
  static constexpr std::uint16_t transient_nack_retry_interval_ms = 1;

  enum class op_kind_t : std::uint8_t
  {
    // Current PMIC descriptions use register writes, delays and GPIO operations;
    // transfer remains a reserved, unsupported operation. wait_ready cannot
    // shorten one lgfx I2C transaction. Deadline overrun depends on the lower I2C implementation
    // (hardware is about 26 ms; software I2C may take tens of milliseconds
    // with SCL stuck), but no new transaction begins at zero remaining time.
    delay_ms,
    gpio_set_mode,
    gpio_write_high,
    gpio_write_low,
    i2c_write8,
    i2c_write16le,
    i2c_masked8,
    i2c_wait_ready,
    i2c_transfer,
  };

  class op_t
  {
  public:
    op_kind_t kind() const { return kind_; }

  private:
    struct reg8_t
    {
      std::uint16_t reg;
      std::uint8_t dev;
      std::uint8_t value;
      std::uint8_t mask;
    };
    struct gpio_t
    {
      std::uint16_t pin;
      std::uint8_t mode;
    };
    struct reg16_t
    {
      std::uint16_t reg;
      std::uint16_t value;
      std::uint8_t dev;
    };
    struct delay_t { std::uint32_t ms; };
    struct wait_t
    {
      std::uint32_t timeout_ms;
      std::uint16_t interval_ms;
      std::uint8_t dev;
    };
    struct transfer_t
    {
      std::uint16_t blob_offset;
      std::uint16_t blob_size;
      std::uint8_t dev;
    };
    union payload_t
    {
      reg8_t reg8;
      reg16_t reg16;
      gpio_t gpio;
      delay_t delay;
      wait_t wait;
      transfer_t transfer;

      constexpr payload_t(reg8_t value) : reg8(value) {}
      constexpr payload_t(reg16_t value) : reg16(value) {}
      constexpr payload_t(gpio_t value) : gpio(value) {}
      constexpr payload_t(delay_t value) : delay(value) {}
      constexpr payload_t(wait_t value) : wait(value) {}
      constexpr payload_t(transfer_t value) : transfer(value) {}
    };

    constexpr op_t(op_kind_t kind, payload_t payload) : kind_(kind), payload_(payload) {}

    op_kind_t kind_;
    payload_t payload_;

    friend constexpr op_t delay_ms(std::uint32_t);
    friend constexpr op_t gpio_set_mode(std::uint16_t, gpio_mode_t);
    friend constexpr op_t gpio_write_high(std::uint16_t);
    friend constexpr op_t gpio_write_low(std::uint16_t);
    friend constexpr op_t i2c_write8(std::uint8_t, std::uint16_t, std::uint8_t);
    friend constexpr op_t i2c_write16le(std::uint8_t, std::uint16_t, std::uint16_t);
    friend constexpr op_t i2c_masked8(std::uint8_t, std::uint16_t, std::uint8_t, std::uint8_t);
    friend constexpr op_t i2c_bit_on(std::uint8_t, std::uint16_t, std::uint8_t);
    friend constexpr op_t i2c_bit_off(std::uint8_t, std::uint16_t, std::uint8_t);
    friend constexpr op_t i2c_wait_ready(std::uint8_t, std::uint32_t, std::uint16_t);
    friend constexpr op_t i2c_transfer(std::uint8_t, std::uint16_t, std::uint16_t);
    friend struct decoded_register_write_t;
    friend struct op_access_t;
  };

  constexpr op_t delay_ms(std::uint32_t ms)
  {
    return op_t(op_kind_t::delay_ms, op_t::payload_t(op_t::delay_t { ms }));
  }

  constexpr op_t gpio_set_mode(std::uint16_t pin, gpio_mode_t mode)
  {
    return op_t(op_kind_t::gpio_set_mode,
                op_t::payload_t(op_t::gpio_t { pin, static_cast<std::uint8_t>(mode) }));
  }

  constexpr op_t gpio_write_high(std::uint16_t pin)
  {
    return op_t(op_kind_t::gpio_write_high,
                op_t::payload_t(op_t::gpio_t { pin, 0 }));
  }

  constexpr op_t gpio_write_low(std::uint16_t pin)
  {
    return op_t(op_kind_t::gpio_write_low,
                op_t::payload_t(op_t::gpio_t { pin, 0 }));
  }

  constexpr op_t i2c_write8(std::uint8_t dev, std::uint16_t reg, std::uint8_t value)
  {
    return op_t(op_kind_t::i2c_write8,
                op_t::payload_t(op_t::reg8_t { reg, dev, value, 0xFF }));
  }

  constexpr op_t i2c_write16le(std::uint8_t dev, std::uint16_t reg, std::uint16_t value)
  {
    return op_t(op_kind_t::i2c_write16le,
                op_t::payload_t(op_t::reg16_t { reg, value, dev }));
  }

  constexpr op_t i2c_masked8(std::uint8_t dev, std::uint16_t reg,
                             std::uint8_t value, std::uint8_t mask)
  {
    return op_t(op_kind_t::i2c_masked8,
                op_t::payload_t(op_t::reg8_t { reg, dev, value, mask }));
  }

  constexpr op_t i2c_bit_on(std::uint8_t dev, std::uint16_t reg, std::uint8_t bits)
  {
    return i2c_masked8(dev, reg, bits, bits);
  }

  constexpr op_t i2c_bit_off(std::uint8_t dev, std::uint16_t reg, std::uint8_t bits)
  {
    return i2c_masked8(dev, reg, 0, bits);
  }

  constexpr op_t i2c_wait_ready(std::uint8_t dev, std::uint32_t timeout_ms,
                                std::uint16_t interval_ms = 1)
  {
    return op_t(op_kind_t::i2c_wait_ready,
                op_t::payload_t(op_t::wait_t { timeout_ms, interval_ms, dev }));
  }

  // Payload bytes live in a companion blob. Execution is deliberately reserved.
  constexpr op_t i2c_transfer(std::uint8_t dev, std::uint16_t blob_offset,
                              std::uint16_t blob_size)
  {
    return op_t(op_kind_t::i2c_transfer,
                op_t::payload_t(op_t::transfer_t { blob_offset, blob_size, dev }));
  }

  struct op_list_t
  {
    const op_t* data;
    std::uint16_t size;
  };

  template <std::size_t N>
  constexpr op_list_t list(const op_t (&data)[N])
  {
    static_assert(N <= 65535, "operation list is too large");
    return { data, static_cast<std::uint16_t>(N) };
  }

  constexpr op_list_t no_ops() { return { nullptr, 0 }; }

  enum class op_status_t : std::uint8_t
  {
    ok,
    invalid_op,
    invalid_device,
    invalid_gpio,
    invalid_mask,
    unsupported,
    i2c_nack,
    i2c_error,
    timeout,
    gpio_error,
  };

  struct run_result_t
  {
    op_status_t status;
    std::uint16_t failed_index;
  };

  struct gpio_scope_t
  {
    std::uint16_t gpio_count;
    const std::int8_t* allowed_pins;
    std::size_t allowed_pin_count;
  };

  struct retry_policy_t
  {
    std::uint32_t deadline_ms;
    std::uint16_t interval_ms;
  };

  struct backend_t
  {
    bool (*i2c_read8)(void*, const i2c_device_t&, std::uint16_t, std::uint8_t*);
    bool (*i2c_write8)(void*, const i2c_device_t&, std::uint16_t, std::uint8_t);
    bool (*i2c_write16le)(void*, const i2c_device_t&, std::uint16_t, std::uint16_t);
    bool (*i2c_ready)(void*, const i2c_device_t&, std::uint32_t);
    bool (*gpio_set_mode)(void*, std::uint16_t, gpio_mode_t);
    bool (*gpio_write)(void*, std::uint16_t, bool);
    void (*delay_ms)(void*, std::uint32_t);
    std::uint32_t (*millis)(void*);
    void* ctx;
  };

  struct decoded_register_write_t
  {
    std::uint8_t dev;
    std::uint16_t reg;
    std::uint8_t value;
    std::uint8_t mask;
  };

  struct decoded_register_write16_t
  {
    std::uint8_t dev;
    std::uint16_t reg;
    std::uint16_t value;
  };

  struct op_access_t
  {
    static std::uint8_t dev(const op_t& op)
    {
      return op.kind_ == op_kind_t::i2c_wait_ready ? op.payload_.wait.dev
           : op.kind_ == op_kind_t::i2c_write16le ? op.payload_.reg16.dev
                                                   : op.payload_.reg8.dev;
    }
    static std::uint16_t pin(const op_t& op) { return op.payload_.gpio.pin; }
    static gpio_mode_t mode(const op_t& op)
    {
      return static_cast<gpio_mode_t>(op.payload_.gpio.mode);
    }
    static std::uint32_t milliseconds(const op_t& op) { return op.payload_.delay.ms; }
    static std::uint32_t timeout(const op_t& op) { return op.payload_.wait.timeout_ms; }
    static std::uint16_t interval(const op_t& op) { return op.payload_.wait.interval_ms; }
    static decoded_register_write_t reg8(const op_t& op)
    {
      return { op.payload_.reg8.dev, op.payload_.reg8.reg,
               op.payload_.reg8.value, op.payload_.reg8.mask };
    }
    static decoded_register_write16_t reg16(const op_t& op)
    {
      return { op.payload_.reg16.dev, op.payload_.reg16.reg, op.payload_.reg16.value };
    }
  };

  inline bool decode_register_write(const op_t& operation,
                                    decoded_register_write_t* decoded)
  {
    if (decoded == nullptr
     || (operation.kind() != op_kind_t::i2c_write8
      && operation.kind() != op_kind_t::i2c_masked8)) { return false; }
    *decoded = op_access_t::reg8(operation);
    return true;
  }

  inline run_result_t result(op_status_t status, std::size_t index)
  {
    return { status, static_cast<std::uint16_t>(index) };
  }

  inline bool gpio_allowed(const gpio_scope_t& scope, std::uint16_t pin)
  {
    if (pin >= scope.gpio_count) { return false; }
    for (std::size_t i = 0; i < scope.allowed_pin_count; ++i)
    {
      if (scope.allowed_pins[i] >= 0
       && static_cast<std::uint16_t>(scope.allowed_pins[i]) == pin) { return true; }
    }
    return false;
  }

  inline run_result_t validate_ops(const i2c_device_t* devices, std::size_t device_count,
                                   const op_t* operations, std::size_t count,
                                   const gpio_scope_t& gpio_scope)
  {
    if ((devices == nullptr && device_count != 0)
     || (operations == nullptr && count != 0)
     || count > 65535) { return result(op_status_t::invalid_op, 0); }
    for (std::size_t i = 0; i < count; ++i)
    {
      const auto kind = operations[i].kind();
      if (kind == op_kind_t::i2c_transfer) { return result(op_status_t::unsupported, i); }
      if (kind == op_kind_t::i2c_write8 || kind == op_kind_t::i2c_write16le
       || kind == op_kind_t::i2c_masked8
       || kind == op_kind_t::i2c_wait_ready)
      {
        const auto dev = op_access_t::dev(operations[i]);
        if (dev >= device_count) { return result(op_status_t::invalid_device, i); }
        const auto& device = devices[dev];
        if (device.register_address_bytes != 1
         || (device.flags & static_cast<std::uint8_t>(~i2c_device_known_flags)) != 0
         || device.addr > 0x7F)
        {
          return result(op_status_t::invalid_device, i);
        }
        if (((kind == op_kind_t::i2c_write8 || kind == op_kind_t::i2c_masked8)
          && op_access_t::reg8(operations[i]).reg > 0xFF)
         || (kind == op_kind_t::i2c_write16le
          && op_access_t::reg16(operations[i]).reg > 0xFF))
        {
          return result(op_status_t::invalid_op, i);
        }
        if (kind == op_kind_t::i2c_wait_ready
         && op_access_t::interval(operations[i]) == 0)
        {
          return result(op_status_t::invalid_op, i);
        }
      }
      if (kind == op_kind_t::i2c_masked8)
      {
        const auto write = op_access_t::reg8(operations[i]);
        if (write.mask == 0 || (write.value & static_cast<std::uint8_t>(~write.mask)) != 0)
        {
          return result(op_status_t::invalid_mask, i);
        }
      }
      if (kind == op_kind_t::gpio_set_mode || kind == op_kind_t::gpio_write_high
       || kind == op_kind_t::gpio_write_low)
      {
        if (!gpio_allowed(gpio_scope, op_access_t::pin(operations[i])))
        {
          return result(op_status_t::invalid_gpio, i);
        }
        if (kind == op_kind_t::gpio_set_mode
         && static_cast<std::uint8_t>(op_access_t::mode(operations[i]))
              > static_cast<std::uint8_t>(gpio_mode_t::input_pulldown))
        {
          return result(op_status_t::invalid_op, i);
        }
      }
      if (static_cast<std::uint8_t>(kind)
        > static_cast<std::uint8_t>(op_kind_t::i2c_transfer))
      {
        return result(op_status_t::invalid_op, i);
      }
    }
    return result(op_status_t::ok, count);
  }

  inline bool retry_available(const backend_t& backend, const retry_policy_t* policy)
  {
    if (policy == nullptr || backend.millis == nullptr) { return false; }
    return static_cast<std::int32_t>(backend.millis(backend.ctx) - policy->deadline_ms) < 0;
  }

  inline const retry_policy_t* operation_retry_policy(
    const backend_t& backend, const i2c_device_t& device,
    const retry_policy_t* policy, retry_policy_t& device_policy)
  {
    if (!(device.flags & i2c_device_retry_transient_nack)) { return policy; }
    // MCU-based PM1/IOE1 slaves can NACK briefly while processing a write.
    device_policy = { backend.millis(backend.ctx) + transient_nack_retry_ms,
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

  inline void retry_delay(const backend_t& backend, const retry_policy_t& policy)
  {
    if (backend.delay_ms != nullptr) { backend.delay_ms(backend.ctx, policy.interval_ms); }
  }

  inline run_result_t run_ops(const backend_t& backend,
                              const i2c_device_t* devices, std::size_t device_count,
                              const op_t* operations, std::size_t count,
                              const gpio_scope_t& gpio_scope,
                              const retry_policy_t* policy = nullptr,
                              void (*wait_ready_finished)(void*, const i2c_device_t&,
                                                          std::uint32_t, bool) = nullptr)
  {
    auto checked = validate_ops(devices, device_count, operations, count, gpio_scope);
    if (checked.status != op_status_t::ok) { return checked; }
    if (policy != nullptr
     && (policy->interval_ms == 0 || backend.millis == nullptr || backend.delay_ms == nullptr))
    {
      return result(op_status_t::unsupported, 0);
    }
    // Validate backend support for the complete list before the first side effect.
    for (std::size_t i = 0; i < count; ++i)
    {
      const auto kind = operations[i].kind();
      if (kind == op_kind_t::delay_ms && backend.delay_ms == nullptr)
      {
        return result(op_status_t::unsupported, i);
      }
      if (kind == op_kind_t::gpio_set_mode && backend.gpio_set_mode == nullptr)
      {
        return result(op_status_t::unsupported, i);
      }
      if ((kind == op_kind_t::gpio_write_high || kind == op_kind_t::gpio_write_low)
       && backend.gpio_write == nullptr)
      {
        return result(op_status_t::unsupported, i);
      }
      if (kind == op_kind_t::i2c_wait_ready
       && (backend.i2c_ready == nullptr || backend.millis == nullptr
        || backend.delay_ms == nullptr))
      {
        return result(op_status_t::unsupported, i);
      }
      if ((kind == op_kind_t::i2c_write8 || kind == op_kind_t::i2c_write16le
        || kind == op_kind_t::i2c_masked8)
       && ((kind == op_kind_t::i2c_write16le ? backend.i2c_write16le == nullptr
                                             : backend.i2c_write8 == nullptr)
        || (kind == op_kind_t::i2c_masked8
         && op_access_t::reg8(operations[i]).mask != 0xFF
         && backend.i2c_read8 == nullptr)
        || ((devices[op_access_t::dev(operations[i])].flags
             & i2c_device_retry_transient_nack) != 0
         && (backend.millis == nullptr || backend.delay_ms == nullptr))))
      {
        return result(op_status_t::unsupported, i);
      }
    }
    for (std::size_t i = 0; i < count; ++i)
    {
      const auto& operation = operations[i];
      const auto kind = operation.kind();
      if (kind == op_kind_t::delay_ms)
      {
        backend.delay_ms(backend.ctx, op_access_t::milliseconds(operation));
      }
      else if (kind == op_kind_t::gpio_set_mode)
      {
        if (!backend.gpio_set_mode(backend.ctx, op_access_t::pin(operation),
                                   op_access_t::mode(operation)))
        {
          return result(op_status_t::gpio_error, i);
        }
      }
      else if (kind == op_kind_t::gpio_write_high || kind == op_kind_t::gpio_write_low)
      {
        if (!backend.gpio_write(backend.ctx, op_access_t::pin(operation),
                                kind == op_kind_t::gpio_write_high))
        {
          return result(op_status_t::gpio_error, i);
        }
      }
      else if (kind == op_kind_t::i2c_wait_ready)
      {
        const auto& device = devices[op_access_t::dev(operation)];
        const auto started = backend.millis(backend.ctx);
        const auto deadline = started + op_access_t::timeout(operation);
        for (;;)
        {
          const auto now = backend.millis(backend.ctx);
          const auto remaining = static_cast<std::int32_t>(deadline - now) > 0
                               ? deadline - now : 0;
          const auto wire_timeout = device.wire_timeout_ms == 0
                                  ? remaining
                                  : (device.wire_timeout_ms < remaining
                                     ? device.wire_timeout_ms : remaining);
          if (backend.i2c_ready(backend.ctx, device, wire_timeout))
          {
            if (wait_ready_finished)
            { wait_ready_finished(backend.ctx, device, backend.millis(backend.ctx) - started, true); }
            break;
          }
          if (static_cast<std::int32_t>(backend.millis(backend.ctx) - deadline) >= 0)
          {
            if (wait_ready_finished)
            { wait_ready_finished(backend.ctx, device, backend.millis(backend.ctx) - started, false); }
            return result(op_status_t::timeout, i);
          }
          const auto left = deadline - backend.millis(backend.ctx);
          const auto interval = op_access_t::interval(operation);
          backend.delay_ms(backend.ctx, interval < left ? interval : left);
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
            backend, device, policy, device_retry_policy);
          while (!backend.i2c_write16le(backend.ctx, device, write.reg, write.value))
          {
            if (!retry_available(backend, operation_policy))
            {
              return result(operation_policy == nullptr ? op_status_t::i2c_nack
                                                         : op_status_t::timeout, i);
            }
            retry_delay(backend, *operation_policy);
          }
          continue;
        }
        const auto write = op_access_t::reg8(operation);
        const auto& device = devices[write.dev];
        retry_policy_t device_retry_policy {};
        const auto* operation_policy = operation_retry_policy(
          backend, device, policy, device_retry_policy);
        for (;;)
        {
          bool ok = false;
          if (kind == op_kind_t::i2c_write8 || write.mask == 0xFF)
          {
            ok = backend.i2c_write8(backend.ctx, device, write.reg, write.value);
          }
          else
          {
            std::uint8_t old_value = 0;
            if (backend.i2c_read8 != nullptr
             && backend.i2c_read8(backend.ctx, device, write.reg, &old_value))
            {
              const auto new_value = static_cast<std::uint8_t>(
                (old_value & static_cast<std::uint8_t>(~write.mask))
                | (write.value & write.mask));
              ok = backend.i2c_write8(backend.ctx, device, write.reg, new_value);
            }
          }
          if (ok) { break; }
          if (!retry_available(backend, operation_policy))
          {
            return result(operation_policy == nullptr ? op_status_t::i2c_nack
                                                       : op_status_t::timeout, i);
          }
          retry_delay(backend, *operation_policy);
        }
      }
    }
    return result(op_status_t::ok, count);
  }
}
}
}
