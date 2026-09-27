// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "board_detect.hpp"

#include <cstdio>
#include <cstring>
#include <driver/gpio.h>
#include <esp_log.h>

namespace m5gfx
{
namespace board_detect
{
  bool board_detector_t::has_member(board_id_t id) const
  {
    if (members == nullptr) { return false; }
    for (auto p = members; *p != nullptr; ++p)
    {
      if ((*p)->id == id) { return true; }
    }
    return false;
  }

  namespace
  {
    static constexpr char tag[] = "board_detect";

    bool enabled(const probe_ctx_t& ctx, board_id_t id)
    {
      if (ctx.enabled_ids == nullptr) { return true; }
      for (auto p = ctx.enabled_ids; *p != board_id_unknown; ++p)
      {
        if (*p == id) { return true; }
      }
      return false;
    }

    bool fallback_only(const board_detector_t* detector)
    {
      if (detector->members == nullptr || detector->members[0] == nullptr) { return false; }
      for (auto p = detector->members; *p != nullptr; ++p)
      {
        if (!((*p)->flags & def_flag_fallback)) { return false; }
      }
      return true;
    }

    bool run_detector(const board_detector_t* detector, probe_ctx_t& ctx, board_result_t* result)
    {
      const char* family = (detector->members && detector->members[0])
                         ? detector->members[0]->name : "empty";
      const bool signature = detector->signature(ctx);
      ESP_LOGD(tag, "stage=1 family=%s detector=%p match=%d", family,
               static_cast<const void*>(detector), signature);
      if (!signature) { return false; }

      board_result_t candidate;
      const bool confirmed = detector->confirm(ctx, &candidate);
      ESP_LOGD(tag, "stage=2 family=%s detector=%p match=%d board=%u name=%s", family,
               static_cast<const void*>(detector), confirmed,
               static_cast<unsigned>(candidate.def ? candidate.def->id : board_id_unknown),
               candidate.def ? candidate.def->name : "invalid");
      if (!confirmed) { return false; }

      if (candidate.desc == nullptr || candidate.def == nullptr
       || candidate.def != &candidate.desc->def
       || candidate.def->id == board_id_unknown)
      {
        ESP_LOGW(tag, "detector=%p returned a match without a consistent board description",
                 static_cast<const void*>(detector));
        return false;
      }
      candidate.status = enabled(ctx, candidate.def->id)
                       ? detect_status_t::matched
                       : detect_status_t::excluded;
      const auto& desc = *candidate.desc;
      char option_text[80] = {};
      std::uint32_t named_bits = 0;
      if (desc.option_names.data != nullptr)
      {
        for (std::uint_fast8_t bit = 0; bit < 32 && bit < desc.option_names.size; ++bit)
        {
          if (!(candidate.option & (std::uint32_t(1) << bit))) { continue; }
          const auto used = std::strlen(option_text);
          std::snprintf(option_text + used, sizeof(option_text) - used, "%s%s",
                        used ? "," : "", desc.option_names.data[bit]);
          named_bits |= std::uint32_t(1) << bit;
        }
      }
      const auto unnamed = candidate.option & ~named_bits;
      if (unnamed)
      {
        const auto used = std::strlen(option_text);
        std::snprintf(option_text + used, sizeof(option_text) - used, "%sunknown:%08x",
                      used ? "," : "",
                      static_cast<unsigned>(unnamed));
      }
      if (option_text[0] == '\0') { std::snprintf(option_text, sizeof(option_text), "none"); }
      ESP_LOGD(tag, "detected board=%u name=%s opt=0x%08x",
               static_cast<unsigned>(candidate.def->id), candidate.def->name,
               static_cast<unsigned>(candidate.option));
      ESP_LOGD(tag, "detected option names board=%u opt=0x%08x(%s)",
               static_cast<unsigned>(candidate.def->id),
               static_cast<unsigned>(candidate.option), option_text);
      *result = candidate;
      return true;
    }
  }

