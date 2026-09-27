// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "../../board_detect.hpp"
#include "../pmic_ops.hpp"
#include "../board_registry.inl"
#include "../generated/esp32s3_wiring.hpp"
#include "../generated/esp32s3_specs.hpp"

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
    pins(wiring::atoms3::hold), no_internal_i2c(), options(generated_options::atoms3::names), pins(wiring::atoms3::hold),
  };

  static constexpr board_desc_t desc_atoms3r = {
    { id(lgfx::board_M5AtomS3R), "M5AtomS3R", 0 },
    no_power(), gpio_reset(wiring::atoms3r::reset_gpio, 2, 10, reset_hold_when_skipped), no_shared_sd(),
    display_pins(wiring::atoms3r::display_sclk, wiring::atoms3r::display_mosi,
                 wiring::atoms3r::display_miso, wiring::atoms3r::display_dc,
                 wiring::atoms3r::display_cs, wiring::atoms3r::display_rst,
                 wiring::atoms3r::display_busy),
    pins(wiring::atoms3r::hold), no_internal_i2c(), options(generated_options::atoms3r::names), pins(wiring::atoms3r::hold),
  };

  static constexpr board_desc_t desc_dinmeter = {
    { id(lgfx::board_M5DinMeter), "M5DinMeter", 0 },
    no_power(), gpio_reset(wiring::dinmeter::reset_gpio, 2, 10, reset_hold_when_skipped), no_shared_sd(),
    display_pins(wiring::dinmeter::display_sclk, wiring::dinmeter::display_mosi,
                 wiring::dinmeter::display_miso, wiring::dinmeter::display_dc,
                 wiring::dinmeter::display_cs, wiring::dinmeter::display_rst,
                 wiring::dinmeter::display_busy),
    pins(wiring::dinmeter::hold), no_internal_i2c(), no_options(), pins(wiring::dinmeter::hold),
  };

  static constexpr board_desc_t desc_airq = {
    { id(lgfx::board_M5AirQ), "M5AirQ", 0 },
    gpio_power(wiring::airq::power_gpio),
    gpio_reset(wiring::airq::reset_gpio, 2, 10, reset_always), no_shared_sd(),
    display_pins(wiring::airq::display_sclk, wiring::airq::display_mosi,
                 wiring::airq::display_miso, wiring::airq::display_dc,
                 wiring::airq::display_cs, wiring::airq::display_rst,
                 wiring::airq::display_busy),
    pins(wiring::airq::hold), no_internal_i2c(), options(generated_options::airq::names), pins(wiring::airq::hold),
  };
  // gpio_power() keeps its active-high meaning; only gpio_power_low() holds low.
  static_assert(desc_airq.power.hold_high, "AirQ power hold stays active high");

  static constexpr board_desc_t desc_stamplc = {
    { id(lgfx::board_M5StamPLC), "M5StamPLC", 0 },
    no_power(), gpio_reset(wiring::stamplc::reset_gpio, 2, 10, reset_hold_when_skipped),
    shared_sd(wiring::stamplc::shared_sd_sclk, wiring::stamplc::shared_sd_mosi,
              wiring::stamplc::shared_sd_miso, wiring::stamplc::shared_sd_sd_cs,
              wiring::stamplc::shared_sd_other_cs),
    display_pins(wiring::stamplc::display_sclk, wiring::stamplc::display_mosi,
                 wiring::stamplc::display_miso, wiring::stamplc::display_dc,
                 wiring::stamplc::display_cs, wiring::stamplc::display_rst,
                 wiring::stamplc::display_busy),
    pins(wiring::stamplc::hold), no_internal_i2c(), no_options(), pins(wiring::stamplc::hold),
  };

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
    no_options(), pins(wiring::dial::hold),
  };

  static constexpr board_desc_t desc_cardputer = {
    { id(lgfx::board_M5Cardputer), "M5Cardputer", 0 },
    no_power(), gpio_reset(wiring::cardputer::reset_gpio, 2, 10, reset_hold_when_skipped), no_shared_sd(),
    display_pins(wiring::cardputer::display_sclk, wiring::cardputer::display_mosi,
                 wiring::cardputer::display_miso, wiring::cardputer::display_dc,
                 wiring::cardputer::display_cs, wiring::cardputer::display_rst,
                 wiring::cardputer::display_busy),
    pins(wiring::cardputer::hold), no_internal_i2c(), no_options(), pins(wiring::cardputer::hold),
  };

  static constexpr board_desc_t desc_cardputer_adv = {
    { id(lgfx::board_M5CardputerADV), "M5CardputerADV", 0 },
    no_power(), gpio_reset(wiring::cardputer_adv::reset_gpio, 2, 10, reset_hold_when_skipped), no_shared_sd(),
    display_pins(wiring::cardputer_adv::display_sclk, wiring::cardputer_adv::display_mosi,
                 wiring::cardputer_adv::display_miso, wiring::cardputer_adv::display_dc,
                 wiring::cardputer_adv::display_cs, wiring::cardputer_adv::display_rst,
                 wiring::cardputer_adv::display_busy),
    pins(wiring::cardputer_adv::hold), no_internal_i2c(),
    no_options(), pins(wiring::cardputer_adv::hold),
  };

  static constexpr board_desc_t desc_vameter = {
    { id(lgfx::board_M5VAMeter), "M5VAMeter", 0 },
    no_power(), gpio_reset(wiring::vameter::reset_gpio, 2, 10, reset_hold_when_skipped), no_shared_sd(),
    display_pins(wiring::vameter::display_sclk, wiring::vameter::display_mosi,
                 wiring::vameter::display_miso, wiring::vameter::display_dc,
                 wiring::vameter::display_cs, wiring::vameter::display_rst,
                 wiring::vameter::display_busy),
    pins(wiring::vameter::hold), no_internal_i2c(),
    no_options(), pins(wiring::vameter::hold),
  };

  // Preserve the register order needed by future rollback support.
  static const std::uint8_t sticks3_restore_order[] = { 0x09, 0x11, 0x13, 0x10, 0x16 };
  static const pmic_variant_t sticks3_pmic_variants[] = {
    pmic_variant_ack_only(specs::sticks3::pmic::i2c_addr, specs::sticks3::pmic::id_reg,
                          ops::list(pmic_ops::sticks3_power_on),
                          reg_bit(0x11, 0x04), reg_bit(0x11, 0x04),
                          ops::no_ops(), ops::no_ops(), registers(sticks3_restore_order), 0),
  };

  static constexpr board_desc_t desc_sticks3 = {
    { id(lgfx::board_M5StickS3), "M5StickS3", 0 },
    i2c_power_polled(specs::sticks3::pmic::i2c_freq, sticks3_pmic_variants,
                     pmic_ops::pm1_devices, 200, 200, 200),
    gpio_reset(wiring::sticks3::reset_gpio, 2, 10, reset_hold_when_skipped), no_shared_sd(),
    display_pins(wiring::sticks3::display_sclk, wiring::sticks3::display_mosi,
                 wiring::sticks3::display_miso, wiring::sticks3::display_dc,
                 wiring::sticks3::display_cs, wiring::sticks3::display_rst,
                 wiring::sticks3::display_busy),
    pins(wiring::sticks3::hold),
    internal_i2c(wiring::sticks3::internal_i2c_sda, wiring::sticks3::internal_i2c_scl,
                 wiring::sticks3::internal_i2c_port),
    no_options(), pins(wiring::sticks3::hold),
  };

  static const pmic_variant_t stopwatch_pmic_variants[] = {
    pmic_variant_ack_only(specs::stopwatch::pmic::i2c_addr,
                          specs::stopwatch::pmic::id_reg,
                          ops::list(pmic_ops::stopwatch_power_on),
                          reg_bit(0, 0), reg_bit(0, 0), ops::no_ops(), ops::no_ops(),
                          { nullptr, 0 }, 0),
  };
  static const pmic_variant_t papermono_pmic_variants[] = {
    pmic_variant_ack_only(specs::papermono::pmic::i2c_addr,
                          specs::papermono::pmic::id_reg,
#if !(defined(CONFIG_ESP32S3_SPIRAM_SUPPORT)) || !defined(CONFIG_SPIRAM_MODE_OCT)
                          ops::list(pmic_ops::papermono_no_display_power_on),
#else
                          ops::list(pmic_ops::papermono_power_on),
#endif
                          reg_bit(0, 0), reg_bit(0, 0), ops::no_ops(), ops::no_ops(),
                          { nullptr, 0 }, 0),
  };
  static constexpr board_desc_t desc_stopwatch = {
    { id(lgfx::board_M5StopWatch), "M5StopWatch", 0 },
    i2c_power_confirmed(specs::stopwatch::pmic::i2c_freq, stopwatch_pmic_variants,
                        pmic_ops::pm1_family_devices),
    no_reset(), no_shared_sd(),
    // display_pins predates QSPI; io0 occupies its mandatory MOSI slot for validation.
    display_pins(wiring::stopwatch::display_sclk, wiring::stopwatch::display_io0,
                 wiring::stopwatch::display_miso, wiring::stopwatch::display_dc,
                 wiring::stopwatch::display_cs, wiring::stopwatch::display_rst,
                 wiring::stopwatch::display_busy),
    pins(wiring::stopwatch::hold),
    internal_i2c(wiring::stopwatch::internal_i2c_sda, wiring::stopwatch::internal_i2c_scl,
                 wiring::stopwatch::internal_i2c_port),
    no_options(), pins(wiring::stopwatch::hold),
  };
  static constexpr board_desc_t desc_papermono = {
    { id(lgfx::board_M5PaperMono), "M5PaperMono", 0 },
    i2c_power_confirmed(specs::papermono::pmic::i2c_freq, papermono_pmic_variants,
                        pmic_ops::pm1_family_devices),
    no_reset(), no_shared_sd(),
    display_pins(wiring::papermono::display_sclk, wiring::papermono::display_mosi,
                 wiring::papermono::display_miso, wiring::papermono::display_dc,
                 wiring::papermono::display_cs, wiring::papermono::display_rst,
                 wiring::papermono::display_busy),
    pins(wiring::papermono::hold),
    internal_i2c(wiring::papermono::internal_i2c_sda, wiring::papermono::internal_i2c_scl,
                 wiring::papermono::internal_i2c_port),
    no_options(), pins(wiring::papermono::hold),
  };

  static const pmic_variant_t chaincaptain_pmic_variants[] = {
    pmic_variant_ack_only(specs::chaincaptain::pmic::i2c_addr,
                          specs::chaincaptain::pmic::id_reg,
#if !(defined(CONFIG_ESP32S3_SPIRAM_SUPPORT)) || !defined(CONFIG_SPIRAM_MODE_OCT)
                          ops::no_ops(), reg_bit(0, 0), reg_bit(0, 0),
                          ops::no_ops(), ops::no_ops(), { nullptr, 0 }, 0),
#else
                          ops::list(pmic_ops::chaincaptain_power_on),
                          reg_bit(0, 0), reg_bit(0, 0),
                          ops::list(pmic_ops::chaincaptain_reset_assert),
                          ops::list(pmic_ops::chaincaptain_reset_release), { nullptr, 0 }, 0),
