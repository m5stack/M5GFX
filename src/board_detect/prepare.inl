  // Call after family-specific evidence gates; a hint is eligible only while
  // that member remains possible. Defaults may be absent for open families.
  // Only some chips' detectors use it.
  __attribute__((unused))
  static bool select_provisional_member(const prepare_ctx_t& ctx, board_result_t* result,
                                const board_desc_t* fallback_if_possible,
                                const board_desc_t* hinted_if_possible,
                                const board_desc_t* family_default, const char* why)
  {
    if (!ctx.final_attempt || result == nullptr) { return false; }
    const auto* chosen = fallback_if_possible ? fallback_if_possible
                       : hinted_if_possible ? hinted_if_possible : family_default;
    if (chosen == nullptr) { return false; }
    result->assign(chosen);
    result->provisional = true;
    ESP_LOGW("board_detect_m5", "%s; using %s for this boot", why, chosen->def.name);
    return true;
  }

  namespace prepare_detail
  {
    class retry_budget_t
    {
    public:
      explicit retry_budget_t(std::uint8_t milliseconds)
      : started_(io::millis()), milliseconds_(milliseconds) {}

      bool exhausted() const { return io::millis() - started_ >= milliseconds_; }
      std::uint32_t deadline_ms() const { return started_ + milliseconds_; }

    private:
      std::uint32_t started_;
      std::uint8_t milliseconds_;
    };

    void pin_level(int pin, bool high)
    {
      if (high) { io::pin_high(pin); }
      else      { io::pin_low(pin); }
      io::pin_mode(pin, io::pin_mode_t::output);
    }

    void hold_pin_level(int pin, bool high)
    {
      bool level = false;
      const bool output = io::pin_output_level(pin, &level);
      if (output && level == high) { return; }
      // Set the output latch before enabling the driver, including after M5Unified prehold.
      pin_level(pin, high);
    }

    bool read_register(int port, const power_desc_t& power,
                       std::uint8_t addr, std::uint8_t reg,
                       std::uint8_t* value, retry_budget_t& retry_budget)
    {
      for (;;)
      {
        if (io::i2c_read_reg8(port, addr, reg, value, power.i2c_freq)) { return true; }
        if (retry_budget.exhausted()) { return false; }
        io::delay_ms(1);
      }
    }

    bool read_register(int port, const power_desc_t& power,
                       std::uint8_t addr, std::uint8_t reg,
                       std::uint8_t* value)
    {
      retry_budget_t retry_budget(power.wake_poll_ms);
      return read_register(port, power, addr, reg, value, retry_budget);
    }

    ops::run_result_t run_sequence(int port, const power_desc_t& power,
                                   ops::op_list_t sequence,
                                   pin_list_t gpio_pins = no_pins(),
                                   const ops::retry_policy_t* policy = nullptr)
    {
      const int ports[] = { port };
      const ops::gpio_scope_t gpio_scope { io::pin_count, gpio_pins.data, gpio_pins.size };
      return ops::run_ops(ports, 1,
                          power.devices, power.device_count,
                          sequence.data, sequence.size, gpio_scope, policy,
                          ops::io_wait_ready_finished);
    }

    const pmic_variant_t* read_variant(const power_desc_t& power, int port,
                                       retry_budget_t& retry_budget)
    {
      if (power.variants == nullptr || power.variant_count == 0
       || power.variant_count > 8) { return nullptr; }
      std::uint8_t last_addr = 0xFF;
      std::uint8_t last_reg = 0xFF;
      std::uint8_t id_value = 0;
      bool id_valid = false;
      for (std::uint_fast8_t i = 0; i < power.variant_count; ++i)
      {
        if (power.variants[i].i2c_addr != last_addr || power.variants[i].id_reg != last_reg)
        {
          last_addr = power.variants[i].i2c_addr;
          last_reg = power.variants[i].id_reg;
          id_valid = read_register(port, power, last_addr, last_reg, &id_value, retry_budget);
        }
        const auto mask = power.variants[i].id_mask;
        if (id_valid && (id_value & mask) == (power.variants[i].id_value & mask))
        {
          return &power.variants[i];
        }
      }
      return nullptr;
    }

    const pmic_variant_t* read_variant(const power_desc_t& power, int port)
    {
      retry_budget_t retry_budget(power.wake_poll_ms);
      return read_variant(power, port, retry_budget);
    }

    void set_sd_spi_mode(const shared_sd_desc_t& sd, const prepare_ctx_t& ctx)
    {
      soft_spi_t bus(sd.sclk, sd.mosi, sd.miso, -1, 2);
      bus.init();
      bus.beginTransaction();
      pin_level(sd.sd_cs, true);

      for (int i = 0; i < 10; ++i) { bus.writeData(0xFF, 8); }
      io::pin_low(sd.sd_cs);
      static const std::uint8_t cmd58[] = { 0x7A, 0, 0, 0, 0, 0xFD, 0xFF, 0xFF };
      std::uint8_t response[sizeof(cmd58)];
      for (std::size_t i = 0; i < sizeof(cmd58); ++i)
      {
        response[i] = static_cast<std::uint8_t>(bus.transferData(cmd58[i], 8));
      }
      if (response[6] == response[7])
      {
        io::pin_high(sd.sd_cs);
        for (int i = 0; i < 10; ++i) { bus.writeData(0xFF, 8); }
        io::pin_low(sd.sd_cs);
        static const std::uint8_t cmd0[] = { 0x40, 0, 0, 0, 0, 0x95, 0xFF, 0xFF };
        for (auto value : cmd0) { bus.transferData(value, 8); }
      }
      io::pin_high(sd.sd_cs);
      bus.endTransaction();
      ctx.transaction->restore_to_snapshot({ sd.sclk, sd.mosi, sd.miso });
    }

    void pin_reset(const reset_desc_t& reset, bool active)
    {
      io::pin_high(reset.pin);
      io::pin_mode(reset.pin, io::pin_mode_t::output);
      io::delay_ms(1);
      if (!active) { return; }
      io::pin_low(reset.pin);
      io::delay_ms(reset.low_ms);
      io::pin_high(reset.pin);
      io::delay_ms(reset.post_ms);
    }

    class i2c_scope_t
    {
    public:
      i2c_scope_t(detection_transaction_t& transaction_, int port_, const i2c_desc_t& wiring)
      : transaction(transaction_), port(port_), pins { wiring.sda, wiring.scl }
      {
        opened = io::i2c_open(port, wiring.sda, wiring.scl);
      }

      ~i2c_scope_t()
      {
        if (opened) { io::i2c_close(port); }
        transaction.restore_to_snapshot(pins);
      }

      detection_transaction_t& transaction;
      int port;
      std::int8_t pins[2];
      bool opened = false;
    };

    // Unchecked primitives; callers must first pass description_valid().
    bool prepare_power(const board_desc_t& desc, board_result_t& result, int i2c_port,
                       bool retain_confirmed_board = false);
    bool prepare_sd_spi(const board_desc_t& desc, board_result_t& result,
                        const prepare_ctx_t& ctx);
  }

  bool prepare_detail::prepare_power(const board_desc_t& desc, board_result_t& result,
                                     int i2c_port, bool retain_confirmed_board)
  {
    if (result.prepared & prepared_power) { return true; }
    const auto& power = desc.power;
    if (power.hold_pin >= 0) { prepare_detail::hold_pin_level(power.hold_pin, power.hold_high); }
    if (power.variants == nullptr || power.variant_count == 0)
    {
      result.prepared |= prepared_power;
      return true;
    }

    // Wake-up polling and every PMIC transaction share one retry window so a
    // partially responsive controller cannot multiply the configured delay.
    prepare_detail::retry_budget_t retry_budget(power.wake_poll_ms);
    const pmic_variant_t* variant = nullptr;
    if (power.variant_confirmed)
    {
      // The detector has already identified the controller. Select
      // the branch only from recorded option bits; operation lists themselves
      // remain linear and contain no hardware-dependent conditionals.
      for (std::uint_fast8_t i = 0; i < power.variant_count; ++i)
      {
        const auto& candidate = power.variants[i];
        if ((result.option & candidate.option_select_mask) == candidate.option_select_value)
        {
          if (variant != nullptr) { return false; }
          variant = &candidate;
        }
      }
    }
    else
    {
      variant = prepare_detail::read_variant(power, i2c_port, retry_budget);
    }
    if (variant == nullptr) { return false; }
    std::uint8_t power_state = variant->power_state.mask;
    if (variant->power_state.mask
     && !prepare_detail::read_register(i2c_port, power, variant->i2c_addr,
                                       variant->power_state.reg, &power_state,
                                       retry_budget)) { return false; }
    std::uint8_t reset_state = power_state;
    if (variant->reset_state.mask && variant->reset_state.reg != variant->power_state.reg
     && !prepare_detail::read_register(i2c_port, power, variant->i2c_addr,
                                       variant->reset_state.reg, &reset_state,
                                       retry_budget)) { return false; }
    const bool power_was_off = !(power_state & variant->power_state.mask);
    const bool reset_was_low = !(reset_state & variant->reset_state.mask);
    const ops::retry_policy_t retry_policy { retry_budget.deadline_ms(), 1 };
    const auto power_result = prepare_detail::run_sequence(i2c_port, power,
                                                            variant->power_on,
                                                            desc.op_gpio_pins,
                                                            &retry_policy);
    if (power_result.status != ops::op_status_t::ok)
    {
      if (!retain_confirmed_board) { return false; }
      // This flag is set-only and has the same lifetime as prepared_power.
      result.prepared |= prepared_power_failed;
      // Detection has already identified the board. Power/IOE writes
      // are post-identification setup, so a partial hardware failure must not
      // turn the confirmed board into a different model; stop the list, warn,
      // and let construction report whatever hardware remains usable.
      ESP_LOGW("board_detect",
               "power_on stopped after board confirmation: op=%u status=%u native=%d",
               static_cast<unsigned>(power_result.failed_index),
               static_cast<unsigned>(power_result.status), 0);
    }
    if (power_was_off) { result.prepared &= ~prepared_sd_spi; }
    io::delay_ms(power_was_off || reset_was_low ? power.cold_wait_ms : power.warm_wait_ms);
    result.prepared |= prepared_power;
    return true;
  }

  bool prepare_detail::prepare_sd_spi(const board_desc_t& desc, board_result_t& result,
                                      const prepare_ctx_t& ctx)
  {
    if (result.prepared & prepared_sd_spi) { return true; }
    const auto& sd = desc.sd;
    if (sd.sd_cs < 0) { return true; }
    // Keep the other CS high; the detection transaction restores it on failure.
    prepare_detail::pin_level(sd.other_cs, true);
    prepare_detail::set_sd_spi_mode(sd, ctx);
    result.prepared |= prepared_sd_spi;
    return true;
  }

  namespace prepare_detail
  {
    template <typename T>
    bool list_valid(const list_desc_t<T>& list)
    {
      return list.data != nullptr || list.size == 0;
    }

    bool list_valid(ops::op_list_t list)
    {
      return list.data != nullptr || list.size == 0;
    }

    bool list_contains(pin_list_t list, int pin)
    {
      for (std::size_t i = 0; i < list.size; ++i)
      {
        if (list.data[i] == pin) { return true; }
      }
      return false;
    }

    bool gpio_valid(int pin)
    {
      return io::pin_valid(pin);
    }

    bool optional_gpio_valid(int pin) { return pin == -1 || gpio_valid(pin); }

    bool pin_list_valid(pin_list_t list)
    {
      if (!list_valid(list)) { return false; }
      for (std::size_t i = 0; i < list.size; ++i)
      {
        if (!gpio_valid(list.data[i])) { return false; }
      }
      return true;
    }

    bool variant_device_valid(const power_desc_t& power, const pmic_variant_t& variant)
    {
      for (std::size_t i = 0; i < power.device_count; ++i)
      {
        const auto& device = power.devices[i];
        if (device.bus_id == 0 && device.addr == variant.i2c_addr)
        {
          return device.freq_hz == power.i2c_freq;
        }
      }
      return false;
    }

    // Checked before any pin or register is touched, on every public entry
    // that acts on a description. Lists are expected to come from ops::list(),
    // pins() and options(); a hand-written size cannot be checked
    // against its array, but a missing array with a non-zero size is rejected here.
    bool description_valid(const board_desc_t& desc)
    {
      const bool has_variants = desc.power.variants != nullptr;
      bool valid = has_variants == (desc.power.variant_count != 0)
                && desc.power.variant_count <= 8
                && (!has_variants
                 || (desc.power.devices != nullptr && desc.power.device_count != 0))
                && !(has_variants && desc.power.hold_pin >= 0)
                && !(desc.reset.kind == reset_kind_t::i2c_regs && !has_variants)
                // A confirmed multi-variant family cannot safely choose an I2C
                // reset list without repeating the variant-identification read.
                && !(desc.power.variant_confirmed && desc.power.variant_count > 1
                  && desc.reset.kind == reset_kind_t::i2c_regs)
                && !((has_variants || desc.reset.kind == reset_kind_t::i2c_regs)
                     && (desc.internal_i2c.sda < 0 || desc.internal_i2c.scl < 0))
                && !(desc.reset.kind == reset_kind_t::custom && desc.reset.custom == nullptr)
                && pin_list_valid(desc.hold_high_pins)
                && list_valid(desc.option_names)
                && pin_list_valid(desc.op_gpio_pins);
      // no_display_pins(): every display pin is absent and nothing below may
      // refer to one. Any other description needs the SPI display signals and
      // holds its chip select high during detection.
      const bool display_absent = desc.display.sclk < 0 && desc.display.mosi < 0
                               && desc.display.miso < 0 && desc.display.dc < 0
                               && desc.display.cs < 0 && desc.display.rst < 0
                               && desc.display.busy < 0;
      valid = valid
           && (display_absent
            || (desc.display.sclk >= 0 && desc.display.mosi >= 0 && desc.display.cs >= 0
             && list_contains(desc.hold_high_pins, desc.display.cs)))
           && !(display_absent && desc.sd.sd_cs >= 0);
      const std::int8_t described_pins[] = {
        desc.power.hold_pin, desc.reset.pin,
        desc.sd.sclk, desc.sd.mosi, desc.sd.miso, desc.sd.sd_cs, desc.sd.other_cs,
        desc.display.sclk, desc.display.mosi, desc.display.miso, desc.display.dc,
        desc.display.cs, desc.display.rst, desc.display.busy,
        desc.internal_i2c.sda, desc.internal_i2c.scl,
      };
      for (auto pin : described_pins) { valid = valid && optional_gpio_valid(pin); }
      // Shared display/SD wires and exposed reset pins describe the same physical pins.
      // Both chip selects must be held high while probing their shared bus.
      if (valid && desc.sd.sd_cs >= 0)
      {
        valid = desc.sd.sclk >= 0 && desc.sd.mosi >= 0 && desc.sd.miso >= 0
             && desc.sd.other_cs >= 0
             && desc.sd.sclk == desc.display.sclk
             && desc.sd.mosi == desc.display.mosi
             && desc.sd.miso == desc.display.miso
             && desc.sd.other_cs == desc.display.cs
             && list_contains(desc.hold_high_pins, desc.sd.sd_cs);
      }
      if (valid && (desc.reset.kind == reset_kind_t::gpio
                 || desc.reset.kind == reset_kind_t::custom))
      {
        valid = desc.reset.pin >= 0 && desc.reset.pin == desc.display.rst;
      }
      for (std::uint_fast8_t i = 0; valid && i < desc.power.variant_count; ++i)
      {
        const auto& variant = desc.power.variants[i];
        const ops::gpio_scope_t gpio_scope {
          io::pin_count, desc.op_gpio_pins.data, desc.op_gpio_pins.size
        };
        valid = list_valid(variant.power_on)
             && list_valid(variant.reset_assert)
             && list_valid(variant.reset_release)
             && variant_device_valid(desc.power, variant)
             && (variant.option_select_value & ~variant.option_select_mask) == 0
             && (desc.reset.kind != reset_kind_t::i2c_regs
                 || (variant.reset_assert.size != 0 && variant.reset_release.size != 0))
             // GPIO operations are only safe on pins captured by the detection
             // transaction and passed as the execution scope.
             && ops::validate_ops(desc.power.devices, desc.power.device_count,
                                  variant.power_on.data, variant.power_on.size,
                                  gpio_scope).status == ops::op_status_t::ok
             && ops::validate_ops(desc.power.devices, desc.power.device_count,
                                  variant.reset_assert.data, variant.reset_assert.size,
                                  gpio_scope).status == ops::op_status_t::ok
             && ops::validate_ops(desc.power.devices, desc.power.device_count,
                                  variant.reset_release.data, variant.reset_release.size,
                                  gpio_scope).status == ops::op_status_t::ok;
      }
      if (!valid)
      {
        ESP_LOGW(tag, "invalid board description id=%u", static_cast<unsigned>(desc.def.id));
      }
      return valid;
    }

    void hold_chip_selects(const board_desc_t& desc)
    {
      for (std::size_t i = 0; i < desc.hold_high_pins.size; ++i)
      {
        pin_level(desc.hold_high_pins.data[i], true);
      }
    }
  }

  bool prepare_reset(const board_desc_t& desc, board_result_t& result,
                     const prepare_ctx_t& ctx, int i2c_port,
                     std::uint32_t* detected_option, bool retain_confirmed_board)
  {
    const auto& reset = desc.reset;
    if (!prepare_detail::description_valid(desc)) { return false; }
    if (result.prepared & prepared_reset) { return true; }
    if (!(reset.flags & reset_always) && !ctx.allow_reset)
    {
      if ((reset.flags & reset_hold_when_skipped)
       && !(result.prepared & prepared_reset_line))
      {
        if (reset.kind == reset_kind_t::i2c_regs)
        {
          // reset_hold_when_skipped releases an externally held reset and waits
          // for stabilization even when the assert pulse is disabled.
          const auto* variant = desc.power.variant_confirmed && desc.power.variant_count == 1
                              ? &desc.power.variants[0]
                              : prepare_detail::read_variant(desc.power, i2c_port);
          if (variant == nullptr) { return false; }
          prepare_detail::retry_budget_t release_budget(desc.power.wake_poll_ms);
          const ops::retry_policy_t release_policy { release_budget.deadline_ms(), 1 };
          const auto release_result = prepare_detail::run_sequence(
            i2c_port, desc.power, variant->reset_release, desc.op_gpio_pins,
            &release_policy);
          if (release_result.status != ops::op_status_t::ok)
          {
            if (!retain_confirmed_board) { return false; }
            // Keep the confirmed model after a reset-write failure: probing
            // another board would observe partially configured hardware.
            ESP_LOGW("board_detect",
                     "reset_release stopped after board confirmation: op=%u status=%u native=%d",
                     static_cast<unsigned>(release_result.failed_index),
                     static_cast<unsigned>(release_result.status),
                     0);
          }
          io::delay_ms(reset.post_ms);
        }
        else
        {
          prepare_detail::pin_reset(reset, false);
        }
        result.prepared |= prepared_reset_line;
      }
      return true;
    }

    bool ok = true;
    if (reset.kind == reset_kind_t::i2c_regs)
    {
      // Confirmed single-variant families already performed their full ID read;
      // do not turn a later setup read fault into loss of the detected model.
      const auto* variant = desc.power.variant_confirmed && desc.power.variant_count == 1
                          ? &desc.power.variants[0]
                          : prepare_detail::read_variant(desc.power, i2c_port);
      if (variant == nullptr) { return false; }
      // Assert and release each receive a full wake-poll budget.
      prepare_detail::retry_budget_t assert_budget(desc.power.wake_poll_ms);
      const ops::retry_policy_t assert_policy { assert_budget.deadline_ms(), 1 };
      const auto assert_result = prepare_detail::run_sequence(
        i2c_port, desc.power, variant->reset_assert, desc.op_gpio_pins,
        &assert_policy);
      ok = assert_result.status == ops::op_status_t::ok;
      if (!ok && retain_confirmed_board)
      {
        ESP_LOGW("board_detect",
                 "reset_assert stopped after board confirmation: op=%u status=%u native=%d",
                 static_cast<unsigned>(assert_result.failed_index),
                 static_cast<unsigned>(assert_result.status), 0);
      }
      if (ok || retain_confirmed_board)
      {
        io::delay_ms(reset.low_ms);
        prepare_detail::retry_budget_t release_budget(desc.power.wake_poll_ms);
        const ops::retry_policy_t release_policy { release_budget.deadline_ms(), 1 };
        const auto release_result = prepare_detail::run_sequence(
          i2c_port, desc.power, variant->reset_release, desc.op_gpio_pins,
          &release_policy);
        ok = release_result.status == ops::op_status_t::ok;
        if (!ok && retain_confirmed_board)
        {
          ESP_LOGW("board_detect",
                   "reset_release stopped after board confirmation: op=%u status=%u native=%d",
                   static_cast<unsigned>(release_result.failed_index),
                   static_cast<unsigned>(release_result.status), 0);
        }
        if (ok || retain_confirmed_board) { io::delay_ms(reset.post_ms); }
      }
      if (retain_confirmed_board) { ok = true; }
    }
    else if (reset.kind == reset_kind_t::custom)
    {
      ok = reset.custom(desc, ctx, detected_option);
    }
    else if (reset.kind == reset_kind_t::gpio)
    {
      prepare_detail::pin_reset(reset, true);
      result.prepared |= prepared_reset_line;
    }
    if (ok) { result.prepared |= prepared_reset; }
    return ok;
  }

  bool observe_spi_variant(probe_ctx_t& ctx, const board_desc_t& desc,
                           const spi_id_probe_t* probes, std::size_t probe_count,
                           std::uint32_t* option, bool three_wire,
                           std::uint8_t slow_retry_half_us, bool deselected_zero_preamble)
  {
    const int read_pin = three_wire || desc.display.miso < 0
                       ? desc.display.mosi : desc.display.miso;
    std::uint8_t last_cmd = 0;
    std::uint8_t last_dummy_bits = 0;
    std::uint32_t id = 0;
    bool have_id = false;
    for (std::size_t index = 0; index < probe_count; ++index)
    {
      const auto& probe = probes[index];
      if (!have_id || probe.cmd != last_cmd || probe.dummy_bits != last_dummy_bits)
      {
        id = soft_spi_read32(ctx, desc.display.sclk, desc.display.mosi, read_pin,
                             desc.display.dc, desc.display.cs, probe.cmd, probe.dummy_bits,
                             1, deselected_zero_preamble);
        last_cmd = probe.cmd;
        last_dummy_bits = probe.dummy_bits;
        have_id = true;
      }
      for (std::size_t value = 0; value < probe.value_count; ++value)
      {
        if ((id & probe.mask) != probe.values[value]) { continue; }
        *option = probe.option_bit;
        return true;
      }
    }
    if (slow_retry_half_us > 0)
    {
      // AtomS3R has shipped with GC9107 batches that answer only at a slow
      // clock. Its alternatives share one command, so re-read that command
      // exactly once and compare the result against every matching probe.
      const auto retry_cmd = probes[0].cmd;
      id = soft_spi_read32(ctx, desc.display.sclk, desc.display.mosi, read_pin,
                           desc.display.dc, desc.display.cs, retry_cmd,
                           probes[0].dummy_bits,
                           slow_retry_half_us, deselected_zero_preamble);
      for (std::size_t index = 0; index < probe_count; ++index)
      {
        const auto& probe = probes[index];
        if (probe.cmd != retry_cmd) { continue; }
        for (std::size_t value = 0; value < probe.value_count; ++value)
        {
          if ((id & probe.mask) != probe.values[value]) { continue; }
          *option = probe.option_bit;
          return true;
        }
      }
    }
    return false;
  }

  bool probe_spi_id(probe_ctx_t& ctx, const board_desc_t& desc,
                    const spi_id_probe_t* probes, std::size_t probe_count,
                    board_result_t* result, bool three_wire,
                    std::uint8_t slow_retry_half_us,
                    bool deselected_zero_preamble)
  {
    if (probes == nullptr || probe_count == 0 || result == nullptr
     || !prepare_detail::description_valid(desc)
     || desc.display.cs < 0) { return false; }  // no_display_pins() has no SPI ID
    const std::int8_t pins[] = {
      desc.display.cs, desc.display.sclk, desc.display.mosi,
      desc.display.dc, desc.display.rst,
    };
    const std::int8_t shared_sd_pins[] = {
      desc.display.cs, desc.display.sclk, desc.display.mosi,
      desc.display.dc, desc.display.rst,
      desc.sd.sclk, desc.sd.mosi, desc.sd.miso, desc.sd.sd_cs,
    };
    const auto restore_probe_pins = [&]()
    {
      if (desc.sd.sd_cs >= 0) { ctx.transaction->restore_to_snapshot(shared_sd_pins); }
      else                    { ctx.transaction->restore_to_snapshot(pins); }
    };
    board_result_t candidate;
    candidate.assign(&desc);
    const prepare_ctx_t& prepare_ctx = ctx;
    if (desc.sd.sd_cs >= 0)
    {
      // Keep both devices deselected while the shared SD bus is switched to
      // SPI mode. Descriptions without shared SD take the original path.
      prepare_detail::pin_level(desc.sd.sd_cs, true);
      prepare_detail::pin_level(desc.display.cs, true);
      if (!prepare_detail::prepare_sd_spi(desc, candidate, prepare_ctx))
      {
        restore_probe_pins();
        return false;
      }
    }
    if (!prepare_reset(desc, candidate, prepare_ctx, ctx.probe_port))
    {
      restore_probe_pins();
      return false;
    }
    if (observe_spi_variant(ctx, desc, probes, probe_count, &candidate.option,
                            three_wire, slow_retry_half_us, deselected_zero_preamble))
    {
      *result = candidate;
      return true;
    }
    restore_probe_pins();
    return false;
  }

  struct spi_id_member_t
  {
    const board_desc_t* desc;
    const spi_id_probe_t* probes;
    std::uint8_t probe_count;
    bool three_wire;
    bool touches_conditional_pins;
    std::uint8_t slow_retry_half_us;
    bool deselected_zero_preamble;
  };

  bool fixed_prepare_spi_variant(board_result_t& result, const prepare_ctx_t& ctx,
                               const spi_id_member_t& member)
  {
    if (!prepare(*result.desc, result, ctx)) { return false; }
    probe_ctx_t probe;
    static_cast<prepare_ctx_t&>(probe) = ctx;
    if (!observe_spi_variant(probe, *result.desc, member.probes, member.probe_count,
                             &result.option, member.three_wire,
                             member.slow_retry_half_us, member.deselected_zero_preamble))
    {
      ESP_LOGW("M5GFX", "Fixed board:%u panel variant unreadable; using default",
               static_cast<unsigned>(result.def->id));
    }
    const auto& display = result.desc->display;
    const std::int8_t signals[] = { display.dc, display.sclk, display.mosi, display.miso };
    ctx.transaction->restore_to_snapshot(signals);
    return true;
  }

  class spi_id_detector_t final : public board_detector_t
  {
  public:
    spi_id_detector_t(const board_def_t* const* board_members, const spi_id_member_t* members_desc,
                      std::uint8_t member_count, bool shared_id_read = false,
                      bool (*signature_probe)(probe_ctx_t&) = nullptr)
    : board_detector_t(board_members),
      members_desc_(members_desc), member_count_(member_count),
      shared_id_read_(shared_id_read), signature_probe_(signature_probe) {}
    bool signature(probe_ctx_t& ctx) const override
    { return signature_probe_ == nullptr || signature_probe_(ctx); }
    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (shared_id_read_) { return probe_family(ctx, result); }
      if (ctx.attempt == 0 && !ctx.final_attempt && ctx.hint != board_id_unknown)
      {
        for (std::uint8_t index = 0; index < member_count_; ++index)
        {
          const auto& member = members_desc_[index];
          if (member.desc->def.id != ctx.hint) { continue; }
          if (probe_member(ctx, member, result)) { return true; }
          return false;
        }
      }
      for (std::uint8_t index = 0; index < member_count_; ++index)
      {
        const auto& member = members_desc_[index];
        if (probe_member(ctx, member, result)) { return true; }
      }
      return false;
    }

  private:
    bool probe_family(probe_ctx_t& ctx, board_result_t* result) const
    {
      if (member_count_ == 0 || result == nullptr) { return false; }
      const auto& first = members_desc_[0];
      if (first.desc == nullptr || first.probes == nullptr || first.probe_count == 0
       || !prepare_detail::description_valid(*first.desc)
       || first.desc->display.cs < 0) { return false; }

      const auto& desc = *first.desc;
      const std::int8_t pins[] = {
        desc.display.cs, desc.display.sclk, desc.display.mosi,
        desc.display.dc, desc.display.rst,
      };
      board_result_t candidate;
      candidate.assign(&desc);
      const prepare_ctx_t& prepare_ctx = ctx;
      if (!prepare_reset(desc, candidate, prepare_ctx, ctx.probe_port))
      {
        ctx.transaction->restore_to_snapshot(pins);
        return false;
      }

      const auto& read_probe = first.probes[0];
      const int read_pin = first.three_wire || desc.display.miso < 0
                         ? desc.display.mosi : desc.display.miso;
      const std::uint32_t panel_id = soft_spi_read32(
        ctx, desc.display.sclk, desc.display.mosi, read_pin,
        desc.display.dc, desc.display.cs, read_probe.cmd, read_probe.dummy_bits,
        1, first.deselected_zero_preamble);

      // Members of this family share one physical read. Compare the
      // captured value in member order instead of resetting the same
      // panel again for each possible member.
      for (std::uint8_t member_index = 0; member_index < member_count_; ++member_index)
      {
        const auto& member = members_desc_[member_index];
        for (std::uint8_t probe_index = 0; probe_index < member.probe_count; ++probe_index)
        {
          const auto& probe = member.probes[probe_index];
          if (probe.cmd != read_probe.cmd || probe.dummy_bits != read_probe.dummy_bits)
          {
            ctx.transaction->restore_to_snapshot(pins);
            return false;
          }
          for (std::size_t value = 0; value < probe.value_count; ++value)
          {
            if ((panel_id & probe.mask) != probe.values[value]) { continue; }
            candidate.assign(member.desc);
            candidate.option = probe.option_bit;
            *result = candidate;
            return true;
          }
        }
      }
      ctx.transaction->restore_to_snapshot(pins);
      return false;
    }

    static bool probe_member(probe_ctx_t& ctx, const spi_id_member_t& member,
                             board_result_t* result)
    {
      if (ctx.conditional_pins_unavailable && member.touches_conditional_pins)
      {
        // OPI PSRAM owns GPIO33..37. Skip only the candidate that touches
        // those pins; another member of the same family may remain safe.
        return false;
      }
      return probe_spi_id(ctx, *member.desc, member.probes, member.probe_count, result,
                          member.three_wire, member.slow_retry_half_us,
                          member.deselected_zero_preamble);
    }

    const spi_id_member_t* members_desc_;
    std::uint8_t member_count_;
    bool shared_id_read_;
    bool (*signature_probe_)(probe_ctx_t&);
  };

  bool prepare(const board_desc_t& desc, board_result_t& result, const prepare_ctx_t& ctx)
  {
    if (ctx.transaction == nullptr)
    {
      ESP_LOGE("board_detect", "prepare requires a detection transaction");
      return false;
    }
    if (result.desc != &desc || result.def != &desc.def || result.def->id == board_id_unknown)
    {
      return false;
    }
    if (!prepare_detail::description_valid(desc)) { return false; }
    const board_desc_t* current = &desc;
    if (!(result.prepared & prepared_power))
    {
      if (desc.power.variants != nullptr)
      {
        prepare_detail::i2c_scope_t i2c(*ctx.transaction, ctx.probe_port, desc.internal_i2c);
        if (!i2c.opened
         || !prepare_detail::prepare_power(desc, result, i2c.port, true)) { return false; }
      }
      else if (!prepare_detail::prepare_power(desc, result, ctx.probe_port, true)) { return false; }
    }
    if (!(result.prepared & prepared_refine) && result.refine != nullptr)
    {
      if (result.prepared & prepared_power_failed)
      {
        // Pending refinement means the member is not yet confirmed. Give a
        // failed power sequence another attempt before retaining it provisionally.
        if (!ctx.final_attempt) { return false; }
        result.provisional = true;
        ESP_LOGW("board_detect",
                 "member refinement skipped after retained power_on failure; board=%u",
                 static_cast<unsigned>(result.def->id));
      }
      else
      {
        if (!result.refine(result, ctx)) { return false; }
        if (result.desc == nullptr || result.def != &result.desc->def
         || result.def->id == board_id_unknown
         || !prepare_detail::description_valid(*result.desc)) { return false; }
        current = result.desc;
      }
      result.prepared |= prepared_refine;
    }
    else if (result.desc != nullptr)
    {
      current = result.desc;
    }
    if (!(result.prepared & prepared_observation) && result.observe_after_power != nullptr)
    {
      // Optional metadata may skip retained power failure. Observers whose
      // controllers can still answer may opt in without changing board identity.
      if ((!(result.prepared & prepared_power_failed) || result.observe_on_power_failure)
       && !result.observe_after_power(result, ctx))
      {
        // Optional panel metadata must not discard a member confirmed by touch.
        ESP_LOGW("board_detect", "post-power observation failed; retaining board=%u",
                 static_cast<unsigned>(result.def->id));
      }
      result.prepared |= prepared_observation;
    }
    if (!prepare_detail::prepare_sd_spi(*current, result, ctx)) { return false; }
    const bool reset_was_prepared = result.prepared & prepared_reset;
    if (!reset_was_prepared)
    {
      if (current->reset.kind == reset_kind_t::i2c_regs)
      {
        prepare_detail::i2c_scope_t i2c(*ctx.transaction, ctx.probe_port, current->internal_i2c);
        if (!i2c.opened || !prepare_reset(*current, result, ctx, i2c.port, nullptr, true))
        {
          return false;
        }
      }
      else if (!prepare_reset(*current, result, ctx, ctx.probe_port, nullptr, true))
      {
        return false;
      }
    }
    prepare_detail::hold_chip_selects(*current);
    // Detection uses software I2C only. Like SPI, hardware I2C belongs to
    // its consumers; preparation leaves only power and reset ready.
    return true;
  }
