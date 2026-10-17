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
  { return REG_GET_FIELD(EFUSE_RD_MAC_SPI_SYS_4_REG, EFUSE_FLASH_CAP); }
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
