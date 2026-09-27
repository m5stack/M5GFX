  namespace cores3_detail
  {
    // vbus_5v is an externally meaningful board option.  The camera bit is
    // detector-private state carried through the same field only to refine().
    constexpr std::uint32_t vbus_5v = generated_options::cores3::vbus_5v;
    constexpr std::uint32_t internal_camera_confirmed =
      generated_options::cores3::internal_camera_confirmed;
    constexpr std::uint32_t release_probe_unavailable = std::uint32_t(1) << 31;
    constexpr int sda = wiring::cores3::internal_i2c_sda;
    constexpr int scl = wiring::cores3::internal_i2c_scl;
    constexpr std::uint32_t i2c_freq = specs::cores3::pmic::i2c_freq;
    constexpr std::uint8_t axp_addr = specs::cores3::pmic::i2c_addr;
    constexpr std::uint8_t aw_addr = specs::cores3::i2c_io_expander::i2c_addr;
    bool refine(board_result_t& result, const prepare_ctx_t& ctx);

    // Detection rollback restores GPIO and chip selects only.  Like the legacy
    // path, completed AW9523/AXP2101 writes are intentionally not rolled back.
    static const pmic_variant_t power_variants[] = {
      pmic_variant_confirmed_option(axp_addr, specs::cores3::pmic::id_reg,
        ops::list(pmic_ops::cores3_vbus_off_power_on), reg_bit(0, 0), reg_bit(0, 0),
        ops::no_ops(), ops::no_ops(), { nullptr, 0 }, vbus_5v, 0),
      pmic_variant_confirmed_option(axp_addr, specs::cores3::pmic::id_reg,
        ops::list(pmic_ops::cores3_vbus_5v_power_on), reg_bit(0, 0), reg_bit(0, 0),
        ops::no_ops(), ops::no_ops(), { nullptr, 0 }, vbus_5v, vbus_5v),
    };
  }

  static constexpr board_desc_t desc_cores3 = {
    { id(lgfx::board_M5StackCoreS3), "M5StackCoreS3", 0 },
    i2c_power_confirmed(cores3_detail::i2c_freq, cores3_detail::power_variants,
                        pmic_ops::cores3_devices),
    no_reset(), shared_sd(wiring::cores3::shared_sd_sclk, wiring::cores3::shared_sd_mosi,
                          wiring::cores3::shared_sd_miso, wiring::cores3::shared_sd_sd_cs,
                          wiring::cores3::shared_sd_other_cs),
    display_pins(wiring::cores3::display_sclk, wiring::cores3::display_mosi,
                 wiring::cores3::display_miso, wiring::cores3::display_dc,
                 wiring::cores3::display_cs, wiring::cores3::display_rst,
                 wiring::cores3::display_busy),
    pins(wiring::cores3::hold),
    internal_i2c(cores3_detail::sda, cores3_detail::scl, wiring::cores3::internal_i2c_port),
    options(generated_options::cores3::names),
    pins(wiring::cores3::hold),
  };
  static constexpr board_desc_t desc_cores3se = {
    { id(lgfx::board_M5StackCoreS3SE), "M5StackCoreS3SE", 0 },
    desc_cores3.power, desc_cores3.reset, desc_cores3.sd, desc_cores3.display,
    desc_cores3.hold_high_pins, desc_cores3.internal_i2c,
    desc_cores3.option_names,
    desc_cores3.op_gpio_pins,
  };
  static constexpr board_desc_t desc_stackchan = {
    { id(lgfx::board_M5StackChan), "M5StackChan", 0 },
    desc_cores3.power, desc_cores3.reset, desc_cores3.sd, desc_cores3.display,
    desc_cores3.hold_high_pins, desc_cores3.internal_i2c,
    desc_cores3.option_names,
    desc_cores3.op_gpio_pins,
  };
  static_assert(wiring::cores3::display_miso == wiring::cores3::display_dc,
                "CoreS3 deliberately shares LCD D/C with SPI MISO");
  static_assert(wiring::cores3::touches_opi_pins,
                "CoreS3 detection must remain gated while OPI pins are unavailable");
  static_assert(sizeof(pmic_ops::cores3_vbus_off_power_on) / sizeof(ops::op_t) == 10
             && sizeof(pmic_ops::cores3_vbus_5v_power_on) / sizeof(ops::op_t) == 10,
                "CoreS3 power variants preserve the ten legacy register operations");
  namespace cores3_detail
  {
    bool read(const prepare_ctx_t& ctx, std::uint8_t addr, std::uint8_t reg,
              std::uint8_t* value, std::uint32_t freq)
    {
      probe_ctx_t probe;
      static_cast<prepare_ctx_t&>(probe) = ctx;
      return probe_i2c_read(probe, sda, scl, addr, reg, value, 1, freq, 0);
    }

    bool camera_id(probe_ctx_t& ctx)
    {
      std::uint8_t camera = 0;
      return probe_i2c_read(ctx, sda, scl, specs::cores3::i2c_camera::i2c_addr,
                            specs::cores3::i2c_camera::id_reg, &camera, 1,
                            specs::cores3::i2c_camera::i2c_freq, 0)
          && camera == specs::cores3::i2c_camera::id_value;
    }

    bool refine_panel(board_result_t& result, const prepare_ctx_t& ctx)
    {
      const auto& display = desc_cores3.display;
      probe_ctx_t probe;
      static_cast<prepare_ctx_t&>(probe) = ctx;
      // The LCD bus is 3-wire: read data returns on MOSI, not on the D/C pin.
      const auto panel_id = soft_spi_read32(
        probe, display.sclk, display.mosi, display.mosi, display.dc, display.cs,
        0x04, 1, 1, true);
      if ((panel_id & specs::cores3::probe_ili9342c::mask)
          != specs::cores3::probe_ili9342c::values[0])
      {
        ESP_LOGW("M5GFX", "[Autodetect] CoreS3 panel ID mismatch: 0x%08x",
                 static_cast<unsigned>(panel_id));
      }
      soft_spi_t bus(display.sclk, display.mosi, display.mosi, display.dc);
      bus.init();
      std::uint32_t keys[4] = {};
      const auto variant = detail::identify_panel_variant(bus, display.cs, keys, 1);
      detail::log_panel_variant(variant, keys);
      if (variant == detail::panel_variant_t::e)
      {
        result.option |= generated_options::cores3::lcd_e;
      }
      const std::int8_t signals[] = {
        display.dc, display.sclk, display.mosi, display.miso
      };
      ctx.transaction->restore_start(signals);
      return true;
    }

    bool refine(board_result_t& result, const prepare_ctx_t& ctx)
    {
      const bool capacitance_said_se = result.desc == &desc_cores3se;
      const bool confirmed_before_power = result.option & internal_camera_confirmed;
      const bool release_was_unavailable = result.option & release_probe_unavailable;
      result.option &= ~release_probe_unavailable;
      bool has_camera = confirmed_before_power;
      if (!confirmed_before_power)
      {
        // One post-power read is deliberately retained for GPIO-classified
        // boards so a now-responsive camera can correct only toward CoreS3.
#if defined(M5GFX_AUTODETECT_TEST_CORES3_NO_CAMERA)
        has_camera = false;
#else
        probe_ctx_t probe;
        static_cast<prepare_ctx_t&>(probe) = ctx;
        has_camera = camera_id(probe);
#endif
        if (!has_camera)
        {
          // Only an unavailable release probe falls back to the legacy
          // camera-absence rule; a completed ambiguous probe still biases CoreS3.
          if (capacitance_said_se || release_was_unavailable)
          {
            if (release_was_unavailable) { result.assign(&desc_cores3se); }
            return refine_panel(result, ctx);
          }
          ESP_LOGW("M5GFX", "[Autodetect] CoreS3 capacitance indicated camera family, but camera ID was unavailable");
        }
        else if (capacitance_said_se)
        {
          ESP_LOGW("M5GFX", "[Autodetect] CoreS3 capacitance indicated SE, but camera ID matched; using camera family");
          result.assign(&desc_cores3);
        }
      }
      std::uint8_t firmware = 0;
#if defined(M5GFX_AUTODETECT_TEST_CORES3_FORCE_STACKCHAN)
      firmware = specs::stackchan::i2c_stackchan_ioe::firmware_min;
      const bool has_ioe = true;
#else
      const bool has_ioe = read(ctx, specs::stackchan::i2c_stackchan_ioe::i2c_addr,
                                specs::stackchan::i2c_stackchan_ioe::firmware_reg,
                                &firmware, specs::stackchan::i2c_stackchan_ioe::i2c_freq);
#endif
      result.assign(has_ioe && firmware >= specs::stackchan::i2c_stackchan_ioe::firmware_min
                  ? &desc_stackchan : &desc_cores3);
      return refine_panel(result, ctx);
    }
  }

  class cores3_family_detector_t final : public board_detector_t
  {
  public:
    cores3_family_detector_t() : board_detector_t(members_) {}
    bool signature(probe_ctx_t& ctx) const override
    {
      // GPIO35-37 belong to OPI PSRAM when conditional pins are unavailable;
      // reject the whole family before even touching its I2C signature.
      if (ctx.conditional_pins_unavailable) { return false; }
      return probe_i2c_ack(ctx, cores3_detail::sda, cores3_detail::scl, cores3_detail::axp_addr)
          && probe_i2c_ack(ctx, cores3_detail::sda, cores3_detail::scl, cores3_detail::aw_addr);
    }
    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (result == nullptr) { return false; }
      std::uint8_t id_value = 0;
      if (!probe_i2c_read(ctx, cores3_detail::sda, cores3_detail::scl,
                          cores3_detail::axp_addr, specs::cores3::pmic::id_reg, &id_value, 1,
                          cores3_detail::i2c_freq, 0)
       || id_value != specs::cores3::pmic::id_value)
      {
        return false;
      }
      if (!probe_i2c_read(ctx, cores3_detail::sda, cores3_detail::scl,
                          cores3_detail::aw_addr, specs::cores3::i2c_io_expander::id_reg,
                          &id_value, 1, specs::cores3::i2c_io_expander::i2c_freq, 0)
       || id_value != specs::cores3::i2c_io_expander::id_value)
      {
        return false;
      }
      bool has_camera = false;
#if !defined(M5GFX_AUTODETECT_TEST_CORES3_NO_CAMERA) \
 && !defined(M5GFX_AUTODETECT_TEST_CORES3_CAMERA_ABSENT_AT_CONFIRM) \
 && !defined(M5GFX_AUTODETECT_TEST_CORES3_FORCE_SE) \
 && !defined(M5GFX_AUTODETECT_TEST_CORES3_RELEASE_AMBIGUOUS)
      has_camera = cores3_detail::camera_id(ctx);
#endif
      pin_release_band_t family_band = pin_release_band_t::long_release;
      if (!has_camera)
      {
        const auto release = probe_dedicated_pin_release(
          ctx, specs::cores3::release_probe::pins,
          sizeof(specs::cores3::release_probe::pins) / sizeof(specs::cores3::release_probe::pins[0]),
          specs::cores3::release_probe::reads, specs::cores3::release_probe::samples,
          specs::cores3::release_probe::settle_us);
        const auto summary = summarize_dedicated_release(
          release, specs::cores3::release_probe::short_max_ns,
          specs::cores3::release_probe::long_min_ns);
        if (!release.available) { result->option |= cores3_detail::release_probe_unavailable; }
        family_band = summary.band;
#if defined(M5GFX_AUTODETECT_TEST_CORES3_FORCE_SE)
        family_band = pin_release_band_t::short_release;
#elif defined(M5GFX_AUTODETECT_TEST_CORES3_RELEASE_AMBIGUOUS)
        family_band = pin_release_band_t::ambiguous;
#endif
        ESP_LOGD("M5GFX", "[Autodetect] CoreS3 release: G%d=%uns G%d=%uns G%d=%uns G%d=%uns "
                           "G%d=%uns G%d=%uns G%d=%uns G%d=%uns avg=%uns valid=%u/%u",
                 specs::cores3::release_probe::pins[0], unsigned(summary.pin_ns[0]),
                 specs::cores3::release_probe::pins[1], unsigned(summary.pin_ns[1]),
                 specs::cores3::release_probe::pins[2], unsigned(summary.pin_ns[2]),
                 specs::cores3::release_probe::pins[3], unsigned(summary.pin_ns[3]),
                 specs::cores3::release_probe::pins[4], unsigned(summary.pin_ns[4]),
                 specs::cores3::release_probe::pins[5], unsigned(summary.pin_ns[5]),
                 specs::cores3::release_probe::pins[6], unsigned(summary.pin_ns[6]),
                 specs::cores3::release_probe::pins[7], unsigned(summary.pin_ns[7]),
                 unsigned(summary.average_ns), unsigned(summary.valid_pins), unsigned(summary.pin_count));
        if (family_band == pin_release_band_t::ambiguous)
        {
          ESP_LOGW("M5GFX", "[Autodetect] CoreS3 release result was ambiguous; biasing toward camera family");
        }
      }
      constexpr std::uint64_t spi_mask =
          (std::uint64_t(1) << wiring::cores3::display_miso)
        | (std::uint64_t(1) << wiring::cores3::display_sclk)
        | (std::uint64_t(1) << wiring::cores3::display_mosi);
      const auto pull = probe_pin_pulls(ctx, spi_mask);
      (void)pull;  // FORCE_VBUS still performs/restores the legacy-equivalent measurement.
      // Unlike the legacy input_pullup sequence, probe_pin_pulls restores the
      // captured pin modes immediately. Construction's SPI init overwrites
      // these pins next, so retaining the temporary pull-ups had no effect.
      result->assign(family_band == pin_release_band_t::short_release ? &desc_cores3se
                                                                      : &desc_cores3);
      if (has_camera) { result->option |= cores3_detail::internal_camera_confirmed; }
#if defined(M5GFX_AUTODETECT_TEST_CORES3_FORCE_VBUS)
      result->option |= cores3_detail::vbus_5v;
#else
      if ((pull.pullup_high & spi_mask) == 0) { result->option |= cores3_detail::vbus_5v; }
#endif
      result->refine = cores3_detail::refine;
      return true;
    }
  private:
    static const board_def_t* const members_[];
  };
  const board_def_t* const cores3_family_detector_t::members_[] = {
    &desc_cores3.def, &desc_cores3se.def, &desc_stackchan.def, nullptr,
  };
  static const cores3_family_detector_t cores3_family_detector;
