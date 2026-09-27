// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "../board_detect.hpp"
#include "board_registry.inl"
#include "generated/esp32_d0wdq6_wiring.hpp"

#include <new>

namespace m5gfx
{
namespace board_detect
{
namespace m5
{
  namespace
  {
    constexpr board_id_t id(lgfx::board_t value)
    {
      return static_cast<board_id_t>(value);
    }
  }

  namespace detail
  {
    bool stack_reset_and_sample_ips(const board_desc_t& desc, const prepare_ctx_t& ctx,
                                    std::uint32_t* detected_option);
  }

  static const pmic_write_t power192[] = {
    pmic_write(0x34, 0x95, 0x84, 0x72), pmic_write(0x34, 0x28, 0xF0, 0x0F),
    pmic_write(0x34, 0x12, 0x04, 0xFF), pmic_write(0x34, 0x92, 0x00, 0xF8),
    pmic_write(0x34, 0x96, 0x02, 0xFF), pmic_write(0x34, 0x94, 0x02, 0xFF),
  };
  static const pmic_write_t power2101[] = {
    pmic_write(0x34, 0x90, 0x08, 0xF7), pmic_write(0x34, 0x80, 0x05, 0xFF),
    pmic_write(0x34, 0x82, 0x12, 0x00), pmic_write(0x34, 0x84, 0x6A, 0x00),
    pmic_write(0x34, 0x90, 0x02, 0xFD),
  };
  static const pmic_write_t reset192[] = {
    pmic_write(0x34, 0x96, 0x00, 0xFD), pmic_write(0x34, 0x94, 0x00, 0xFD),
  };
  static const pmic_write_t release192[] = {
    pmic_write(0x34, 0x96, 0x02, 0xFF), pmic_write(0x34, 0x94, 0x02, 0xFF),
  };
  static const pmic_write_t reset2101[] = {
    pmic_write(0x34, 0x90, 0x00, 0xFD),
  };
  static const pmic_write_t release2101[] = {
    pmic_write(0x34, 0x90, 0x02, 0xFF),
  };
  static constexpr std::size_t max_restore_regs = max_pmic_restore_registers;
  static const std::uint8_t restore_order192[] = { 0x12, 0x28, 0x92, 0x95, 0x96, 0x94 };
  static const std::uint8_t restore_order2101[] = { 0x80, 0x82, 0x84, 0x90 };
  static_assert(sizeof(restore_order192) <= max_restore_regs
             && sizeof(restore_order2101) <= max_restore_regs,
                "PMIC restore list exceeds backup capacity");
  static_assert(generated_options::core2::new_pmic == generated_options::tough::reserved,
                "Core2 new-PMIC and Tough reserved option bits must remain shared");
  static const pmic_variant_t core_pmic_variants[] = {
    pmic_variant(0x34, 0x03, 0x03, sequence(power192),
                 reg_bit(0x12, 0x04), reg_bit(0x96, 0x02),
                 sequence(reset192), sequence(release192), registers(restore_order192), 0),
    pmic_variant(0x34, 0x03, 0x4A, sequence(power2101),
                 reg_bit(0x90, 0x08), reg_bit(0x90, 0x02),
                 sequence(reset2101), sequence(release2101), registers(restore_order2101),
                 generated_options::core2::new_pmic),
  };

