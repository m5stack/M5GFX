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
    bool fixed_prepare_core(board_result_t& result, const prepare_ctx_t& ctx);
    bool stack_reset_and_sample_ips(const board_desc_t& desc, const prepare_ctx_t& ctx,
                                    std::uint32_t* detected_option);
  }

  static_assert(generated_options::core2::new_pmic == generated_options::tough::reserved,
                "Core2 new-PMIC and Tough reserved option bits must remain shared");
  static const pmic_variant_t core_pmic_variants[] = {
    pmic_variant(0x34, 0x03, 0x03, ops::list(pmic_ops::power192),
                 reg_bit(0x12, 0x04), reg_bit(0x96, 0x02),
                 ops::list(pmic_ops::reset192), ops::list(pmic_ops::release192), 0),
    pmic_variant(0x34, 0x03, 0x4A, ops::list(pmic_ops::power2101),
                 reg_bit(0x90, 0x08), reg_bit(0x90, 0x02),
                 ops::list(pmic_ops::reset2101), ops::list(pmic_ops::release2101),
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
  static constexpr board_desc_t desc_timercam = {
    { id(lgfx::board_M5TimerCam), "M5TimerCam", 0 },
    gpio_power(wiring::timercam::power_gpio), no_reset(), no_shared_sd(),
    no_display_pins(), no_pins(),
    internal_i2c(wiring::timercam::internal_i2c_sda,
                 wiring::timercam::internal_i2c_scl,
                 wiring::timercam::internal_i2c_port),
    no_options(), no_pins(),
  };
  // gpio_power() keeps its active-high meaning; only gpio_power_low() holds low.
  static_assert(desc_paper.power.hold_high, "Paper power hold stays active high");

  namespace detail
  {
    static constexpr std::uint8_t tough_touch_address = 0x2E;
    static constexpr std::uint8_t core2_touch_address = 0x38;
    static constexpr std::uint8_t touch_probe_register = 0;
    static constexpr std::uint32_t tough_touch_i2c_frequency = 400000;
    // FT6336U Tpon/Trsi is at least 300 ms (datasheet table 3-5).
    static constexpr std::uint32_t touch_startup_poll_ms = 300;
    static constexpr std::uint8_t panel_id_command = 0x04;
    static constexpr std::uint32_t panel_id_mask = 0xFF;
    static constexpr std::uint32_t common_panel_id = 0xE3;
    static constexpr std::uint8_t station_pmic_id = 0x03;
    static constexpr std::uint32_t station_id_mask = 0xFB;
    static constexpr std::uint32_t station_id = 0x81;
    static constexpr std::uint32_t paper_panel_size = 0x03C0021C;

    bool sd_pull_mask(const board_desc_t& desc, std::uint64_t* mask)
    {
      if (mask == nullptr || !prepare_detail::gpio_valid(desc.sd.sd_cs)
       || !prepare_detail::gpio_valid(desc.sd.sclk)
       || !prepare_detail::gpio_valid(desc.sd.mosi))
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
      prepare_detail::pin_level(reset.pin, true);
      io::delay_ms(1);
      io::pin_low(reset.pin);
      io::delay_ms(reset.low_ms);
      io::pin_mode(reset.pin, io::pin_mode_t::input_pulldown);
      io::pin_high(reset.pin);
      const bool ips = io::pin_read(reset.pin);
      io::pin_mode(reset.pin, io::pin_mode_t::output);
      io::delay_ms(reset.post_ms);
      if (ctx.allow_reset && ips && detected_option != nullptr)
      {
        *detected_option |= generated_options::stack::ips;
      }
      return true;
    }
  }

// ESP32 PICO-D4 / PICOV3_02 boards. Included inside m5gfx::board_detect::m5.

static constexpr board_desc_t desc_stickc = {
  { id(lgfx::board_M5StickC), "M5StickC", 0 },
  no_power(), gpio_reset(wiring::stickc::reset_gpio, 2, 10, reset_hold_when_skipped),
  no_shared_sd(),
  display_pins(wiring::stickc::display_sclk, wiring::stickc::display_mosi,
               wiring::stickc::display_miso, wiring::stickc::display_dc,
               wiring::stickc::display_cs, wiring::stickc::display_rst,
               wiring::stickc::display_busy),
  pins(wiring::stickc::hold),
  internal_i2c(wiring::stickc::internal_i2c_sda, wiring::stickc::internal_i2c_scl,
               wiring::stickc::internal_i2c_port),
  no_options(), pins(wiring::stickc::hold),
};

static constexpr board_desc_t desc_stickcplus = {
  { id(lgfx::board_M5StickCPlus), "M5StickCPlus", 0 },
  no_power(), gpio_reset(wiring::stickcplus::reset_gpio, 2, 10, reset_hold_when_skipped),
  no_shared_sd(),
  display_pins(wiring::stickcplus::display_sclk, wiring::stickcplus::display_mosi,
               wiring::stickcplus::display_miso, wiring::stickcplus::display_dc,
               wiring::stickcplus::display_cs, wiring::stickcplus::display_rst,
               wiring::stickcplus::display_busy),
  pins(wiring::stickcplus::hold),
  internal_i2c(wiring::stickcplus::internal_i2c_sda, wiring::stickcplus::internal_i2c_scl,
               wiring::stickcplus::internal_i2c_port),
  no_options(), pins(wiring::stickcplus::hold),
};

static constexpr board_desc_t desc_coreink = {
  { id(lgfx::board_M5StackCoreInk), "M5StackCoreInk", 0 },
  gpio_power(wiring::coreink::power_gpio),
  gpio_reset(wiring::coreink::reset_gpio, 2, 10, reset_always),
  no_shared_sd(),
  display_pins(wiring::coreink::display_sclk, wiring::coreink::display_mosi,
               wiring::coreink::display_miso, wiring::coreink::display_dc,
               wiring::coreink::display_cs, wiring::coreink::display_rst,
               wiring::coreink::display_busy),
  pins(wiring::coreink::hold), no_internal_i2c(),
  options(generated_options::coreink::names),
  pins(wiring::coreink::hold),
};

static constexpr board_desc_t desc_stickcplus2 = {
  { id(lgfx::board_M5StickCPlus2), "M5StickCPlus2", 0 },
  gpio_power(wiring::stickcplus2::power_gpio),
  gpio_reset(wiring::stickcplus2::reset_gpio, 2, 10, reset_hold_when_skipped),
  no_shared_sd(),
  display_pins(wiring::stickcplus2::display_sclk, wiring::stickcplus2::display_mosi,
               wiring::stickcplus2::display_miso, wiring::stickcplus2::display_dc,
               wiring::stickcplus2::display_cs, wiring::stickcplus2::display_rst,
               wiring::stickcplus2::display_busy),
  pins(wiring::stickcplus2::hold), no_internal_i2c(),
  no_options(), pins(wiring::stickcplus2::hold),
};

static constexpr board_desc_t desc_atompsram = {
  { id(lgfx::board_M5AtomPsram), "M5AtomPsram", 0 },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), no_pins(),
};

static constexpr board_desc_t desc_atomvoice = {
  { id(lgfx::board_M5AtomVoice), "M5AtomVoice", 0 },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), no_pins(),
};
static constexpr board_desc_t desc_atommatrix = {
  { id(lgfx::board_M5AtomMatrix), "M5AtomMatrix", 0 },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), no_pins(),
};
static constexpr board_desc_t desc_atomlite = {
  { id(lgfx::board_M5AtomLite), "M5AtomLite", 0 },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), no_pins(),
};
static constexpr board_desc_t desc_atomu = {
  { id(lgfx::board_M5AtomU), "M5AtomU", 0 },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), no_pins(),
};
static constexpr board_desc_t desc_stamppico = {
  { id(lgfx::board_M5StampPico), "M5StampPico", 0 },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), no_pins(),
};

