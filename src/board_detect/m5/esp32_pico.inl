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
  // The legacy read checks the low byte only; the part-catalog probe describes
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
static bool fixed_start_coreink(board_result_t& result, const prepare_ctx_t& ctx)
{ return fixed_start_spi_variant(result, ctx, coreink_member_descs[0]); }
static const spi_id_detector_t coreink_detector(coreink_members, coreink_member_descs, 1);

static const board_def_t* const stickcplus2_members[] = { &desc_stickcplus2.def, nullptr };
static const spi_id_member_t stickcplus2_member_descs[] = {
  { &desc_stickcplus2, stickcplus2_probes, 1, true, false, 0, true },
};
static const spi_id_detector_t stickcplus2_detector(
  stickcplus2_members, stickcplus2_member_descs, 1);

#include "atom_touch.inl"

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
    lgfx::pinMode(2, lgfx::pin_mode_t::input_pullup);
    esp_rom_delay_us(5);
    const bool atom_g2 = lgfx::gpio_in(2);
    ctx.transaction->restore_start({ 2 });
    if (!atom_g2) { return false; }
    const auto read_g34 = [](lgfx::pin_mode_t mode) {
      lgfx::pinMode(23, mode); // G23 is exposed: internal pull only, never drive it.
      esp_rom_delay_us(5);
      return lgfx::gpio_in(34);
    };
    lgfx::pinMode(34, lgfx::pin_mode_t::input);
    const bool down = read_g34(lgfx::pin_mode_t::input_pulldown);
    const bool up1 = read_g34(lgfx::pin_mode_t::input_pullup);
    const bool down2 = read_g34(lgfx::pin_mode_t::input_pulldown);
    const bool up2 = read_g34(lgfx::pin_mode_t::input_pullup);
    ctx.transaction->restore_start({ 23, 34 });

    if (down)
    {
      result->assign(&desc_atomvoice); // Microphone DATA remains high with G23 low.
      if (!(up1 && down2 && up2)) { result->provisional = true; }
      return true;
    }
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
      lgfx::pinMode(21, lgfx::pin_mode_t::input_pulldown);
      lgfx::pinMode(25, lgfx::pin_mode_t::input_pulldown);
      esp_rom_delay_us(10);
      const bool matrix_pulls = lgfx::gpio_in(21) && lgfx::gpio_in(25);
      ctx.transaction->restore_start({ 21, 25 });
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