  static constexpr board_desc_t desc_station = {
    { id(lgfx::board_M5Station), "M5Station", 0 },
    no_power(), gpio_reset(wiring::station::reset_gpio, 2, 10, reset_hold_when_skipped),
    no_shared_sd(),
    display_pins(wiring::station::display_sclk, wiring::station::display_mosi,
                 wiring::station::display_miso, wiring::station::display_dc,
                 wiring::station::display_cs, wiring::station::display_rst,
                 wiring::station::display_busy),
    pins(wiring::station::hold),
    internal_i2c(wiring::station::internal_i2c_sda, wiring::station::internal_i2c_scl,
                 wiring::station::internal_i2c_port),
    no_direct_reset_panel_reload_wait(), no_options(),
  };
  static constexpr board_desc_t desc_core2 = {
    { id(lgfx::board_M5StackCore2), "M5StackCore2", 0 },
    i2c_power(400000, core_pmic_variants, 5, 20), i2c_reset(1, 10),
    shared_sd(wiring::core2::shared_sd_sclk, wiring::core2::shared_sd_mosi,
              wiring::core2::shared_sd_miso, wiring::core2::shared_sd_sd_cs,
              wiring::core2::shared_sd_other_cs),
    display_pins(wiring::core2::display_sclk, wiring::core2::display_mosi,
                 wiring::core2::display_miso, wiring::core2::display_dc,
                 wiring::core2::display_cs, wiring::core2::display_rst,
                 wiring::core2::display_busy),
    pins(wiring::core2::hold),
    internal_i2c(wiring::core2::internal_i2c_sda, wiring::core2::internal_i2c_scl,
                 wiring::core2::internal_i2c_port),
    direct_reset_panel_reload_wait(110),
    options(generated_options::core2::names),
  };
  static constexpr board_desc_t desc_tough = {
    { id(lgfx::board_M5Tough), "M5Tough", 0 },
    i2c_power(400000, core_pmic_variants, 5, 20), i2c_reset(1, 10),
    shared_sd(wiring::tough::shared_sd_sclk, wiring::tough::shared_sd_mosi,
              wiring::tough::shared_sd_miso, wiring::tough::shared_sd_sd_cs,
              wiring::tough::shared_sd_other_cs),
    display_pins(wiring::tough::display_sclk, wiring::tough::display_mosi,
                 wiring::tough::display_miso, wiring::tough::display_dc,
                 wiring::tough::display_cs, wiring::tough::display_rst,
                 wiring::tough::display_busy),
    pins(wiring::tough::hold),
    internal_i2c(wiring::tough::internal_i2c_sda, wiring::tough::internal_i2c_scl,
                 wiring::tough::internal_i2c_port),
    direct_reset_panel_reload_wait(110),
    options(generated_options::tough::names),
  };
  static constexpr board_desc_t desc_stack = {
    { id(lgfx::board_M5Stack), "M5Stack", 0 },
    no_power(), custom_reset(wiring::stack::reset_gpio, 2, 10, detail::stack_reset_and_sample_ips),
    shared_sd(wiring::stack::shared_sd_sclk, wiring::stack::shared_sd_mosi,
              wiring::stack::shared_sd_miso, wiring::stack::shared_sd_sd_cs,
              wiring::stack::shared_sd_other_cs),
    display_pins(wiring::stack::display_sclk, wiring::stack::display_mosi,
                 wiring::stack::display_miso, wiring::stack::display_dc,
                 wiring::stack::display_cs, wiring::stack::display_rst,
                 wiring::stack::display_busy),
    pins(wiring::stack::hold), no_internal_i2c(), no_direct_reset_panel_reload_wait(),
    options(generated_options::stack::names),
  };
  static constexpr board_desc_t desc_paper = {
    { id(lgfx::board_M5Paper), "M5Paper", 0 },
    gpio_power(wiring::paper::power_gpio), gpio_reset(wiring::paper::reset_gpio, 2, 10, reset_always),
    shared_sd(wiring::paper::shared_sd_sclk, wiring::paper::shared_sd_mosi,
              wiring::paper::shared_sd_miso, wiring::paper::shared_sd_sd_cs,
              wiring::paper::shared_sd_other_cs),
    display_pins(wiring::paper::display_sclk, wiring::paper::display_mosi,
                 wiring::paper::display_miso, wiring::paper::display_dc,
                 wiring::paper::display_cs, wiring::paper::display_rst,
                 wiring::paper::display_busy),
    pins(wiring::paper::hold), no_internal_i2c(), no_direct_reset_panel_reload_wait(), no_options(),
  };

  static const board_def_t& board_station = desc_station.def;
  static const board_def_t& board_core2 = desc_core2.def;
  static const board_def_t& board_tough = desc_tough.def;
  static const board_def_t& board_stack = desc_stack.def;
  static const board_def_t& board_paper = desc_paper.def;

  namespace detail
  {
    static constexpr std::uint8_t tough_touch_address = 0x2E;
    static constexpr std::uint8_t touch_probe_register = 0;
    static constexpr std::uint32_t tough_touch_i2c_frequency = 400000;
    static constexpr std::uint8_t panel_id_command = 0x04;
    static constexpr std::uint32_t panel_id_mask = 0xFF;
    static constexpr std::uint32_t common_panel_id = 0xE3;
    static constexpr std::uint8_t station_pmic_id = 0x03;
    static constexpr std::uint32_t station_id_mask = 0xFB;
    static constexpr std::uint32_t station_id = 0x81;
    static constexpr std::uint32_t paper_panel_size = 0x03C0021C;

    bool sd_pull_mask(const board_desc_t& desc, std::uint64_t* mask)
    {
      if (mask == nullptr || !startup_detail::gpio_valid(desc.sd.sd_cs)
       || !startup_detail::gpio_valid(desc.sd.sclk)
       || !startup_detail::gpio_valid(desc.sd.mosi))
      {
        return false;
      }
      *mask = (std::uint64_t(1) << desc.sd.sd_cs)
            | (std::uint64_t(1) << desc.sd.sclk)
            | (std::uint64_t(1) << desc.sd.mosi);
      return true;
    }

    struct i2c_scope_t
    {
      i2c_scope_t(probe_ctx_t& ctx, int sda, int scl)
      : i2c_scope_t(ctx.i2c_port_probe, sda, scl) {}