  board_result_t detect_board(const board_detector_t* const* list, board_id_t hint, probe_ctx_t& ctx)
  {
    board_result_t result;
    if (list == nullptr) { return result; }
    ctx.hint = hint;

    const board_detector_t* hinted = nullptr;
    if (hint != board_id_unknown)
    {
      bool hint_found = false;
      for (auto p = list; *p != nullptr; ++p)
      {
        if ((*p)->has_member(hint))
        {
          hint_found = true;
          if (!fallback_only(*p)) { hinted = *p; }
          else
          {
            ESP_LOGD(tag, "hint=%u belongs to a fallback-only detector",
                     static_cast<unsigned>(hint));
          }
          break;
        }
      }
      if (!hint_found)
      {
        // A hint belonging to a detector outside this package-specific list is normal.
        ESP_LOGD(tag, "hint=%u is not present in detector list", static_cast<unsigned>(hint));
      }
      else if (hinted != nullptr)
      {
        if (run_detector(hinted, ctx, &result)) { return result; }
        ESP_LOGD(tag, "hint=%u did not match its detector family", static_cast<unsigned>(hint));
      }
    }

    for (auto p = list; *p != nullptr; ++p)
    {
      if (*p == hinted) { continue; }
      if (run_detector(*p, ctx, &result)) { return result; }
    }
    return result;
  }

  bool release_held_sda(int pin_sda, int pin_scl)
  {
    lgfx::gpio::pin_backup_t backup[] = { pin_sda, pin_scl };
    lgfx::pinMode(pin_sda, lgfx::pin_mode_t::input);
    lgfx::pinMode(pin_scl, lgfx::pin_mode_t::input);
    lgfx::delayMicroseconds(10);
    auto wait_scl_high = [pin_scl]() -> bool
    {
      lgfx::gpio_hi(pin_scl);
      lgfx::delayMicroseconds(5);
      if (lgfx::gpio_in(pin_scl)) { return true; }
      const auto started = lgfx::micros();
      while (lgfx::micros() - started < 25000)
      {
        if (lgfx::gpio_in(pin_scl)) { return true; }
        lgfx::delayMicroseconds(1);
      }
      return false;
    };

    // A reset of this MCU during a transfer can leave a peripheral holding
    // SDA low while it waits for more clocks. Clock it out (up to 9 bits) and
    // finish with STOP. Input mode is open drain with the output latch high.
    bool bus_released = lgfx::gpio_in(pin_scl);
    if (bus_released && !lgfx::gpio_in(pin_sda))
    {
      for (int i = 0; i < 9 && !lgfx::gpio_in(pin_sda); ++i)
      {
        lgfx::gpio_lo(pin_scl);
        lgfx::delayMicroseconds(5);
        if (!wait_scl_high()) { bus_released = false; break; }
      }
      if (bus_released)
      {
        lgfx::gpio_lo(pin_scl);
        lgfx::gpio_lo(pin_sda);
        lgfx::delayMicroseconds(5);
        bus_released = wait_scl_high();
        if (bus_released)
        {
          lgfx::gpio_hi(pin_sda);
          lgfx::delayMicroseconds(10);
        }
      }
    }
    const bool released = bus_released && lgfx::gpio_in(pin_sda) && lgfx::gpio_in(pin_scl);
    for (auto& pin : backup) { pin.restore(); }
    return released;
  }

  bool probe_i2c_ack(probe_ctx_t& ctx, int pin_sda, int pin_scl, std::uint8_t addr)
  {
    // Reserved addresses are never touched, even if requested accidentally.
    if (addr < 0x08 || addr > 0x77) { return false; }

    auto& cache = ctx.i2c_cache;
    if (cache.pin_sda != pin_sda || cache.pin_scl != pin_scl)
    {
      cache = {};
      cache.pin_sda = pin_sda;
      cache.pin_scl = pin_scl;

      lgfx::gpio::pin_backup_t backup[] = { pin_sda, pin_scl };
      lgfx::pinMode(pin_sda, lgfx::pin_mode_t::input_pulldown);
      lgfx::pinMode(pin_scl, lgfx::pin_mode_t::input_pulldown);
      lgfx::delayMicroseconds(10);
      lgfx::pinMode(pin_sda, lgfx::pin_mode_t::input);
      lgfx::pinMode(pin_scl, lgfx::pin_mode_t::input);
      lgfx::delayMicroseconds(10);
      // The first check does not wait: a pin pair without pull-ups reads SCL
      // low here, and waiting for it would delay every such detection. Only a
      // bus that has pull-ups and a held SDA is clocked, and only those clocks
      // wait for a stretching peripheral.
      bool bus_released = lgfx::gpio_in(pin_scl);
      if (bus_released && !lgfx::gpio_in(pin_sda))
      {
        bus_released = release_held_sda(pin_sda, pin_scl);
      }
      cache.pullup_ok = bus_released && lgfx::gpio_in(pin_sda) && lgfx::gpio_in(pin_scl);
      for (auto& pin : backup) { pin.restore(); }
    }

    const std::uint32_t bit = 1u << (addr & 31);
    if (!(cache.checked[addr >> 5] & bit))
    {
      cache.checked[addr >> 5] |= bit;
      if (cache.pullup_ok)
      {
        lgfx::gpio::pin_backup_t backup[] = { pin_sda, pin_scl };
        if (lgfx::i2c::init(ctx.i2c_port_probe, pin_sda, pin_scl).has_value())
        {
          const bool hit = lgfx::i2c::beginTransaction(ctx.i2c_port_probe, addr, 100000, false).has_value()
                        && lgfx::i2c::endTransaction(ctx.i2c_port_probe).has_value();
          if (hit) { cache.ack[addr >> 5] |= bit; }
          lgfx::i2c::release(ctx.i2c_port_probe);
        }
        for (auto& pin : backup) { pin.restore(); }
      }
    }
    return cache.pullup_ok && (cache.ack[addr >> 5] & bit);
  }

