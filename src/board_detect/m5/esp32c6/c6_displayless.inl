static constexpr board_desc_t desc_stampc6 = {
  { static_cast<board_id_t>(lgfx::board_M5StampC6), "M5StampC6", 0 },
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
  // The package's flash capacity is a stable eFuse fact; check it before
  // probing NanoC6 GPIO3, which is exposed on StampC6 as a user header pin.
  inline unsigned flash_capacity()
  { return REG_GET_FIELD(EFUSE_RD_MAC_SPI_SYS_4_REG, EFUSE_FLASH_CAP); }
}

class stampc6_detector_t final : public board_detector_t
{
public:
  stampc6_detector_t() : board_detector_t(members_) {}
  bool signature(probe_ctx_t&) const override
  { return c6_displayless_detail::flash_capacity() == 2; }
  bool confirm(probe_ctx_t&, board_result_t* result) const override
  { if (!result) return false; result->assign(&desc_stampc6); return true; }
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
  { return c6_displayless_detail::flash_capacity() != 2 && probe_pin_pullup_low(ctx, 3); }
  bool confirm(probe_ctx_t&, board_result_t* result) const override
  { if (!result) return false; result->assign(&desc_nanoc6); return true; }
private:
  static const board_def_t* const members_[2];
};
const board_def_t* const nanoc6_detector_t::members_[2] = { &desc_nanoc6.def, nullptr };
static const nanoc6_detector_t nanoc6_detector;
