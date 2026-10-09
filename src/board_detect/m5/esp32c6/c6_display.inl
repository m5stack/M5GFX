namespace c6_display_detail
{
  constexpr int sda = wiring::nesson1::internal_i2c_sda;
  constexpr int scl = wiring::nesson1::internal_i2c_scl;
  constexpr int_fast16_t i2c_port = I2C_NUM_0;
  constexpr std::uint32_t pi4io_freq = 100000;
  constexpr std::uint64_t signature_bit = std::uint64_t(1) << 18;
  enum class candidate_t : std::uint8_t { none, unitc6l, nesson1 };

  static const pmic_variant_t nesson1_power_variants[] = {
    pmic_variant_ack_only(specs::nesson1::i2c_pi4io2::i2c_addr, 0,
                          ops::list(pmic_ops::nesson1_power_on),
                          reg_bit(0, 0), reg_bit(0, 0),
                          { nullptr, 0 }, { nullptr, 0 }, { nullptr, 0 }, 0),
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
        // GPIO18 alone does not identify UnitC6L. Preserve a known UnitC6L
        // only on the last retry, without caching the unverified fallback.
        if (!ctx.final_attempt || ctx.hint != desc_unitc6l.def.id) { return false; }
        result->assign(&desc_unitc6l);
        result->transient_fallback = true;
        ESP_LOGW("board_detect_m5", "UnitC6L PI4IO unanswered; using hinted board for this boot");
        return true;
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
#include "../generated/esp32c6_detector_order.hpp"

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
