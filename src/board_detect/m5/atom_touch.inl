// Classic ESP32 touch measurement for PICO-D4. Included inside m5gfx::board_detect::m5.
namespace atom_touch
{
  inline volatile std::uint32_t& reg(std::uint32_t addr)
  { return *reinterpret_cast<volatile std::uint32_t*>(addr); }

  static bool measure(std::uint16_t* t4, std::uint16_t* t7)
  {
    constexpr int channels[] = { 4, 7 }; // T4=G13, T7=G27
    const auto state0 = reg(RTC_CNTL_STATE0_REG);
    const auto ctrl1 = reg(SENS_SAR_TOUCH_CTRL1_REG);
    const auto ctrl2 = reg(SENS_SAR_TOUCH_CTRL2_REG);
    const auto enable = reg(SENS_SAR_TOUCH_ENABLE_REG);
    const auto cfg = reg(RTC_IO_TOUCH_CFG_REG);
    std::uint32_t pads[2];
    for (int i = 0; i < 2; ++i) { pads[i] = reg(RTC_IO_TOUCH_PAD0_REG + channels[i] * 4); }

    constexpr std::uint32_t clear = RTC_IO_TOUCH_PAD0_RDE_M | RTC_IO_TOUCH_PAD0_RUE_M
      | RTC_IO_TOUCH_PAD0_DAC_M | RTC_IO_TOUCH_PAD0_TIE_OPT_M | RTC_IO_TOUCH_PAD0_XPD_M
      | RTC_IO_TOUCH_PAD0_MUX_SEL_M | RTC_IO_TOUCH_PAD0_FUN_SEL_M
      | RTC_IO_TOUCH_PAD0_FUN_IE_M | RTC_IO_TOUCH_PAD0_TO_GPIO_M | RTC_IO_TOUCH_PAD0_START_M;
    constexpr std::uint32_t set = (7u << RTC_IO_TOUCH_PAD0_DAC_S)
      | RTC_IO_TOUCH_PAD0_XPD_M | RTC_IO_TOUCH_PAD0_MUX_SEL_M;

    // Stop the automatic timer, use a software start, and disable pad pulls.
    // Only these saved control registers are changed; wake thresholds/masks stay intact.
    reg(RTC_CNTL_STATE0_REG) = state0 & ~RTC_CNTL_TOUCH_SLP_TIMER_EN;
    reg(SENS_SAR_TOUCH_CTRL2_REG) = (ctrl2 & ~SENS_TOUCH_START_EN)
                                  | SENS_TOUCH_START_FORCE | SENS_TOUCH_START_FSM_EN;
    reg(SENS_SAR_TOUCH_CTRL1_REG) = (ctrl1 & ~((SENS_TOUCH_XPD_WAIT << SENS_TOUCH_XPD_WAIT_S)
                                  | (SENS_TOUCH_MEAS_DELAY << SENS_TOUCH_MEAS_DELAY_S)))
                                  | (0xFFu << SENS_TOUCH_XPD_WAIT_S)
                                  | ((800u & SENS_TOUCH_MEAS_DELAY) << SENS_TOUCH_MEAS_DELAY_S); // 0.1 ms at 8 MHz
    // Investigation atom-picod4-pin-edge-2026-09-28: DREFH=3, DRANGE=0
    // caps the charge at 1.2 V. A 2.7 V scan can light the WS2812 array white.
    reg(RTC_IO_TOUCH_CFG_REG) = (cfg & ~((RTC_IO_TOUCH_DREFH << RTC_IO_TOUCH_DREFH_S)
                             | (RTC_IO_TOUCH_DREFL << RTC_IO_TOUCH_DREFL_S)
                             | (RTC_IO_TOUCH_DRANGE << RTC_IO_TOUCH_DRANGE_S)))
                             | (3u << RTC_IO_TOUCH_DREFH_S);
    for (int i = 0; i < 2; ++i)
    { reg(RTC_IO_TOUCH_PAD0_REG + channels[i] * 4) = (pads[i] & ~clear) | set; }
    reg(SENS_SAR_TOUCH_ENABLE_REG) = (enable & ~(SENS_TOUCH_PAD_WORKEN << SENS_TOUCH_PAD_WORKEN_S))
                                   | (((1u << 4) | (1u << 7)) << SENS_TOUCH_PAD_WORKEN_S);

    reg(SENS_SAR_TOUCH_CTRL2_REG) = reg(SENS_SAR_TOUCH_CTRL2_REG) & ~SENS_TOUCH_START_EN;
    reg(SENS_SAR_TOUCH_CTRL2_REG) = reg(SENS_SAR_TOUCH_CTRL2_REG) | SENS_TOUCH_START_EN;
    const auto started = esp_timer_get_time();
    while (!(reg(SENS_SAR_TOUCH_CTRL2_REG) & SENS_TOUCH_MEAS_DONE))
    {
      if (esp_timer_get_time() - started > 200000) { break; }
    }
    const bool ok = (reg(SENS_SAR_TOUCH_CTRL2_REG) & SENS_TOUCH_MEAS_DONE) != 0;
    *t4 = ok ? (reg(SENS_SAR_TOUCH_OUT3_REG) >> 16) : 0;
    *t7 = ok ? (reg(SENS_SAR_TOUCH_OUT4_REG) & 0xFFFF) : 0;
    reg(SENS_SAR_TOUCH_CTRL2_REG) = reg(SENS_SAR_TOUCH_CTRL2_REG) & ~SENS_TOUCH_START_EN;

    reg(SENS_SAR_TOUCH_ENABLE_REG) = enable;
    for (int i = 0; i < 2; ++i) { reg(RTC_IO_TOUCH_PAD0_REG + channels[i] * 4) = pads[i]; }
    reg(RTC_IO_TOUCH_CFG_REG) = cfg;
    reg(SENS_SAR_TOUCH_CTRL1_REG) = ctrl1;
    reg(SENS_SAR_TOUCH_CTRL2_REG) = ctrl2;
    reg(RTC_CNTL_STATE0_REG) = state0;
    reg(RTC_CNTL_INT_CLR_REG) = RTC_CNTL_TOUCH_INT_CLR;
    return ok && *t4 != 0;
  }