      i2c_scope_t(int port_, int sda, int scl)
      : port(port_), pins { sda, scl }
      {
        opened = lgfx::i2c::init(port, sda, scl).has_value();
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

    void pin_level(int pin, bool high)
    {
      if (high) { lgfx::gpio_hi(pin); }
      else      { lgfx::gpio_lo(pin); }
      lgfx::pinMode(pin, lgfx::pin_mode_t::output);
    }

    class retained_pin_guard_t
    {
    public:
      explicit retained_pin_guard_t(int pin) : pin_(pin), backup_(pin) {}
      ~retained_pin_guard_t()
      {
        if (active_ && !retained_) { backup_.restore(); }
      }

      void activate_high()
      {
        pin_level(pin_, true);
        active_ = true;
      }
      void retain() { retained_ = true; }

    private:
      int pin_;
      lgfx::gpio::pin_backup_t backup_;
      bool active_ = false;
      bool retained_ = false;
    };

    void pin_reset(int pin, bool reset)
    {
      lgfx::gpio_hi(pin);
      lgfx::pinMode(pin, lgfx::pin_mode_t::output);
      lgfx::delay(1);
      if (!reset) { return; }
      lgfx::gpio_lo(pin);
      lgfx::delay(2);
      lgfx::gpio_hi(pin);
      lgfx::delay(10);
    }

    std::uint32_t read_panel_id(soft_spi_t& bus, int pin_cs,
                                std::uint8_t cmd = panel_id_command,
                                std::uint8_t dummy_bits = 1)
    {
      bus.beginTransaction();
      pin_level(pin_cs, true);
      bus.writeCommand(0, 8);
      bus.wait();
      lgfx::gpio_lo(pin_cs);
      bus.writeCommand(cmd, 8);
      bus.beginRead(dummy_bits);
      const auto value = bus.readData(32);
      lgfx::gpio_hi(pin_cs);
      bus.endTransaction();
      ESP_LOGD("board_detect_m5", "read cmd:%02x = %08x",
               static_cast<unsigned>(cmd), static_cast<unsigned>(value));
      return value;
    }

    struct register_backup_t
    {
      std::uint8_t addr = 0;
      std::uint8_t reg = 0;
      std::uint8_t value = 0;
      std::uint8_t mask = 0;
    };

    struct register_spec_t
    {
      std::uint8_t addr;
      std::uint8_t reg;
      std::uint8_t mask;
    };

    std::uint8_t changed_mask(pmic_sequence_t sequence, std::uint8_t addr,
                              std::uint8_t reg)
    {
      std::uint8_t mask = 0;
      for (std::size_t i = 0; i < sequence.size; ++i)
      {
        const auto& write = sequence.data[i];
        if (write.addr == addr && write.reg == reg)
        {
          mask |= static_cast<std::uint8_t>(~write.mask) | write.value;
        }
      }
      return mask;
    }

    std::uint8_t changed_mask(const pmic_variant_t& variant, std::uint8_t addr,
                              std::uint8_t reg)
    {
      return changed_mask(variant.power_on, addr, reg)
           | changed_mask(variant.reset_assert, addr, reg)
           | changed_mask(variant.reset_release, addr, reg);
    }

    bool restore_list_covers(const pmic_variant_t& variant)
    {
      const pmic_sequence_t sequences[] = {
        variant.power_on, variant.reset_assert, variant.reset_release
      };
      for (auto sequence : sequences)
      {
        for (std::size_t i = 0; i < sequence.size; ++i)
        {
          const auto& write = sequence.data[i];
          bool covered = false;
          if (write.addr == variant.i2c_addr)
          {
            for (std::size_t reg = 0; reg < variant.restore_registers.size; ++reg)
            {
              if (variant.restore_registers.data[reg] == write.reg)
              {
                covered = true;
                break;
              }
            }
          }
          if (!covered)
          {
            ESP_LOGW("board_detect_m5", "PMIC write is absent from restore list addr=%02x reg=%02x",
                     static_cast<unsigned>(write.addr),
                     static_cast<unsigned>(write.reg));
            return false;
          }
        }
      }
      return true;
    }

    bool save_registers(int port, const register_spec_t* specs,
                        register_backup_t* saved, std::size_t count,
                        std::uint32_t i2c_freq)
    {
      for (std::size_t i = 0; i < count; ++i)
      {
        auto value = lgfx::i2c::readRegister8(port, specs[i].addr, specs[i].reg, i2c_freq);
        if (!value.has_value()) { return false; }
        saved[i].addr = specs[i].addr;
        saved[i].reg = specs[i].reg;
        saved[i].value = value.value();
        saved[i].mask = specs[i].mask;
      }
      return true;
    }

    bool restore_registers(int port, const register_backup_t* saved, std::size_t count,
                           std::uint32_t i2c_freq)
    {
      bool restored_all = true;
      while (count)
      {
        --count;
        bool restored = false;
        for (int attempt = 0; attempt < 4; ++attempt)
        {
          if (lgfx::i2c::writeRegister8(port, saved[count].addr, saved[count].reg,
                                       saved[count].value & saved[count].mask,
                                       static_cast<std::uint8_t>(~saved[count].mask),
                                       i2c_freq).has_value())
          {
            restored = true;
            break;
          }
        }
        if (!restored)
        {
          restored_all = false;
          ESP_LOGW("board_detect_m5", "PMIC register restore failed addr=%02x reg=%02x",
                   static_cast<unsigned>(saved[count].addr),
                   static_cast<unsigned>(saved[count].reg));
        }
      }
      return restored_all;
    }

    enum class panel_variant_t : std::uint8_t { unknown, c, e };

    void write8(soft_spi_t& bus, int pin_cs, std::uint8_t cmd, std::uint8_t data)
    {
      bus.beginTransaction();
      lgfx::gpio_lo(pin_cs);
      bus.writeCommand(cmd, 8);
      bus.writeData(data, 8);
      // An uninitialised Panel_LCD still has _nop_closing=true. Its endWrite()
      // emits NOP before raising CS, so reproduce that probe transaction exactly.
      bus.writeCommand(0x00, 8);
      bus.wait();
      lgfx::gpio_hi(pin_cs);
      bus.endTransaction();
    }

    std::uint8_t read_parameter(soft_spi_t& bus, int pin_cs, std::uint8_t cmd, std::uint8_t index)
    {
      write8(bus, pin_cs, 0xD9, 0x10 | index);
      bus.beginTransaction();
      lgfx::gpio_lo(pin_cs);
      bus.writeCommand(cmd, 8);
      bus.beginRead(1);
      const auto value = static_cast<std::uint8_t>(bus.readData(8));
      lgfx::gpio_hi(pin_cs);
      bus.endRead();
      // Panel_LCD::readCommand() raises CS before endWrite(). The latter still
      // clocks its closing NOP (with CS high) because the panel is uninitialised.
      bus.writeCommand(0x00, 8);
      bus.wait();
      bus.endTransaction();
      return (value >> 1) & 0x7F;
    }

    panel_variant_t probe_panel_variant(soft_spi_t& bus, int pin_cs,
                                        std::uint32_t keys[4], bool try_c_key)
    {
      write8(bus, pin_cs, 0xD9, 0x00);
      write8(bus, pin_cs, 0xDD, 0x01);
      write8(bus, pin_cs, 0xCB, 0x1C);
      keys[0] = read_parameter(bus, pin_cs, 0xDD, 1);
      keys[1] = read_parameter(bus, pin_cs, 0xCB, 1);
      write8(bus, pin_cs, 0xD9, 0x00);
      if (keys[0] == 0x01 && keys[1] == 0x1C) { return panel_variant_t::e; }
      if (!try_c_key) { return panel_variant_t::unknown; }

      bus.beginTransaction();
      lgfx::gpio_lo(pin_cs);
      bus.writeCommand(0xC8, 8);
      bus.writeData(0xFF, 8);
      bus.writeData(0x93, 8);
      bus.writeData(0x42, 8);
      bus.writeCommand(0x00, 8);
      bus.wait();
      lgfx::gpio_hi(pin_cs);
      bus.endTransaction();
      keys[2] = read_parameter(bus, pin_cs, 0xD3, 2);
      keys[3] = read_parameter(bus, pin_cs, 0xD3, 3);
      write8(bus, pin_cs, 0xD9, 0x00);
      return (keys[2] == (0x93 & 0x7F) && keys[3] == 0x42)
           ? panel_variant_t::c : panel_variant_t::unknown;
    }

    panel_variant_t identify_panel_variant(soft_spi_t& bus, int pin_cs,
                                           std::uint32_t keys[4], std::uint32_t poll_ms,
                                           bool try_c_key = true)
    {
      auto variant = probe_panel_variant(bus, pin_cs, keys, try_c_key);
      const auto started = lgfx::millis();
      while (variant == panel_variant_t::unknown && poll_ms)
      {
        lgfx::delay(1);
        variant = probe_panel_variant(bus, pin_cs, keys, try_c_key);
        if (lgfx::millis() - started >= poll_ms) { break; }
      }
      return variant;
    }

    bool stack_reset_and_sample_ips(const board_desc_t& desc, const prepare_ctx_t& ctx,
                                    std::uint32_t* detected_option)
    {
      const auto& reset = desc.reset;
      pin_level(reset.pin, true);
      lgfx::delay(1);
      lgfx::gpio_lo(reset.pin);
      lgfx::delay(reset.low_ms);
      lgfx::pinMode(reset.pin, lgfx::pin_mode_t::input_pulldown);
      lgfx::gpio_hi(reset.pin);
      const bool ips = lgfx::gpio_in(reset.pin);
      lgfx::pinMode(reset.pin, lgfx::pin_mode_t::output);
      lgfx::delay(reset.post_ms);
      if (ctx.allow_reset && ips && detected_option != nullptr)
      {
        *detected_option |= generated_options::stack::ips;
      }
      return true;
    }
  }

