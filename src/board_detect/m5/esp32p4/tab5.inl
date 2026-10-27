namespace tab5_detail
{
  constexpr int sda = wiring::tab5::internal_i2c_sda;
  constexpr int scl = wiring::tab5::internal_i2c_scl;
  constexpr int touch_int = wiring::tab5::touch_int;
  constexpr int_fast16_t i2c_port = io::i2c_port_1;
  constexpr std::uint32_t i2c_freq = 100000;
  constexpr std::uint8_t pi4io1_addr = 0x43;
  constexpr std::uint8_t pi4io2_addr = 0x44;
  // Board-private prepared bits carry observations to construction. They never
  // enter the public option or NVS; detection must use software I2C only.
  enum : std::uint32_t
  {
    read_st_touch_fw = 1u << 16,
    touch_st7121 = 1u << 17,
    touch_st7123 = 1u << 18,
    touch_gt911 = 1u << 19,
  };

  bool observe_touch(board_result_t& result, const prepare_ctx_t& ctx)
  {
#if CONFIG_SPIRAM
    startup_detail::i2c_scope_t i2c(*ctx.transaction, ctx.i2c_port_probe,
                                    result.desc->internal_i2c);
    if (!i2c.opened) { return false; }
    int last_logged_fw = -1;
    for (int i = 0; i < 60; ++i)
    {
      std::uint8_t fw_version = 0;
      std::uint8_t fw_reg[2] = { 0, 0 };
      if (io::i2c_write_read(i2c.port, io::st_touch_address(),
                                           fw_reg, sizeof(fw_reg), &fw_version, 1,
                                           i2c_freq))
      {
        result.prepared |= read_st_touch_fw;
        if (fw_version != last_logged_fw)
        {
          last_logged_fw = fw_version;
          ESP_LOGI("M5GFX", "M5Tab5 ST touch FW version %02x", fw_version);
          if (fw_version != 1 && fw_version != 3)
          {
            ESP_LOGW("M5GFX", "M5Tab5 unknown ST touch FW version %02x", fw_version);
          }
        }
        if (fw_version == 1) { result.prepared |= touch_st7121; break; }
        if (fw_version == 3) { result.prepared |= touch_st7123; break; }
      }
      else if (io::i2c_ack(i2c.port, io::gt911_touch_address(), i2c_freq))
      {
        result.prepared |= touch_gt911;
        ESP_LOGI("M5GFX", "M5Tab5 GT911 touch detected");
        break;
      }
      io::delay_ms(10);
    }
    if (!(result.prepared & (read_st_touch_fw | touch_gt911)))
    {
      ESP_LOGW("M5GFX", "M5Tab5 ST touch FW version read failed");
    }
#endif
    return true;
  }

  bool fixed_start(board_result_t& result, const prepare_ctx_t& ctx)
  {
    result.observe_after_power = observe_touch;
    result.observe_on_power_failure = true;
    return prepare(*result.desc, result, ctx);
  }

  // prepare drives TP_INT while releasing the touch controller, so it must be
  // inside both the operation scope and the detection transaction snapshot.
  static constexpr std::int8_t prepare_gpio_pins[] = { touch_int };
  static const pmic_variant_t power_variants[] = {
    pmic_variant_ack_only(pi4io1_addr, 0, ops::list(pmic_ops::tab5_power_on),
                          reg_bit(0, 0), reg_bit(0, 0),
                          { nullptr, 0 }, { nullptr, 0 }, 0),
  };
}

static constexpr board_desc_t desc_tab5 = {
  { static_cast<board_id_t>(lgfx::board_M5Tab5), "M5Tab5", 0 },
  i2c_power_confirmed(tab5_detail::i2c_freq, tab5_detail::power_variants,
                      pmic_ops::tab5_devices),
  no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  internal_i2c(tab5_detail::sda, tab5_detail::scl, tab5_detail::i2c_port),
  no_options(), pins(tab5_detail::prepare_gpio_pins),
  };
static constexpr board_desc_t desc_tab5x = {
  { static_cast<board_id_t>(lgfx::board_M5Tab5X), "M5Tab5X", 0 },
  i2c_power_confirmed(tab5_detail::i2c_freq, tab5_detail::power_variants,
                      pmic_ops::tab5_devices),
  no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  internal_i2c(tab5_detail::sda, tab5_detail::scl, tab5_detail::i2c_port),
  no_options(), pins(tab5_detail::prepare_gpio_pins),
  };

class tab5_family_detector_t final : public board_detector_t
{
public:
  tab5_family_detector_t() : board_detector_t(members_) {}
  bool signature(probe_ctx_t& ctx) const override
  {
    constexpr detect_class_expected_t expected = wiring::tab5::detect_class_expected;
    const bool bus_present = probe_i2c_bus_present(ctx, tab5_detail::sda, tab5_detail::scl);
    // Backlight wiring only suggests a recovery candidate; it must not gate
    // PI4IO confirmation, which remains valid with the wireless supply off.
    if (bus_present && ctx.candidate == nullptr
     && match_detect_class(expected, probe_pin_pulls(ctx, expected.mask)))
    {
      ctx.candidate = &revision_desc()->def;
    }
    return bus_present;
  }
  bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
  {
    if (result == nullptr) { return false; }
    std::uint8_t value = 0;
    if (!probe_i2c_read(ctx, tab5_detail::sda, tab5_detail::scl,
                        tab5_detail::pi4io1_addr, pi4io_id_register, &value, 1,
                        tab5_detail::i2c_freq, 0)
     || !is_pi4io(value)
     || !probe_i2c_read(ctx, tab5_detail::sda, tab5_detail::scl,
                        tab5_detail::pi4io2_addr, pi4io_id_register, &value, 1,
                        tab5_detail::i2c_freq, 0)
     || !is_pi4io(value))
    {
      // The recovery candidate from signature() is what the caller will use.
      if (ctx.candidate == &desc_tab5.def || ctx.candidate == &desc_tab5x.def)
      { warn_hint_mismatch(ctx, ctx.candidate); }
      return false;
    }
    // Tab5X upgrades the P4 silicon revision; saved hints cannot identify it.
    const auto* desc = revision_desc();
    warn_hint_mismatch(ctx, &desc->def);
    result->assign(desc);
    result->observe_after_power = tab5_detail::observe_touch;
    // The touch controller may be readable even if a later power write failed.
    result->observe_on_power_failure = true;
    result->candidate = ctx.candidate;
    return true;
  }
private:
  static const board_desc_t* revision_desc()
  {
    return io::chip_revision() >= 300 ? &desc_tab5x : &desc_tab5;
  }
  static void warn_hint_mismatch(const probe_ctx_t& ctx, const board_def_t* def)
  {
    if (ctx.attempt == 0
     && (ctx.hint == desc_tab5.def.id || ctx.hint == desc_tab5x.def.id)
     && ctx.hint != def->id)
    {
      const auto revision = io::chip_revision();
      ESP_LOGW("M5GFX", "[Autodetect] Tab5 hint:%u disagrees with chip revision:%u; using %s",
               static_cast<unsigned>(ctx.hint), static_cast<unsigned>(revision),
               def->name);
    }
  }
  static const board_def_t* const members_[3];
};
const board_def_t* const tab5_family_detector_t::members_[3] = {
  &desc_tab5.def, &desc_tab5x.def, nullptr
};
static const tab5_family_detector_t tab5_family_detector;

construct_status_t construct_tab5(const board_result_t&, display_parts_t*);