  pin_pull_result_t probe_pin_pulls(std::uint64_t pin_mask)
  {
    static constexpr std::size_t max_pins = 64;
    pin_pull_result_t result;

    for (std::size_t pin = 0; pin < max_pins; ++pin)
    {
      const std::uint64_t bit = std::uint64_t(1) << pin;
      if (!(pin_mask & bit)) { continue; }
      // Measure one pin completely before touching the next. On a native-mode
      // SD bus this avoids raising CLK while CMD is temporarily pulled low.
      lgfx::gpio::pin_backup_t backup(pin);
      lgfx::pinMode(pin, lgfx::pin_mode_t::input_pulldown);
      lgfx::delayMicroseconds(10);
      if (lgfx::gpio_in(pin)) { result.pulldown_high |= bit; }
      lgfx::pinMode(pin, lgfx::pin_mode_t::input_pullup);
      lgfx::delayMicroseconds(10);
      if (lgfx::gpio_in(pin)) { result.pullup_high |= bit; }
      backup.restore();
    }
    return result;
  }

  void soft_spi_t::init()
  {
    lgfx::gpio_lo(pin_sclk_);
    lgfx::pinMode(pin_sclk_, lgfx::pin_mode_t::output);
    lgfx::gpio_hi(pin_mosi_);
    lgfx::pinMode(pin_mosi_, lgfx::pin_mode_t::output);
    if (pin_miso_ != pin_mosi_) { lgfx::pinMode(pin_miso_, lgfx::pin_mode_t::input); }
    if (pin_dc_ >= 0) { lgfx::pinMode(pin_dc_, lgfx::pin_mode_t::output); }
  }

  void soft_spi_t::beginTransaction()
  {
    lgfx::gpio_lo(pin_sclk_);
  }

  void soft_spi_t::endTransaction()
  {
    endRead();
    lgfx::gpio_lo(pin_sclk_);
  }

  void soft_spi_t::clock()
  {
    lgfx::delayMicroseconds(half_us_);
    lgfx::gpio_hi(pin_sclk_);
    lgfx::delayMicroseconds(half_us_);
    lgfx::gpio_lo(pin_sclk_);
    lgfx::delayMicroseconds(half_us_);
  }

  void soft_spi_t::send(std::uint32_t data, std::uint_fast8_t bits)
  {
    for (std::uint_fast8_t i = 0; i < bits; ++i)
    {
      if (data & (std::uint32_t(1) << bit_index(i))) { lgfx::gpio_hi(pin_mosi_); }
      else                                           { lgfx::gpio_lo(pin_mosi_); }
      clock();
    }
  }

  void soft_spi_t::writeCommand(std::uint32_t data, std::uint_fast8_t bits)
  {
    if (pin_dc_ >= 0) { lgfx::gpio_lo(pin_dc_); }
    send(data, bits);
  }

  void soft_spi_t::writeData(std::uint32_t data, std::uint_fast8_t bits)
  {
    if (pin_dc_ >= 0) { lgfx::gpio_hi(pin_dc_); }
    send(data, bits);
  }

