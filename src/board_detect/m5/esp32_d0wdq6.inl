// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "../board_detect.hpp"
#include "board_registry.inl"
#include "generated/esp32_d0wdq6_specs.hpp"
#include "generated/esp32_d0wdq6_wiring.hpp"
#include "pmic_ops.hpp"

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

  static_assert(generated_options::core2::new_pmic == generated_options::tough::reserved,
                "Core2 new-PMIC and Tough reserved option bits must remain shared");
  static const pmic_variant_t core_pmic_variants[] = {
    pmic_variant(0x34, 0x03, 0x03, ops::list(pmic_ops::power192),
                 reg_bit(0x12, 0x04), reg_bit(0x96, 0x02),
                 ops::list(pmic_ops::reset192), ops::list(pmic_ops::release192),
                 { nullptr, 0 }, 0),
    pmic_variant(0x34, 0x03, 0x4A, ops::list(pmic_ops::power2101),
                 reg_bit(0x90, 0x08), reg_bit(0x90, 0x02),
                 ops::list(pmic_ops::reset2101), ops::list(pmic_ops::release2101),
                 { nullptr, 0 },
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
    no_options(), pins(wiring::station::hold),
  };
  static constexpr board_desc_t desc_core2 = {
    { id(lgfx::board_M5StackCore2), "M5StackCore2", 0 },
    i2c_power(400000, core_pmic_variants, pmic_ops::core2_devices, 5, 20), i2c_reset(1, 10),
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
    options(generated_options::core2::names), pins(wiring::core2::hold),
  };
  static constexpr board_desc_t desc_tough = {
    { id(lgfx::board_M5Tough), "M5Tough", 0 },
    i2c_power(400000, core_pmic_variants, pmic_ops::core2_devices, 5, 20), i2c_reset(1, 10),
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
    options(generated_options::tough::names), pins(wiring::tough::hold),
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
    pins(wiring::stack::hold), no_internal_i2c(), options(generated_options::stack::names), pins(wiring::stack::hold),
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
    pins(wiring::paper::hold), no_internal_i2c(), no_options(),
    pins(wiring::paper::hold),
  };
  // gpio_power() keeps its active-high meaning; only gpio_power_low() holds low.
  static_assert(desc_paper.power.hold_high, "Paper power hold stays active high");

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

    bool stack_reset_and_sample_ips(const board_desc_t& desc, const prepare_ctx_t& ctx,
                                    std::uint32_t* detected_option)
    {
      const auto& reset = desc.reset;
      startup_detail::pin_level(reset.pin, true);
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

  #include "esp32_pico.inl"

  construct_status_t construct_station(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_core2(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_tough(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_stack(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_paper(const board_result_t& result, display_parts_t* parts);
  static const board_entry_t esp32_d0wdq6_boards[] = {
    { &desc_station, construct_station, nullptr, nullptr },
    { &desc_core2, construct_core2, nullptr, nullptr },
    { &desc_tough, construct_tough, nullptr, nullptr },
    { &desc_stack, construct_stack, nullptr, nullptr },
    { &desc_paper, construct_paper, nullptr, nullptr },
    { &desc_stickcplus, construct_stickcplus, "M5StickCPlus", nullptr },
    { &desc_stickc, construct_stickc, "M5StickC", nullptr },
    { &desc_coreink, construct_coreink, "M5StackCoreInk", nullptr },
    { &desc_stickcplus2, construct_stickcplus2, "M5StickCPlus2", nullptr },
    { &desc_atompsram, construct_atompsram, "", nullptr },
  };

  namespace detail
  {
    bool refine_core_family(board_result_t& result, const prepare_ctx_t& ctx)
    {
      const auto& display = desc_core2.display;
      const std::int8_t signals[] = {
        display.dc, display.sclk, display.mosi, display.miso
      };
      startup_detail::pin_level(display.cs, true);
      startup_detail::pin_level(desc_core2.sd.sd_cs, true);
      soft_spi_t bus(display.sclk, display.mosi, display.mosi, display.dc);
      bus.init();
      std::uint32_t keys[4] = {};
      // Without reset, leave the same 120 ms window for a waking panel.
      auto variant = identify_panel_variant(bus, display.cs, keys,
                                            ctx.allow_reset ? 1 : 120);
      if (ctx.allow_reset)
      {
        startup_detail::i2c_scope_t i2c(*ctx.transaction, ctx.i2c_port_probe,
                                         desc_core2.internal_i2c);
        if (!i2c.opened || !prepare_reset(desc_core2, result, ctx, i2c.port))
        {
          ctx.transaction->restore_start(signals);
          return false;
        }
        std::uint32_t after_keys[4] = {};
        const auto after = identify_panel_variant(bus, display.cs, after_keys,
                                                  120, variant != panel_variant_t::e);
        if (after != panel_variant_t::unknown)
        {
          variant = after;
          for (int i = 0; i < 4; ++i) { keys[i] = after_keys[i]; }
        }
      }
      startup_detail::i2c_scope_t i2c(*ctx.transaction, ctx.i2c_port_probe,
                                       desc_core2.internal_i2c);
      if (!i2c.opened)
      {
        ctx.transaction->restore_start(signals);
        return false;
      }
      const bool tough = lgfx::i2c::readRegister8(
        i2c.port, tough_touch_address, touch_probe_register,
        tough_touch_i2c_frequency).has_value();
      // AXP192 is shared with Station. If neither LCD key answers and Tough's
      // touch is absent, a transient Station probe miss must not become Core2.
      if (!tough && variant == panel_variant_t::unknown
       && !(result.option & generated_options::core2::new_pmic))
      {
        ctx.transaction->restore_start(signals);
        return false;
      }
      log_panel_variant(variant, keys);
      result.assign(tough ? &desc_tough : &desc_core2);
      if (variant == panel_variant_t::e) { result.option |= generated_options::core2::lcd_e; }
      if (tough) { result.option &= ~generated_options::core2::new_pmic; }
      ctx.transaction->restore_start(signals);
      return true;
    }
  }

  class axp_family_detector_t final : public board_detector_t
  {
  public:
    axp_family_detector_t() : board_detector_t(members_) {}

    bool signature(probe_ctx_t& ctx) const override
    {
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
      startup_detail::i2c_scope_t i2c(*ctx.transaction, ctx.i2c_port_probe,
                                      desc_core2.internal_i2c);
      if (!i2c.opened) { return false; }
      prepare_ctx_t prepare_ctx = ctx;
      prepare_ctx.i2c_port_probe = i2c.port;
      const auto* pmic = startup_detail::read_variant(desc_core2.power, i2c.port);
      if (pmic == nullptr) { return false; }
      ESP_LOGD("board_detect_m5", "power controller id=%02x", pmic->id_value);

      std::uint64_t sd_mask;
      if (!detail::sd_pull_mask(desc_core2, &sd_mask)) { return false; }
      // Even pull probing toggles shared clocks, so deselect the LCD first.
      startup_detail::pin_level(desc_core2.display.cs, true);
      const auto sd_pulls = probe_pin_pulls(ctx, sd_mask);
      const bool sd_present = sd_pulls.pulldown_high == sd_mask
                           && sd_pulls.pullup_high == sd_mask;
      std::uint32_t preprepared = 0;
      if (sd_present)
      {
        // This exceptional pre-power transition protects the Station probe on
        // powered Core2 revisions. Unpowered cards transition after PMIC power.
        startup_detail::pin_level(desc_core2.sd.sd_cs, true);
        board_result_t sd_result;
        sd_result.assign(&desc_core2);
        if (!startup_detail::prepare_sd_spi(desc_core2, sd_result, prepare_ctx))
        {
          return false;
        }
        preprepared |= sd_result.prepared & prepared_sd_spi;
      }

      auto try_station = [&]() -> bool
      {
#if !defined (M5GFX_AUTODETECT_TEST_STATION_TO_CORE2)
        if (pmic->id_value != detail::station_pmic_id) { return false; }
#endif
        *result = {};
        result->assign(&desc_station);
        result->prepared = preprepared;
        if (!prepare_reset(desc_station, *result, prepare_ctx, i2c.port))
        {
          ctx.transaction->restore_start(desc_station.reset.pin);
          return false;
        }
        const auto& display = desc_station.display;
        const auto detected_id = soft_spi_read32(
          ctx, display.sclk, display.mosi, display.mosi, display.dc, display.cs,
          detail::panel_id_command, 1);
        if ((detected_id & detail::station_id_mask) != detail::station_id)
        {
          ctx.transaction->restore_start(desc_station.reset.pin);
          return false;
        }
        // A matching Station keeps reset and LCD CS inactive through prepare.
        // Core2's SD CS may have been raised on the way here; Station does not own it.
        ctx.transaction->restore_start(desc_core2.sd.sd_cs);
        return true;
      };

      // Station must be excluded before accepting the shared AXP signature.
      if (try_station()) { return true; }
      *result = {};
      result->assign(&desc_core2);
      result->option = pmic->detected_option;
      result->prepared = preprepared;
      result->refine = detail::refine_core_family;
      return true;
    }

  private:
    static const board_def_t* const members_[];
  };

  const board_def_t* const axp_family_detector_t::members_[] = {
    &desc_station.def, &desc_core2.def, &desc_tough.def, nullptr
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
      startup_detail::pin_level(desc_stack.display.cs, true);
      const auto pulls = probe_pin_pulls(ctx, sd_mask);
      values[0] = pulls.pulldown_high;
      values[1] = pulls.pullup_high;
      ctx.transaction->restore_start(desc_stack.display.cs);
      const bool pull_match = values[0] == sd_mask && values[1] == sd_mask;
      const bool bypassed = !pull_match && (ctx.final_attempt || ctx.hint == desc_stack.def.id);
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
      const prepare_ctx_t& prepare_ctx = ctx;
      const std::int8_t signals[] = {
        desc_stack.display.sclk, desc_stack.display.miso,
        desc_stack.display.mosi, desc_stack.display.dc
      };
      // Both devices are deselected before the first shared-wire operation.
      startup_detail::pin_level(desc_stack.sd.sd_cs, true);
      startup_detail::pin_level(desc_stack.display.cs, true);
      if (!startup_detail::prepare_power(desc_stack, *result, ctx.i2c_port_probe)
       || !startup_detail::prepare_sd_spi(desc_stack, *result, prepare_ctx)) { return false; }
      startup_detail::pin_level(desc_stack.reset.pin, true);
      if (!prepare_reset(desc_stack, *result, prepare_ctx, ctx.i2c_port_probe, &result->option))
      { return false; }
      startup_detail::hold_chip_selects(desc_stack);
      const auto& display = desc_stack.display;
      const auto panel_id = soft_spi_read32(
        ctx, display.sclk, display.mosi, display.mosi, display.dc, display.cs,
        detail::panel_id_command, 1);
      if ((panel_id & detail::panel_id_mask) != detail::common_panel_id)
      { return false; }
      ctx.transaction->restore_start(signals);
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

  const board_def_t* const stack_family_detector_t::members_[] = { &desc_stack.def, nullptr };

  class paper_family_detector_t final : public board_detector_t
  {
  public:
    paper_family_detector_t() : board_detector_t(members_) {}

    bool signature(probe_ctx_t& ctx) const override
    {
      if (!startup_detail::description_valid(desc_paper)
       || !startup_detail::gpio_valid(desc_paper.display.busy)) { return false; }
      // This family contract keeps the mandatory reset from stage 1 through
      // stage 2, where prepared_reset records that it already completed.
      startup_detail::pin_reset(desc_paper.reset, true);
      lgfx::pinMode(desc_paper.display.busy, lgfx::pin_mode_t::input_pullup);
      const bool matched = !lgfx::gpio_in(desc_paper.display.busy);
      ctx.transaction->restore_start(desc_paper.display.busy);
      if (!matched) { ctx.transaction->restore_start(desc_paper.reset.pin); }
      return matched;
    }

    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (!startup_detail::description_valid(desc_paper)
       || !startup_detail::gpio_valid(desc_paper.display.busy)) { return false; }
      *result = {};
      result->assign(&desc_paper);
      result->prepared = prepared_reset;
      const std::int8_t pins[] = {
        desc_paper.power.hold_pin, desc_paper.sd.sd_cs,
        desc_paper.display.mosi, desc_paper.display.miso,
        desc_paper.display.sclk, desc_paper.display.cs, desc_paper.display.busy,
        desc_paper.reset.pin
      };
      const std::int8_t success_restore[] = {
        desc_paper.display.mosi, desc_paper.display.miso,
        desc_paper.display.sclk, desc_paper.display.busy
      };
      const prepare_ctx_t& prepare_ctx = ctx;
      auto restore_and_fail = [&]() -> bool
      {
        ctx.transaction->restore_start(pins);
        return false;
      };
#if defined (M5GFX_AUTODETECT_TEST_FAIL_PAPER_CONFIRM)
      // Test-only failure injection after stage 1 retained reset state.
      return restore_and_fail();
#endif
      if (!startup_detail::prepare_power(desc_paper, *result, ctx.i2c_port_probe)
       || !startup_detail::prepare_sd_spi(desc_paper, *result, prepare_ctx))
      { return restore_and_fail(); }
      startup_detail::hold_chip_selects(desc_paper);
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
      { return restore_and_fail(); }

      ctx.transaction->restore_start(success_restore);
      // Success retains the display reset, power, and chip-select pins.
      return true;
    }

  private:
    static const board_def_t* const members_[];
  };

  const board_def_t* const paper_family_detector_t::members_[] = { &desc_paper.def, nullptr };

  static const axp_family_detector_t axp_family_detector;
  static const stack_family_detector_t stack_family_detector;
  static const paper_family_detector_t paper_family_detector;

  static const board_detector_t* const esp32_d0wdq6_detectors[] = {
    &axp_family_detector,
    &stack_family_detector,
    &paper_family_detector,
    nullptr,
  };

  success_log_t success_log(const board_result_t& result)
  {
    return success_log(esp32_d0wdq6_boards, result);
  }
}
}
}