#endif
  };
  static const pmic_variant_t papercolor_pmic_variants[] = {
    pmic_variant_ack_only(specs::papercolor::pmic::i2c_addr,
                          specs::papercolor::pmic::id_reg,
#if !(defined(CONFIG_ESP32S3_SPIRAM_SUPPORT)) || !defined(CONFIG_SPIRAM_MODE_OCT)
                          ops::no_ops(),
#else
                          ops::list(pmic_ops::papercolor_power_on),
#endif
                          reg_bit(0, 0), reg_bit(0, 0), ops::no_ops(), ops::no_ops(),
                          { nullptr, 0 }, 0),
  };
  static constexpr board_desc_t desc_chaincaptain = {
    { id(lgfx::board_M5ChainCaptain), "M5ChainCaptain", 0 },
    i2c_power_confirmed(specs::chaincaptain::pmic::i2c_freq,
                        chaincaptain_pmic_variants, pmic_ops::pm1_family_devices),
#if !(defined(CONFIG_ESP32S3_SPIRAM_SUPPORT)) || !defined(CONFIG_SPIRAM_MODE_OCT)
    no_reset(),
#else
    i2c_reset(10, 20, reset_hold_when_skipped),
#endif
    no_shared_sd(),
    display_pins(wiring::chaincaptain::display_sclk, wiring::chaincaptain::display_mosi,
                 wiring::chaincaptain::display_miso, wiring::chaincaptain::display_dc,
                 wiring::chaincaptain::display_cs, wiring::chaincaptain::display_rst,
                 wiring::chaincaptain::display_busy),
    pins(wiring::chaincaptain::hold),
    internal_i2c(wiring::chaincaptain::internal_i2c_sda,
                 wiring::chaincaptain::internal_i2c_scl,
                 wiring::chaincaptain::internal_i2c_port),
    no_options(), pins(wiring::chaincaptain::hold),
  };
  static constexpr board_desc_t desc_papercolor = {
    { id(lgfx::board_M5PaperColor), "M5PaperColor", 0 },
    i2c_power_confirmed(specs::papercolor::pmic::i2c_freq,
                        papercolor_pmic_variants, pmic_ops::pm1_devices),
#if !(defined(CONFIG_ESP32S3_SPIRAM_SUPPORT)) || !defined(CONFIG_SPIRAM_MODE_OCT)
    no_reset(), no_shared_sd(),
#else
    gpio_reset(wiring::papercolor::reset_gpio, 2, 10, reset_hold_when_skipped),
    shared_sd(wiring::papercolor::shared_sd_sclk, wiring::papercolor::shared_sd_mosi,
              wiring::papercolor::shared_sd_miso, wiring::papercolor::shared_sd_sd_cs,
              wiring::papercolor::shared_sd_other_cs),
#endif
    display_pins(wiring::papercolor::display_sclk, wiring::papercolor::display_mosi,
                 wiring::papercolor::display_miso, wiring::papercolor::display_dc,
                 wiring::papercolor::display_cs, wiring::papercolor::display_rst,
                 wiring::papercolor::display_busy),
    pins(wiring::papercolor::hold),
    internal_i2c(wiring::papercolor::internal_i2c_sda,
                 wiring::papercolor::internal_i2c_scl,
                 wiring::papercolor::internal_i2c_port),
    no_options(), pins(wiring::papercolor::hold),
  };

  // Paper family: a parallel EPD on the LCD peripheral, so the description
  // carries no SPI display pins. PaperS3 holds its power-off request line low
  // after confirmation (PWROFF_PULSE, active low); PaperDIY powers the EPD
  // through the M5PM1 instead. Both boards share the same internal I2C pins.
  static_assert(specs::papers3::power_hold::active_low,
                "PaperS3 power hold description follows the catalog level");
  static constexpr board_desc_t desc_papers3 = {
    { id(lgfx::board_M5PaperS3), "M5PaperS3", 0 },
    gpio_power_low(wiring::papers3::power_gpio), no_reset(), no_shared_sd(),
    no_display_pins(), no_pins(),
    internal_i2c(wiring::papers3::internal_i2c_sda, wiring::papers3::internal_i2c_scl,
                 wiring::papers3::internal_i2c_port),
    no_options(), no_pins(),
  };
  static const pmic_variant_t paperdiy_pmic_variants[] = {
    pmic_variant_ack_only(specs::paperdiy::pmic::i2c_addr, specs::paperdiy::pmic::id_reg,
                          ops::list(pmic_ops::paperdiy_power_on),
                          reg_bit(0, 0), reg_bit(0, 0), ops::no_ops(), ops::no_ops(),
                          { nullptr, 0 }, 0),
  };
  static constexpr board_desc_t desc_paperdiy = {
    { id(lgfx::board_M5PaperDIY), "M5PaperDIY", 0 },
    i2c_power_confirmed(specs::paperdiy::pmic::i2c_freq, paperdiy_pmic_variants,
                        pmic_ops::pm1_devices),
    no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
    // hw_port = -1: the legacy path released the probe port after the PM1
    // writes and never opened the hardware port (no touch on PaperDIY).
    internal_i2c(wiring::paperdiy::internal_i2c_sda, wiring::paperdiy::internal_i2c_scl, -1),
    no_options(), no_pins(),
  };
  static_assert(wiring::papers3::internal_i2c_sda == wiring::paperdiy::internal_i2c_sda
             && wiring::papers3::internal_i2c_scl == wiring::paperdiy::internal_i2c_scl,
                "Paper family members share one internal I2C pin pair");

  static const spi_id_probe_t atoms3_probes[] = {
    spi_id_probe(specs::atoms3::probe_st7735s::cmd, specs::atoms3::probe_st7735s::mask,
                 specs::atoms3::probe_st7735s::values, 0),
    spi_id_probe(specs::atoms3::probe_gc9107::cmd, specs::atoms3::probe_gc9107::mask,
                 specs::atoms3::probe_gc9107::values, generated_options::atoms3::gc9107),
  };
  static const spi_id_probe_t atoms3r_probes[] = {
    spi_id_probe(specs::atoms3r::probe_st7735s::cmd, specs::atoms3r::probe_st7735s::mask,
                 specs::atoms3r::probe_st7735s::values, 0),
    spi_id_probe(specs::atoms3r::probe_gc9107::cmd, specs::atoms3r::probe_gc9107::mask,
                 specs::atoms3r::probe_gc9107::values, generated_options::atoms3r::gc9107),
  };
  static const spi_id_probe_t dinmeter_probes[] = {
    spi_id_probe(specs::dinmeter::probe_st7789v2::cmd, specs::dinmeter::probe_st7789v2::mask,
                 specs::dinmeter::probe_st7789v2::values, 0),
  };
  static constexpr spi_id_probe_t airq_probes[] = {
    spi_id_probe(specs::airq::probe_gdew0154d67::cmd,
                 specs::airq::probe_gdew0154d67::mask,
                 specs::airq::probe_gdew0154d67::values, 0,
                 specs::airq::probe_gdew0154d67::dummy_bits),
    spi_id_probe(specs::airq::probe_gdew0154m09::cmd,
                 specs::airq::probe_gdew0154m09::mask,
                 specs::airq::probe_gdew0154m09::values,
                 generated_options::airq::m09,
                 specs::airq::probe_gdew0154m09::dummy_bits),
  };
  static const spi_id_probe_t dial_probes[] = {
    spi_id_probe(specs::dial::probe_gc9a01::cmd, specs::dial::probe_gc9a01::mask,
                 specs::dial::probe_gc9a01::values, 0),
  };
  static const spi_id_probe_t cardputer_probes[] = {
    spi_id_probe(specs::cardputer::probe_st7789v2::cmd,
                 specs::cardputer::probe_st7789v2::mask,
                 specs::cardputer::probe_st7789v2::values, 0),
  };
  static const spi_id_probe_t stamplc_probes[] = {
    spi_id_probe(specs::stamplc::probe_st7789v2::cmd,
                 specs::stamplc::probe_st7789v2::mask,
                 specs::stamplc::probe_st7789v2::values, 0),
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
        auto pulls = probe_pin_pulls(ctx, mask);
        if (pulls.pulldown_high == mask) { return true; }
        // A reset may leave SDA held low even when there is no saved hint.
        pulls = recover_held_sda_and_resample(
          ctx, pulls, mask, (*desc)->internal_i2c.sda, (*desc)->internal_i2c.scl);
        if (pulls.pulldown_high == mask) { return true; }
      }
      return false;
    }
    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (result == nullptr) { return false; }
      for (auto desc = descriptions_; *desc != nullptr; ++desc)
      {
        startup_detail::i2c_scope_t i2c(*ctx.transaction, ctx.i2c_port_probe,
                                        (*desc)->internal_i2c);
        if (!i2c.opened) { continue; }
        const auto& power = (*desc)->power;
        startup_detail::retry_budget_t retry_budget(power.wake_poll_ms);
        if (startup_detail::read_variant(power, i2c.port, retry_budget) == nullptr) { continue; }
        // PM1 is identified by both bytes at register 0, not an ACK alone.
        const std::uint8_t reg = specs::sticks3::pmic::id_reg;
        std::uint8_t pm1_id[2] = {};
        if (!lgfx::i2c::transactionWriteRead(
              i2c.port, specs::sticks3::pmic::i2c_addr, &reg, 1,
              pm1_id, sizeof(pm1_id), specs::sticks3::pmic::i2c_freq).has_value()
         || (static_cast<std::uint16_t>(pm1_id[1]) << 8 | pm1_id[0])
              != pmic_ops::pm1_device_id)
        { continue; }
        result->assign(*desc);
        return true;
      }
      return false;
    }

  private:
    static const board_def_t* const members_[];
    static const board_desc_t* const descriptions_[];
  };

  class pm1_family_detector_t final : public board_detector_t
  {
  public:
    pm1_family_detector_t() : board_detector_t(members_) {}
    bool signature(probe_ctx_t& ctx) const override
    {
      return probe_i2c_ack(ctx, wiring::stopwatch::internal_i2c_sda,
                           wiring::stopwatch::internal_i2c_scl, 0x32)
          && probe_i2c_ack(ctx, wiring::stopwatch::internal_i2c_sda,
                           wiring::stopwatch::internal_i2c_scl, 0x68);
    }
    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (result == nullptr) { return false; }
      std::uint8_t pm1_id[2] = {};
      if (!probe_i2c_read(ctx, wiring::stopwatch::internal_i2c_sda,
                          wiring::stopwatch::internal_i2c_scl, 0x6E, 0,
                          pm1_id, sizeof(pm1_id), 100000, 200)
       || (static_cast<std::uint16_t>(pm1_id[1]) << 8 | pm1_id[0]) != 0x2050)
      {
        return false;
      }
      std::uint8_t ioe_id[2] = {};
      if (!probe_i2c_read(ctx, wiring::stopwatch::internal_i2c_sda,
                          wiring::stopwatch::internal_i2c_scl, 0x4F, 0,
                          ioe_id, sizeof(ioe_id), 100000, 200))
      {
        return false;
      }
      // Each member must answer at its own touch address; two misses (or two
      // answers) do not identify either board.
      const bool stopwatch_touch = probe_i2c_ack(
        ctx, wiring::stopwatch::internal_i2c_sda,
        wiring::stopwatch::internal_i2c_scl, specs::stopwatch::touch::i2c_addr);
      const bool papermono_touch = probe_i2c_ack(
        ctx, wiring::papermono::internal_i2c_sda,
        wiring::papermono::internal_i2c_scl, specs::papermono::touch::i2c_addr);
      if (stopwatch_touch == papermono_touch) { return false; }
      result->assign(stopwatch_touch ? &desc_stopwatch : &desc_papermono);
      return true;
    }

  private:
    static const board_def_t* const members_[];
  };

  class pm1_ext_family_detector_t final : public board_detector_t
  {
  public:
    pm1_ext_family_detector_t() : board_detector_t(members_) {}
    bool signature(probe_ctx_t& ctx) const override
    {
      static constexpr std::uint8_t chain_addresses[] = { 0x32, 0x4F, 0x68, 0x6E };
      bool all_ack = true;
      for (auto addr : chain_addresses)
      {
        all_ack &= probe_i2c_ack(ctx, wiring::chaincaptain::internal_i2c_sda,
                                 wiring::chaincaptain::internal_i2c_scl, addr);
      }
      if (all_ack)
      {
        candidate_ = candidate_t::chaincaptain;
        return true;
      }
      static constexpr std::uint8_t paper_addresses[] = { 0x32, 0x44 };
      all_ack = true;
      for (auto addr : paper_addresses)
      {
        all_ack &= probe_i2c_ack(ctx, wiring::papercolor::internal_i2c_sda,
                                 wiring::papercolor::internal_i2c_scl, addr);
      }
      candidate_ = all_ack ? candidate_t::papercolor : candidate_t::none;
      return all_ack;
    }
    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (result == nullptr || candidate_ == candidate_t::none) { return false; }
      std::uint8_t pm1_id[2] = {};
      if (!probe_i2c_read(ctx, wiring::chaincaptain::internal_i2c_sda,
                          wiring::chaincaptain::internal_i2c_scl,
                          pmic_ops::pm1_i2c_addr, 0, pm1_id, sizeof(pm1_id),
                          pmic_ops::pm1_i2c_freq, 200)
       || (static_cast<std::uint16_t>(pm1_id[1]) << 8 | pm1_id[0])
            != pmic_ops::pm1_device_id)
      {
        return false;
      }
      if (candidate_ == candidate_t::chaincaptain)
      {
        std::uint8_t ioe_id[2] = {};
        if (probe_i2c_read(ctx, wiring::chaincaptain::internal_i2c_sda,
                           wiring::chaincaptain::internal_i2c_scl,
                           pmic_ops::ioe1_i2c_addr, 0, ioe_id, sizeof(ioe_id),
                           pmic_ops::pm1_i2c_freq, 200))
        {
          result->assign(&desc_chaincaptain);
          return true;
        }
        // PaperColor also answers the ChainCaptain addresses; without IOE1,
        // accept only PaperColor's own SHT40 below.
      }
      // 0x44 on PaperColor is the SHT40 humidity sensor (no ID register).
      if (!probe_i2c_ack(ctx, wiring::papercolor::internal_i2c_sda,
                         wiring::papercolor::internal_i2c_scl, 0x44))
      { return false; }
      result->assign(&desc_papercolor);
      return true;
    }

  private:
    enum class candidate_t : std::uint8_t { none, chaincaptain, papercolor };
    static const board_def_t* const members_[];
    mutable candidate_t candidate_ = candidate_t::none;
  };

  // PaperDIY and PaperS3 share the internal I2C pins, and DinMeter's encoder
  // and button sit on the same GPIOs. The signature only checks that both
  // lines carry an external pull-up (they read high against the internal
  // pull-down); confirmation reads device IDs and never writes. Recover only
  // when SCL is high and SDA is low, so DinMeter's pins incur no SCL wait.
  class paper_family_detector_t final : public board_detector_t
  {
  public:
    paper_family_detector_t() : board_detector_t(members_) {}
    bool signature(probe_ctx_t& ctx) const override
    {
      const auto mask = (std::uint64_t(1) << wiring::papers3::internal_i2c_sda)
                      | (std::uint64_t(1) << wiring::papers3::internal_i2c_scl);
      auto pulls = probe_pin_pulls(ctx, mask);
      pulls = recover_held_sda_and_resample(
        ctx, pulls, mask, wiring::papers3::internal_i2c_sda,
        wiring::papers3::internal_i2c_scl);
      return pulls.pulldown_high == mask;
    }
    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (result == nullptr) { return false; }
      // The legacy block skipped the PM1 read when hinted PaperS3 and the
      // GT911 read when hinted PaperDIY; each skip saves up to 200 ms or two
      // touch-controller transactions, so the hint keeps that meaning here.
      if (ctx.hint != desc_papers3.def.id)
      {
        std::uint8_t pm1_id[2] = {};
        if (probe_i2c_read(ctx, wiring::papers3::internal_i2c_sda,
                           wiring::papers3::internal_i2c_scl,
                           pmic_ops::pm1_i2c_addr, 0, pm1_id, sizeof(pm1_id),
                           pmic_ops::pm1_i2c_freq, 200)
         && (static_cast<std::uint16_t>(pm1_id[1]) << 8 | pm1_id[0]) == pmic_ops::pm1_device_id)
        {
          result->assign(&desc_paperdiy);
          return true;
        }
      }
      if (ctx.hint != desc_paperdiy.def.id)
      {
        // An ACK alone is not enough to identify the PaperS3 touch controller
        // (DinMeter shares these pins): require the GT911 product ID string.
        static constexpr std::uint8_t gt911_addresses[] = { 0x14, 0x5D };
        static constexpr std::uint16_t gt911_product_id_reg = 0x8140;
        for (auto addr : gt911_addresses)
        {
          std::uint8_t product_id[4] = {};
          if (probe_i2c_read(ctx, wiring::papers3::internal_i2c_sda,
                               wiring::papers3::internal_i2c_scl, addr,
                               gt911_product_id_reg, product_id, sizeof(product_id),
                               specs::papers3::touch::i2c_freq, 0, true)
           && product_id[0] == '9' && product_id[1] == '1'
           && product_id[2] == '1' && product_id[3] == 0)
          {
            result->assign(&desc_papers3);
            return true;
          }
        }
      }
      return false;
    }

  private:
    static const board_def_t* const members_[];
  };

  class cardputer_family_detector_t final : public board_detector_t
  {
  public:
    cardputer_family_detector_t() : board_detector_t(members_) {}
    bool signature(probe_ctx_t&) const override { return true; }
    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (result == nullptr || ctx.conditional_pins_unavailable) { return false; }
      if (!probe_spi_id(ctx, desc_cardputer, cardputer_probes,
                        sizeof(cardputer_probes) / sizeof(cardputer_probes[0]), result,
                        specs::cardputer::bus_three_wire))
      {
        return false;
      }

      std::uint64_t sense_mask = 0;
      for (const auto pin : wiring::cardputer::cardputer_subdivision::sense_pins)
      {
        sense_mask |= std::uint64_t(1) << pin;
      }
      auto pulls = probe_pin_pulls(ctx, sense_mask);
      const std::uint64_t vameter_mask =
        (std::uint64_t(1) << wiring::cardputer::cardputer_subdivision::vameter_i2c_sda)
        | (std::uint64_t(1) << wiring::cardputer::cardputer_subdivision::vameter_i2c_scl);
      const std::uint64_t adv_mask =
        (std::uint64_t(1) << wiring::cardputer_adv::internal_i2c_sda)
        | (std::uint64_t(1) << wiring::cardputer_adv::internal_i2c_scl);

      // These pins also serve non-I2C variants. Recover a held SDA only when
      // SCL is already high; a low SCL must not add a clock-stretch wait.
      pulls = recover_held_sda_and_resample(
        ctx, pulls, sense_mask,
        wiring::cardputer::cardputer_subdivision::vameter_i2c_sda,
        wiring::cardputer::cardputer_subdivision::vameter_i2c_scl);
      pulls = recover_held_sda_and_resample(
        ctx, pulls, sense_mask, wiring::cardputer_adv::internal_i2c_sda,
        wiring::cardputer_adv::internal_i2c_scl);

      const board_desc_t* chosen = &desc_cardputer;
      if ((pulls.pulldown_high & vameter_mask) == vameter_mask)
      {
        // INA226 manufacturer ID register FEh is 5449h (TI). The second
        // VAMeter device must also answer; a probe miss cannot prove Cardputer.
        std::uint8_t manufacturer[2] = {};
        if (!probe_i2c_read(ctx,
                            wiring::cardputer::cardputer_subdivision::vameter_i2c_sda,
                            wiring::cardputer::cardputer_subdivision::vameter_i2c_scl,
                            wiring::cardputer::cardputer_subdivision::vameter_i2c_addrs[0],
                            0xFE, manufacturer, sizeof(manufacturer), 100000, 0, false)
         || manufacturer[0] != 0x54 || manufacturer[1] != 0x49
         || !probe_i2c_ack(ctx,
                           wiring::cardputer::cardputer_subdivision::vameter_i2c_sda,
                           wiring::cardputer::cardputer_subdivision::vameter_i2c_scl,
                           wiring::cardputer::cardputer_subdivision::vameter_i2c_addrs[1]))
        { return false; }
        chosen = &desc_vameter;
      }
      else if ((pulls.pulldown_high & adv_mask) == adv_mask)
      {
        // The ADV keyboard scanner is a TCA8418 at its fixed 34h address.
        if (!probe_i2c_ack(ctx, wiring::cardputer_adv::internal_i2c_sda,
                           wiring::cardputer_adv::internal_i2c_scl, 0x34))
        { return false; }
        chosen = &desc_cardputer_adv;
      }
      result->assign(chosen);
      return true;
    }

  private:
    static const board_def_t* const members_[];
  };