static constexpr std::uint32_t stickc_id_values[] = { 0x7C };
// Both 0x81 and 0x85 have been read from real units; bit 2 differs, so the
// probes compare under mask 0xFB and accept both with this single value.
static constexpr std::uint32_t stickcplus_id_values[] = { 0x81 };
static const spi_id_probe_t stickc_probes[] = {
  // This family checks the low byte only; the part-catalog probe describes
  // full IDs used by other boards and is intentionally not emitted here.
  spi_id_probe(0x04, 0xFF, stickc_id_values, 0, 1),
};
static const spi_id_probe_t stickcplus_probes[] = {
  spi_id_probe(specs::stickcplus::probe_st7789v2::cmd,
               0xFB, stickcplus_id_values, 0, 1),
};
static const spi_id_probe_t coreink_probes[] = {
  spi_id_probe(specs::coreink::probe_gdew0154d67::cmd,
               specs::coreink::probe_gdew0154d67::mask,
               specs::coreink::probe_gdew0154d67::values, 0,
               specs::coreink::probe_gdew0154d67::dummy_bits),
  // First lot: 0x00F00000; 2023-11-17 lot: 0x00F01600.
  spi_id_probe(specs::coreink::probe_gdew0154m09::cmd,
               specs::coreink::probe_gdew0154m09::mask,
               specs::coreink::probe_gdew0154m09::values,
               generated_options::coreink::m09,
               specs::coreink::probe_gdew0154m09::dummy_bits),
};
static const spi_id_probe_t stickcplus2_probes[] = {
  spi_id_probe(specs::stickcplus2::probe_st7789v2::cmd,
               0xFB, stickcplus_id_values, 0, 1),
};

