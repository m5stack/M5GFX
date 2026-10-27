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
