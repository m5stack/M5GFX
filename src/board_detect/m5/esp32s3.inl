// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "../board_detect.hpp"
#include "board_registry.inl"
#include "generated/esp32s3_wiring.hpp"
#include "generated/esp32s3_specs.hpp"

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

  static constexpr board_desc_t desc_atoms3 = {
    { id(lgfx::board_M5AtomS3), "M5AtomS3", 0 },
    no_power(), gpio_reset(wiring::atoms3::reset_gpio, 2, 10, reset_hold_when_skipped), no_shared_sd(),
    display_pins(wiring::atoms3::display_sclk, wiring::atoms3::display_mosi,
                 wiring::atoms3::display_miso, wiring::atoms3::display_dc,
                 wiring::atoms3::display_cs, wiring::atoms3::display_rst,
                 wiring::atoms3::display_busy),
    pins(wiring::atoms3::hold), no_internal_i2c(), no_direct_reset_panel_reload_wait(),
    options(generated_options::atoms3::names),
  };
  static const board_def_t& board_atoms3 = desc_atoms3.def;

  static constexpr board_desc_t desc_dinmeter = {
    { id(lgfx::board_M5DinMeter), "M5DinMeter", 0 },
    no_power(), gpio_reset(wiring::dinmeter::reset_gpio, 2, 10, reset_hold_when_skipped), no_shared_sd(),
    display_pins(wiring::dinmeter::display_sclk, wiring::dinmeter::display_mosi,
                 wiring::dinmeter::display_miso, wiring::dinmeter::display_dc,
                 wiring::dinmeter::display_cs, wiring::dinmeter::display_rst,
                 wiring::dinmeter::display_busy),
    pins(wiring::dinmeter::hold), no_internal_i2c(), no_direct_reset_panel_reload_wait(),
    no_options(),
  };
  static const board_def_t& board_dinmeter = desc_dinmeter.def;

  static constexpr board_desc_t desc_dial = {
    { id(lgfx::board_M5Dial), "M5Dial", 0 },
    no_power(), gpio_reset(wiring::dial::reset_gpio, 2, 10, reset_hold_when_skipped), no_shared_sd(),
    display_pins(wiring::dial::display_sclk, wiring::dial::display_mosi,
                 wiring::dial::display_miso, wiring::dial::display_dc,
                 wiring::dial::display_cs, wiring::dial::display_rst,
                 wiring::dial::display_busy),
    pins(wiring::dial::hold),
    internal_i2c(wiring::dial::internal_i2c_sda, wiring::dial::internal_i2c_scl,
                 wiring::dial::internal_i2c_port),
    no_direct_reset_panel_reload_wait(), no_options(),
  };
  static const board_def_t& board_dial = desc_dial.def;

  static const pmic_write_t sticks3_power_on[] = {
    pmic_write(specs::sticks3::pmic::i2c_addr, 0x09, 0x00, 0x00),
    pmic_write(specs::sticks3::pmic::i2c_addr, 0x16, 0x00, 0xFB),
    pmic_write(specs::sticks3::pmic::i2c_addr, 0x10, 0x04, 0xFF),
    pmic_write(specs::sticks3::pmic::i2c_addr, 0x13, 0x00, 0xFB),
    pmic_write(specs::sticks3::pmic::i2c_addr, 0x11, 0x04, 0xFF),
  };
  // Preserve the register order needed by future rollback support.
  static const std::uint8_t sticks3_restore_order[] = { 0x09, 0x11, 0x13, 0x10, 0x16 };
  static const pmic_variant_t sticks3_pmic_variants[] = {
    pmic_variant_ack_only(specs::sticks3::pmic::i2c_addr, specs::sticks3::pmic::id_reg,
                          sequence(sticks3_power_on), reg_bit(0x11, 0x04), reg_bit(0x11, 0x04),
                          no_sequence(), no_sequence(), registers(sticks3_restore_order), 0),
  };

  static constexpr board_desc_t desc_sticks3 = {
    { id(lgfx::board_M5StickS3), "M5StickS3", 0 },
    i2c_power_polled(specs::sticks3::pmic::i2c_freq, sticks3_pmic_variants, 200, 200, 200),
    gpio_reset(wiring::sticks3::reset_gpio, 2, 10, reset_hold_when_skipped), no_shared_sd(),
    display_pins(wiring::sticks3::display_sclk, wiring::sticks3::display_mosi,
                 wiring::sticks3::display_miso, wiring::sticks3::display_dc,
                 wiring::sticks3::display_cs, wiring::sticks3::display_rst,
                 wiring::sticks3::display_busy),
    pins(wiring::sticks3::hold),
    internal_i2c(wiring::sticks3::internal_i2c_sda, wiring::sticks3::internal_i2c_scl,
                 wiring::sticks3::internal_i2c_port),
    no_direct_reset_panel_reload_wait(), no_options(),
  };
  static const board_def_t& board_sticks3 = desc_sticks3.def;

  static const spi_id_probe_t atoms3_probes[] = {
    spi_id_probe(specs::atoms3::probe_st7735s::cmd, specs::atoms3::probe_st7735s::mask,
                 specs::atoms3::probe_st7735s::values, 0),
    spi_id_probe(specs::atoms3::probe_gc9107::cmd, specs::atoms3::probe_gc9107::mask,
                 specs::atoms3::probe_gc9107::values, generated_options::atoms3::gc9107),
  };
  static const spi_id_probe_t dinmeter_probes[] = {
    spi_id_probe(specs::dinmeter::probe_st7789v2::cmd, specs::dinmeter::probe_st7789v2::mask,
                 specs::dinmeter::probe_st7789v2::values, 0),
  };
  static const spi_id_probe_t dial_probes[] = {
    spi_id_probe(specs::dial::probe_gc9a01::cmd, specs::dial::probe_gc9a01::mask,
                 specs::dial::probe_gc9a01::values, 0),
  };

  struct spi_id_member_t
  {
    const board_desc_t* desc;
    const spi_id_probe_t* probes;
    std::uint8_t probe_count;
  };

  class spi_id_detector_t final : public board_detector_t
  {
  public:
    spi_id_detector_t(const board_def_t* const* members, const spi_id_member_t* members_desc,
                      std::uint8_t member_count)
    : board_detector_t(members), members_desc_(members_desc), member_count_(member_count) {}
    bool signature(probe_ctx_t&) const override { return true; }
    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      for (std::uint8_t index = 0; index < member_count_; ++index)
      {
        const auto& member = members_desc_[index];
        if (probe_spi_id(ctx, *member.desc, member.probes, member.probe_count, result)) { return true; }
      }
      return false;
    }

  private:
    const spi_id_member_t* members_desc_;
    std::uint8_t member_count_;
  };

  class pmic_id_detector_t final : public board_detector_t
  {
  public:
    pmic_id_detector_t() : board_detector_t(members_) {}
    bool signature(probe_ctx_t& ctx) const override
    {
      for (auto desc = descriptions_; *desc != nullptr; ++desc)
      {
        const auto mask = (std::uint64_t(1) << (*desc)->internal_i2c.sda)
                        | (std::uint64_t(1) << (*desc)->internal_i2c.scl);
        auto pulls = probe_pin_pulls(mask);
        if (pulls.pulldown_high == mask) { return true; }
        const auto sda_bit = std::uint64_t(1) << (*desc)->internal_i2c.sda;
        const auto scl_bit = std::uint64_t(1) << (*desc)->internal_i2c.scl;
        if (ctx.hint == (*desc)->def.id
         && (pulls.pulldown_high & scl_bit) && !(pulls.pulldown_high & sda_bit))
        {
          release_held_sda((*desc)->internal_i2c.sda, (*desc)->internal_i2c.scl);
          pulls = probe_pin_pulls(mask);
          if (pulls.pulldown_high == mask) { return true; }
        }
      }
      return false;
    }
    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (result == nullptr) { return false; }
      for (auto desc = descriptions_; *desc != nullptr; ++desc)
      {
        startup_detail::i2c_scope_t i2c(ctx.i2c_port_probe, (*desc)->internal_i2c);
        if (!i2c.opened) { continue; }
        const auto& power = (*desc)->power;
        startup_detail::retry_budget_t retry_budget(power.wake_poll_ms);
        if (startup_detail::read_variant(power, i2c.port, retry_budget) == nullptr) { continue; }
        result->assign(*desc);
        return true;
      }
      return false;
    }

  private:
    static const board_def_t* const members_[];
    static const board_desc_t* const descriptions_[];
  };
  static const board_def_t* const spi_id_members[] = { &board_atoms3, &board_dinmeter, nullptr };
  static const spi_id_member_t spi_id_member_descs[] = {
    { &desc_atoms3, atoms3_probes, sizeof(atoms3_probes) / sizeof(atoms3_probes[0]) },
    { &desc_dinmeter, dinmeter_probes, sizeof(dinmeter_probes) / sizeof(dinmeter_probes[0]) },
  };
  static const spi_id_detector_t spi_id_detector(
    spi_id_members, spi_id_member_descs,
    sizeof(spi_id_member_descs) / sizeof(spi_id_member_descs[0]));
  static const board_detector_t* const esp32s3_detectors_spi_id[] = { &spi_id_detector, nullptr };
  static const board_def_t* const dial_members[] = { &board_dial, nullptr };
  static const spi_id_member_t dial_member_descs[] = {
    { &desc_dial, dial_probes, sizeof(dial_probes) / sizeof(dial_probes[0]) },
  };
  static const spi_id_detector_t dial_detector(
    dial_members, dial_member_descs,
    sizeof(dial_member_descs) / sizeof(dial_member_descs[0]));
  static const board_detector_t* const esp32s3_detectors_dial[] = { &dial_detector, nullptr };
  const board_def_t* const pmic_id_detector_t::members_[] = { &board_sticks3, nullptr };
  const board_desc_t* const pmic_id_detector_t::descriptions_[] = { &desc_sticks3, nullptr };
  static const pmic_id_detector_t pmic_id_detector;
  static const board_detector_t* const esp32s3_detectors_pmic[] = { &pmic_id_detector, nullptr };
  static const board_detector_t* const esp32s3_detectors[] = { &dial_detector, &spi_id_detector, &pmic_id_detector, nullptr };

  bool construct_atoms3(const board_result_t& result, display_parts_t* parts);
  bool construct_dinmeter(const board_result_t& result, display_parts_t* parts);
  bool construct_dial(const board_result_t& result, display_parts_t* parts);
  bool construct_sticks3(const board_result_t& result, display_parts_t* parts);

  const char* atoms3_success_annotation(const board_result_t& result)
  {
    return result.option & generated_options::atoms3::gc9107 ? " (GC9107)" : " (ST7735)";
  }

  static const board_entry_t esp32s3_boards[] = {
    { &desc_atoms3, construct_atoms3, "board_M5AtomS3", atoms3_success_annotation },
    { &desc_dinmeter, construct_dinmeter, "board_M5DinMeter", nullptr },
    { &desc_dial, construct_dial, "board_M5Dial", nullptr },
    { &desc_sticks3, construct_sticks3, "board_M5StickS3", nullptr },
  };

  const board_desc_t* find_board_desc(board_id_t board)
  {
    return find_board_desc(esp32s3_boards, board);
  }

  board_result_t detect_board_family(board_id_t board, probe_ctx_t& ctx)
  {
    return detect_board_family(esp32s3_detectors, board, ctx);
  }

  success_log_t success_log(const board_result_t& result)
  {
    return success_log(esp32s3_boards, result);
  }
}
}
}