static const board_def_t* const stickc_family_members[] = {
  &desc_stickcplus.def, &desc_stickc.def, nullptr,
};
static const spi_id_member_t stickc_family_member_descs[] = {
  { &desc_stickcplus, stickcplus_probes, 1, true, false, 0, true },
  { &desc_stickc, stickc_probes, 1, true, false, 0, true },
};
static const spi_id_detector_t stickc_family_detector(
  stickc_family_members, stickc_family_member_descs, 2, true);

static const board_def_t* const coreink_members[] = { &desc_coreink.def, nullptr };
static const spi_id_member_t coreink_member_descs[] = {
  { &desc_coreink, coreink_probes, 2, true, false, 0, true },
};
static bool fixed_prepare_coreink(board_result_t& result, const prepare_ctx_t& ctx)
{ return fixed_prepare_spi_variant(result, ctx, coreink_member_descs[0]); }
static const spi_id_detector_t coreink_detector(coreink_members, coreink_member_descs, 1);

static const board_def_t* const stickcplus2_members[] = { &desc_stickcplus2.def, nullptr };
static const spi_id_member_t stickcplus2_member_descs[] = {
  { &desc_stickcplus2, stickcplus2_probes, 1, true, false, 0, true },
};
static const spi_id_detector_t stickcplus2_detector(
  stickcplus2_members, stickcplus2_member_descs, 1);

// Shared pad mapping for the touch implementation and its host event model.
namespace atom_touch
{
  struct touch_pad_t { int channel; int gpio; };
  static constexpr touch_pad_t t4_pad = { 4, 13 };
  static constexpr touch_pad_t t7_pad = { 7, 27 };
  // T7 measures the LED DIN line, so LED clearing uses the same pad.
  static constexpr int led_gpio = t7_pad.gpio;
}
// Classic ESP32 touch/LED transport is supplied by the detection I/O layer.
namespace atom_touch
{
  static bool measure(std::uint16_t* t4, std::uint16_t* t7)
  {
    return io::classic_touch_measure<t4_pad.channel, t4_pad.gpio,
                                     t7_pad.channel, t7_pad.gpio, 200000>(t4, t7) && *t4 != 0;
  }

  static void clear_led() { io::classic_led_clear<led_gpio, 25>(); }
}

class atom_family_detector_t final : public board_detector_t
{
public:
  atom_family_detector_t() : board_detector_t(members_) {}
  bool signature(probe_ctx_t& ctx) const override
  {
    cached_ = board_result_t {};
    return probe(ctx, &cached_);
  }
  bool confirm(probe_ctx_t&, board_result_t* result) const override
  {
    if (result == nullptr || cached_.desc == nullptr) { return false; }
    *result = cached_;
    return true;
  }
private:
  bool probe(probe_ctx_t& ctx, board_result_t* result) const
  {
    if (result == nullptr) { return false; }
    // StampPico holds G2 low; exclude it before probing the Atom family.
    io::pin_mode(2, io::pin_mode_t::input_pullup);
    io::delay_rom_us(5);
    const bool atom_g2 = io::pin_read(2);
    ctx.transaction->restore_to_snapshot({ 2 });
    if (!atom_g2) { return false; }
    // StickC LCD reset is pulled high; Atom leaves G18 unconnected.
    const auto g18 = probe_pin_pulls(ctx, 1ULL << 18);
    if (g18.pulldown_high || g18.pullup_high != (1ULL << 18)) { return false; }
    const auto read_g34 = [](io::pin_mode_t mode) {
      io::pin_mode(23, mode); // G23 is exposed on Lite/Matrix: internal pull only, never drive it.
      io::delay_rom_us(5);
      return io::pin_read(34);
    };
    io::pin_mode(34, io::pin_mode_t::input);
    const bool down = read_g34(io::pin_mode_t::input_pulldown);
    const bool up1 = read_g34(io::pin_mode_t::input_pullup);
    const bool down2 = read_g34(io::pin_mode_t::input_pulldown);
    const bool up2 = read_g34(io::pin_mode_t::input_pullup);
    ctx.transaction->restore_to_snapshot({ 23, 34 });

    if (down)
    {
      result->assign(&desc_atomvoice); // Microphone DATA remains high with G23 pulled down.
      if (!(up1 && down2 && up2)) { result->provisional = true; }
      return true;
    }
    // The IR transmitter supplies the strong pull-down on Lite/Matrix/U.
    // Reject unrelated PICO boards before touch scanning or LED writes.
    const auto g12 = probe_pin_pulls(ctx, 1ULL << 12);
    if (classify_pin_pull(g12, 12) != pull_class_t::down) { return false; }
    if (!measured_)
    {
      measured_ = true; // One touch scan per boot, including retries.
      touch_ok_ = atom_touch::measure(&t4_, &t7_);
      atom_touch::clear_led(); // 25 black pixels; leave G27 output low.
    }
    if (!touch_ok_ || t7_ == 0)
    {
      // No successful touch measurement means no physical board evidence.
      return false;
    }
    // At 1.2 V and 0.1 ms/ch the measured T7/T4 ratios are Matrix 0.26,
    // Lite 0.42-0.45, U 0.71 (atom-picod4-pin-edge-2026-09-28).
    constexpr std::uint32_t matrix_limit = 34;
    constexpr std::uint32_t lite_limit = 58;
    const std::uint32_t ratio100 = std::uint32_t(t7_) * 100;
    const std::uint32_t reference = t4_;
    const bool linked = !down && up1 && !down2 && up2;
    if (ratio100 < matrix_limit * reference)
    {
      result->assign(&desc_atommatrix);
      io::pin_mode(21, io::pin_mode_t::input_pulldown);
      io::pin_mode(25, io::pin_mode_t::input_pulldown);
      io::delay_rom_us(10);
      const bool matrix_pulls = io::pin_read(21) && io::pin_read(25);
      ctx.transaction->restore_to_snapshot({ 21, 25 });
      if (!linked || !matrix_pulls) { result->provisional = true; }
    }
    else if (ratio100 <= lite_limit * reference)
    {
      result->assign(&desc_atomlite);
      if (!linked) { result->provisional = true; }
    }
    else
    {
      result->assign(&desc_atomu);
      if (linked || up1 || down2 || up2) { result->provisional = true; }
    }
    return true;
  }
  mutable board_result_t cached_;
  mutable bool measured_ = false;
  mutable bool touch_ok_ = false;
  mutable std::uint16_t t4_ = 0, t7_ = 0;
  static const board_def_t* const members_[5];
};
const board_def_t* const atom_family_detector_t::members_[5] = {
  &desc_atomvoice.def, &desc_atommatrix.def, &desc_atomlite.def,
  &desc_atomu.def, nullptr,
};
static const atom_family_detector_t atom_family_detector;