  bool construct_station(const board_result_t& result, display_parts_t* parts);
  bool construct_core2(const board_result_t& result, display_parts_t* parts);
  bool construct_tough(const board_result_t& result, display_parts_t* parts);
  bool construct_stack(const board_result_t& result, display_parts_t* parts);
  bool construct_paper(const board_result_t& result, display_parts_t* parts);
  static const board_entry_t esp32_d0wdq6_boards[] = {
    { &desc_station, construct_station, nullptr, nullptr },
    { &desc_core2, construct_core2, nullptr, nullptr },
    { &desc_tough, construct_tough, nullptr, nullptr },
    { &desc_stack, construct_stack, nullptr, nullptr },
    { &desc_paper, construct_paper, nullptr, nullptr },
  };

  const board_desc_t* find_board_desc(board_id_t board)
  {
    return find_board_desc(esp32_d0wdq6_boards, board);
  }

  class axp_family_detector_t final : public board_detector_t
  {
  public:
    axp_family_detector_t() : board_detector_t(members_) {}

    bool signature(probe_ctx_t& ctx) const override
    {
      if (!startup_detail::description_valid(desc_station)
       || !startup_detail::description_valid(desc_core2)
       || !startup_detail::description_valid(desc_tough)
       || !startup_detail::gpio_valid(desc_station.display.dc)
       || !startup_detail::gpio_valid(desc_core2.display.dc)
       || !startup_detail::gpio_valid(desc_tough.display.dc))
      {
        return false;
      }
      // Station deliberately uses the Core2 family's internal-I2C pins and
      // first PMIC address: its AXP192 ACK is the shared family signature.
      return probe_i2c_ack(ctx, desc_core2.internal_i2c.sda, desc_core2.internal_i2c.scl,
                           desc_core2.power.variants[0].i2c_addr);
    }

    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (!startup_detail::description_valid(desc_station)
       || !startup_detail::description_valid(desc_core2)
       || !startup_detail::description_valid(desc_tough)
       || !startup_detail::gpio_valid(desc_station.display.dc)
       || !startup_detail::gpio_valid(desc_core2.display.dc)
       || !startup_detail::gpio_valid(desc_tough.display.dc))
      {
        return false;
      }
      detail::i2c_scope_t i2c(ctx, desc_core2.internal_i2c.sda,
                              desc_core2.internal_i2c.scl);
      if (!i2c.opened) { return false; }
      prepare_ctx_t prepare_ctx;
      prepare_ctx.allow_reset = ctx.allow_reset;
      prepare_ctx.i2c_port_probe = i2c.port;
      const auto* pmic = startup_detail::read_variant(desc_core2.power, i2c.port);
      if (pmic == nullptr) { return false; }
      ESP_LOGD("board_detect_m5", "power controller id=%02x", pmic->id_value);