  std::uint32_t soft_spi_t::transferData(std::uint32_t data, std::uint_fast8_t bits)
  {
    if (pin_dc_ >= 0) { lgfx::gpio_hi(pin_dc_); }
    std::uint32_t value = 0;
    for (std::uint_fast8_t i = 0; i < bits; ++i)
    {
      const auto index = bit_index(i);
      if (data & (std::uint32_t(1) << index)) { lgfx::gpio_hi(pin_mosi_); }
      else                                    { lgfx::gpio_lo(pin_mosi_); }
      lgfx::delayMicroseconds(half_us_);
      if (lgfx::gpio_in(pin_miso_)) { value |= std::uint32_t(1) << index; }
      lgfx::gpio_hi(pin_sclk_);
      lgfx::delayMicroseconds(half_us_);
      lgfx::gpio_lo(pin_sclk_);
    }
    return value;
  }

  void soft_spi_t::beginRead(std::uint_fast8_t dummy_bits)
  {
    if (pin_dc_ >= 0) { lgfx::gpio_hi(pin_dc_); }
    if (pin_miso_ == pin_mosi_) { lgfx::pinMode(pin_mosi_, lgfx::pin_mode_t::input); }
    for (std::uint_fast8_t i = 0; i < dummy_bits; ++i) { clock(); }
  }

  std::uint32_t soft_spi_t::readData(std::uint_fast8_t bits)
  {
    std::uint32_t value = 0;
    for (std::uint_fast8_t i = 0; i < bits; ++i)
    {
      lgfx::delayMicroseconds(half_us_);
      if (lgfx::gpio_in(pin_miso_)) { value |= std::uint32_t(1) << bit_index(i); }
      lgfx::gpio_hi(pin_sclk_);
      lgfx::delayMicroseconds(half_us_);
      lgfx::gpio_lo(pin_sclk_);
    }
    return value;
  }

  void soft_spi_t::readBytes(std::uint8_t* dst, std::size_t length)
  {
    while (length--) { *dst++ = static_cast<std::uint8_t>(readData(8)); }
  }

  void soft_spi_t::endRead()
  {
    if (pin_miso_ == pin_mosi_) { lgfx::pinMode(pin_mosi_, lgfx::pin_mode_t::output); }
  }

  std::uint32_t soft_spi_read32(probe_ctx_t&, int pin_sclk, int pin_mosi, int pin_miso,
                                int pin_dc, int pin_cs, std::uint8_t cmd, std::uint8_t dummy_bits)
  {
    lgfx::gpio::pin_backup_t backup_sclk(pin_sclk);
    lgfx::gpio::pin_backup_t backup_mosi(pin_mosi);
    lgfx::gpio::pin_backup_t backup_miso(pin_miso);
    lgfx::gpio::pin_backup_t backup_dc(pin_dc);
    lgfx::gpio::pin_backup_t backup_cs(pin_cs);

    lgfx::gpio_hi(pin_cs);
    lgfx::pinMode(pin_cs, lgfx::pin_mode_t::output);
    soft_spi_t bus(pin_sclk, pin_mosi, pin_miso, pin_dc);
    bus.init();
    bus.beginTransaction();
    lgfx::gpio_lo(pin_cs);
    bus.writeCommand(cmd, 8);
    bus.beginRead(dummy_bits);
    const auto value = bus.readData(32);
    lgfx::gpio_hi(pin_cs);
    bus.endTransaction();

    backup_cs.restore();
    backup_dc.restore();
    backup_miso.restore();
    backup_mosi.restore();
    backup_sclk.restore();
    return value;
  }

  namespace startup_detail
  {
    class retry_budget_t
    {
    public:
      explicit retry_budget_t(std::uint8_t milliseconds)
      : started_(lgfx::millis()), milliseconds_(milliseconds) {}

      bool exhausted() const { return lgfx::millis() - started_ >= milliseconds_; }

    private:
      std::uint32_t started_;
      std::uint8_t milliseconds_;
    };

    void pin_level(int pin, bool high)
    {
      if (high) { lgfx::gpio_hi(pin); }
      else      { lgfx::gpio_lo(pin); }
      lgfx::pinMode(pin, lgfx::pin_mode_t::output);
    }

    bool read_register(int port, const power_desc_t& power,
                       std::uint8_t addr, std::uint8_t reg,
                       std::uint8_t* value, retry_budget_t& retry_budget)
    {
      for (;;)
      {
        const auto result = lgfx::i2c::readRegister8(port, addr, reg, power.i2c_freq);
        if (result.has_value())
        {
          *value = result.value();
          return true;
        }
        if (retry_budget.exhausted()) { return false; }
        lgfx::delay(1);
      }
    }

