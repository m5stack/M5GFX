// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "../board_detect.hpp"
#include "board_registry.inl"
#include "generated/esp32c6_board_data.hpp"
#include "pmic_ops.hpp"

namespace m5gfx
{
namespace board_detect
{
namespace m5
{
static constexpr board_desc_t desc_stampc6 = {
  { static_cast<board_id_t>(lgfx::board_M5StampC6), "M5StampC6", def_flag_candidate_only },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), no_pins(),
};
static constexpr board_desc_t desc_nanoc6 = {
  { static_cast<board_id_t>(lgfx::board_M5NanoC6), "M5NanoC6", 0 },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), no_pins(),
};

namespace c6_displayless_detail
{
  // StampC6 uses ESP32-C6FH8 (FLASH_CAP 2); NanoC6 uses FH4 (FLASH_CAP 1).
  // Raw-value mapping: esptool esp32c6.get_chip_description().
  static constexpr unsigned stampc6_flash_cap = 2;
  // The package's flash capacity is a stable eFuse fact; check it before
  // probing NanoC6 GPIO3, which is exposed on StampC6 as a user header pin.
  inline unsigned flash_capacity()
  { return io::flash_capacity_code(); }
}

class stampc6_detector_t final : public board_detector_t
{
public:
  stampc6_detector_t() : board_detector_t(members_) {}
  bool signature(probe_ctx_t& ctx) const override
  {
    if (c6_displayless_detail::flash_capacity() != c6_displayless_detail::stampc6_flash_cap) { return false; }
    // Package facts only suggest a board; keep later families from probing
    // this module's user pins without treating it as confirmed.
    if (ctx.candidate == nullptr) { ctx.candidate = &desc_stampc6.def; }
    ctx.family_identified = true;
    return false;
  }
  bool confirm(probe_ctx_t&, board_result_t*) const override { return false; }
private:
  static const board_def_t* const members_[2];
};
const board_def_t* const stampc6_detector_t::members_[2] = { &desc_stampc6.def, nullptr };
static const stampc6_detector_t stampc6_detector;

class nanoc6_detector_t final : public board_detector_t
{
public:
  nanoc6_detector_t() : board_detector_t(members_) {}
  bool signature(probe_ctx_t& ctx) const override
  {
    // Normal scans run StampC6 first (generated detector_order_edges) and stop at its family flag.
    // This local gate also avoids GPIO3 probing when this detector is called alone.
    return c6_displayless_detail::flash_capacity() != c6_displayless_detail::stampc6_flash_cap
      && probe_pin_pullup_low(ctx, 3); // The IR LED clamps the internal pull-up low.
  }
  bool confirm(probe_ctx_t&, board_result_t* result) const override
  { if (!result) return false; result->assign(&desc_nanoc6); return true; }
private:
  static const board_def_t* const members_[2];
};
const board_def_t* const nanoc6_detector_t::members_[2] = { &desc_nanoc6.def, nullptr };
static const nanoc6_detector_t nanoc6_detector;
namespace c6_display_detail
{
  constexpr int sda = wiring::nesson1::internal_i2c_sda;
  constexpr int scl = wiring::nesson1::internal_i2c_scl;
  constexpr int_fast16_t i2c_port = io::i2c_port_0;
  constexpr std::uint32_t pi4io_freq = 100000;
  constexpr std::uint64_t signature_bit = std::uint64_t(1) << 18;
  enum class candidate_t : std::uint8_t { none, unitc6l, nesson1 };

  static const pmic_variant_t nesson1_power_variants[] = {
    pmic_variant_ack_only(specs::nesson1::i2c_pi4io2::i2c_addr, 0,
                          ops::list(pmic_ops::nesson1_power_on),
                          reg_bit(0, 0), reg_bit(0, 0),
                          { nullptr, 0 }, { nullptr, 0 }, 0),
  };
}

static constexpr board_desc_t desc_unitc6l = {
  { static_cast<board_id_t>(lgfx::board_M5UnitC6L), "M5UnitC6L", 0 },
  gpio_power(wiring::unitc6l::power_gpio), gpio_reset(wiring::unitc6l::reset_gpio, 2, 10, reset_hold_when_skipped),
  no_shared_sd(),
  display_pins(wiring::unitc6l::display_sclk, wiring::unitc6l::display_mosi,
               wiring::unitc6l::display_miso, wiring::unitc6l::display_dc,
               wiring::unitc6l::display_cs, wiring::unitc6l::display_rst,
               wiring::unitc6l::display_busy),
  pins(wiring::unitc6l::hold),
  internal_i2c(wiring::unitc6l::internal_i2c_sda,
               wiring::unitc6l::internal_i2c_scl,
               c6_display_detail::i2c_port),
  no_options(), pins(wiring::unitc6l::hold),
};

static constexpr board_desc_t desc_nesson1 = {
  { static_cast<board_id_t>(lgfx::board_ArduinoNessoN1), "ArduinoNessoN1", 0 },
  i2c_power_confirmed(c6_display_detail::pi4io_freq,
                      c6_display_detail::nesson1_power_variants,
                      pmic_ops::nesson1_devices),
  no_reset(), no_shared_sd(),
  display_pins(wiring::nesson1::display_sclk, wiring::nesson1::display_mosi,
               wiring::nesson1::display_miso, wiring::nesson1::display_dc,
               wiring::nesson1::display_cs, wiring::nesson1::display_rst,
               wiring::nesson1::display_busy),
  pins(wiring::nesson1::hold),
  internal_i2c(c6_display_detail::sda, c6_display_detail::scl,
               c6_display_detail::i2c_port),
  no_options(), no_pins(),
};

class c6_display_family_detector_t final : public board_detector_t
{
public:
  c6_display_family_detector_t() : board_detector_t(members_) {}

