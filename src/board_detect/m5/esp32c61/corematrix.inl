  namespace corematrix_detail
  {
    constexpr int sda = wiring::corematrix::internal_i2c_sda;
    constexpr int scl = wiring::corematrix::internal_i2c_scl;
    constexpr int_fast16_t i2c_port = I2C_NUM_0;
    constexpr std::uint32_t i2c_freq = pmic_ops::pm1_i2c_freq;
    static const pmic_variant_t power_variants[] = {
      pmic_variant_ack_only(pmic_ops::pm1_i2c_addr, 0,
                            ops::list(pmic_ops::corematrix_power_on),
                            reg_bit(0, 0), reg_bit(0, 0),
                            { nullptr, 0 }, { nullptr, 0 }, { nullptr, 0 }, 0),
    };
  }

  static constexpr board_desc_t desc_corematrix = {
    { static_cast<board_id_t>(lgfx::board_M5CoreMatrix), "M5CoreMatrix", 0 },
    i2c_power_confirmed(corematrix_detail::i2c_freq, corematrix_detail::power_variants,
                        pmic_ops::pm1_family_devices),
    no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
    internal_i2c(corematrix_detail::sda, corematrix_detail::scl,
                 corematrix_detail::i2c_port),
    no_options(), no_pins(),
  };

  class corematrix_detector_t final : public board_detector_t
  {
  public:
    corematrix_detector_t() : board_detector_t(members_) {}
    bool signature(probe_ctx_t& ctx) const override
    {
      return probe_i2c_bus_present(ctx, corematrix_detail::sda, corematrix_detail::scl);
    }
    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (result == nullptr) { return false; }
      std::uint8_t id[2] = {};
      if (!probe_i2c_read(ctx, corematrix_detail::sda, corematrix_detail::scl,
                          pmic_ops::pm1_i2c_addr, 0, id, sizeof(id),
                          corematrix_detail::i2c_freq, 200)
       || (static_cast<std::uint16_t>(id[1]) << 8 | id[0]) != pmic_ops::pm1_device_id)
      {
        return false;
      }
      if (!probe_i2c_read(ctx, corematrix_detail::sda, corematrix_detail::scl,
                          pmic_ops::ioe1_i2c_addr, 0, id, sizeof(id),
                          corematrix_detail::i2c_freq, 200))
      {
        return false;
      }
      result->assign(&desc_corematrix);
      return true;
    }
  private:
    static const board_def_t* const members_[2];
  };
  const board_def_t* const corematrix_detector_t::members_[2] = { &desc_corematrix.def, nullptr };
  static const corematrix_detector_t corematrix_detector;
  static const board_detector_t* const esp32c61_detectors[] = { &corematrix_detector, nullptr };

  construct_status_t construct_corematrix(const board_result_t&, display_parts_t*);
  static const board_entry_t esp32c61_boards[] = {
    { &desc_corematrix, construct_corematrix, "board_M5CoreMatrix", nullptr },
  };
  success_log_t success_log(const board_result_t& result) { return success_log(esp32c61_boards, result); }