#include "generated/esp32_pico_detector_order.hpp"

construct_status_t construct_stickc(const board_result_t&, display_parts_t*);
construct_status_t construct_stickcplus(const board_result_t&, display_parts_t*);
construct_status_t construct_coreink(const board_result_t&, display_parts_t*);
construct_status_t construct_stickcplus2(const board_result_t&, display_parts_t*);
construct_status_t construct_atompsram(const board_result_t&, display_parts_t*);

  construct_status_t construct_station(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_core2(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_tough(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_stack(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_paper(const board_result_t& result, display_parts_t* parts);
  static const board_entry_t esp32_d0wdq6_boards[] = {
    { &desc_timercam, construct_displayless, "board_M5TimerCam", nullptr },
    { &desc_station, construct_station, nullptr, nullptr },
    { &desc_core2, construct_core2, nullptr, nullptr, detail::fixed_prepare_core },
    { &desc_tough, construct_tough, nullptr, nullptr, detail::fixed_prepare_core },
    { &desc_stack, construct_stack, nullptr, nullptr },
    { &desc_paper, construct_paper, nullptr, nullptr },
    { &desc_stickcplus, construct_stickcplus, "M5StickCPlus", nullptr },
    { &desc_stickc, construct_stickc, "M5StickC", nullptr },
    { &desc_coreink, construct_coreink, "M5StackCoreInk", nullptr, fixed_prepare_coreink },
    { &desc_stickcplus2, construct_stickcplus2, "M5StickCPlus2", nullptr },
    { &desc_atompsram, construct_atompsram, "", nullptr },
    { &desc_atomvoice, construct_displayless, "board_M5AtomVoice", nullptr },
    { &desc_atommatrix, construct_displayless, "board_M5AtomMatrix", nullptr },
    { &desc_atomlite, construct_displayless, "board_M5AtomLite", nullptr },
    { &desc_atomu, construct_displayless, "board_M5AtomU", nullptr },
    { &desc_stamppico, construct_displayless, "board_M5StampPico", nullptr },
  };

  namespace detail
  {
    const pmic_variant_t* observe_core_pmic(int port)
    { return prepare_detail::read_variant(desc_core2.power, port); }

    bool observe_core_panel(board_result_t& result, const prepare_ctx_t& ctx,
                            panel_variant_t& variant, std::uint32_t keys[4])
    {
      // Power may have just enabled a cold SD card. Put it in SPI mode before
      // sharing the panel clocks, including the fixed-board startup path.
      if (!prepare_detail::prepare_sd_spi(*result.desc, result, ctx)) { return false; }
      ESP_LOGD("board_detect_m5", "Core panel observation after SD SPI preparation");
      const auto& display = desc_core2.display;
      const std::int8_t signals[] = {
        display.dc, display.sclk, display.mosi, display.miso
      };
      prepare_detail::pin_level(display.cs, true);
      prepare_detail::pin_level(desc_core2.sd.sd_cs, true);
      soft_spi_t bus(display.sclk, display.mosi, display.mosi, display.dc);
      bus.init();
      // Without reset, leave the same 120 ms window for a waking panel.
      variant = identify_panel_variant(bus, display.cs, keys,
                                            ctx.allow_reset ? 1 : 120);
      if (ctx.allow_reset)
      {
        prepare_detail::i2c_scope_t i2c(*ctx.transaction, ctx.probe_port,
                                         desc_core2.internal_i2c);
        if (!i2c.opened || !prepare_reset(desc_core2, result, ctx, i2c.port))
        {
          ctx.transaction->restore_to_snapshot(signals);
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
      return true;
    }

    bool fixed_prepare_core(board_result_t& result, const prepare_ctx_t& ctx)
    {
      const auto& desc = *result.desc;
      {
        prepare_detail::i2c_scope_t i2c(*ctx.transaction, ctx.probe_port, desc.internal_i2c);
        const auto* pmic = i2c.opened ? observe_core_pmic(i2c.port) : nullptr;
        // Never send a default controller's write list when its identity is unknown.
        if (pmic == nullptr)
        {
          ESP_LOGW("M5GFX", "Fixed board:%u PMIC unreadable; startup stopped",
                   static_cast<unsigned>(desc.def.id));
          return false;
        }
        result.option |= pmic->detected_option;
        if (!prepare_detail::prepare_power(desc, result, i2c.port, true)) { return false; }
      }
      panel_variant_t variant;
      std::uint32_t keys[4] = {};
      if (!observe_core_panel(result, ctx, variant, keys)) { return false; }
      log_panel_variant(variant, keys);
      if (variant == panel_variant_t::e) { result.option |= generated_options::core2::lcd_e; }
      const auto& display = desc.display;
      const std::int8_t signals[] = { display.dc, display.sclk, display.mosi, display.miso };
      ctx.transaction->restore_to_snapshot(signals);
      return prepare(desc, result, ctx);
    }

    bool observe_confirmed_core_panel(board_result_t& result, const prepare_ctx_t& ctx)
    {
      panel_variant_t variant;
      std::uint32_t keys[4] = {};
      const bool observed = observe_core_panel(result, ctx, variant, keys);
      const auto& display = desc_core2.display;
      const std::int8_t signals[] = { display.dc, display.sclk, display.mosi, display.miso };
      ctx.transaction->restore_to_snapshot(signals);
      if (!observed) { return false; }
      log_panel_variant(variant, keys);
      if (variant == panel_variant_t::e) { result.option |= generated_options::core2::lcd_e; }
      return true;
    }

    bool refine_core_family(board_result_t& result, const prepare_ctx_t& ctx)
    {
      const auto& display = desc_core2.display;
      const std::int8_t signals[] = {
        display.dc, display.sclk, display.mosi, display.miso
      };
      std::uint32_t keys[4] = {};
      panel_variant_t variant;
      if (!observe_core_panel(result, ctx, variant, keys)) { return false; }
      prepare_detail::i2c_scope_t i2c(*ctx.transaction, ctx.probe_port,
                                       desc_core2.internal_i2c);
      if (!i2c.opened)
      {
        ctx.transaction->restore_to_snapshot(signals);
        return false;
      }
      bool tough = false;
      bool core2 = false;
      const auto touch_start = io::millis();
      do
      {
        std::uint8_t touch_value = 0;
        tough = io::i2c_read_reg8(i2c.port, tough_touch_address, touch_probe_register,
                                  &touch_value, tough_touch_i2c_frequency);
        core2 = io::i2c_read_reg8(i2c.port, core2_touch_address, touch_probe_register,
                                  &touch_value, tough_touch_i2c_frequency);
#if defined (M5GFX_AUTODETECT_TEST_FAIL_CORE2_TOUCH) || defined (M5GFX_AUTODETECT_TEST_FAIL_CORE2_TOUCH_POST)
        tough = core2 = false;
#endif
        // M-Bus devices can answer at either address. Read both even if Tough
        // answers first; simultaneous responses cannot confirm a member.
        if (tough || core2) { break; }
        io::delay_ms(1);
      } while (io::millis() - touch_start < touch_startup_poll_ms);
      if ((tough && core2)
       || (tough && (result.option & generated_options::core2::new_pmic)))
      {
        ctx.transaction->restore_to_snapshot(signals);
        return false;
      }
      if (!tough && !core2)
      {
        const auto possible = [&](board_id_t id) -> const board_desc_t*
        {
          if (id == desc_core2.def.id) { return &desc_core2; }
          // Tough has no AXP2101 variant; an explicit preference cannot negate it.
          if (id == desc_tough.def.id
           && !(result.option & generated_options::core2::new_pmic)) { return &desc_tough; }
          return nullptr;
        };
        if (!select_provisional_member(ctx, &result,
                               possible(ctx.fallback), possible(ctx.hint),
                               &desc_core2, "Core2/Tough touch unidentified"))
        {
          ctx.transaction->restore_to_snapshot(signals);
          return false;
        }
      }
      log_panel_variant(variant, keys);
      if (tough || core2)
      {
        result.assign(tough ? &desc_tough : &desc_core2);
      }
      if (variant == panel_variant_t::e) { result.option |= generated_options::core2::lcd_e; }
      if (result.def->id == desc_tough.def.id)
      { result.option &= ~generated_options::core2::new_pmic; }
      ctx.transaction->restore_to_snapshot(signals);
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
      if (!prepare_detail::description_valid(desc_station)
       || !prepare_detail::description_valid(desc_core2)
       || !prepare_detail::description_valid(desc_tough)
       || !prepare_detail::gpio_valid(desc_station.display.dc)
       || !prepare_detail::gpio_valid(desc_core2.display.dc)
       || !prepare_detail::gpio_valid(desc_tough.display.dc))
      {
        return false;
      }
      prepare_detail::i2c_scope_t i2c(*ctx.transaction, ctx.probe_port,
                                      desc_core2.internal_i2c);
      if (!i2c.opened) { return false; }
      prepare_ctx_t prepare_ctx = ctx;
      prepare_ctx.probe_port = i2c.port;
      const auto* pmic = detail::observe_core_pmic(i2c.port);
      if (pmic == nullptr) { return false; }
      ctx.family_identified = true;
      ESP_LOGD("board_detect_m5", "power controller id=%02x", pmic->id_value);

      const bool new_pmic = pmic->id_value != detail::station_pmic_id;
      // Keep the scoped port alive; probe_i2c_ack would release it between reads.
      const auto touch_answers = [&](std::uint8_t addr) -> bool
      {
        return io::i2c_ack(i2c.port, addr, 100000, true);
      };
      bool core2_touch = touch_answers(detail::core2_touch_address);
      bool tough_touch = touch_answers(detail::tough_touch_address);
#if defined (M5GFX_AUTODETECT_TEST_FAIL_CORE2_TOUCH) || defined (M5GFX_AUTODETECT_TEST_FAIL_CORE2_TOUCH_PRE) || defined (M5GFX_AUTODETECT_TEST_CORE_FAMILY_FORCE_STATION)
      core2_touch = tough_touch = false;
#endif
      char g12 = '-', g32 = '-', g33 = '-';
      // Port A is external. Sample its pulls only for silent AXP192 members;
      // positive touch evidence and AXP2101 do not need this subdivision.
      if (!core2_touch && !tough_touch && !new_pmic)
      {
        const auto pulls = probe_pin_pulls(ctx, (1ULL << 12) | (1ULL << 32) | (1ULL << 33));
        const auto classify = [&](unsigned pin) -> char
        {
          const unsigned sample = ((pulls.pulldown_high >> pin) & 1)
                                | (((pulls.pullup_high >> pin) & 1) << 1);
          return "DXFU"[sample];
        };
        g12 = classify(12); g32 = classify(32); g33 = classify(33);
      }
#if defined (M5GFX_AUTODETECT_TEST_CORE_FAMILY_FORCE_STATION)
      g12 = 'F';
#endif
      ESP_LOGD("board_detect_m5", "AXP attempt=%u pmic=%02x touch38=%u touch2e=%u G12=%c G32=%c G33=%c",
               ctx.attempt, pmic->id_value, core2_touch, tough_touch, g12, g32, g33);
      *result = {};
      // External M-Bus ICs at these addresses can make the evidence contradictory.
      if ((core2_touch && tough_touch) || (new_pmic && tough_touch)) { return false; }
      if (core2_touch || tough_touch)
      {
        result->assign(core2_touch ? &desc_core2 : &desc_tough);
        result->option = pmic->detected_option;
        // Observe panel metadata without selecting the confirmed member again.
        result->observe_after_power = detail::observe_confirmed_core_panel;
        return true;
      }
      if (new_pmic)
      {
        // AXP2101 belongs only to Core2, but absent touch is abnormal. A failed
        // touch controller deliberately consumes all attempts before fallback.
        if (!select_provisional_member(ctx, result, &desc_core2, nullptr, nullptr,
                                       "AXP2101 Core2 touch unanswered")) { return false; }
        result->option = pmic->detected_option;
        result->observe_after_power = detail::observe_confirmed_core_panel;
        return true;
      }
      if (g12 == 'D' && g32 == 'U' && g33 == 'U')
      {
        // Cold AXP192 Core2 needs the shared Core2/Tough power list before touch
        // confirmation. Station's weak G12 pull-down may also read D, so both
        // Port A lines must read U. External pull-ups on both lines still admit
        // such a Station; no second member power list runs in this attempt.
        result->assign(&desc_core2);
        result->option = pmic->detected_option;
        result->refine = detail::refine_core_family;
        return true;
      }
      if (g12 != 'F' && g12 != 'D') { return false; }

      std::uint64_t sd_mask;
      if (!detail::sd_pull_mask(desc_core2, &sd_mask)) { return false; }
      // A cold AXP192 Core2/Tough with either Port A line externally held low
      // also enters this Station path and stays unidentified after an LCD miss.
      // Only the Station probe needs the exceptional pre-power SD transition.
      prepare_detail::pin_level(desc_core2.display.cs, true);
      const auto sd_pulls = probe_pin_pulls(ctx, sd_mask);
      const bool sd_present = sd_pulls.pulldown_high == sd_mask
                           && sd_pulls.pullup_high == sd_mask;
      std::uint32_t preprepared = 0;
      if (sd_present)
      {
        prepare_detail::pin_level(desc_core2.sd.sd_cs, true);
        board_result_t sd_result;
        sd_result.assign(&desc_core2);
        if (!prepare_detail::prepare_sd_spi(desc_core2, sd_result, prepare_ctx)) { return false; }
        preprepared |= sd_result.prepared & prepared_sd_spi;
      }
      result->assign(&desc_station);
      result->prepared = preprepared;
      if (!prepare_reset(desc_station, *result, prepare_ctx, i2c.port))
      {
        ctx.transaction->restore_to_snapshot(desc_station.reset.pin);
        return false;
      }
      const auto& display = desc_station.display;
      const auto detected_id = soft_spi_read32(
        ctx, display.sclk, display.mosi, display.mosi, display.dc, display.cs,
        detail::panel_id_command, 1);
      if ((detected_id & detail::station_id_mask) != detail::station_id)
      {
        ctx.transaction->restore_to_snapshot(desc_station.reset.pin);
        // Neither hint nor preference may send Core2 power after a Station miss.
        *result = {};
        return false;
      }
      ctx.transaction->restore_to_snapshot(desc_core2.sd.sd_cs);
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
      if (!prepare_detail::description_valid(desc_stack)
       || !prepare_detail::gpio_valid(desc_stack.display.dc)) { return false; }
      std::uint64_t sd_mask;
      if (!detail::sd_pull_mask(desc_stack, &sd_mask)) { return false; }
      prepare_detail::pin_level(desc_stack.display.cs, true);
      const auto pulls = probe_pin_pulls(ctx, sd_mask);
      ctx.transaction->restore_to_snapshot(desc_stack.display.cs);
      const bool pull_match = pulls.pulldown_high == sd_mask && pulls.pullup_high == sd_mask;
      if (!pull_match)
      {
        ESP_LOGD("board_detect_m5",
                 "M5Stack pull signature mismatch pd=%08x%08x pu=%08x%08x",
                 static_cast<unsigned>(pulls.pulldown_high >> 32),
                 static_cast<unsigned>(pulls.pulldown_high),
                 static_cast<unsigned>(pulls.pullup_high >> 32),
                 static_cast<unsigned>(pulls.pullup_high));
      }
      // G32 can be camera output or external SDA on other boards. Its low
      // level must not admit Stack reset, which can drop their power hold.
      // Require SD evidence even for hints and the final attempt.
      return pull_match;
    }

    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (!prepare_detail::description_valid(desc_stack)
       || !prepare_detail::gpio_valid(desc_stack.display.dc)) { return false; }
      *result = {};
      result->assign(&desc_stack);
      const prepare_ctx_t& prepare_ctx = ctx;
      const std::int8_t signals[] = {
        desc_stack.display.sclk, desc_stack.display.miso,
        desc_stack.display.mosi, desc_stack.display.dc
      };
      // Both devices are deselected before the first shared-wire operation.
      prepare_detail::pin_level(desc_stack.sd.sd_cs, true);
      prepare_detail::pin_level(desc_stack.display.cs, true);
      if (!prepare_detail::prepare_power(desc_stack, *result, ctx.probe_port)
       || !prepare_detail::prepare_sd_spi(desc_stack, *result, prepare_ctx)) { return false; }
      prepare_detail::pin_level(desc_stack.reset.pin, true);
      if (!prepare_reset(desc_stack, *result, prepare_ctx, ctx.probe_port, &result->option))
      { return false; }
      prepare_detail::hold_chip_selects(desc_stack);
      const auto& display = desc_stack.display;
      const auto panel_id = soft_spi_read32(
        ctx, display.sclk, display.mosi, display.mosi, display.dc, display.cs,
        detail::panel_id_command, 1);
      if ((panel_id & detail::panel_id_mask) != detail::common_panel_id)
      { return false; }
      ctx.transaction->restore_to_snapshot(signals);
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
      if (!prepare_detail::description_valid(desc_paper)
       || !prepare_detail::gpio_valid(desc_paper.display.busy)) { return false; }
      // This family contract keeps the mandatory reset from stage 1 through
      // stage 2, where prepared_reset records that it already completed.
      prepare_detail::pin_reset(desc_paper.reset, true);
      io::pin_mode(desc_paper.display.busy, io::pin_mode_t::input_pullup);
      const bool matched = !io::pin_read(desc_paper.display.busy);
      ctx.transaction->restore_to_snapshot(desc_paper.display.busy);
      if (!matched) { ctx.transaction->restore_to_snapshot(desc_paper.reset.pin); }
      return matched;
    }

    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (!prepare_detail::description_valid(desc_paper)
       || !prepare_detail::gpio_valid(desc_paper.display.busy)) { return false; }
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
        ctx.transaction->restore_to_snapshot(pins);
        return false;
      };
#if defined (M5GFX_AUTODETECT_TEST_FAIL_PAPER_CONFIRM)
      // Test-only failure injection after stage 1 retained reset state.
      return restore_and_fail();
#endif
      if (!prepare_detail::prepare_power(desc_paper, *result, ctx.probe_port)
       || !prepare_detail::prepare_sd_spi(desc_paper, *result, prepare_ctx))
      { return restore_and_fail(); }
      prepare_detail::hold_chip_selects(desc_paper);
      const auto& display = desc_paper.display;
      io::pin_mode(display.busy, io::pin_mode_t::input);

      soft_spi_t bus(display.sclk, display.mosi, display.miso, display.dc);
      bus.init();
      bool matched = false;
      auto started = io::millis();
      while (!io::pin_read(display.busy) && io::millis() - started <= 1024) { io::delay_ms(1); }
      if (io::pin_read(display.busy))
      {
        bus.beginTransaction();
        io::pin_low(display.cs);
        bus.writeData(__builtin_bswap16(0x6000), 16);
        bus.writeData(__builtin_bswap16(0x0302), 16);
        bus.wait();
        io::pin_high(display.cs);
        started = io::millis();
        while (!io::pin_read(display.busy) && io::millis() - started <= 192) { io::delay_ms(1); }
        io::pin_low(display.cs);
        bus.writeData(__builtin_bswap16(0x1000), 16);
        bus.writeData(__builtin_bswap16(0x0000), 16);
        std::uint8_t data[40] = {};
        bus.beginRead();
        bus.readBytes(data, sizeof(data));
        bus.endRead();
        bus.endTransaction();
        io::pin_high(display.cs);
        const std::uint32_t panel_size = (std::uint32_t(data[0]) << 24)
                                       | (std::uint32_t(data[1]) << 16)
                                       | (std::uint32_t(data[2]) << 8)
                                       | data[3];
        matched = panel_size == detail::paper_panel_size;
      }
      if (!matched)
      { return restore_and_fail(); }

      ctx.transaction->restore_to_snapshot(success_restore);
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

  class timercam_detector_t final : public board_detector_t
  {
  public:
    timercam_detector_t() : board_detector_t(members_) {}
    bool signature(probe_ctx_t& ctx) const override
    {
      return probe_i2c_bus_present(ctx, wiring::timercam::internal_i2c_sda,
                                   wiring::timercam::internal_i2c_scl);
    }
    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (!result || !probe_i2c_ack(ctx, wiring::timercam::internal_i2c_sda,
                                    wiring::timercam::internal_i2c_scl, 0x51))
      { return false; } // BM8563 RTC address in m5timercam catalog.
      result->assign(&desc_timercam);
      return true;
    }
  private:
    static const board_def_t* const members_[2];
  };
  const board_def_t* const timercam_detector_t::members_[2] = { &desc_timercam.def, nullptr };
  static const timercam_detector_t timercam_detector;

  #include "generated/esp32_d0wdq6_detector_order.hpp"

  success_log_t success_log(const board_result_t& result)
  {
    return success_log(esp32_d0wdq6_boards, result);
  }
}
}
}
