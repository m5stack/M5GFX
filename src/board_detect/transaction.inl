  static void restore_if_changed(lgfx::gpio::pin_backup_t& saved)
  {
    // restore() briefly disables output driving. Skip it when every saved
    // register already matches so an unchanged output cannot momentarily float.
    if (!saved.matches_current()) { saved.restore(); }
  }

  bool detection_gpio_snapshot_t::capture(pin_list_t unconditional,
                                          pin_list_t conditional,
                                          bool include_conditional)
  {
    const std::size_t total = unconditional.size
                            + (include_conditional ? conditional.size : 0);
    if ((unconditional.data == nullptr && unconditional.size != 0)
     || (conditional.data == nullptr && conditional.size != 0)
     || total > max_detection_pins)
    {
      return false;
    }
    const pin_list_t lists[] = { unconditional, include_conditional ? conditional : pin_list_t { nullptr, 0 } };
    for (const auto& list : lists)
    {
      for (std::size_t index = 0; index < list.size; ++index)
      {
        const auto pin = list.data[index];
        if (pin < 0 || pin >= GPIO_NUM_MAX || !GPIO_IS_VALID_GPIO(pin)) { return false; }
        for (std::size_t prior = 0; prior < count_; ++prior)
        {
          if (saved_[prior].getPin() == pin) { return false; }
        }
        saved_[count_].setPin(pin);
        saved_[count_].backup();
        ++count_;
      }
    }
    // LP-I2C routing can survive a system reset and make ordinary GPIO reads
    // observe a controller-held line instead of the pad.  Reclaim every pin
    // only after the complete snapshot succeeds.  Like the legacy soft-I2C
    // probe, rollback restores normal GPIO state but deliberately does not
    // return pads to stale low-power ownership.
    for (std::size_t index = 0; index < count_; ++index)
    {
      lgfx::gpio::release_lp_pad(saved_[index].getPin());
    }
    return true;
  }

  void detection_gpio_snapshot_t::rollback()
  {
    for (std::size_t index = count_; index != 0; --index) { restore_if_changed(saved_[index - 1]); }
  }

  void detection_gpio_snapshot_t::restore_start(const std::int8_t* pins,
                                                 std::size_t count)
  {
    if (pins == nullptr && count != 0)
    {
      ESP_LOGE("board_detect", "restore_start missing pin list");
      return;
    }
    bool missing = false;
    int missing_pin = -1;
    for (std::size_t requested = 0; requested < count; ++requested)
    {
      const int pin = pins[requested];
      if (pin < 0) { continue; }  // an unused optional signal
      bool found = false;
      if (pin >= 0 && pin < GPIO_NUM_MAX && GPIO_IS_VALID_GPIO(pin))
      {
        for (std::size_t index = count_; index != 0; --index)
        {
          if (saved_[index - 1].getPin() == pin) { found = true; break; }
        }
      }
      if (!found && !missing) { missing = true; missing_pin = pin; }
    }
    // Match full rollback ordering while restoring each captured pin at most once.
    for (std::size_t index = count_; index != 0; --index)
    {
      const auto pin = saved_[index - 1].getPin();
      for (std::size_t requested = 0; requested < count; ++requested)
      {
        if (pins[requested] == pin)
        {
          restore_if_changed(saved_[index - 1]);
          break;
        }
      }
    }
    if (missing)
    {
      ESP_LOGE("board_detect", "restore_start pin=%d is absent from snapshot", missing_pin);
    }
  }

  void detection_bus_journal_t::opened_i2c(int port)
  {
    if (port < 0) { return; }
    for (std::size_t index = 0; index < i2c_count_; ++index)
    {
      if (i2c_ports_[index] == port) { return; }
    }
    if (i2c_count_ < sizeof(i2c_ports_) / sizeof(i2c_ports_[0]))
    {
      i2c_ports_[i2c_count_++] = static_cast<std::int8_t>(port);
    }
  }

  void detection_bus_journal_t::rollback()
  {
    while (i2c_count_ != 0) { lgfx::i2c::release(i2c_ports_[--i2c_count_]); }
  }

  void detection_bus_journal_t::commit()
  {
    i2c_count_ = 0;
  }

  detection_transaction_t::detection_transaction_t(
    pin_list_t unconditional, pin_list_t conditional, bool include_conditional)
  : valid_ { gpio_.capture(unconditional, conditional, include_conditional) }
  {
  }

  detection_transaction_t::~detection_transaction_t()
  {
    rollback();
  }

  void detection_transaction_t::restore_start(std::initializer_list<int> pins)
  {
    std::int8_t values[max_detection_pins];
    std::size_t count = 0;
    for (auto pin : pins)
    {
      if (count == max_detection_pins) { break; }
      // Anything outside int8_t is not a GPIO; keep it absent instead of aliasing.
      values[count++] = (pin < -1 || pin > INT8_MAX) ? INT8_MAX : static_cast<std::int8_t>(pin);
    }
    gpio_.restore_start(values, count);
  }

  void detection_transaction_t::rollback()
  {
    if (!valid_ || committed_) { return; }
    buses_.rollback();
    gpio_.rollback();
  }

  void detection_transaction_t::commit()
  {
    if (!valid_ || committed_) { return; }
    buses_.commit();
    committed_ = true;
  }
