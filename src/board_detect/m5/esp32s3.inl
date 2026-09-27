// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "../board_detect.hpp"
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

  enum : std::uint8_t { atoms3_option_gc9107_index, atoms3_option_count };
  static constexpr std::uint32_t option_atoms3_gc9107 = 1u << atoms3_option_gc9107_index;
  static const char* const atoms3_options[] = { "gc9107" };
  static_assert(atoms3_option_count == sizeof(atoms3_options) / sizeof(atoms3_options[0]),
                "AtomS3 option name count must match the option bit count");

  static constexpr board_desc_t desc_atoms3 = {
    { id(lgfx::board_M5AtomS3), "M5AtomS3", 0 },
    no_power(), gpio_reset(wiring::atoms3::reset_gpio, 2, 10, reset_hold_when_skipped), no_shared_sd(),
    display_pins(wiring::atoms3::display_sclk, wiring::atoms3::display_mosi,
                 wiring::atoms3::display_miso, wiring::atoms3::display_dc,
                 wiring::atoms3::display_cs, wiring::atoms3::display_rst,
                 wiring::atoms3::display_busy),
    pins(wiring::atoms3::hold), no_internal_i2c(), no_direct_reset_panel_reload_wait(),
    options(atoms3_options),
  };
  static const board_def_t& board_atoms3 = desc_atoms3.def;

  static const pmic_write_t sticks3_power_on[] = {
    pmic_write(specs::sticks3::pmic::i2c_addr, 0x09, 0x00, 0x00),
    pmic_write(specs::sticks3::pmic::i2c_addr, 0x16, 0x00, 0xFB),
    pmic_write(specs::sticks3::pmic::i2c_addr, 0x10, 0x04, 0xFF),
    pmic_write(specs::sticks3::pmic::i2c_addr, 0x13, 0x00, 0xFB),
    pmic_write(specs::sticks3::pmic::i2c_addr, 0x11, 0x04, 0xFF),
  };
  // S3 has no restore path yet; this records what to restore and in which order when one is added.
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
                 specs::atoms3::probe_gc9107::values, option_atoms3_gc9107),
  };

  // [atoms3:family]
  // [/atoms3:family]
  class spi_id_detector_t final : public board_detector_t
  {
  public:
    spi_id_detector_t() : board_detector_t(members_) {}
    bool signature(probe_ctx_t&) const override { return true; }
    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (!probe_spi_id(ctx, desc_atoms3, atoms3_probes,
                        sizeof(atoms3_probes) / sizeof(atoms3_probes[0]), result)) { return false; }
      return true;
    }

  private:
    static const board_def_t* const members_[];
  };

  // [sticks3:family]
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
        if (ctx.hint == id(lgfx::board_M5StickS3)
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
  // [/sticks3:family]

  // [atoms3:register]
  const board_def_t* const spi_id_detector_t::members_[] = { &board_atoms3, nullptr };
  static const spi_id_detector_t spi_id_detector;
  static const board_detector_t* const esp32s3_detectors_spi_id[] = { &spi_id_detector, nullptr };
  // [/atoms3:register]
  // [sticks3:register]
  const board_def_t* const pmic_id_detector_t::members_[] = { &board_sticks3, nullptr };
  const board_desc_t* const pmic_id_detector_t::descriptions_[] = { &desc_sticks3, nullptr };
  static const board_desc_t* const esp32s3_descriptions[] = { &desc_atoms3, &desc_sticks3, nullptr };
  static const pmic_id_detector_t pmic_id_detector;
  static const board_detector_t* const esp32s3_detectors_pmic[] = { &pmic_id_detector, nullptr };
  static const board_detector_t* const esp32s3_detectors[] = { &spi_id_detector, &pmic_id_detector, nullptr };
  // [/sticks3:register]

  const board_desc_t* find_board_desc(board_id_t board)
  {
    for (auto desc = esp32s3_descriptions; *desc != nullptr; ++desc)
    {
      if ((*desc)->def.id == board) { return *desc; }
    }
    ESP_LOGD("board_detect_m5", "board=%u is not available on the new detection path",
             static_cast<unsigned>(board));
    return nullptr;
  }

  const board_def_t* find_board_def(board_id_t board)
  {
    const auto* desc = find_board_desc(board);
    return desc == nullptr ? nullptr : &desc->def;
  }

  bool prepare(board_result_t& result, const prepare_ctx_t& ctx)
  {
    if (result.desc == nullptr || result.def != &result.desc->def
     || result.def->id == board_id_unknown) { return false; }
    return board_detect::prepare(*result.desc, result, ctx);
  }

  board_result_t detect_board_family(board_id_t board, probe_ctx_t& ctx)
  {
    if (!spi_id_detector.has_member(board) && !pmic_id_detector.has_member(board)) { return {}; }
    return detect_board(esp32s3_detectors, board, ctx);
  }
}
}
}