      std::uint64_t sd_mask;
      if (!detail::sd_pull_mask(desc_core2, &sd_mask)) { return false; }
      detail::retained_pin_guard_t lcd_cs(desc_core2.display.cs);
      detail::retained_pin_guard_t sd_cs(desc_core2.sd.sd_cs);
      // Even pull probing toggles shared clocks, so deselect the LCD first.
      lcd_cs.activate_high();
      const auto sd_pulls = probe_pin_pulls(sd_mask);
      const bool sd_present = sd_pulls.pulldown_high == sd_mask
                           && sd_pulls.pullup_high == sd_mask;
      std::uint32_t preprepared = 0;
      if (sd_present)
      {
        // This exceptional pre-power transition protects the Station probe on
        // powered Core2 revisions. Unpowered cards transition after PMIC power.
        sd_cs.activate_high();
        board_result_t sd_result;
        sd_result.assign(&desc_core2);
        startup_detail::prepare_sd_spi(desc_core2, sd_result);
        preprepared |= sd_result.prepared & prepared_sd_spi;
      }

      auto try_station = [&]() -> bool
      {
        if (pmic->id_value != detail::station_pmic_id) { return false; }
        *result = {};
        result->assign(&desc_station);
        result->prepared = preprepared;
        lgfx::gpio::pin_backup_t reset_backup(desc_station.reset.pin);
        if (!prepare_reset(desc_station, *result, prepare_ctx, i2c.port))
        {
          reset_backup.restore();
          return false;
        }
        const auto& display = desc_station.display;
        const auto detected_id = soft_spi_read32(
          ctx, display.sclk, display.mosi, display.mosi, display.dc, display.cs,
          detail::panel_id_command, 1);
        if ((detected_id & detail::station_id_mask) != detail::station_id)
        {
          reset_backup.restore();
          return false;
        }
        // A matching Station keeps reset and LCD CS inactive through prepare.
        lcd_cs.retain();
        return true;
      };

      const bool core_first = ctx.hint == id(lgfx::board_M5StackCore2)
                           || ctx.hint == id(lgfx::board_M5Tough);
      if (!core_first && try_station()) { return true; }

      // The variant owns the legacy read order; masks remain derived from all
      // of its power/reset writes so the restore description cannot drift.
      const auto& restore_regs = pmic->restore_registers;
      if (restore_regs.size > max_restore_regs || !detail::restore_list_covers(*pmic))
      {
        return false;
      }
      detail::register_spec_t regs[max_restore_regs];
      for (std::size_t i = 0; i < restore_regs.size; ++i)
      {
        regs[i].addr = pmic->i2c_addr;
        regs[i].reg = restore_regs.data[i];
        regs[i].mask = detail::changed_mask(*pmic, pmic->i2c_addr, restore_regs.data[i]);
      }
      detail::register_backup_t saved[max_restore_regs];
      if (!detail::save_registers(i2c.port, regs, saved, restore_regs.size,
                                  desc_core2.power.i2c_freq))
      {
        return core_first && try_station();
      }