  static IRAM_ATTR std::uint32_t cycles()
  {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    return esp_cpu_get_cycle_count();
#else
    return esp_cpu_get_ccount();
#endif
  }

  // Send 25 black GRB pixels. Flash stalls must not extend a high pulse.
  static IRAM_ATTR void send_black(std::uint32_t mhz)
  {
    // WS2812C-2020 T0H = 0.22-0.38 us. Aim at the middle: loop overhead adds to the set value, and 0.35 us
    // sat at the upper edge (single LEDs mid-chain lit, varying with the USB hub = LED supply voltage).
    const std::uint32_t high = mhz * 28 / 100; // 0.28 us
    const std::uint32_t low = mhz * 90 / 100;  // 0.9 us
    for (int bit = 0; bit < 25 * 24; ++bit)
    {
      auto at = cycles();
      GPIO.out_w1ts = 1u << 27;
      while (cycles() - at < high) {}
      at = cycles();
      GPIO.out_w1tc = 1u << 27;
      while (cycles() - at < low) {}
    }
  }

  static void clear_led()
  {
    gpio_set_pull_mode(GPIO_NUM_27, GPIO_FLOATING);
    GPIO.out_w1tc = 1u << 27; // Set low before enabling output: no high spike on DIN.
    gpio_set_direction(GPIO_NUM_27, GPIO_MODE_OUTPUT);
    rtc_cpu_freq_config_t cpu_freq;
    rtc_clk_cpu_freq_get_config(&cpu_freq); // Same cross-IDF API as lgfx::getApbFrequency.
    const auto mhz = cpu_freq.freq_mhz;
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    esp_rom_delay_us(300);
    taskENTER_CRITICAL(&mux);
    send_black(mhz);
    taskEXIT_CRITICAL(&mux);
    esp_rom_delay_us(300);
  }
}