  bool signature(probe_ctx_t& ctx) const override
  {
    candidate_ = c6_display_detail::candidate_t::none;
    // Both boards pull the internal I2C lines up. Check (and recover a slave
    // holding SDA across a reset) before sampling, or a held bus stays unknown.
    if (!probe_i2c_bus_present(ctx, c6_display_detail::sda,
                               c6_display_detail::scl))
    { return false; }
    const auto pulls = probe_pin_pulls(ctx, c6_display_detail::signature_bit);
    candidate_ = (pulls.pullup_high & c6_display_detail::signature_bit)
               ? c6_display_detail::candidate_t::unitc6l
               : c6_display_detail::candidate_t::nesson1;
    return true;
  }

  bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
  {
    if (result == nullptr) { return false; }
    if (candidate_ == c6_display_detail::candidate_t::unitc6l)
    {
      std::uint8_t value = 0;
      if (!probe_i2c_read(ctx, wiring::unitc6l::internal_i2c_sda,
                          wiring::unitc6l::internal_i2c_scl,
                          specs::unitc6l::i2c_ioe::i2c_addr,
                          specs::unitc6l::i2c_ioe::id_reg,
                          &value, 1, specs::unitc6l::i2c_ioe::i2c_freq, 0)
       || !is_pi4io(value))
      {
        // Presence and GPIO18 only suggest UnitC6L; never construct on a hint.
        if (ctx.candidate == nullptr) { ctx.candidate = &desc_unitc6l.def; }
        return false;
      }
      result->assign(&desc_unitc6l);
      return true;
    }
    if (candidate_ != c6_display_detail::candidate_t::nesson1) { return false; }
    std::uint8_t value = 0;
    if (!probe_i2c_read(ctx, c6_display_detail::sda, c6_display_detail::scl,
                        specs::nesson1::i2c_pi4io2::i2c_addr,
                        specs::nesson1::i2c_pi4io2::id_reg,
                        &value, 1, c6_display_detail::pi4io_freq, 0)
     || !is_pi4io(value)
     || !probe_i2c_read(ctx, c6_display_detail::sda, c6_display_detail::scl,
                        specs::nesson1::i2c_pi4io1::i2c_addr,
                        specs::nesson1::i2c_pi4io1::id_reg,
                        &value, 1, c6_display_detail::pi4io_freq, 0)
     || !is_pi4io(value))
    { return false; }
    result->assign(&desc_nesson1);
    return true;
  }

private:
  mutable c6_display_detail::candidate_t candidate_ = c6_display_detail::candidate_t::none;
  static const board_def_t* const members_[3];
};

const board_def_t* const c6_display_family_detector_t::members_[3] = {
  &desc_unitc6l.def, &desc_nesson1.def, nullptr,
};
static const c6_display_family_detector_t c6_display_family_detector;
#include "generated/esp32c6_detector_order.hpp"

construct_status_t construct_unitc6l(const board_result_t&, display_parts_t*);
construct_status_t construct_nesson1(const board_result_t&, display_parts_t*);
static const board_entry_t esp32c6_boards[] = {
  { &desc_stampc6, construct_displayless, "board_M5StampC6", nullptr },
  { &desc_nanoc6, construct_displayless, "board_M5NanoC6", nullptr },
  { &desc_unitc6l, construct_unitc6l, "board_M5UnitC6L", nullptr },
  { &desc_nesson1, construct_nesson1, "board_ArduinoNessoN1", nullptr },
};
success_log_t success_log(const board_result_t& result)
{ return success_log(esp32c6_boards, result); }
}
}
}