      *result = {};
      result->assign(&desc_core2);
      result->option = pmic->detected_option;
      result->prepared = preprepared;
      lgfx::gpio::pin_backup_t signals[] = {
        desc_core2.display.dc, desc_core2.display.sclk,
        desc_core2.display.mosi, desc_core2.display.miso
      };
      auto restore_and_fail = [&]() -> bool
      {
        for (auto& pin : signals) { pin.restore(); }
        detail::restore_registers(i2c.port, saved, restore_regs.size,
                                  desc_core2.power.i2c_freq);
        return core_first && try_station();
      };
      if (!startup_detail::prepare_power(desc_core2, *result, i2c.port)) { return restore_and_fail(); }
      sd_cs.activate_high();
      if (!startup_detail::prepare_sd_spi(desc_core2, *result)) { return restore_and_fail(); }
      if (!startup_detail::hold_chip_selects(desc_core2)) { return restore_and_fail(); }

      const auto& display = desc_core2.display;
      soft_spi_t bus(display.sclk, display.mosi, display.mosi, display.dc);
      bus.init();
      auto panel_id = detail::read_panel_id(bus, display.cs);
      bool reset_before_identify = false;
      if ((panel_id & detail::panel_id_mask) != detail::common_panel_id && ctx.allow_reset)
      {
        // Some valid panels do not answer until reset. Retry here because a
        // later detection attempt must not relax the caller's reset policy.
        auto forced_prepare_ctx = prepare_ctx;
        forced_prepare_ctx.allow_reset = true;
        if (!prepare_reset(desc_core2, *result, forced_prepare_ctx, i2c.port))
        {
          return restore_and_fail();
        }
        reset_before_identify = true;
        panel_id = detail::read_panel_id(bus, display.cs);
      }
      if ((panel_id & detail::panel_id_mask) != detail::common_panel_id)
      {
        return restore_and_fail();
      }

      std::uint32_t keys[4] = {};
      auto variant = detail::identify_panel_variant(
        bus, display.cs, keys, reset_before_identify ? 120 : 1);
      result->prepared |= panel_dirty;
      if (ctx.allow_reset && !reset_before_identify)
      {
        auto forced_prepare_ctx = prepare_ctx;
        forced_prepare_ctx.allow_reset = true;
        if (!prepare_reset(desc_core2, *result, forced_prepare_ctx, i2c.port))
        {
          return restore_and_fail();
        }
        std::uint32_t after_keys[4] = {};
        const auto after = detail::identify_panel_variant(
          bus, display.cs, after_keys, 120, variant != detail::panel_variant_t::e);
        if (after != detail::panel_variant_t::unknown)
        {
          variant = after;
          for (int i = 0; i < 4; ++i) { keys[i] = after_keys[i]; }
        }
      }
      if (variant == detail::panel_variant_t::e)
      {
        ESP_LOGI("board_detect_m5", "ILI9342 read-back DDh:%02x CBh:%02x -> ILI9342E",
                 static_cast<unsigned>(keys[0]), static_cast<unsigned>(keys[1]));
      }
      else if (variant == detail::panel_variant_t::c)
      {
        ESP_LOGI("board_detect_m5", "ILI9342 read-back DDh:%02x CBh:%02x ID4:%02x%02x -> ILI9342C",
                 static_cast<unsigned>(keys[0]), static_cast<unsigned>(keys[1]),
                 static_cast<unsigned>(keys[2]), static_cast<unsigned>(keys[3]));
      }
      else
      {
        ESP_LOGW("board_detect_m5", "ILI9342 read-back DDh:%02x CBh:%02x ID4:%02x%02x -> neither key answered, ILI9342C assumed",
                 static_cast<unsigned>(keys[0]), static_cast<unsigned>(keys[1]),
                 static_cast<unsigned>(keys[2]), static_cast<unsigned>(keys[3]));
      }

      const bool tough = lgfx::i2c::readRegister8(
        i2c.port, detail::tough_touch_address, detail::touch_probe_register,
        detail::tough_touch_i2c_frequency).has_value();
      result->assign(tough ? &desc_tough : &desc_core2);
      result->option = pmic->detected_option
                     | (variant == detail::panel_variant_t::e
                        ? (tough ? generated_options::tough::lcd_e
                                 : generated_options::core2::lcd_e)
                        : 0);
      if (tough) { result->option &= ~generated_options::core2::new_pmic; }
      for (auto& pin : signals) { pin.restore(); }
      sd_cs.retain();
      lcd_cs.retain();
      return true;
    }

  private:
    static const board_def_t* const members_[];
  };

  const board_def_t* const axp_family_detector_t::members_[] = {
    &board_station, &board_core2, &board_tough, nullptr
  };

