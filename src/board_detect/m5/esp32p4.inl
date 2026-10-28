// Copyright (c) M5Stack. All rights reserved.
#pragma once

#include "../board_detect.hpp"
#include "board_registry.inl"
#include "generated/esp32p4_board_data.hpp"
#include "pmic_ops.hpp"

namespace m5gfx
{
namespace board_detect
{
namespace m5
{
namespace corep4x_detail
{
  constexpr int sda = wiring::corep4x::internal_i2c_sda;
  constexpr int scl = wiring::corep4x::internal_i2c_scl;
  constexpr int_fast16_t i2c_port = io::i2c_port_1;
  constexpr std::uint32_t i2c_freq = pmic_ops::pm1_i2c_freq;
  static const pmic_variant_t power_variants[] = {
    pmic_variant_ack_only(pmic_ops::pm1_i2c_addr, 0,
                          ops::list(pmic_ops::corep4x_power_on),
                          reg_bit(0, 0), reg_bit(0, 0),
                          { nullptr, 0 }, { nullptr, 0 }, 0),
  };
}

static constexpr board_desc_t desc_corep4x = {
  { static_cast<board_id_t>(lgfx::board_M5CoreP4X), "M5CoreP4X", 0 },
  i2c_power_confirmed(corep4x_detail::i2c_freq, corep4x_detail::power_variants,
                      pmic_ops::pm1_family_devices),
  no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  internal_i2c(corep4x_detail::sda, corep4x_detail::scl,
               corep4x_detail::i2c_port),
  no_options(), no_pins(),
  };

class corep4x_detector_t final : public board_detector_t
{
public:
  corep4x_detector_t() : board_detector_t(members_) {}
  bool signature(probe_ctx_t& ctx) const override
  {
    return probe_i2c_bus_present(ctx, corep4x_detail::sda, corep4x_detail::scl);
  }
  bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
  {
    if (result == nullptr) { return false; }
    std::uint8_t id[2] = {};
    if (!probe_i2c_read(ctx, corep4x_detail::sda, corep4x_detail::scl,
                        pmic_ops::pm1_i2c_addr, 0, id, sizeof(id),
                        corep4x_detail::i2c_freq, 200)
     || (static_cast<std::uint16_t>(id[1]) << 8 | id[0]) != pmic_ops::pm1_device_id)
    {
      return false;
    }
    if (!probe_i2c_read(ctx, corep4x_detail::sda, corep4x_detail::scl,
                        pmic_ops::ioe1_i2c_addr, 0, id, sizeof(id),
                        corep4x_detail::i2c_freq, 200))
    {
      return false;
    }
    result->assign(&desc_corep4x);
    return true;
  }
private:
  static const board_def_t* const members_[2];
  };
const board_def_t* const corep4x_detector_t::members_[2] = { &desc_corep4x.def, nullptr };
static const corep4x_detector_t corep4x_detector;

construct_status_t construct_corep4x(const board_result_t&, display_parts_t*);
namespace tab5_detail
{
  constexpr int sda = wiring::tab5::internal_i2c_sda;
  constexpr int scl = wiring::tab5::internal_i2c_scl;
  constexpr int touch_int = wiring::tab5::touch_int;
  constexpr int_fast16_t i2c_port = io::i2c_port_1;
  constexpr std::uint32_t i2c_freq = 100000;
  constexpr std::uint8_t pi4io1_addr = 0x43;
  constexpr std::uint8_t pi4io2_addr = 0x44;
  // Board-private prepared bits carry observations to construction. They never
  // enter the public option or NVS; detection must use software I2C only.
  enum : std::uint32_t
  {
    read_st_touch_fw = 1u << 16,
    touch_st7121 = 1u << 17,
    touch_st7123 = 1u << 18,
    touch_gt911 = 1u << 19,
  };

  bool observe_touch(board_result_t& result, const prepare_ctx_t& ctx)
  {
#if CONFIG_SPIRAM
    prepare_detail::i2c_scope_t i2c(*ctx.transaction, ctx.probe_port,
                                    result.desc->internal_i2c);
    if (!i2c.opened) { return false; }
    int last_logged_fw = -1;
    for (int i = 0; i < 60; ++i)
    {
      std::uint8_t fw_version = 0;
      std::uint8_t fw_reg[2] = { 0, 0 };
      if (io::i2c_write_read(i2c.port, io::st_touch_address(),
                                           fw_reg, sizeof(fw_reg), &fw_version, 1,
                                           i2c_freq))
      {
        result.prepared |= read_st_touch_fw;
        if (fw_version != last_logged_fw)
        {
          last_logged_fw = fw_version;
          ESP_LOGI("M5GFX", "M5Tab5 ST touch FW version %02x", fw_version);
          if (fw_version != 1 && fw_version != 3)
          {
            ESP_LOGW("M5GFX", "M5Tab5 unknown ST touch FW version %02x", fw_version);
          }
        }
        if (fw_version == 1) { result.prepared |= touch_st7121; break; }
        if (fw_version == 3) { result.prepared |= touch_st7123; break; }
      }
      else if (io::i2c_ack(i2c.port, io::gt911_touch_address(), i2c_freq))
      {
        result.prepared |= touch_gt911;
        ESP_LOGI("M5GFX", "M5Tab5 GT911 touch detected");
        break;
      }
      io::delay_ms(10);
    }
    if (!(result.prepared & (read_st_touch_fw | touch_gt911)))
    {
      ESP_LOGW("M5GFX", "M5Tab5 ST touch FW version read failed");
    }
#endif
    return true;
  }