    bool read_register(int port, const power_desc_t& power,
                       std::uint8_t addr, std::uint8_t reg,
                       std::uint8_t* value)
    {
      retry_budget_t retry_budget(power.wake_poll_ms);
      return read_register(port, power, addr, reg, value, retry_budget);
    }

    bool write_register(int port, const power_desc_t& power, const pmic_write_t& write,
                        retry_budget_t& retry_budget)
    {
      for (;;)
      {
        // writeRegister8 performs the whole read-modify-write for a masked
        // write. Retry the complete transaction if either half is NACKed.
        if (lgfx::i2c::writeRegister8(port, write.addr, write.reg,
                                     write.value, write.mask,
                                     power.i2c_freq).has_value())
        {
          return true;
        }
        if (retry_budget.exhausted()) { return false; }
        lgfx::delay(1);
      }
    }

    bool write_sequence(int port, const power_desc_t& power, pmic_sequence_t sequence,
                        retry_budget_t& retry_budget)
    {
      for (std::size_t i = 0; i < sequence.size; ++i)
      {
        const auto& write = sequence.data[i];
        if (!write_register(port, power, write, retry_budget)) { return false; }
      }
      return true;
    }

    bool write_sequence(int port, const power_desc_t& power, pmic_sequence_t sequence)
    {
      retry_budget_t retry_budget(power.wake_poll_ms);
      return write_sequence(port, power, sequence, retry_budget);
    }

    const pmic_variant_t* read_variant(const power_desc_t& power, int port,
                                       retry_budget_t& retry_budget)
    {
      if (power.variants == nullptr || power.variant_count == 0
       || power.variant_count > 8) { return nullptr; }
      std::uint8_t last_addr = 0xFF;
      std::uint8_t last_reg = 0xFF;
      std::uint8_t id_value = 0;
      bool id_valid = false;
      for (std::uint_fast8_t i = 0; i < power.variant_count; ++i)
      {
        if (power.variants[i].i2c_addr != last_addr || power.variants[i].id_reg != last_reg)
        {
          last_addr = power.variants[i].i2c_addr;
          last_reg = power.variants[i].id_reg;
          id_valid = read_register(port, power, last_addr, last_reg, &id_value, retry_budget);
        }
        const auto mask = power.variants[i].id_mask;
        if (id_valid && (id_value & mask) == (power.variants[i].id_value & mask))
        {
          return &power.variants[i];
        }
      }
      return nullptr;
    }

    const pmic_variant_t* read_variant(const power_desc_t& power, int port)
    {
      retry_budget_t retry_budget(power.wake_poll_ms);
      return read_variant(power, port, retry_budget);
    }

    void set_sd_spi_mode(const shared_sd_desc_t& sd)
    {
      lgfx::gpio::pin_backup_t pins[] = { sd.sclk, sd.mosi, sd.miso };
      soft_spi_t bus(sd.sclk, sd.mosi, sd.miso, -1, 2);
      bus.init();
      bus.beginTransaction();
      pin_level(sd.sd_cs, true);

      for (int i = 0; i < 10; ++i) { bus.writeData(0xFF, 8); }
      lgfx::gpio_lo(sd.sd_cs);
      static const std::uint8_t cmd58[] = { 0x7A, 0, 0, 0, 0, 0xFD, 0xFF, 0xFF };
      std::uint8_t response[sizeof(cmd58)];
      for (std::size_t i = 0; i < sizeof(cmd58); ++i)
      {
        response[i] = static_cast<std::uint8_t>(bus.transferData(cmd58[i], 8));
      }
      if (response[6] == response[7])
      {
        lgfx::gpio_hi(sd.sd_cs);
        for (int i = 0; i < 10; ++i) { bus.writeData(0xFF, 8); }
        lgfx::gpio_lo(sd.sd_cs);
        static const std::uint8_t cmd0[] = { 0x40, 0, 0, 0, 0, 0x95, 0xFF, 0xFF };
        for (auto value : cmd0) { bus.transferData(value, 8); }
      }
      lgfx::gpio_hi(sd.sd_cs);
      bus.endTransaction();
      for (auto& pin : pins) { pin.restore(); }
    }