  class stack_family_detector_t final : public board_detector_t
  {
  public:
    stack_family_detector_t() : board_detector_t(members_) {}

    bool signature(probe_ctx_t& ctx) const override
    {
      if (!startup_detail::description_valid(desc_stack)
       || !startup_detail::gpio_valid(desc_stack.display.dc)) { return false; }
      std::uint64_t sd_mask;
      if (!detail::sd_pull_mask(desc_stack, &sd_mask) || sd_mask == 0) { return false; }
      auto& values = ctx.detector_workspace.values;
      {
        detail::retained_pin_guard_t lcd_cs(desc_stack.display.cs);
        lcd_cs.activate_high();
        const auto pulls = probe_pin_pulls(sd_mask);
        values[0] = pulls.pulldown_high;
        values[1] = pulls.pullup_high;
      }
      const bool pull_match = values[0] == sd_mask && values[1] == sd_mask;
      const bool bypassed = !pull_match && (ctx.final_attempt || ctx.hint == board_stack.id);
      values[2] = (pull_match ? 1u : 0u) | (bypassed ? 2u : 0u);
      if (!pull_match)
      {
        ESP_LOGD("board_detect_m5",
                 "M5Stack pull signature mismatch pd=%08x%08x pu=%08x%08x",
                 static_cast<unsigned>(values[0] >> 32),
                 static_cast<unsigned>(values[0]),
                 static_cast<unsigned>(values[1] >> 32),
                 static_cast<unsigned>(values[1]));
      }
      return pull_match || bypassed;
    }

    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (!startup_detail::description_valid(desc_stack)
       || !startup_detail::gpio_valid(desc_stack.display.dc)) { return false; }
      *result = {};
      result->assign(&desc_stack);
      prepare_ctx_t prepare_ctx;
      prepare_ctx.allow_reset = ctx.allow_reset;
      prepare_ctx.i2c_port_probe = ctx.i2c_port_probe;
      lgfx::gpio::pin_backup_t signals[] = {
        desc_stack.display.sclk, desc_stack.display.miso,
        desc_stack.display.mosi, desc_stack.display.dc
      };
      detail::retained_pin_guard_t sd_cs(desc_stack.sd.sd_cs);
      detail::retained_pin_guard_t lcd_cs(desc_stack.display.cs);
      detail::retained_pin_guard_t reset(desc_stack.reset.pin);
      // Both devices are deselected before the first shared-wire operation.
      sd_cs.activate_high();
      lcd_cs.activate_high();
      if (!startup_detail::prepare_power(desc_stack, *result, ctx.i2c_port_probe)
       || !startup_detail::prepare_sd_spi(desc_stack, *result))
      {
        for (auto& pin : signals) { pin.restore(); }
        return false;
      }
      reset.activate_high();
      if (!prepare_reset(desc_stack, *result, prepare_ctx, ctx.i2c_port_probe, &result->option))
      {
        for (auto& pin : signals) { pin.restore(); }
        return false;
      }
      if (!startup_detail::hold_chip_selects(desc_stack))
      {
        for (auto& pin : signals) { pin.restore(); }
        return false;
      }
      const auto& display = desc_stack.display;
      const auto panel_id = soft_spi_read32(
        ctx, display.sclk, display.mosi, display.mosi, display.dc, display.cs,
        detail::panel_id_command, 1);
      if ((panel_id & detail::panel_id_mask) != detail::common_panel_id)
      {
        for (auto& pin : signals) { pin.restore(); }
        return false;
      }
      for (auto& pin : signals) { pin.restore(); }
      sd_cs.retain();
      lcd_cs.retain();
      reset.retain();
      const auto& values = ctx.detector_workspace.values;
      if (values[2] & 2u)
      {
        ESP_LOGI("board_detect_m5",
                 "M5Stack detected after bypassing pull signature pd=%08x%08x pu=%08x%08x",
                 static_cast<unsigned>(values[0] >> 32),
                 static_cast<unsigned>(values[0]),
                 static_cast<unsigned>(values[1] >> 32),
                 static_cast<unsigned>(values[1]));
      }
      return true;
    }

  private:
    static const board_def_t* const members_[];
  };

  const board_def_t* const stack_family_detector_t::members_[] = { &board_stack, nullptr };

  class paper_family_detector_t final : public board_detector_t
  {
  public:
    paper_family_detector_t() : board_detector_t(members_) {}