  bool fixed_prepare(board_result_t& result, const prepare_ctx_t& ctx)
  {
    result.observe_after_power = observe_touch;
    result.observe_on_power_failure = true;
    return prepare(*result.desc, result, ctx);
  }

  // prepare drives TP_INT while releasing the touch controller, so it must be
  // inside both the operation scope and the detection transaction snapshot.
  static constexpr std::int8_t prepare_gpio_pins[] = { touch_int };
  static const pmic_variant_t power_variants[] = {
    pmic_variant_ack_only(pi4io1_addr, 0, ops::list(pmic_ops::tab5_power_on),
                          reg_bit(0, 0), reg_bit(0, 0),
                          { nullptr, 0 }, { nullptr, 0 }, 0),
  };
}

static constexpr board_desc_t desc_tab5 = {
  { static_cast<board_id_t>(lgfx::board_M5Tab5), "M5Tab5", 0 },
  i2c_power_confirmed(tab5_detail::i2c_freq, tab5_detail::power_variants,
                      pmic_ops::tab5_devices),
  no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  internal_i2c(tab5_detail::sda, tab5_detail::scl, tab5_detail::i2c_port),
  no_options(), pins(tab5_detail::prepare_gpio_pins),
  };
static constexpr board_desc_t desc_tab5x = {
  { static_cast<board_id_t>(lgfx::board_M5Tab5X), "M5Tab5X", 0 },
  i2c_power_confirmed(tab5_detail::i2c_freq, tab5_detail::power_variants,
                      pmic_ops::tab5_devices),
  no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  internal_i2c(tab5_detail::sda, tab5_detail::scl, tab5_detail::i2c_port),
  no_options(), pins(tab5_detail::prepare_gpio_pins),
  };

class tab5_family_detector_t final : public board_detector_t
{
public:
  tab5_family_detector_t() : board_detector_t(members_) {}
  bool signature(probe_ctx_t& ctx) const override
  {
    constexpr detect_class_expected_t expected = wiring::tab5::detect_class_expected;
    const bool bus_present = probe_i2c_bus_present(ctx, tab5_detail::sda, tab5_detail::scl);
    // Backlight wiring only suggests a recovery candidate; it must not gate
    // PI4IO confirmation, which remains valid with the wireless supply off.
    if (bus_present && ctx.candidate == nullptr
     && match_detect_class(expected, probe_pin_pulls(ctx, expected.mask)))
    {
      ctx.candidate = &revision_desc()->def;
    }
    return bus_present;
  }
  bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
  {
    if (result == nullptr) { return false; }
    std::uint8_t value = 0;
    if (!probe_i2c_read(ctx, tab5_detail::sda, tab5_detail::scl,
                        tab5_detail::pi4io1_addr, pi4io_id_register, &value, 1,
                        tab5_detail::i2c_freq, 0)
     || !is_pi4io(value)
     || !probe_i2c_read(ctx, tab5_detail::sda, tab5_detail::scl,
                        tab5_detail::pi4io2_addr, pi4io_id_register, &value, 1,
                        tab5_detail::i2c_freq, 0)
     || !is_pi4io(value))
    {
      // The recovery candidate from signature() is what the caller will use.
      if (ctx.candidate == &desc_tab5.def || ctx.candidate == &desc_tab5x.def)
      { warn_hint_mismatch(ctx, ctx.candidate); }
      return false;
    }
    // Tab5X upgrades the P4 silicon revision; saved hints cannot identify it.
    const auto* desc = revision_desc();
    warn_hint_mismatch(ctx, &desc->def);
    result->assign(desc);
    result->observe_after_power = tab5_detail::observe_touch;
    // The touch controller may be readable even if a later power write failed.
    result->observe_on_power_failure = true;
    result->candidate = ctx.candidate;
    return true;
  }
private:
  static const board_desc_t* revision_desc()
  {
    return io::chip_revision() >= 300 ? &desc_tab5x : &desc_tab5;
  }
  static void warn_hint_mismatch(const probe_ctx_t& ctx, const board_def_t* def)
  {
    if (ctx.attempt == 0
     && (ctx.hint == desc_tab5.def.id || ctx.hint == desc_tab5x.def.id)
     && ctx.hint != def->id)
    {
      const auto revision = io::chip_revision();
      ESP_LOGW("M5GFX", "[Autodetect] Tab5 hint:%u disagrees with chip revision:%u; using %s",
               static_cast<unsigned>(ctx.hint), static_cast<unsigned>(revision),
               def->name);
    }
  }
  static const board_def_t* const members_[3];
};
const board_def_t* const tab5_family_detector_t::members_[3] = {
  &desc_tab5.def, &desc_tab5x.def, nullptr
};
static const tab5_family_detector_t tab5_family_detector;

construct_status_t construct_tab5(const board_result_t&, display_parts_t*);
namespace unitpoep4_detail
{
  constexpr int mdc = 31;
  constexpr int mdio = 52;
  static constexpr std::int8_t gpio_pins[] = { mdc, mdio };

