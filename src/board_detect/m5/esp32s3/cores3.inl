  namespace cores3_detail
  {
    // vbus_5v is an externally meaningful board option.  The camera bit is
    // detector-private state carried through the same field only to refine().
    constexpr std::uint32_t vbus_5v = generated_options::cores3::vbus_5v;
    constexpr std::uint32_t internal_camera_confirmed =
      generated_options::cores3::internal_camera_confirmed;
    constexpr std::uint8_t release_unavailable = 3;
    static_assert(release_unavailable != static_cast<unsigned>(pin_release_band_t::short_release)
               && release_unavailable != static_cast<unsigned>(pin_release_band_t::ambiguous)
               && release_unavailable != static_cast<unsigned>(pin_release_band_t::long_release),
                  "Unavailable release must not alias a measured band");
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
  static_assert(sizeof(pmic_ops::cores3_vbus_off_power_on) / sizeof(ops::op_t) == 11
             && sizeof(pmic_ops::cores3_vbus_5v_power_on) / sizeof(ops::op_t) == 11,
                "CoreS3 power variants include UVP configuration before load enable");
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

    std::uint32_t observe_vbus(probe_ctx_t& ctx)
    {
      constexpr std::uint64_t spi_mask =
          (std::uint64_t(1) << wiring::cores3::display_miso)
        | (std::uint64_t(1) << wiring::cores3::display_sclk)
        | (std::uint64_t(1) << wiring::cores3::display_mosi);
      const auto pull = probe_pin_pulls(ctx, spi_mask);
      (void)pull;  // FORCE_VBUS still performs/restores the legacy-equivalent measurement.
      // Unlike the legacy input_pullup sequence, probe_pin_pulls restores the
      // captured pin modes immediately. Construction's SPI init overwrites
      // these pins next, so retaining the temporary pull-ups had no effect.
#if defined(M5GFX_AUTODETECT_TEST_CORES3_FORCE_VBUS)
      return vbus_5v;
#else
      return (pull.pullup_high & spi_mask) == 0 ? vbus_5v : 0;
#endif
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

    // Enabling BUS_OUT_EN into an empty BUS_OUT can pull the AXP2101 DCDC under its
    // threshold on a weak USB supply and power the board off. Precharge it with
    // pulses of growing on-time first: the same sequence as M5Unified's setExtOutput(),
    // including its 100 kHz bus, since the on-time is one register write plus i * 16 us.
    // Failing to enable BUS_OUT_EN does not fail detection: failing the refine would lose
    // the board (and the display) over the 5 V output, and M5Unified's setExtOutput()
    // enables it later with the same precharge.
    // After a failed on- or off-pulse, retry OFF and read back up to three times. Confirmed
    // OFF aborts a failed ON pulse or continues the ramp after a failed OFF pulse; confirmed
    // ON skips to final ON, and an unknown state fails. The final on write is verified:
    // a precharged BUS_OUT left with BUS_OUT_EN off latches to VBUS and blocks later enables.
    // Returns nullptr on success, else the failed step.
    const char* precharge_bus_out(int port)
    {
      constexpr std::uint8_t port0_reg = 0x02;
      constexpr std::uint8_t bus_en = 0x02;
      constexpr std::uint32_t freq = 100000;
      const auto port0 = lgfx::i2c::readRegister8(port, aw_addr, port0_reg, freq);
      if (!port0.has_value()) { return "read failed"; }
      if (port0.value() & bus_en) { return nullptr; }
      const std::uint8_t off_value = port0.value();
      enum class off_state_t { unknown, off, on };
      const auto confirm_off = [&]()
      {
        bool read_ok = false;
        for (int retry = 0; retry < 3; ++retry)
        {
          lgfx::i2c::writeRegister8(port, aw_addr, port0_reg, off_value, 0, freq);
          const auto read = lgfx::i2c::readRegister8(port, aw_addr, port0_reg, freq);
          read_ok = read.has_value();
          if (read_ok && !(read.value() & bus_en)) { return off_state_t::off; }
        }
        return read_ok ? off_state_t::on : off_state_t::unknown;
      };
      for (std::uint32_t i = 0; i < 8; ++i)
      {
        if (!lgfx::i2c::writeRegister8(port, aw_addr, port0_reg, off_value | bus_en, 0, freq).has_value())
        {
          const auto state = confirm_off();
          if (state == off_state_t::on) { break; }
          return state == off_state_t::off ? "precharge write failed" : "precharge read failed";
        }
        lgfx::delayMicroseconds(i * 16);
        if (!lgfx::i2c::writeRegister8(port, aw_addr, port0_reg, off_value, 0, freq).has_value())
        {
          const auto state = confirm_off();
          if (state == off_state_t::unknown) { return "precharge read failed"; }
          if (state == off_state_t::on) { break; }
        }
        lgfx::delayMicroseconds(1000);
      }
      for (int retry = 0; retry < 3; ++retry)
      {
        if (lgfx::i2c::writeRegister8(port, aw_addr, port0_reg, off_value | bus_en, 0, freq).has_value()
         && lgfx::i2c::readRegister8(port, aw_addr, port0_reg, freq).value_or(0) == (off_value | bus_en))
        {
          return nullptr;
        }
      }
      return "final write failed";
    }

    void enable_bus_out(const prepare_ctx_t& ctx)
    {
      const char* failed = "no I2C transaction";
      if (ctx.transaction != nullptr)
      {
        startup_detail::i2c_scope_t i2c(*ctx.transaction, ctx.i2c_port_probe, desc_cores3.internal_i2c);
        failed = i2c.opened ? precharge_bus_out(i2c.port) : "I2C open failed";
      }
      if (failed != nullptr)
      {
        ESP_LOGW("M5GFX", "[Autodetect] CoreS3 BUS_OUT_EN not enabled: %s", failed);
      }
    }

    bool fixed_start(board_result_t& result, const prepare_ctx_t& ctx)
    {
      probe_ctx_t probe;
      static_cast<prepare_ctx_t&>(probe) = ctx;
      result.option |= observe_vbus(probe);
      const auto& desc = *result.desc;
      {
        startup_detail::i2c_scope_t i2c(*ctx.transaction, ctx.i2c_port_probe, desc.internal_i2c);
        if (!i2c.opened || !startup_detail::prepare_power(desc, result, i2c.port, true))
        { return false; }
      }
      if (result.option & vbus_5v) { enable_bus_out(ctx); }
      // Identity is fixed: no capacitance, camera or IOE firmware observation.
      if (!refine_panel(result, ctx)) { return false; }
      return prepare(desc, result, ctx);
    }

    bool stackchan_base_gate(probe_ctx_t& probe)
    {
      // Sample one pin at a time: the base couples G6 and G7 electrically.
      constexpr std::uint64_t mask = (std::uint64_t(1) << 5)
                                   | (std::uint64_t(1) << 6) | (std::uint64_t(1) << 7);
      const auto pulls = probe_pin_pulls(probe, mask);
      const auto high = std::uint64_t(1) << 6;
      bool gate = (pulls.pulldown_high & mask) == high
               && (pulls.pullup_high & mask) == high;
#if defined(M5GFX_AUTODETECT_TEST_CORES3_STACKCHAN_GATE)
      gate = true;
#endif
      return gate;
    }

    bool stackchan_ack(const prepare_ctx_t& ctx, bool gate)
    {
#if defined(M5GFX_AUTODETECT_TEST_CORES3_FORCE_STACKCHAN)
      return true;
#else
      // Initialize once; the ACK loop does not repeat the base's pull measurements.
      // Software I2C START may recover held SDA on each ACK with up to nine
      // clocks and STOP.
      startup_detail::i2c_scope_t i2c(*ctx.transaction, ctx.i2c_port_probe, desc_cores3.internal_i2c);
      if (!i2c.opened) { return false; }
      const auto started = lgfx::millis();
      do
      {
#if defined(M5GFX_AUTODETECT_TEST_CORES3_STACKCHAN_NACK)
        const bool ack = false;
#else
        const bool began = lgfx::i2c::beginTransaction(
          i2c.port, specs::stackchan::i2c_stackchan_ioe::i2c_addr,
          specs::stackchan::i2c_stackchan_ioe::i2c_freq, false).has_value();
        const bool ended = lgfx::i2c::endTransaction(i2c.port).has_value();
        const bool ack = began && ended;
#endif
        // In-flight ACK timeout and START recovery may overrun the 50 ms window.
        if (ack || !gate || lgfx::millis() - started >= 50) { return ack; }
        lgfx::delay(1);
      } while (true);
#endif
    }

    bool refine(board_result_t& result, const prepare_ctx_t& ctx)
    {
      if (result.option & vbus_5v) { enable_bus_out(ctx); }
      const unsigned band = result.refine_state;
      result.refine_state = 0;
      const bool confirmed_before_power = result.option & internal_camera_confirmed;
      bool has_camera = confirmed_before_power;
      if (!confirmed_before_power)
      {
        // One post-power read is deliberately retained to recover waking cameras.
#if defined(M5GFX_AUTODETECT_TEST_CORES3_NO_CAMERA)
        has_camera = false;
#else
        probe_ctx_t probe;
        static_cast<prepare_ctx_t&>(probe) = ctx;
        has_camera = camera_id(probe);
#endif
      }
      probe_ctx_t probe;
      static_cast<prepare_ctx_t&>(probe) = ctx;
      const bool gate = stackchan_base_gate(probe);
      const bool ioe_ack = stackchan_ack(ctx, gate);
      std::uint8_t firmware = 0;
      bool firmware_read = false;
      if (ioe_ack)
      {
#if defined(M5GFX_AUTODETECT_TEST_CORES3_FORCE_STACKCHAN)
        firmware = specs::stackchan::i2c_stackchan_ioe::firmware_min;
        firmware_read = true;
#else
        firmware_read = read(ctx, specs::stackchan::i2c_stackchan_ioe::i2c_addr,
                             specs::stackchan::i2c_stackchan_ioe::firmware_reg,
                             &firmware, specs::stackchan::i2c_stackchan_ioe::i2c_freq);
#endif
      }
      const auto possible = [&](board_id_t id) -> const board_desc_t*
      {
        if (id == desc_cores3.def.id) { return &desc_cores3; }
        if (id == desc_cores3se.def.id && !has_camera
         && band != static_cast<unsigned>(pin_release_band_t::long_release)) { return &desc_cores3se; }
        if (id == desc_stackchan.def.id && ioe_ack) { return &desc_stackchan; }
        return nullptr;
      };
      const auto fallback = [&](const board_desc_t* representative, const char* why) -> bool
      {
        if (!select_provisional_member(ctx, &result, possible(ctx.preferred),
                                       possible(ctx.hint), representative, why)) { return false; }
        return refine_panel(result, ctx);
      };
      if (!has_camera)
      {
        if (band == static_cast<unsigned>(pin_release_band_t::short_release))
        { result.assign(&desc_cores3se); return refine_panel(result, ctx); }
        if (band == static_cast<unsigned>(pin_release_band_t::ambiguous))
        { return fallback(&desc_cores3, "CoreS3 release ambiguous and camera unanswered"); }
        if (band == release_unavailable)
        {
          // Dedicated GPIO timing can be unavailable from an unpinned task.
          // Retain legacy camera-absence selection so normal SE boots do not
          // regress to five retries and an uncached candidate; elimination remains.
          result.assign(&desc_cores3se);
          return refine_panel(result, ctx);
        }
        ESP_LOGW("M5GFX", "[Autodetect] CoreS3 capacitance indicated camera family, but camera ID was unavailable");
      }
      else if (band == static_cast<unsigned>(pin_release_band_t::short_release))
      {
        ESP_LOGW("M5GFX", "[Autodetect] CoreS3 capacitance indicated SE, but camera ID matched; using camera family");
      }
      if (ioe_ack && !firmware_read)
      { return fallback(&desc_stackchan, "StackChan ACK but firmware unreadable"); }
      if (gate && !ioe_ack)
      {
        // An M-Bus module can mimic the base gate, adding at most a 50 ms
        // new-probe window; an in-flight fixed-time I2C call may overrun it.
        ESP_LOGW("M5GFX", "[Autodetect] StackChan base gate without IOE ACK");
      }
      // Older base firmware remains CoreS3, matching the established contract.
      result.assign(firmware_read && firmware >= specs::stackchan::i2c_stackchan_ioe::firmware_min
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
      bool release_unavailable = false;
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
        release_unavailable = !release.available;
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
          ESP_LOGW("M5GFX", "[Autodetect] CoreS3 release result was ambiguous; awaiting member refinement");
        }
      }
      result->refine_state = release_unavailable ? cores3_detail::release_unavailable
                                                : static_cast<std::uint8_t>(family_band);
      const auto vbus_option = cores3_detail::observe_vbus(ctx);
      result->assign(family_band == pin_release_band_t::short_release ? &desc_cores3se
                                                                      : &desc_cores3);
      if (has_camera) { result->option |= cores3_detail::internal_camera_confirmed; }
      result->option |= vbus_option;
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