    void pin_reset(const reset_desc_t& reset, bool active)
    {
      lgfx::gpio_hi(reset.pin);
      lgfx::pinMode(reset.pin, lgfx::pin_mode_t::output);
      lgfx::delay(1);
      if (!active) { return; }
      lgfx::gpio_lo(reset.pin);
      lgfx::delay(reset.low_ms);
      lgfx::gpio_hi(reset.pin);
      lgfx::delay(reset.post_ms);
    }

    class i2c_scope_t
    {
    public:
      i2c_scope_t(int port_, const i2c_desc_t& wiring)
      : port(port_), pins { wiring.sda, wiring.scl }
      {
        opened = lgfx::i2c::init(port, wiring.sda, wiring.scl).has_value();
      }

      ~i2c_scope_t()
      {
        if (opened) { lgfx::i2c::release(port); }
        for (auto& pin : pins) { pin.restore(); }
      }

      int port;
      lgfx::gpio::pin_backup_t pins[2];
      bool opened = false;
    };

    // Unchecked primitives; callers must first pass description_valid().
    bool prepare_power(const board_desc_t& desc, board_result_t& result, int i2c_port);
    bool prepare_sd_spi(const board_desc_t& desc, board_result_t& result);
  }

  bool startup_detail::prepare_power(const board_desc_t& desc, board_result_t& result,
                                     int i2c_port)
  {
    if (result.prepared & prepared_power) { return true; }
    const auto& power = desc.power;
    if (power.hold_pin >= 0) { startup_detail::pin_level(power.hold_pin, true); }
    if (power.variants == nullptr || power.variant_count == 0)
    {
      result.prepared |= prepared_power;
      return true;
    }

    // Wake-up polling and every PMIC transaction share one retry window so a
    // partially responsive controller cannot multiply the configured delay.
    startup_detail::retry_budget_t retry_budget(power.wake_poll_ms);
    const auto* variant = startup_detail::read_variant(power, i2c_port, retry_budget);
    if (variant == nullptr) { return false; }
    std::uint8_t power_state;
    if (!startup_detail::read_register(i2c_port, power, variant->i2c_addr,
                                       variant->power_state.reg, &power_state,
                                       retry_budget)) { return false; }
    std::uint8_t reset_state = power_state;
    if (variant->reset_state.reg != variant->power_state.reg
     && !startup_detail::read_register(i2c_port, power, variant->i2c_addr,
                                       variant->reset_state.reg, &reset_state,
                                       retry_budget)) { return false; }
    const bool power_was_off = !(power_state & variant->power_state.mask);
    const bool reset_was_low = !(reset_state & variant->reset_state.mask);
    if (!startup_detail::write_sequence(i2c_port, power, variant->power_on,
                                        retry_budget)) { return false; }
    if (power_was_off) { result.prepared &= ~prepared_sd_spi; }
    lgfx::delay(power_was_off || reset_was_low ? power.cold_wait_ms : power.warm_wait_ms);
    result.prepared |= prepared_power;
    return true;
  }

  bool startup_detail::prepare_sd_spi(const board_desc_t& desc, board_result_t& result)
  {
    if (result.prepared & prepared_sd_spi) { return true; }
    const auto& sd = desc.sd;
    if (sd.sd_cs < 0) { return true; }
    // Keep the other CS high; the detector's outer guard restores it on failure.
    lgfx::gpio::pin_backup_t other_cs(sd.other_cs);
    startup_detail::pin_level(sd.other_cs, true);
    startup_detail::set_sd_spi_mode(sd);
    result.prepared |= prepared_sd_spi;
    return true;
  }

  namespace startup_detail
  {
    template <typename T>
    bool list_valid(const list_desc_t<T>& list)
    {
      return list.data != nullptr || list.size == 0;
    }

    bool list_contains(pin_list_t list, int pin)
    {
      for (std::size_t i = 0; i < list.size; ++i)
      {
        if (list.data[i] == pin) { return true; }
      }
      return false;
    }

    bool gpio_valid(int pin)
    {
      return pin >= 0 && pin < GPIO_NUM_MAX && GPIO_IS_VALID_GPIO(pin);
    }

    bool optional_gpio_valid(int pin) { return pin == -1 || gpio_valid(pin); }

    bool pin_list_valid(pin_list_t list)
    {
      if (!list_valid(list)) { return false; }
      for (std::size_t i = 0; i < list.size; ++i)
      {
        if (!gpio_valid(list.data[i])) { return false; }
      }
      return true;
    }

