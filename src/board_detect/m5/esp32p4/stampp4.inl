static constexpr board_desc_t desc_stampp4 = {
  { static_cast<board_id_t>(lgfx::board_M5StampP4), "M5StampP4", def_flag_fallback },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), no_pins(),
  };
static constexpr board_desc_t desc_stampp4x = {
  { static_cast<board_id_t>(lgfx::board_M5StampP4X), "M5StampP4X", def_flag_fallback },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), no_pins(),
  };

class stampp4_detector_t final : public board_detector_t
{
public:
  stampp4_detector_t() : board_detector_t(members_) {}
  bool signature(probe_ctx_t& ctx) const override
  {
    // A carrier can add pulls to any pin, so StampP4 is only a weak candidate.
    if (ctx.candidate == nullptr)
    {
      esp_chip_info_t info;
      esp_chip_info(&info);
      ctx.candidate = info.revision >= 300 ? &desc_stampp4x.def : &desc_stampp4.def;
    }
    return false;
  }
  bool confirm(probe_ctx_t&, board_result_t*) const override { return false; }
private:
  static const board_def_t* const members_[3];
};
const board_def_t* const stampp4_detector_t::members_[3] = {
  &desc_stampp4.def, &desc_stampp4x.def, nullptr,
};
static const stampp4_detector_t stampp4_detector;