    bool signature(probe_ctx_t& ctx) const override
    {
      if (!startup_detail::description_valid(desc_paper)
       || !startup_detail::gpio_valid(desc_paper.display.busy)) { return false; }
      if (ctx.detector_workspace.active) { restore_reset(ctx); }
      static_assert(sizeof(lgfx::gpio::pin_backup_t)
                    <= sizeof(ctx.detector_workspace.object),
                    "detector workspace is too small for pin backup");
      static_assert(alignof(lgfx::gpio::pin_backup_t) <= alignof(std::uint64_t),
                    "detector workspace alignment is insufficient");
      new (ctx.detector_workspace.object) lgfx::gpio::pin_backup_t(desc_paper.reset.pin);
      ctx.detector_workspace.active = true;
      lgfx::gpio::pin_backup_t busy(desc_paper.display.busy);
      // This family contract keeps the mandatory reset from stage 1 through
      // stage 2, where prepared_reset records that it already completed.
      detail::pin_reset(desc_paper.reset.pin, true);
      lgfx::pinMode(desc_paper.display.busy, lgfx::pin_mode_t::input_pullup);
      const bool matched = !lgfx::gpio_in(desc_paper.display.busy);
      busy.restore();
      if (!matched) { restore_reset(ctx); }
      return matched;
    }

    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (!startup_detail::description_valid(desc_paper)
       || !startup_detail::gpio_valid(desc_paper.display.busy)) { return false; }
      if (!ctx.detector_workspace.active) { return false; }
      *result = {};
      result->assign(&desc_paper);
      result->prepared = prepared_reset;
      lgfx::gpio::pin_backup_t pins[] = {
        desc_paper.power.hold_pin, desc_paper.sd.sd_cs,
        desc_paper.display.mosi, desc_paper.display.miso,
        desc_paper.display.sclk, desc_paper.display.cs, desc_paper.display.busy
      };
      if (!startup_detail::prepare_power(desc_paper, *result, ctx.i2c_port_probe)
       || !startup_detail::prepare_sd_spi(desc_paper, *result))
      {
        for (auto& pin : pins) { pin.restore(); }
        restore_reset(ctx);
        return false;
      }
      if (!startup_detail::hold_chip_selects(desc_paper))
      {
        for (auto& pin : pins) { pin.restore(); }
        restore_reset(ctx);
        return false;
      }
      const auto& display = desc_paper.display;
      lgfx::pinMode(display.busy, lgfx::pin_mode_t::input);

      soft_spi_t bus(display.sclk, display.mosi, display.miso, display.dc);
      bus.init();
      bool matched = false;
      auto started = lgfx::millis();
      while (!lgfx::gpio_in(display.busy) && lgfx::millis() - started <= 1024) { lgfx::delay(1); }
      if (lgfx::gpio_in(display.busy))
      {
        bus.beginTransaction();
        lgfx::gpio_lo(display.cs);
        bus.writeData(__builtin_bswap16(0x6000), 16);
        bus.writeData(__builtin_bswap16(0x0302), 16);
        bus.wait();
        lgfx::gpio_hi(display.cs);
        started = lgfx::millis();
        while (!lgfx::gpio_in(display.busy) && lgfx::millis() - started <= 192) { lgfx::delay(1); }
        lgfx::gpio_lo(display.cs);
        bus.writeData(__builtin_bswap16(0x1000), 16);
        bus.writeData(__builtin_bswap16(0x0000), 16);
        std::uint8_t data[40] = {};
        bus.beginRead();
        bus.readBytes(data, sizeof(data));
        bus.endRead();
        bus.endTransaction();
        lgfx::gpio_hi(display.cs);
        const std::uint32_t panel_size = (std::uint32_t(data[0]) << 24)
                                       | (std::uint32_t(data[1]) << 16)
                                       | (std::uint32_t(data[2]) << 8)
                                       | data[3];
        matched = panel_size == detail::paper_panel_size;
      }
      if (!matched)
      {
        for (auto& pin : pins) { pin.restore(); }
        restore_reset(ctx);
        return false;
      }

      pins[2].restore(); // MOSI
      pins[3].restore(); // MISO
      pins[4].restore(); // SCLK
      pins[6].restore(); // busy
      release_reset(ctx); // Success retains the display reset pin high.
      return true;
    }

  private:
    static lgfx::gpio::pin_backup_t* reset_backup(probe_ctx_t& ctx)
    {
      return reinterpret_cast<lgfx::gpio::pin_backup_t*>(ctx.detector_workspace.object);
    }

    static void release_reset(probe_ctx_t& ctx)
    {
      reset_backup(ctx)->~pin_backup_t();
      ctx.detector_workspace.active = false;
    }

    static void restore_reset(probe_ctx_t& ctx)
    {
      reset_backup(ctx)->restore();
      release_reset(ctx);
    }

    static const board_def_t* const members_[];
  };

  const board_def_t* const paper_family_detector_t::members_[] = { &board_paper, nullptr };

  static const axp_family_detector_t axp_family_detector;
  static const stack_family_detector_t stack_family_detector;
  static const paper_family_detector_t paper_family_detector;

  static const board_detector_t* const esp32_d0wdq6_detectors[] = {
    &axp_family_detector,
    &stack_family_detector,
    &paper_family_detector,
    nullptr,
  };

  board_result_t detect_board_family(board_id_t board, probe_ctx_t& ctx)
  {
    return detect_board_family(esp32_d0wdq6_detectors, board, ctx);
  }

  success_log_t success_log(const board_result_t& result)
  {
    return success_log(esp32_d0wdq6_boards, result);
  }
}
}
}