    // Checked before any pin or register is touched, on every public entry
    // that acts on a description. Lists are expected to come from sequence(),
    // registers(), pins() and options(); a hand-written size cannot be checked
    // against its array, but a missing array with a non-zero size is rejected here.
    bool description_valid(const board_desc_t& desc)
    {
      const bool has_variants = desc.power.variants != nullptr;
      bool valid = has_variants == (desc.power.variant_count != 0)
                && desc.power.variant_count <= 8
                && !(has_variants && desc.power.hold_pin >= 0)
                && !(desc.reset.kind == reset_kind_t::i2c_regs && !has_variants)
                && !((has_variants || desc.reset.kind == reset_kind_t::i2c_regs)
                     && (desc.internal_i2c.sda < 0 || desc.internal_i2c.scl < 0))
                && !(desc.reset.kind == reset_kind_t::custom && desc.reset.custom == nullptr)
                && pin_list_valid(desc.hold_high_pins)
                && list_valid(desc.option_names)
                && desc.display.sclk >= 0 && desc.display.mosi >= 0 && desc.display.cs >= 0
                && list_contains(desc.hold_high_pins, desc.display.cs);
      const std::int8_t described_pins[] = {
        desc.power.hold_pin, desc.reset.pin,
        desc.sd.sclk, desc.sd.mosi, desc.sd.miso, desc.sd.sd_cs, desc.sd.other_cs,
        desc.display.sclk, desc.display.mosi, desc.display.miso, desc.display.dc,
        desc.display.cs, desc.display.rst, desc.display.busy,
        desc.internal_i2c.sda, desc.internal_i2c.scl,
      };
      for (auto pin : described_pins) { valid = valid && optional_gpio_valid(pin); }
      // Shared display/SD wires and exposed reset pins describe the same physical pins.
      // Both chip selects must be held high while probing their shared bus.
      if (valid && desc.sd.sd_cs >= 0)
      {
        valid = desc.sd.sclk >= 0 && desc.sd.mosi >= 0 && desc.sd.miso >= 0
             && desc.sd.other_cs >= 0
             && desc.sd.sclk == desc.display.sclk
             && desc.sd.mosi == desc.display.mosi
             && desc.sd.miso == desc.display.miso
             && desc.sd.other_cs == desc.display.cs
             && list_contains(desc.hold_high_pins, desc.sd.sd_cs);
      }
      if (valid && (desc.reset.kind == reset_kind_t::gpio
                 || desc.reset.kind == reset_kind_t::custom))
      {
        valid = desc.reset.pin >= 0 && desc.reset.pin == desc.display.rst;
      }
      for (std::uint_fast8_t i = 0; valid && i < desc.power.variant_count; ++i)
      {
        const auto& variant = desc.power.variants[i];
        valid = list_valid(variant.power_on)
             && list_valid(variant.reset_assert)
             && list_valid(variant.reset_release)
             && list_valid(variant.restore_registers)
             && variant.restore_registers.size <= max_pmic_restore_registers
             && (desc.reset.kind != reset_kind_t::i2c_regs
                 || (variant.reset_assert.size != 0 && variant.reset_release.size != 0));
      }
      if (!valid)
      {
        ESP_LOGW(tag, "invalid board description id=%u", static_cast<unsigned>(desc.def.id));
      }
      return valid;
    }

    bool hold_chip_selects(const board_desc_t& desc)
    {
      for (std::size_t i = 0; i < desc.hold_high_pins.size; ++i)
      {
        pin_level(desc.hold_high_pins.data[i], true);
      }
      return true;
    }
  }

  bool prepare_reset(const board_desc_t& desc, board_result_t& result,
                     const prepare_ctx_t& ctx, int i2c_port,
                     std::uint32_t* detected_option)
  {
    const auto& reset = desc.reset;
    if (!startup_detail::description_valid(desc)) { return false; }
    if (result.prepared & prepared_reset) { return true; }
    if (!(reset.flags & reset_always) && !ctx.allow_reset)
    {
      if (reset.flags & reset_hold_when_skipped) { startup_detail::pin_reset(reset, false); }
      return true;
    }

    bool ok = true;
    if (reset.kind == reset_kind_t::i2c_regs)
    {
      const auto* variant = startup_detail::read_variant(desc.power, i2c_port);
      if (variant == nullptr) { return false; }
      ok = startup_detail::write_sequence(i2c_port, desc.power, variant->reset_assert);
      if (ok)
      {
        lgfx::delay(reset.low_ms);
        ok = startup_detail::write_sequence(i2c_port, desc.power, variant->reset_release);
        if (ok) { lgfx::delay(reset.post_ms); }
      }
    }
    else if (reset.kind == reset_kind_t::custom)
    {
      ok = reset.custom(desc, ctx, detected_option);
    }
    else if (reset.kind == reset_kind_t::gpio)
    {
      startup_detail::pin_reset(reset, true);
    }
    if (ok) { result.prepared |= prepared_reset; }
    return ok;
  }

