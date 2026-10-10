  namespace toughc5_detail
  {
    constexpr int sda = wiring::toughc5::internal_i2c_sda;
    constexpr int scl = wiring::toughc5::internal_i2c_scl;
#if defined(SOC_LP_I2C_NUM) && SOC_LP_I2C_NUM > 0 && __has_include(<driver/i2c_master.h>) \
 && defined(ESP_IDF_VERSION_VAL) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 0)
    constexpr int_fast16_t i2c_port = LP_I2C_NUM_0;
#else
    constexpr int_fast16_t i2c_port = I2C_NUM_0;
#endif
    constexpr std::uint32_t i2c_freq = pmic_ops::pm1_i2c_freq;
    static const pmic_variant_t power_variants[] = {
      pmic_variant_ack_only(pmic_ops::pm1_i2c_addr, 0,
                            ops::list(pmic_ops::toughc5_power_on),
                            reg_bit(0, 0), reg_bit(0, 0),
                            ops::list(pmic_ops::toughc5_reset_assert),
                            ops::list(pmic_ops::toughc5_reset_release),
                            { nullptr, 0 }, 0),
    };
  }

  static constexpr std::int8_t toughc5_hold[] = { wiring::toughc5::display_cs };

  static constexpr board_desc_t desc_toughc5 = {
    { static_cast<board_id_t>(lgfx::board_M5ToughC5), "M5ToughC5", 0 },
    i2c_power_confirmed(toughc5_detail::i2c_freq, toughc5_detail::power_variants,
                        pmic_ops::pm1_family_devices),
    i2c_reset(2, 10), no_shared_sd(),
    display_pins(wiring::toughc5::display_sclk, wiring::toughc5::display_mosi,
                 wiring::toughc5::display_miso, wiring::toughc5::display_dc,
                 wiring::toughc5::display_cs, wiring::toughc5::display_rst,
                 wiring::toughc5::display_busy),
    pins(toughc5_hold),
    internal_i2c(toughc5_detail::sda, toughc5_detail::scl, toughc5_detail::i2c_port),
    no_options(), pins(toughc5_hold),
  };

  static constexpr board_desc_t desc_stampc5 = {
    { static_cast<board_id_t>(lgfx::board_M5StampC5), "M5StampC5", def_flag_fallback },
    no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
    no_internal_i2c(), no_options(), no_pins(),
  };

  class stampc5_detector_t final : public board_detector_t
  {
  public:
    stampc5_detector_t() : board_detector_t(members_) {}
    bool signature(probe_ctx_t& ctx) const override
    {
      const auto sys2 = REG_READ(EFUSE_RD_MAC_SYS2_REG);
      if (((sys2 >> EFUSE_FLASH_CAP_S) & EFUSE_FLASH_CAP_V) != 1
       || ((sys2 >> EFUSE_PSRAM_CAP_S) & EFUSE_PSRAM_CAP_V) != 0) { return false; }
      // Package facts only suggest a board; keep later families from probing
      // this module's user pins without treating it as confirmed.
      if (ctx.candidate == nullptr) { ctx.candidate = &desc_stampc5.def; }
      ctx.family_identified = true;
      return false;
    }
    bool confirm(probe_ctx_t&, board_result_t*) const override { return false; }
  private:
    static const board_def_t* const members_[2];
  };
  const board_def_t* const stampc5_detector_t::members_[2] = { &desc_stampc5.def, nullptr };
  static const stampc5_detector_t stampc5_detector;

  class toughc5_detector_t final : public board_detector_t
  {
  public:
    toughc5_detector_t() : board_detector_t(members_) {}
    bool signature(probe_ctx_t& ctx) const override
    {
      return probe_i2c_bus_present(ctx, toughc5_detail::sda, toughc5_detail::scl);
    }
    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (result == nullptr) { return false; }
      std::uint8_t id[2] = {};
      if (!probe_i2c_read(ctx, toughc5_detail::sda, toughc5_detail::scl,
                          pmic_ops::pm1_i2c_addr, 0, id, sizeof(id),
                          toughc5_detail::i2c_freq, 200)
       || (static_cast<std::uint16_t>(id[1]) << 8 | id[0]) != pmic_ops::pm1_device_id)
      {
        return false;
      }
      if (!probe_i2c_read(ctx, toughc5_detail::sda, toughc5_detail::scl,
                          pmic_ops::ioe1_i2c_addr, 0, id, sizeof(id),
                          toughc5_detail::i2c_freq, 200))
      {
        return false;
      }
      const std::uint8_t addrs[] = { pmic_ops::pm1_i2c_addr, pmic_ops::ioe1_i2c_addr,
                                     pmic_ops::ioe1_i2c_addr, pmic_ops::ioe1_i2c_addr };
      const std::uint8_t regs[] = { 0x06, 0x03, 0x05, 0x13 };
      std::uint8_t value = 0;
      for (std::size_t i = 0; i < sizeof(regs); ++i)
      {
        if (!probe_i2c_read(ctx, toughc5_detail::sda, toughc5_detail::scl,
                            addrs[i], regs[i], &value, 1,
                            toughc5_detail::i2c_freq, 0))
        {
          return false;
        }
      }
      result->assign(&desc_toughc5);
      return true;
    }
  private:
    static const board_def_t* const members_[2];
  };
  const board_def_t* const toughc5_detector_t::members_[2] = { &desc_toughc5.def, nullptr };
  static const toughc5_detector_t toughc5_detector;
  #include "../generated/esp32c5_detector_order.hpp"

  construct_status_t construct_toughc5(const board_result_t&, display_parts_t*);
  static const board_entry_t esp32c5_boards[] = {
    { &desc_stampc5, construct_displayless, "board_M5StampC5", nullptr },
    { &desc_toughc5, construct_toughc5, "board_M5ToughC5", nullptr },
  };
  success_log_t success_log(const board_result_t& result) { return success_log(esp32c5_boards, result); }
