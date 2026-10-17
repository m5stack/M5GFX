  bool wait_i2c_scl_high(int pin_scl)
  {
    lgfx::gpio_hi(pin_scl);
    lgfx::delayMicroseconds(5);
    if (lgfx::gpio_in(pin_scl)) { return true; }
    const auto started = lgfx::micros();
    while (lgfx::micros() - started < 25000)
    {
      if (lgfx::gpio_in(pin_scl)) { return true; }
      lgfx::delayMicroseconds(1);
    }
    return false;
  }

  bool release_held_sda(int pin_sda, int pin_scl)
  {
    lgfx::pinMode(pin_sda, lgfx::pin_mode_t::input);
    lgfx::pinMode(pin_scl, lgfx::pin_mode_t::input);
    lgfx::delayMicroseconds(10);

    // A reset of this MCU during a transfer can leave a peripheral holding
    // SDA low while it waits for more clocks. Clock it out (up to 9 bits) and
    // finish with STOP. Input mode is open drain with the output latch high.
    bool bus_released = lgfx::gpio_in(pin_scl);
    if (bus_released && !lgfx::gpio_in(pin_sda))
    {
      for (int i = 0; i < 9 && !lgfx::gpio_in(pin_sda); ++i)
      {
        lgfx::gpio_lo(pin_scl);
        lgfx::delayMicroseconds(5);
        if (!wait_i2c_scl_high(pin_scl)) { bus_released = false; break; }
      }
      if (bus_released)
      {
        lgfx::gpio_lo(pin_scl);
        lgfx::gpio_lo(pin_sda);
        lgfx::delayMicroseconds(5);
        bus_released = wait_i2c_scl_high(pin_scl);
        if (bus_released)
        {
          lgfx::gpio_hi(pin_sda);
          lgfx::delayMicroseconds(10);
        }
      }
    }
    const bool released = bus_released && lgfx::gpio_in(pin_sda) && lgfx::gpio_in(pin_scl);
    return released;
  }

  pin_pull_result_t recover_held_sda_and_resample(
    probe_ctx_t& ctx, pin_pull_result_t pulls, std::uint64_t pin_mask,
    int pin_sda, int pin_scl)
  {
    const auto high = pulls.pulldown_high;
    if ((high & (std::uint64_t(1) << pin_scl))
     && !(high & (std::uint64_t(1) << pin_sda)))
    {
      release_held_sda(pin_sda, pin_scl);
      pulls = probe_pin_pulls(ctx, pin_mask);
      ctx.transaction->restore_start({ pin_sda, pin_scl });
    }
    return pulls;
  }

  bool probe_i2c_bus_present(probe_ctx_t& ctx, int pin_sda, int pin_scl)
  {
    const std::uint64_t sda_bit = std::uint64_t(1) << pin_sda;
    const std::uint64_t scl_bit = std::uint64_t(1) << pin_scl;
    const std::uint64_t mask = sda_bit | scl_bit;
    auto sample = [&]() -> i2c_bus_probe_detail::line_state_t
    {
      const auto pulls = probe_pin_pulls(ctx, mask);
      return { bool(pulls.pulldown_high & sda_bit),
               bool(pulls.pulldown_high & scl_bit),
               !(pulls.pullup_high & scl_bit) };
    };
    auto wait_scl = [&]()
    {
      // A floating pin is raised by the internal pull-up during sample() and
      // never reaches this path. Only an externally held-low SCL receives the
      // legacy soft-I2C clock-stretch allowance.
      lgfx::pinMode(pin_sda, lgfx::pin_mode_t::input);
      lgfx::pinMode(pin_scl, lgfx::pin_mode_t::input);
      wait_i2c_scl_high(pin_scl);
      ctx.transaction->restore_start({ pin_sda, pin_scl });
    };
    auto recover = [&]()
    {
      // A reset during a transaction can leave a slave holding SDA forever.
      // The legacy soft-I2C startup recovered it before attempting detection.
      ESP_LOGD("M5GFX", "[Autodetect] recovering held SDA on I2C pins SDA=%d SCL=%d",
               pin_sda, pin_scl);
      release_held_sda(pin_sda, pin_scl);
      ctx.transaction->restore_start({ pin_sda, pin_scl });
    };
    return i2c_bus_probe_detail::probe_i2c_bus_present(sample, wait_scl, recover);
  }

  bool probe_pin_pullup_low(probe_ctx_t& ctx, int pin)
  {
    lgfx::pinMode(pin, lgfx::pin_mode_t::input_pullup);
    lgfx::delayMicroseconds(10);
    const bool held_low = !lgfx::gpio_in(pin);
    ctx.transaction->restore_start({ pin });
    return held_low;
  }

  bool probe_i2c_ack(probe_ctx_t& ctx, int pin_sda, int pin_scl, std::uint8_t addr)
  {
    // Reserved addresses are never touched, even if requested accidentally.
    if (addr < 0x08 || addr > 0x77) { return false; }
    auto& cache = ctx.i2c_cache;
    if (cache.pin_sda != pin_sda || cache.pin_scl != pin_scl)
    {
      cache = {};
      cache.pin_sda = pin_sda;
      cache.pin_scl = pin_scl;

      lgfx::pinMode(pin_sda, lgfx::pin_mode_t::input_pulldown);
      lgfx::pinMode(pin_scl, lgfx::pin_mode_t::input_pulldown);
      lgfx::delayMicroseconds(10);
      lgfx::pinMode(pin_sda, lgfx::pin_mode_t::input);
      lgfx::pinMode(pin_scl, lgfx::pin_mode_t::input);
      lgfx::delayMicroseconds(10);
      // The first check does not wait: a pin pair without pull-ups reads SCL
      // low here, and waiting for it would delay every such detection. Only a
      // bus that has pull-ups and a held SDA is clocked, and only those clocks
      // wait for a stretching peripheral.
      bool bus_released = lgfx::gpio_in(pin_scl);
      if (bus_released && !lgfx::gpio_in(pin_sda))
      {
        bus_released = release_held_sda(pin_sda, pin_scl);
      }
      cache.pullup_ok = bus_released && lgfx::gpio_in(pin_sda) && lgfx::gpio_in(pin_scl);
      ctx.transaction->restore_start({ pin_sda, pin_scl });
    }

    const std::uint32_t bit = 1u << (addr & 31);
    if (!(cache.checked[addr >> 5] & bit))
    {
      cache.checked[addr >> 5] |= bit;
      if (cache.pullup_ok)
      {
        if (lgfx::i2c::init(ctx.i2c_port_probe, pin_sda, pin_scl).has_value())
        {
          const bool hit = lgfx::i2c::beginTransaction(ctx.i2c_port_probe, addr, 100000, false).has_value()
                        && lgfx::i2c::endTransaction(ctx.i2c_port_probe).has_value();
          if (hit) { cache.ack[addr >> 5] |= bit; }
          lgfx::i2c::release(ctx.i2c_port_probe);
        }
        ctx.transaction->restore_start({ pin_sda, pin_scl });
      }
    }
    return cache.pullup_ok && (cache.ack[addr >> 5] & bit);
  }

  bool probe_i2c_read(probe_ctx_t& ctx, int pin_sda, int pin_scl, std::uint8_t addr,
                      std::uint16_t reg, std::uint8_t* data, std::size_t length,
                      std::uint32_t freq, std::uint32_t poll_ms, bool reg16)
  {
    if (data == nullptr || length == 0 || addr < 0x08 || addr > 0x77) { return false; }
    if (!lgfx::i2c::init(ctx.i2c_port_probe, pin_sda, pin_scl).has_value()) { return false; }
    const std::uint8_t reg_bytes[] = {
      static_cast<std::uint8_t>(reg >> 8), static_cast<std::uint8_t>(reg),
    };
    const auto started = lgfx::millis();
    bool success = false;
    do
    {
      success = lgfx::i2c::transactionWriteRead(
        ctx.i2c_port_probe, addr, reg_bytes + !reg16, 1 + reg16,
        data, length, freq).has_value();
      if (success) { break; }
      lgfx::delay(1);
    } while (lgfx::millis() - started < poll_ms);
    lgfx::i2c::release(ctx.i2c_port_probe);
    ctx.transaction->restore_start({ pin_sda, pin_scl });
    return success;
  }

  pin_pull_result_t probe_pin_pulls(probe_ctx_t& ctx, std::uint64_t pin_mask,
                                    std::uint32_t release_us)
  {
    static constexpr std::size_t max_pins = 64;
    pin_pull_result_t result;
    result.release_sampled = release_us != 0;
    for (std::size_t pin = 0; pin < max_pins; ++pin)
    {
      const std::uint64_t bit = std::uint64_t(1) << pin;
      if (!(pin_mask & bit)) { continue; }
      // Measure one pin completely before touching the next. On a native-mode
      // SD bus this avoids raising CLK while CMD is temporarily pulled low.
      lgfx::pinMode(pin, lgfx::pin_mode_t::input_pulldown);
      lgfx::delayMicroseconds(10);
      if (lgfx::gpio_in(pin)) { result.pulldown_high |= bit; }
      if (release_us != 0)
      {
        lgfx::pinMode(pin, lgfx::pin_mode_t::input);
        lgfx::delayMicroseconds(release_us);
        if (lgfx::gpio_in(pin)) { result.pulldown_release_high |= bit; }
      }
      lgfx::pinMode(pin, lgfx::pin_mode_t::input_pullup);
      lgfx::delayMicroseconds(10);
      if (lgfx::gpio_in(pin)) { result.pullup_high |= bit; }
      if (release_us != 0)
      {
        lgfx::pinMode(pin, lgfx::pin_mode_t::input);
        lgfx::delayMicroseconds(release_us);
        if (lgfx::gpio_in(pin)) { result.pullup_release_high |= bit; }
      }
      ctx.transaction->restore_start(static_cast<std::int8_t>(pin));
    }
    return result;
  }

  dedicated_release_result_t probe_dedicated_pin_release(
    probe_ctx_t& ctx, const std::int8_t* pins, std::uint8_t pin_count,
    std::uint16_t reads, std::uint8_t samples, std::uint32_t settle_us)
  {
    dedicated_release_result_t result;
    result.pin_count = pin_count;
    result.requested_samples = samples;
    result.reads = reads;
    for (auto& value : result.median_first_high) { value = dedicated_release_no_high; }
#if defined (M5GFX_HAS_S3_DEDICATED_GPIO_RELEASE_PROBE)
    if (ctx.transaction == nullptr || pins == nullptr || pin_count == 0
     || pin_count > max_dedicated_release_pins || reads == 0
     || reads > max_dedicated_release_reads || samples == 0
     || samples > max_dedicated_release_samples || settle_us == 0)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe has invalid parameters");
      return result;
    }
    const int pinned_core = dedicated_release_task_core();
    if (pinned_core == static_cast<int>(tskNO_AFFINITY)
     || dedicated_release_core_id() != pinned_core)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe requires a core-pinned task");
      return result;
    }
    // The S3 dedicated-GPIO clock/reset bit is shared by both CPU cores.
    // IDF 4.4-5.3 reaches it through periph_module's reference count, while
    // IDF 5.5+ manipulates it directly, making this preflight check necessary
    // to avoid resetting or stopping another live bundle. The check and
    // new_bundle are not atomic: applications that initialize dedicated GPIO
    // concurrently on another core during this brief M5.begin probe are not
    // supported.
    if (REG_GET_BIT(SYSTEM_CPU_PERI_CLK_EN_REG, SYSTEM_CLK_EN_DEDICATED_GPIO))
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO in use; skipping release probe");
      return result;
    }
    int bundle_pins[max_dedicated_release_pins];
    for (std::uint8_t index = 0; index < pin_count; ++index)
    {
      const int pin = pins[index];
      if (pin < 0 || pin >= GPIO_NUM_MAX || !GPIO_IS_VALID_GPIO(pin))
      {
        ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe has invalid pin (%d)", pin);
        return result;
      }
      bundle_pins[index] = pin;
    }
    for (std::uint8_t index = 0; index < pin_count; ++index)
    {
      const int pin = bundle_pins[index];
      // input_pullup retains output-enable but sets OD with a High latch, so
      // the pad is Hi-Z. Prepare the weakest drive before new_bundle connects
      // its active output route to avoid a transient push-pull/strong drive.
      lgfx::pinMode(pin, lgfx::pin_mode_t::input_pullup);
      const auto io_mux = GPIO_PIN_MUX_REG[pin];
      REG_WRITE(io_mux, (REG_READ(io_mux) & ~(FUN_PU | FUN_PD | FUN_DRV_M)) | FUN_PU);
      gpio_ll_od_enable(&GPIO, static_cast<gpio_num_t>(pin));
    }

    dedic_gpio_bundle_handle_t bundle = nullptr;
    dedic_gpio_bundle_config_t config = {};
    config.gpio_array = bundle_pins;
    config.array_size = pin_count;
    config.flags.in_en = 1;
    config.flags.out_en = 1;
    const int creation_core = dedicated_release_core_id();
    if (creation_core != pinned_core)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe task migrated before bundle creation");
      ctx.transaction->restore_start(pins, pin_count);
      return result;
    }
    const esp_err_t bundle_error = dedic_gpio_new_bundle(&config, &bundle);
    if (bundle_error != ESP_OK)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO bundle creation failed (%d)", int(bundle_error));
      ctx.transaction->restore_start(pins, pin_count);
      return result;
    }
    if (dedicated_release_core_id() != pinned_core)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe task migrated after bundle creation");
      delete_dedicated_release_bundle(bundle);
      ctx.transaction->restore_start(pins, pin_count);
      return result;
    }
    std::uint32_t input_mask = 0;
    std::uint32_t output_mask = 0;
    esp_err_t input_mask_error = ESP_OK;
    esp_err_t output_mask_error = ESP_OK;
    dedicated_release_channel_masks(bundle, pin_count, &input_mask, &output_mask,
                                    &input_mask_error, &output_mask_error);
    if (input_mask_error != ESP_OK || output_mask_error != ESP_OK)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO channel lookup failed (in=%d out=%d)",
               int(input_mask_error), int(output_mask_error));
      delete_dedicated_release_bundle(bundle);
      ctx.transaction->restore_start(pins, pin_count);
      return result;
    }
    const std::uint32_t channel_mask = (1u << pin_count) - 1u;
    const std::uint32_t input_first_bit = input_mask & (~input_mask + 1u);
    if (input_first_bit == 0 || input_mask != input_first_bit * channel_mask
     || output_mask == 0)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO channel allocation is not contiguous");
      delete_dedicated_release_bundle(bundle);
      ctx.transaction->restore_start(pins, pin_count);
      return result;
    }
    static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    const auto measure = [&](std::uint16_t* first_high, std::uint32_t* cycles)
    {
      volatile std::uint32_t raw[max_dedicated_release_reads];
      taskENTER_CRITICAL(&mux);
      // A dedicated bundle belongs to its creation core. Check immediately
      // after scheduling is stopped and reject the measurement if it migrated.
      if (dedicated_release_core_id() != creation_core)
      {
        taskEXIT_CRITICAL(&mux);
        return false;
      }
      dedicated_release_write_mask(output_mask, 0);
      lgfx::delayMicroseconds(settle_us);
      const std::uint32_t started = dedicated_release_cycle_count();
      dedicated_release_write_mask(output_mask, output_mask);
      for (std::uint16_t read = 0; read < reads; ++read)
      { raw[read] = dedicated_release_read_in(); }
      const std::uint32_t stopped = dedicated_release_cycle_count();
      taskEXIT_CRITICAL(&mux);
      *cycles = stopped - started;
      for (std::uint8_t index = 0; index < pin_count; ++index)
      {
        first_high[index] = dedicated_release_no_high;
        const std::uint32_t bit = input_first_bit << index;
        for (std::uint16_t read = 0; read < reads; ++read)
        {
          if (raw[read] & bit) { first_high[index] = read; break; }
        }
      }
      return true;
    };

    bool complete = true;
    std::uint16_t discarded[max_dedicated_release_pins];
    std::uint32_t cycles = 0;
    complete = measure(discarded, &cycles);  // Discard the first, cache-warming pass.
    std::uint16_t measured[max_dedicated_release_pins][max_dedicated_release_samples];
    for (std::uint8_t sample = 0; complete && sample < samples; ++sample)
    {
      std::uint16_t current[max_dedicated_release_pins];
      complete = measure(current, &cycles);
      if (!complete) { break; }
      result.total_cycles += cycles;
      for (std::uint8_t index = 0; index < pin_count; ++index)
      { measured[index][sample] = current[index]; }
    }
    if (complete)
    {
      result.cpu_hz = static_cast<std::uint32_t>(esp_clk_cpu_freq());
      result.available = result.cpu_hz != 0 && result.total_cycles != 0;
      if (result.cpu_hz == 0)
      {
        ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe CPU frequency is unavailable");
      }
      else if (result.total_cycles == 0)
      {
        ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe cycle counter did not advance");
      }
      std::uint8_t missing_transitions = 0;
      for (std::uint8_t index = 0; index < pin_count; ++index)
      {
        result.median_first_high[index] = median_dedicated_release_samples(
          measured[index], samples, reads, &result.valid_samples[index]);
        missing_transitions += result.valid_samples[index] != samples;
      }
      if (missing_transitions != 0)
      {
        ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO release transition missing on %u/%u pins",
                 unsigned(missing_transitions), unsigned(pin_count));
      }
    }
    else
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe task migrated during capture");
    }

    if (!delete_dedicated_release_bundle(bundle)) { result.available = false; }
    // del_bundle pin cleanup differs by IDF version (4.4/5.2/5.3 leave the
    // pads alone; newer versions disable output), and out_sel is not restored.
    // Dedicated input routing is peripheral-side. In every version,
    // The transaction's restore_start (pin_backup_t::restore) restores out_sel,
    // GPIO_PINn, output-enable, latch, and the complete IO_MUX register
    // (including temporary FUN_DRV).
    ctx.transaction->restore_start(pins, pin_count);
    return result;