  bool probe_spi_id(probe_ctx_t& ctx, const board_desc_t& desc,
                    const spi_id_probe_t* probes, std::size_t probe_count,
                    board_result_t* result)
  {
    if (probes == nullptr || probe_count == 0 || result == nullptr
     || !startup_detail::description_valid(desc)) { return false; }
    lgfx::gpio::pin_backup_t pins[] = {
      desc.display.cs, desc.display.sclk, desc.display.mosi,
      desc.display.dc, desc.display.rst,
    };
    board_result_t candidate;
    candidate.assign(&desc);
    prepare_ctx_t prepare_ctx;
    prepare_ctx.allow_reset = ctx.allow_reset;
    prepare_ctx.i2c_port_probe = ctx.i2c_port_probe;
    if (!prepare_reset(desc, candidate, prepare_ctx, ctx.i2c_port_probe))
    {
      for (auto& pin : pins) { pin.restore(); }
      return false;
    }
    const int read_pin = desc.display.miso >= 0 ? desc.display.miso : desc.display.mosi;
    std::uint8_t last_cmd = 0;
    std::uint32_t id = 0;
    bool have_id = false;
    for (std::size_t index = 0; index < probe_count; ++index)
    {
      const auto& probe = probes[index];
      if (!have_id || probe.cmd != last_cmd)
      {
        id = soft_spi_read32(ctx, desc.display.sclk, desc.display.mosi, read_pin,
                             desc.display.dc, desc.display.cs, probe.cmd, 1);
        last_cmd = probe.cmd;
        have_id = true;
      }
      for (std::size_t value = 0; value < probe.value_count; ++value)
      {
        if ((id & probe.mask) != probe.values[value]) { continue; }
        candidate.option = probe.option_bit;
        *result = candidate;
        return true;
      }
    }
    for (auto& pin : pins) { pin.restore(); }
    return false;
  }

  bool prepare(const board_desc_t& desc, board_result_t& result, const prepare_ctx_t& ctx)
  {
    if (result.desc != &desc || result.def != &desc.def || result.def->id == board_id_unknown)
    {
      return false;
    }
    if (!startup_detail::description_valid(desc)) { return false; }
    if (!(result.prepared & prepared_power))
    {
      if (desc.power.variants != nullptr)
      {
        startup_detail::i2c_scope_t i2c(ctx.i2c_port_probe, desc.internal_i2c);
        if (!i2c.opened || !startup_detail::prepare_power(desc, result, i2c.port)) { return false; }
      }
      else if (!startup_detail::prepare_power(desc, result, ctx.i2c_port_probe)) { return false; }
    }
    if (!startup_detail::prepare_sd_spi(desc, result)) { return false; }
    const bool reset_was_prepared = result.prepared & prepared_reset;
    if (!reset_was_prepared)
    {
      if (desc.reset.kind == reset_kind_t::i2c_regs)
      {
        startup_detail::i2c_scope_t i2c(ctx.i2c_port_probe, desc.internal_i2c);
        if (!i2c.opened || !prepare_reset(desc, result, ctx, i2c.port)) { return false; }
      }
      else if (!prepare_reset(desc, result, ctx, ctx.i2c_port_probe)) { return false; }
    }
    if (!reset_was_prepared && (result.prepared & prepared_reset)
     && desc.direct_reset_panel_reload_wait_ms)
    {
      lgfx::delay(desc.direct_reset_panel_reload_wait_ms);
    }
    if (!startup_detail::hold_chip_selects(desc)) { return false; }
    if (desc.internal_i2c.hw_port >= 0
     && !lgfx::i2c::init(desc.internal_i2c.hw_port, desc.internal_i2c.sda,
                         desc.internal_i2c.scl).has_value())
    {
      ESP_LOGW("board_detect", "I2C%d could not be opened for SDA=%d SCL=%d",
               desc.internal_i2c.hw_port, desc.internal_i2c.sda, desc.internal_i2c.scl);
    }
    return true;
  }
}
}