  // Open-drain signaling: output latches stay Low; High releases the pin.
  inline void drive(int pin, bool high)
  {
    if (high) { io::pin_mode(pin, io::pin_mode_t::input); }
    else
    {
      io::pin_low(pin);
      io::pin_mode(pin, io::pin_mode_t::output);
    }
  }

  inline int read_phy_register(int reg)
  {
    auto clock = []()
    {
      drive(mdc, true);
      io::delay_us(2);
      drive(mdc, false);
    };
    auto bit_out = [&](bool bit)
    {
      drive(mdio, bit);
      io::delay_us(2);
      clock();
    };
    auto bit_in = [&]()
    {
      // IP101 changes MDIO after the rising edge; sample while MDC is Low.
      io::delay_us(2);
      const bool bit = io::pin_read(mdio);
      clock();
      return bit;
    };
    drive(mdc, false);
    // The first frame after boot needs more than the usual 32 preamble bits.
    for (int i = 0; i < 64; ++i) { bit_out(true); }
    bit_out(false); bit_out(true); // ST = 01
    bit_out(true); bit_out(false); // OP = read
    for (int i = 4; i >= 0; --i) { bit_out((1 >> i) & 1); }
    for (int i = 4; i >= 0; --i) { bit_out((reg >> i) & 1); }
    drive(mdio, true);
    bit_in(); // TA: Z
    const bool no_response = bit_in(); // TA: 0
    int value = 0;
    for (int i = 0; i < 16; ++i) { value = (value << 1) | bit_in(); }
    bit_in();
    drive(mdc, true);
    return no_response ? -1 : value;
  }
}

static constexpr board_desc_t desc_unitpoep4 = {
  { static_cast<board_id_t>(lgfx::board_M5UnitPoEP4), "M5UnitPoEP4", 0 },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), pins(unitpoep4_detail::gpio_pins),
  };

class unitpoep4_detector_t final : public board_detector_t
{
public:
  unitpoep4_detector_t() : board_detector_t(members_) {}
  bool signature(probe_ctx_t& ctx) const override
  {
    constexpr detect_class_expected_t expected = wiring::unitpoep4::detect_class_expected;
    // Tab5 SCL G32 stays pulled up even with a unit attached to Port B G52;
    // reject it before G31 can be driven as MDC on the shared internal bus.
    return match_detect_class(expected, probe_pin_pulls(ctx, expected.mask));
  }
  bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
  {
    if (result == nullptr || ctx.transaction == nullptr) { return false; }
    const ops::gpio_scope_t scope = {
      io::pin_count, desc_unitpoep4.op_gpio_pins.data, desc_unitpoep4.op_gpio_pins.size,
    };
    if (!ops::gpio_allowed(scope, unitpoep4_detail::mdc)
     || !ops::gpio_allowed(scope, unitpoep4_detail::mdio)) { return false; }
    const int id1 = unitpoep4_detail::read_phy_register(2);
    const int id2 = unitpoep4_detail::read_phy_register(3);
    // Even successful displayless detection must return both management pins.
    ctx.transaction->restore_to_snapshot(unitpoep4_detail::gpio_pins,
                                   sizeof(unitpoep4_detail::gpio_pins));
    if (id1 != 0x0243 || id2 != 0x0C54) { return false; }
    result->assign(&desc_unitpoep4);
    return true;
  }
private:
  static const board_def_t* const members_[2];
};
const board_def_t* const unitpoep4_detector_t::members_[2] = { &desc_unitpoep4.def, nullptr };
static const unitpoep4_detector_t unitpoep4_detector;

// No detector: selectable only through fixed_board.
static constexpr board_desc_t desc_stampp4 = {
  { static_cast<board_id_t>(lgfx::board_M5StampP4), "M5StampP4", 0 },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), no_pins(),
};

// Module revision alone supplies no board evidence; leave defaults to M5Unified.
#include "generated/esp32p4_detector_order.hpp"
static const board_entry_t esp32p4_boards[] = {
  { &desc_corep4x, construct_corep4x, "board_M5CoreP4X", nullptr },
  { &desc_tab5, construct_tab5, "board_M5Tab5", nullptr, tab5_detail::fixed_prepare },
  { &desc_tab5x, construct_tab5, "board_M5Tab5X", nullptr, tab5_detail::fixed_prepare },
  { &desc_unitpoep4, construct_displayless, "board_M5UnitPoEP4", nullptr },
  { &desc_stampp4, construct_displayless, "board_M5StampP4", nullptr },
};
success_log_t success_log(const board_result_t& result) { return success_log(esp32p4_boards, result); }
}
}
}