#else
    ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO release probe is unavailable in this SDK");
    (void)ctx;
    (void)pins;
    (void)settle_us;
    return result;
#endif
  }

  void soft_spi_t::init()
  {
    lgfx::gpio_lo(pin_sclk_);
    lgfx::pinMode(pin_sclk_, lgfx::pin_mode_t::output);
    lgfx::gpio_lo(pin_mosi_);
    lgfx::pinMode(pin_mosi_, lgfx::pin_mode_t::output);
    if (pin_miso_ != pin_mosi_) { lgfx::pinMode(pin_miso_, lgfx::pin_mode_t::input); }
    if (pin_dc_ >= 0) { lgfx::pinMode(pin_dc_, lgfx::pin_mode_t::output); }
  }

  void soft_spi_t::beginTransaction()
  {
    lgfx::gpio_lo(pin_sclk_);
  }

  void soft_spi_t::endTransaction()
  {
    endRead();
    lgfx::gpio_lo(pin_sclk_);
  }

  void soft_spi_t::clock()
  {
    lgfx::delayMicroseconds(half_us_);
    lgfx::gpio_hi(pin_sclk_);
    lgfx::delayMicroseconds(half_us_);
    lgfx::gpio_lo(pin_sclk_);
    lgfx::delayMicroseconds(half_us_);
  }

  void soft_spi_t::send(std::uint32_t data, std::uint_fast8_t bits)
  {
    for (std::uint_fast8_t i = 0; i < bits; ++i)
    {
      if (data & (std::uint32_t(1) << bit_index(i))) { lgfx::gpio_hi(pin_mosi_); }
      else                                           { lgfx::gpio_lo(pin_mosi_); }
      clock();
    }
  }

  void soft_spi_t::writeCommand(std::uint32_t data, std::uint_fast8_t bits)
  {
    if (pin_dc_ >= 0) { lgfx::gpio_lo(pin_dc_); }
    send(data, bits);
  }

  void soft_spi_t::writeData(std::uint32_t data, std::uint_fast8_t bits)
  {
    if (pin_dc_ >= 0) { lgfx::gpio_hi(pin_dc_); }
    send(data, bits);
  }

  std::uint32_t soft_spi_t::transferData(std::uint32_t data, std::uint_fast8_t bits)
  {
    if (pin_dc_ >= 0) { lgfx::gpio_hi(pin_dc_); }
    std::uint32_t value = 0;
    for (std::uint_fast8_t i = 0; i < bits; ++i)
    {
      const auto index = bit_index(i);
      if (data & (std::uint32_t(1) << index)) { lgfx::gpio_hi(pin_mosi_); }
      else                                    { lgfx::gpio_lo(pin_mosi_); }
      lgfx::delayMicroseconds(half_us_);
      if (lgfx::gpio_in(pin_miso_)) { value |= std::uint32_t(1) << index; }
      lgfx::gpio_hi(pin_sclk_);
      lgfx::delayMicroseconds(half_us_);
      lgfx::gpio_lo(pin_sclk_);
    }
    return value;
  }

  void soft_spi_t::beginRead(std::uint_fast8_t dummy_bits)
  {
    if (pin_dc_ >= 0) { lgfx::gpio_hi(pin_dc_); }
    if (pin_miso_ == pin_mosi_) { lgfx::pinMode(pin_mosi_, lgfx::pin_mode_t::input); }
    for (std::uint_fast8_t i = 0; i < dummy_bits; ++i) { clock(); }
  }

  std::uint32_t soft_spi_t::readData(std::uint_fast8_t bits)
  {
    std::uint32_t value = 0;
    for (std::uint_fast8_t i = 0; i < bits; ++i)
    {
      lgfx::delayMicroseconds(half_us_);
      if (lgfx::gpio_in(pin_miso_)) { value |= std::uint32_t(1) << bit_index(i); }
      lgfx::gpio_hi(pin_sclk_);
      lgfx::delayMicroseconds(half_us_);
      lgfx::gpio_lo(pin_sclk_);
    }
    return value;
  }

  void soft_spi_t::readBytes(std::uint8_t* dst, std::size_t length)
  {
    while (length--) { *dst++ = static_cast<std::uint8_t>(readData(8)); }
  }

  void soft_spi_t::endRead()
  {
    if (pin_miso_ == pin_mosi_)
    {
      // Input mode leaves the latch high. Clear it before driving MOSI again
      // to avoid a long high pulse on shared data lines such as WS2812 inputs.
      lgfx::gpio_lo(pin_mosi_);
      lgfx::pinMode(pin_mosi_, lgfx::pin_mode_t::output);
    }
  }

  std::uint32_t soft_spi_read32(probe_ctx_t& ctx, int pin_sclk, int pin_mosi, int pin_miso,
                                int pin_dc, int pin_cs, std::uint8_t cmd, std::uint8_t dummy_bits,
                                std::uint32_t half_us, bool legacy_zero_preamble)
  {
    soft_spi_t bus(pin_sclk, pin_mosi, pin_miso, pin_dc, half_us);
    if (legacy_zero_preamble)
    {
      // Legacy _read_panel_id clocked a zero command while CS was high before
      // selecting the panel. ESP32 PICO display probes retain that exact read.
      bus.init();
      bus.beginTransaction();
      lgfx::gpio_hi(pin_cs);
      lgfx::pinMode(pin_cs, lgfx::pin_mode_t::output);
      bus.writeCommand(0, 8);
      bus.wait();
    }
    else
    {
      lgfx::gpio_hi(pin_cs);
      lgfx::pinMode(pin_cs, lgfx::pin_mode_t::output);
      bus.init();
      bus.beginTransaction();
    }
    lgfx::gpio_lo(pin_cs);
    bus.writeCommand(cmd, 8);
    bus.beginRead(dummy_bits);
    const auto value = bus.readData(32);
    lgfx::gpio_hi(pin_cs);
    bus.endTransaction();

    // CS is owned by the caller. Restore only the shared bus signals that this
    // helper borrowed, using their state at transaction start.
    ctx.transaction->restore_start({ pin_sclk, pin_mosi, pin_miso, pin_dc });
    return value;
  }