#include "cores3.inl"
  static const board_def_t* const spi_id_members[] = { &desc_atoms3.def, &desc_dinmeter.def, nullptr };
  static constexpr spi_id_member_t spi_id_member_descs[] = {
    { &desc_atoms3, atoms3_probes, sizeof(atoms3_probes) / sizeof(atoms3_probes[0]),
      specs::atoms3::bus_three_wire, wiring::atoms3::touches_opi_pins, 0, false },
    { &desc_dinmeter, dinmeter_probes, sizeof(dinmeter_probes) / sizeof(dinmeter_probes[0]),
      specs::dinmeter::bus_three_wire, wiring::dinmeter::touches_opi_pins, 0, false },
  };
  static const spi_id_detector_t spi_id_detector(
    spi_id_members, spi_id_member_descs,
    sizeof(spi_id_member_descs) / sizeof(spi_id_member_descs[0]));
  static const board_def_t* const dial_members[] = { &desc_dial.def, nullptr };
  static constexpr spi_id_member_t dial_member_descs[] = {
    { &desc_dial, dial_probes, sizeof(dial_probes) / sizeof(dial_probes[0]),
      specs::dial::bus_three_wire, wiring::dial::touches_opi_pins, 0, false },
  };
  static const spi_id_detector_t dial_detector(
    dial_members, dial_member_descs,
    sizeof(dial_member_descs) / sizeof(dial_member_descs[0]));
  static const board_def_t* const atoms3r_members[] = { &desc_atoms3r.def, nullptr };
  static constexpr spi_id_member_t atoms3r_member_descs[] = {
    { &desc_atoms3r, atoms3r_probes, sizeof(atoms3r_probes) / sizeof(atoms3r_probes[0]),
      specs::atoms3r::bus_three_wire, wiring::atoms3r::touches_opi_pins, 5, false },
  };
  static const spi_id_detector_t atoms3r_detector(
    atoms3r_members, atoms3r_member_descs,
    sizeof(atoms3r_member_descs) / sizeof(atoms3r_member_descs[0]));

  static const board_def_t* const airq_members[] = { &desc_airq.def, nullptr };
  static constexpr spi_id_member_t airq_member_descs[] = {
    { &desc_airq, airq_probes, sizeof(airq_probes) / sizeof(airq_probes[0]),
      specs::airq::bus_three_wire, wiring::airq::touches_opi_pins, 0, false },
  };
  static const spi_id_detector_t airq_detector(
    airq_members, airq_member_descs,
    sizeof(airq_member_descs) / sizeof(airq_member_descs[0]));

  static const board_def_t* const stamplc_members[] = { &desc_stamplc.def, nullptr };
  static constexpr spi_id_member_t stamplc_member_descs[] = {
    { &desc_stamplc, stamplc_probes, sizeof(stamplc_probes) / sizeof(stamplc_probes[0]),
      specs::stamplc::bus_three_wire, wiring::stamplc::touches_opi_pins, 0, false },
  };
  static const spi_id_detector_t stamplc_detector(
    stamplc_members, stamplc_member_descs,
    sizeof(stamplc_member_descs) / sizeof(stamplc_member_descs[0]));

  // Shared-SD preparation must not add hardware steps to existing members.
  static_assert(desc_atoms3.sd.sd_cs < 0 && desc_dinmeter.sd.sd_cs < 0
             && desc_dial.sd.sd_cs < 0 && desc_atoms3r.sd.sd_cs < 0,
                "existing ESP32-S3 SPI ID members do not prepare shared SD");

  const board_def_t* const cardputer_family_detector_t::members_[] = {
    &desc_cardputer.def, &desc_cardputer_adv.def, &desc_vameter.def, nullptr,
  };
  static const cardputer_family_detector_t cardputer_family_detector;
  const board_def_t* const pmic_id_detector_t::members_[] = { &desc_sticks3.def, nullptr };
  const board_desc_t* const pmic_id_detector_t::descriptions_[] = { &desc_sticks3, nullptr };
  static const pmic_id_detector_t pmic_id_detector;
  const board_def_t* const pm1_family_detector_t::members_[] = {
    &desc_stopwatch.def, &desc_papermono.def, nullptr,
  };
  static const pm1_family_detector_t pm1_family_detector;
  const board_def_t* const pm1_ext_family_detector_t::members_[] = {
    &desc_chaincaptain.def, &desc_papercolor.def, nullptr,
  };
  static const pm1_ext_family_detector_t pm1_ext_family_detector;
  const board_def_t* const paper_family_detector_t::members_[] = {
    &desc_paperdiy.def, &desc_papers3.def, nullptr,
  };
  static const paper_family_detector_t paper_family_detector;
  // Detection order per package (QFN56 / LGA56).
  static const board_detector_t* const esp32s3_detectors_qfn56[] = {
    &cores3_family_detector, &dial_detector, &pm1_family_detector,
    &pm1_ext_family_detector, &paper_family_detector, &spi_id_detector,
    &cardputer_family_detector, &airq_detector, &stamplc_detector, nullptr,
  };
  static const board_detector_t* const esp32s3_detectors_lga56[] = {
    &atoms3r_detector, &pmic_id_detector, nullptr,
  };

  construct_status_t construct_atoms3(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_atoms3r(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_dinmeter(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_airq(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_stamplc(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_dial(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_cardputer(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_cardputer_adv(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_vameter(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_sticks3(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_stopwatch(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_papermono(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_chaincaptain(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_papercolor(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_papers3(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_paperdiy(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_cores3(const board_result_t& result, display_parts_t* parts);

  const char* atoms3_success_annotation(const board_result_t& result)
  {
    return result.option & generated_options::atoms3::gc9107 ? " (GC9107)" : " (ST7735)";
  }

  const char* atoms3r_success_annotation(const board_result_t& result)
  {
    return result.option & generated_options::atoms3r::gc9107 ? " (GC9107)" : " (ST7735)";
  }

  static const board_entry_t esp32s3_boards[] = {
    { &desc_cores3, construct_cores3, "board_M5StackCoreS3", nullptr },
    { &desc_cores3se, construct_cores3, "board_M5StackCoreS3SE", nullptr },
    { &desc_stackchan, construct_cores3, "board_M5StackChan", nullptr },
    { &desc_atoms3, construct_atoms3, "board_M5AtomS3", atoms3_success_annotation },
    { &desc_atoms3r, construct_atoms3r, "board_M5AtomS3R", atoms3r_success_annotation },
    { &desc_dinmeter, construct_dinmeter, "board_M5DinMeter", nullptr },
    { &desc_airq, construct_airq, "M5AirQ", nullptr },
    { &desc_stamplc, construct_stamplc, "board_M5StamPLC", nullptr },
    { &desc_dial, construct_dial, "board_M5Dial", nullptr },
    { &desc_cardputer, construct_cardputer, "board_M5Cardputer", nullptr },
    { &desc_cardputer_adv, construct_cardputer_adv, "board_M5CardputerADV", nullptr },
    { &desc_vameter, construct_vameter, "board_M5VAMeter", nullptr },
    { &desc_sticks3, construct_sticks3, "board_M5StickS3", nullptr },
    { &desc_stopwatch, construct_stopwatch, "board_M5StopWatch", nullptr },
    { &desc_papermono, construct_papermono, "board_M5PaperMono", nullptr },
    { &desc_chaincaptain, construct_chaincaptain, "board_M5ChainCaptain", nullptr },
    { &desc_papercolor, construct_papercolor, "board_M5PaperColor", nullptr },
    { &desc_papers3, construct_papers3, "board_M5PaperS3", nullptr },
    { &desc_paperdiy, construct_paperdiy, "board_M5PaperDIY", nullptr },
  };

  success_log_t success_log(const board_result_t& result)
  {
    return success_log(esp32s3_boards, result);
  }
}
}
}
