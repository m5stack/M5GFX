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
  { id(lgfx::board_M5AtomPsram), "M5AtomPsram", def_flag_fallback },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), no_pins(),
};

static constexpr std::uint32_t stickc_id_values[] = { 0x7C };
static constexpr std::uint32_t stickcplus_id_values[] = { 0x81, 0x85 };
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
static const spi_id_detector_t coreink_detector(coreink_members, coreink_member_descs, 1);

static const board_def_t* const stickcplus2_members[] = { &desc_stickcplus2.def, nullptr };
static const spi_id_member_t stickcplus2_member_descs[] = {
  { &desc_stickcplus2, stickcplus2_probes, 1, true, false, 0, true },
};
static const spi_id_detector_t stickcplus2_detector(
  stickcplus2_members, stickcplus2_member_descs, 1);

class atompsram_detector_t final : public board_detector_t
{
public:
  atompsram_detector_t() : board_detector_t(members_) {}
  bool signature(probe_ctx_t&) const override { return true; }
  bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
  {
    if (result == nullptr) { return false; }
    // AtomPsram has no probe of its own yet; it is chosen only because the
    // Plus2 panel did not answer. Without a stored AtomPsram hint, wait for
    // the caller's final retry so one missed read does not store AtomPsram.
    // With the hint, accept after a single Plus2 miss (Plus2 is still probed
    // first), which is why this no-display board is recorded in NVS.
    if (!ctx.final_attempt && ctx.hint != id(lgfx::board_M5AtomPsram))
    { return false; }
    result->assign(&desc_atompsram);
    return true;
  }
private:
  static const board_def_t* const members_[];
};
const board_def_t* const atompsram_detector_t::members_[] = {
  &desc_atompsram.def, nullptr,
};
static const atompsram_detector_t atompsram_detector;

static const board_detector_t* const esp32_pico_d4_detectors[] = {
  &stickc_family_detector, &coreink_detector, nullptr,
};
static const board_detector_t* const esp32_picov3_detectors[] = {
  &stickcplus2_detector, &atompsram_detector, nullptr,
};

construct_status_t construct_stickc(const board_result_t&, display_parts_t*);
construct_status_t construct_stickcplus(const board_result_t&, display_parts_t*);
construct_status_t construct_coreink(const board_result_t&, display_parts_t*);
construct_status_t construct_stickcplus2(const board_result_t&, display_parts_t*);
construct_status_t construct_atompsram(const board_result_t&, display_parts_t*);
