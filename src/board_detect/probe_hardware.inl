  bool wait_i2c_scl_high(int pin_scl)
  {
    io::pin_high(pin_scl);
    io::delay_us(5);
    if (io::pin_read(pin_scl)) { return true; }
    const auto started = io::micros();
    while (io::micros() - started < 25000)
    {
      if (io::pin_read(pin_scl)) { return true; }
      io::delay_us(1);
    }
    return false;
  }

  bool release_held_sda(int pin_sda, int pin_scl)
  {
    io::pin_mode(pin_sda, io::pin_mode_t::input);
    io::pin_mode(pin_scl, io::pin_mode_t::input);
    io::delay_us(10);

    // A reset of this MCU during a transfer can leave a peripheral holding
    // SDA low while it waits for more clocks. Clock it out (up to 9 bits) and
    // finish with STOP. Input mode is open drain with the output latch high.
    bool bus_released = io::pin_read(pin_scl);
    if (bus_released && !io::pin_read(pin_sda))
    {
      for (int i = 0; i < 9 && !io::pin_read(pin_sda); ++i)
      {
        io::pin_low(pin_scl);
        io::delay_us(5);
        if (!wait_i2c_scl_high(pin_scl)) { bus_released = false; break; }
      }
      if (bus_released)
      {
        io::pin_low(pin_scl);
        io::pin_low(pin_sda);
        io::delay_us(5);
        bus_released = wait_i2c_scl_high(pin_scl);
        if (bus_released)
        {
          io::pin_high(pin_sda);
          io::delay_us(10);
        }
      }
    }
    const bool released = bus_released && io::pin_read(pin_sda) && io::pin_read(pin_scl);
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
      ctx.transaction->restore_to_snapshot({ pin_sda, pin_scl });
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
      // soft-I2C clock-stretch allowance.
      io::pin_mode(pin_sda, io::pin_mode_t::input);
      io::pin_mode(pin_scl, io::pin_mode_t::input);
      wait_i2c_scl_high(pin_scl);
      ctx.transaction->restore_to_snapshot({ pin_sda, pin_scl });
    };
    auto recover = [&]()
    {
      // A reset during a transaction can leave a slave holding SDA forever.
      // Recover it before attempting detection.
      ESP_LOGD("M5GFX", "[Autodetect] recovering held SDA on I2C pins SDA=%d SCL=%d",
               pin_sda, pin_scl);
      release_held_sda(pin_sda, pin_scl);
      ctx.transaction->restore_to_snapshot({ pin_sda, pin_scl });
    };
    return i2c_bus_probe_detail::probe_i2c_bus_present(sample, wait_scl, recover);
  }

  bool probe_pin_pullup_low(probe_ctx_t& ctx, int pin)
  {
    io::pin_mode(pin, io::pin_mode_t::input_pullup);
    io::delay_us(10);
    const bool held_low = !io::pin_read(pin);
    ctx.transaction->restore_to_snapshot({ pin });
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

      io::pin_mode(pin_sda, io::pin_mode_t::input_pulldown);
      io::pin_mode(pin_scl, io::pin_mode_t::input_pulldown);
      io::delay_us(10);
      io::pin_mode(pin_sda, io::pin_mode_t::input);
      io::pin_mode(pin_scl, io::pin_mode_t::input);
      io::delay_us(10);
      // The first check does not wait: a pin pair without pull-ups reads SCL
      // low here, and waiting for it would delay every such detection. Only a
      // bus that has pull-ups and a held SDA is clocked, and only those clocks
      // wait for a stretching peripheral.
      bool bus_released = io::pin_read(pin_scl);
      if (bus_released && !io::pin_read(pin_sda))
      {
        bus_released = release_held_sda(pin_sda, pin_scl);
      }
      cache.pullup_ok = bus_released && io::pin_read(pin_sda) && io::pin_read(pin_scl);
      ctx.transaction->restore_to_snapshot({ pin_sda, pin_scl });
    }

    const std::uint32_t bit = 1u << (addr & 31);
    if (!(cache.checked[addr >> 5] & bit))
    {
      cache.checked[addr >> 5] |= bit;
      if (cache.pullup_ok)
      {
        if (io::i2c_open(ctx.probe_port, pin_sda, pin_scl))
        {
          const bool hit = io::i2c_ack(ctx.probe_port, addr, 100000);
          if (hit) { cache.ack[addr >> 5] |= bit; }
          io::i2c_close(ctx.probe_port);
        }
        ctx.transaction->restore_to_snapshot({ pin_sda, pin_scl });
      }
    }
    return cache.pullup_ok && (cache.ack[addr >> 5] & bit);
  }

  bool probe_i2c_read(probe_ctx_t& ctx, int pin_sda, int pin_scl, std::uint8_t addr,
                      std::uint16_t reg, std::uint8_t* data, std::size_t length,
                      std::uint32_t freq, std::uint32_t poll_ms, bool reg16)
  {
    if (data == nullptr || length == 0 || addr < 0x08 || addr > 0x77) { return false; }
    if (!io::i2c_open(ctx.probe_port, pin_sda, pin_scl)) { return false; }
    const std::uint8_t reg_bytes[] = {
      static_cast<std::uint8_t>(reg >> 8), static_cast<std::uint8_t>(reg),
    };
    const auto started = io::millis();
    bool success = false;
    do
    {
      success = io::i2c_write_read(
        ctx.probe_port, addr, reg_bytes + !reg16, 1 + reg16,
        data, length, freq);
      if (success) { break; }
      io::delay_ms(1);
    } while (io::millis() - started < poll_ms);
    io::i2c_close(ctx.probe_port);
    ctx.transaction->restore_to_snapshot({ pin_sda, pin_scl });
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
      io::pin_mode(pin, io::pin_mode_t::input_pulldown);
      io::delay_us(10);
      if (io::pin_read(pin)) { result.pulldown_high |= bit; }
      if (release_us != 0)
      {
        io::pin_mode(pin, io::pin_mode_t::input);
        io::delay_us(release_us);
        if (io::pin_read(pin)) { result.pulldown_release_high |= bit; }
      }
      io::pin_mode(pin, io::pin_mode_t::input_pullup);
      io::delay_us(10);
      if (io::pin_read(pin)) { result.pullup_high |= bit; }
      if (release_us != 0)
      {
        io::pin_mode(pin, io::pin_mode_t::input);
        io::delay_us(release_us);
        if (io::pin_read(pin)) { result.pullup_release_high |= bit; }
      }
      ctx.transaction->restore_to_snapshot(static_cast<std::int8_t>(pin));
    }
    return result;
  }

  static bool close_dedicated_release_bundle(io::dedicated_gpio_handle_t bundle)
  {
    const int error = io::dedicated_gpio_close(bundle);
    if (error != 0)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO bundle cleanup failed (%d)", error);
      return false;
    }
    return true;
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
    if (!io::dedicated_gpio_supported)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO release probe is unavailable in this SDK");
      return result;
    }
    if (ctx.transaction == nullptr || pins == nullptr || pin_count == 0
     || pin_count > max_dedicated_release_pins || reads == 0
     || reads > max_dedicated_release_reads || samples == 0
     || samples > max_dedicated_release_samples || settle_us == 0)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe has invalid parameters");
      return result;
    }
    const int pinned_core = io::dedicated_release_task_core();
    if (pinned_core == io::no_core_affinity()
     || io::dedicated_release_core_id() != pinned_core)
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
    if (io::dedicated_gpio_in_use())
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO in use; skipping release probe");
      return result;
    }
    int bundle_pins[max_dedicated_release_pins];
    for (std::uint8_t index = 0; index < pin_count; ++index)
    {
      const int pin = pins[index];
      if (!io::pin_valid(pin))
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
      io::pin_weak_pullup_open_drain(pin);
    }

    io::dedicated_gpio_handle_t bundle;
    const int creation_core = io::dedicated_release_core_id();
    if (creation_core != pinned_core)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe task migrated before bundle creation");
      ctx.transaction->restore_to_snapshot(pins, pin_count);
      return result;
    }
    const int bundle_error = io::dedicated_gpio_open(bundle_pins, pin_count, &bundle);
    if (bundle_error != 0)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO bundle creation failed (%d)", int(bundle_error));
      ctx.transaction->restore_to_snapshot(pins, pin_count);
      return result;
    }
    if (io::dedicated_release_core_id() != pinned_core)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe task migrated after bundle creation");
      close_dedicated_release_bundle(bundle);
      ctx.transaction->restore_to_snapshot(pins, pin_count);
      return result;
    }
    std::uint32_t input_mask = 0;
    std::uint32_t output_mask = 0;
    int input_mask_error = 0;
    int output_mask_error = 0;
    io::dedicated_release_channel_masks(bundle, pin_count, &input_mask, &output_mask,
                                    &input_mask_error, &output_mask_error);
    if (input_mask_error != 0 || output_mask_error != 0)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO channel lookup failed (in=%d out=%d)",
               int(input_mask_error), int(output_mask_error));
      close_dedicated_release_bundle(bundle);
      ctx.transaction->restore_to_snapshot(pins, pin_count);
      return result;
    }
    const std::uint32_t channel_mask = (1u << pin_count) - 1u;
    const std::uint32_t input_first_bit = input_mask & (~input_mask + 1u);
    if (input_first_bit == 0 || input_mask != input_first_bit * channel_mask
     || output_mask == 0)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO channel allocation is not contiguous");
      close_dedicated_release_bundle(bundle);
      ctx.transaction->restore_to_snapshot(pins, pin_count);
      return result;
    }
    static io::critical_section_t mux;
    const auto measure = [&](std::uint16_t* first_high, std::uint32_t* cycles)
    {
      volatile std::uint32_t raw[max_dedicated_release_reads];
      mux.enter();
      // A dedicated bundle belongs to its creation core. Check immediately
      // after scheduling is stopped and reject the measurement if it migrated.
      if (io::dedicated_release_core_id() != creation_core)
      {
        mux.exit();
        return false;
      }
      io::dedicated_release_write_mask(output_mask, 0);
      io::delay_us(settle_us);
      const std::uint32_t started = io::dedicated_release_cycle_count();
      io::dedicated_release_write_mask(output_mask, output_mask);
      for (std::uint16_t read = 0; read < reads; ++read)
      { raw[read] = io::dedicated_release_read_in(); }
      const std::uint32_t stopped = io::dedicated_release_cycle_count();
      mux.exit();
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
      result.cpu_hz = io::cpu_hz();
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

    if (!close_dedicated_release_bundle(bundle)) { result.available = false; }
    // del_bundle pin cleanup differs by IDF version (4.4/5.2/5.3 leave the
    // pads alone; newer versions disable output), and out_sel is not restored.
    // Dedicated input routing is peripheral-side. In every version,
    // The transaction's restore_to_snapshot (pin_state_t::restore) restores out_sel,
    // GPIO_PINn, output-enable, latch, and the complete IO_MUX register
    // (including temporary FUN_DRV).
    ctx.transaction->restore_to_snapshot(pins, pin_count);
    return result;
  }

  void soft_spi_t::init()
  {
    io::pin_low(pin_sclk_);
    io::pin_mode(pin_sclk_, io::pin_mode_t::output);
    io::pin_low(pin_mosi_);
    io::pin_mode(pin_mosi_, io::pin_mode_t::output);
    if (pin_miso_ != pin_mosi_) { io::pin_mode(pin_miso_, io::pin_mode_t::input); }
    if (pin_dc_ >= 0) { io::pin_mode(pin_dc_, io::pin_mode_t::output); }
  }

  void soft_spi_t::beginTransaction()
  {
    io::pin_low(pin_sclk_);
  }

  void soft_spi_t::endTransaction()
  {
    endRead();
    io::pin_low(pin_sclk_);
  }

  void soft_spi_t::clock()
  {
    io::delay_us(half_us_);
    io::pin_high(pin_sclk_);
    io::delay_us(half_us_);
    io::pin_low(pin_sclk_);
    io::delay_us(half_us_);
  }

  void soft_spi_t::send(std::uint32_t data, std::uint_fast8_t bits)
  {
    for (std::uint_fast8_t i = 0; i < bits; ++i)
    {
      if (data & (std::uint32_t(1) << bit_index(i))) { io::pin_high(pin_mosi_); }
      else                                           { io::pin_low(pin_mosi_); }
      clock();
    }
  }

  void soft_spi_t::writeCommand(std::uint32_t data, std::uint_fast8_t bits)
  {
    if (pin_dc_ >= 0) { io::pin_low(pin_dc_); }
    send(data, bits);
  }

  void soft_spi_t::writeData(std::uint32_t data, std::uint_fast8_t bits)
  {
    if (pin_dc_ >= 0) { io::pin_high(pin_dc_); }
    send(data, bits);
  }

  std::uint32_t soft_spi_t::transferData(std::uint32_t data, std::uint_fast8_t bits)
  {
    if (pin_dc_ >= 0) { io::pin_high(pin_dc_); }
    std::uint32_t value = 0;
    for (std::uint_fast8_t i = 0; i < bits; ++i)
    {
      const auto index = bit_index(i);
      if (data & (std::uint32_t(1) << index)) { io::pin_high(pin_mosi_); }
      else                                    { io::pin_low(pin_mosi_); }
      io::delay_us(half_us_);
      if (io::pin_read(pin_miso_)) { value |= std::uint32_t(1) << index; }
      io::pin_high(pin_sclk_);
      io::delay_us(half_us_);
      io::pin_low(pin_sclk_);
    }
    return value;
  }

  void soft_spi_t::beginRead(std::uint_fast8_t dummy_bits)
  {
    if (pin_dc_ >= 0) { io::pin_high(pin_dc_); }
    if (pin_miso_ == pin_mosi_) { io::pin_mode(pin_mosi_, io::pin_mode_t::input); }
    for (std::uint_fast8_t i = 0; i < dummy_bits; ++i) { clock(); }
  }

  std::uint32_t soft_spi_t::readData(std::uint_fast8_t bits)
  {
    std::uint32_t value = 0;
    for (std::uint_fast8_t i = 0; i < bits; ++i)
    {
      io::delay_us(half_us_);
      if (io::pin_read(pin_miso_)) { value |= std::uint32_t(1) << bit_index(i); }
      io::pin_high(pin_sclk_);
      io::delay_us(half_us_);
      io::pin_low(pin_sclk_);
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
      io::pin_low(pin_mosi_);
      io::pin_mode(pin_mosi_, io::pin_mode_t::output);
    }
  }

  std::uint32_t soft_spi_read32(probe_ctx_t& ctx, int pin_sclk, int pin_mosi, int pin_miso,
                                int pin_dc, int pin_cs, std::uint8_t cmd, std::uint8_t dummy_bits,
                                std::uint32_t half_us, bool deselected_zero_preamble)
  {
    soft_spi_t bus(pin_sclk, pin_mosi, pin_miso, pin_dc, half_us);
    if (deselected_zero_preamble)
    {
      // ESP32 PICO display probes clock a zero command with CS high
      // before selecting the panel.
      bus.init();
      bus.beginTransaction();
      io::pin_high(pin_cs);
      io::pin_mode(pin_cs, io::pin_mode_t::output);
      bus.writeCommand(0, 8);
      bus.wait();
    }
    else
    {
      io::pin_high(pin_cs);
      io::pin_mode(pin_cs, io::pin_mode_t::output);
      bus.init();
      bus.beginTransaction();
    }
    io::pin_low(pin_cs);
    bus.writeCommand(cmd, 8);
    bus.beginRead(dummy_bits);
    const auto value = bus.readData(32);
    io::pin_high(pin_cs);
    bus.endTransaction();

    // CS is owned by the caller. Restore only the shared bus signals that this
    // helper borrowed, using their state at transaction start.
    ctx.transaction->restore_to_snapshot({ pin_sclk, pin_mosi, pin_miso, pin_dc });
    return value;
  }
