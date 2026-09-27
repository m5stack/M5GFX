namespace tab5_detail
{
  constexpr int sda = wiring::tab5::internal_i2c_sda;
  constexpr int scl = wiring::tab5::internal_i2c_scl;
  constexpr int touch_int = wiring::tab5::touch_int;
  constexpr int_fast16_t i2c_port = I2C_NUM_1;
  constexpr std::uint32_t i2c_freq = 100000;
  constexpr std::uint8_t pi4io1_addr = 0x43;
  constexpr std::uint8_t pi4io2_addr = 0x44;
  // prepare drives TP_INT while releasing the touch controller, so it must be
  // inside both the operation scope and the detection transaction snapshot.
  static constexpr std::int8_t prepare_gpio_pins[] = { touch_int };
  static const pmic_variant_t power_variants[] = {
    pmic_variant_ack_only(pi4io1_addr, 0, ops::list(pmic_ops::tab5_power_on),
                          reg_bit(0, 0), reg_bit(0, 0),
                          { nullptr, 0 }, { nullptr, 0 }, { nullptr, 0 }, 0),
  };
}

static constexpr board_desc_t desc_tab5 = {
  { static_cast<board_id_t>(lgfx::board_M5Tab5), "M5Tab5", 0 },
  i2c_power_confirmed(tab5_detail::i2c_freq, tab5_detail::power_variants,
                      pmic_ops::tab5_devices),
  no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  internal_i2c(tab5_detail::sda, tab5_detail::scl, tab5_detail::i2c_port),
  no_direct_reset_panel_reload_wait(), no_options(), pins(tab5_detail::prepare_gpio_pins),
  };
static constexpr board_desc_t desc_tab5x = {
  { static_cast<board_id_t>(lgfx::board_M5Tab5X), "M5Tab5X", 0 },
  i2c_power_confirmed(tab5_detail::i2c_freq, tab5_detail::power_variants,
                      pmic_ops::tab5_devices),
  no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  internal_i2c(tab5_detail::sda, tab5_detail::scl, tab5_detail::i2c_port),
  no_direct_reset_panel_reload_wait(), no_options(), pins(tab5_detail::prepare_gpio_pins),
  };
static const board_def_t& board_tab5 = desc_tab5.def;
static const board_def_t& board_tab5x = desc_tab5x.def;

class tab5_family_detector_t final : public board_detector_t
{
public:
  tab5_family_detector_t() : board_detector_t(members_) {}
  bool signature(probe_ctx_t& ctx) const override
  {
    return probe_i2c_bus_present(ctx, tab5_detail::sda, tab5_detail::scl);
  }
  bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
  {
    if (result == nullptr) { return false; }
    std::uint8_t value = 0;
    if (!probe_i2c_read(ctx, tab5_detail::sda, tab5_detail::scl,
                        tab5_detail::pi4io1_addr, 0x01, &value, 1,
                        tab5_detail::i2c_freq, 0)
     || !probe_i2c_read(ctx, tab5_detail::sda, tab5_detail::scl,
                        tab5_detail::pi4io2_addr, 0x01, &value, 1,
                        tab5_detail::i2c_freq, 0))
    {
      return false;
    }
    result->assign(ctx.hint == desc_tab5x.def.id ? &desc_tab5x : &desc_tab5);
    return true;
  }
private:
  static const board_def_t* const members_[3];
};
const board_def_t* const tab5_family_detector_t::members_[3] = {
  &board_tab5, &board_tab5x, nullptr
};
static const tab5_family_detector_t tab5_family_detector;

construct_status_t construct_tab5(const board_result_t&, display_parts_t*);
