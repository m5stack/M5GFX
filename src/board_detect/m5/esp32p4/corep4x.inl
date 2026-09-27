namespace corep4x_detail
{
  constexpr int sda = wiring::corep4x::internal_i2c_sda;
  constexpr int scl = wiring::corep4x::internal_i2c_scl;
  constexpr int_fast16_t i2c_port = I2C_NUM_1;
  constexpr std::uint32_t i2c_freq = pmic_ops::pm1_i2c_freq;
  static const pmic_variant_t power_variants[] = {
    pmic_variant_ack_only(pmic_ops::pm1_i2c_addr, 0,
                          ops::list(pmic_ops::corep4x_power_on),
                          reg_bit(0, 0), reg_bit(0, 0),
                          { nullptr, 0 }, { nullptr, 0 }, { nullptr, 0 }, 0),
  };
}

static constexpr board_desc_t desc_corep4x = {
  { static_cast<board_id_t>(lgfx::board_M5CoreP4X), "M5CoreP4X", 0 },
  i2c_power_confirmed(corep4x_detail::i2c_freq, corep4x_detail::power_variants,
                      pmic_ops::pm1_family_devices),
  no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  internal_i2c(corep4x_detail::sda, corep4x_detail::scl,
               corep4x_detail::i2c_port),
  no_direct_reset_panel_reload_wait(), no_options(), no_pins(),
  };
static const board_def_t& board_corep4x = desc_corep4x.def;

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
const board_def_t* const corep4x_detector_t::members_[2] = { &board_corep4x, nullptr };
static const corep4x_detector_t corep4x_detector;

construct_status_t construct_corep4x(const board_result_t&, display_parts_t*);
