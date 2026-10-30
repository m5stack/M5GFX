// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "../lgfx/v1/platforms/esp32/common.hpp"
#include <esp_idf_version.h>
#include <esp_rom_sys.h>
#include <esp_cpu.h>
#include <esp_attr.h>
#if __has_include(<esp_private/esp_clk.h>)
#include <esp_private/esp_clk.h>
#else
#include <esp_clk.h>
#endif
#include <soc/efuse_reg.h>
#include <soc/io_mux_reg.h>
#if __has_include(<driver/i2c_master.h>)
#include <driver/i2c_master.h>
#else
#include <driver/i2c.h>
#endif
#include <driver/gpio.h>
#include <soc/gpio_reg.h>
#include <esp_log.h> // Detection code needs ESP_LOGx through the I/O header contract.
#if defined (CONFIG_IDF_TARGET_ESP32S3) \
 && __has_include(<driver/dedic_gpio.h>) \
 && (__has_include(<hal/dedic_gpio_cpu_ll.h>) || __has_include(<hal/cpu_ll.h>)) \
 && (__has_include(<esp_private/esp_clk.h>) || __has_include(<esp_clk.h>))
 #define M5GFX_HAS_S3_DEDICATED_GPIO_RELEASE_PROBE
 #include <driver/dedic_gpio.h>
 #include <esp_cpu.h>
 #include <esp_idf_version.h>
 #if __has_include(<esp_private/esp_clk.h>)
  #include <esp_private/esp_clk.h>
 #else
  #include <esp_clk.h>
 #endif
 #include <freertos/FreeRTOS.h>
 #if __has_include(<hal/dedic_gpio_cpu_ll.h>)
  #include <hal/dedic_gpio_cpu_ll.h>
 #else
  // IDF 4.4 exposes the same S3 instructions through cpu_ll.h and reports
  // allocated channels as masks rather than offsets.
  #define M5GFX_S3_DEDICATED_GPIO_IDF44_CPU_LL
  #include <hal/cpu_ll.h>
 #endif
 #include <hal/gpio_ll.h>
 #include <soc/gpio_periph.h>
 #include <soc/gpio_reg.h>
 #include <soc/io_mux_reg.h>
 #include <soc/system_reg.h>
#endif

#if defined (CONFIG_IDF_TARGET_ESP32C3)
#include <soc/usb_serial_jtag_reg.h>
#endif
#if defined (CONFIG_IDF_TARGET_ESP32P4)
#include <esp_chip_info.h>
#include "../lgfx/v1/platforms/esp32p4/Touch_ST7123.hpp"
#include "../lgfx/v1/touch/Touch_GT911.hpp"
#endif
#if defined (CONFIG_IDF_TARGET_ESP32S3) && defined (CONFIG_SPIRAM_MODE_OCT)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include <esp_psram.h>
#else
#include <esp32s3/spiram.h>
#endif
#endif
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
#include <esp_cpu.h>
#include <esp_rom_sys.h>
#include <esp_timer.h>
#include <soc/gpio_struct.h>
#include <soc/rtc.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/rtc_io_reg.h>
#include <soc/sens_reg.h>
#endif

namespace m5gfx { namespace board_detect { namespace io {
  static constexpr int i2c_port_0 = I2C_NUM_0;
#if (defined(SOC_HP_I2C_NUM) && SOC_HP_I2C_NUM > 1) || (!defined(SOC_HP_I2C_NUM) && defined(SOC_I2C_NUM) && SOC_I2C_NUM > 1)
  static constexpr int i2c_port_1 = I2C_NUM_1;
#endif
#if defined(SOC_LP_I2C_NUM) && SOC_LP_I2C_NUM > 0 && __has_include(<driver/i2c_master.h>) \
 && defined(ESP_IDF_VERSION_VAL) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 0)
  static constexpr int low_power_i2c_port_0 = LP_I2C_NUM_0;
#else
  static constexpr int low_power_i2c_port_0 = I2C_NUM_0;
#endif
  __attribute__((always_inline)) inline void pin_mode(int pin, pin_mode_t mode)
  { lgfx::pinMode(pin, static_cast<lgfx::pin_mode_t>(mode)); }
  __attribute__((always_inline)) inline void pin_high(int pin) { lgfx::gpio_hi(pin); }
  __attribute__((always_inline)) inline void pin_low(int pin) { lgfx::gpio_lo(pin); }
  __attribute__((always_inline)) inline bool pin_read(int pin) { return lgfx::gpio_in(pin); }
  __attribute__((always_inline)) inline bool pin_valid(int pin)
  { return pin >= 0 && pin < GPIO_NUM_MAX && GPIO_IS_VALID_GPIO(pin); }
  static constexpr int pin_count = GPIO_NUM_MAX;
  __attribute__((always_inline)) inline bool pin_output_level(int pin, bool* level)
  {
    const auto bit = std::uint32_t(1) << (pin & 31);
#if !defined (SOC_GPIO_PIN_COUNT) || SOC_GPIO_PIN_COUNT > 32
    const bool output = pin & 32 ? REG_READ(GPIO_ENABLE1_REG) & bit : REG_READ(GPIO_ENABLE_REG) & bit;
    *level = pin & 32 ? REG_READ(GPIO_OUT1_REG) & bit : REG_READ(GPIO_OUT_REG) & bit;
#else
    const bool output = REG_READ(GPIO_ENABLE_REG) & bit;
    *level = REG_READ(GPIO_OUT_REG) & bit;
#endif
    return output;
  }
  class pin_state_t
  {
  public:
    __attribute__((always_inline)) void save(int pin) { native_.setPin(pin); native_.backup(); }
    __attribute__((always_inline)) bool matches_current() const { return native_.matches_current(); }
    __attribute__((always_inline)) void restore() { native_.restore(); }
    __attribute__((always_inline)) int pin() const { return native_.getPin(); }
  private:
    lgfx::gpio::pin_backup_t native_;
  };
  __attribute__((always_inline)) inline void release_lp_pad(int pin) { lgfx::gpio::release_lp_pad(pin); }
  __attribute__((always_inline)) inline volatile std::uint32_t* pin_low_register(int pin) { return lgfx::get_gpio_lo_reg(pin); }
  __attribute__((always_inline)) inline volatile std::uint32_t* pin_high_register(int pin) { return lgfx::get_gpio_hi_reg(pin); }
  __attribute__((always_inline)) inline IRAM_ATTR std::uint32_t cpu_cycles()
  {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    return esp_cpu_get_cycle_count();
#else
    return esp_cpu_get_ccount();
#endif
  }

  __attribute__((always_inline)) inline std::uint32_t cpu_hz() { return static_cast<std::uint32_t>(esp_clk_cpu_freq()); }
  __attribute__((always_inline)) inline std::uint32_t millis() { return lgfx::millis(); }
  __attribute__((always_inline)) inline std::uint32_t micros() { return lgfx::micros(); }
  __attribute__((always_inline)) inline void delay_ms(std::uint32_t ms) { lgfx::delay(ms); }
  __attribute__((always_inline)) inline void delay_us(std::uint32_t us) { lgfx::delayMicroseconds(us); }
  __attribute__((always_inline)) inline void delay_rom_us(std::uint32_t us) { esp_rom_delay_us(us); }

  __attribute__((always_inline)) inline bool i2c_open(int port, int sda, int scl)
  { return lgfx::i2c::init(port, sda, scl).has_value(); }
  __attribute__((always_inline)) inline void i2c_close(int port) { lgfx::i2c::release(port); }
  // Callers choose whether to end a failed begin through end_on_failure.
  __attribute__((always_inline)) inline bool i2c_ack(int port, std::uint16_t addr, std::uint32_t freq, bool end_on_failure = false)
  {
    const bool began = lgfx::i2c::beginTransaction(port, addr, freq, false).has_value();
    if (!began && !end_on_failure) { return false; }
    const bool ended = lgfx::i2c::endTransaction(port).has_value();
    return began && ended;
  }
  __attribute__((always_inline)) inline bool i2c_read_reg8(int port, std::uint16_t addr, std::uint8_t reg,
                           std::uint8_t* value, std::uint32_t freq)
  {
    const auto read = lgfx::i2c::readRegister8(port, addr, reg, freq);
    if (!read.has_value()) { return false; }
    *value = read.value();
    return true;
  }
  __attribute__((always_inline)) inline bool i2c_write_reg8(int port, std::uint16_t addr, std::uint8_t reg,
                            std::uint8_t value, std::uint32_t freq)
  { return lgfx::i2c::writeRegister8(port, addr, reg, value, 0, freq).has_value(); }
  __attribute__((always_inline)) inline bool i2c_write(int port, std::uint16_t addr, const std::uint8_t* data,
                        std::size_t len, std::uint32_t freq)
  { return lgfx::i2c::transactionWrite(port, addr, data, len, freq).has_value(); }
  __attribute__((always_inline)) inline bool i2c_write_read(int port, std::uint16_t addr, const std::uint8_t* wdata,
                             std::size_t wlen, std::uint8_t* rdata, std::size_t rlen,
                             std::uint32_t freq)
  { return lgfx::i2c::transactionWriteRead(port, addr, wdata, wlen, rdata, rlen, freq).has_value(); }
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
  static constexpr unsigned package_classic_q6 = EFUSE_RD_CHIP_VER_PKG_ESP32D0WDQ6;
  static constexpr unsigned package_classic_pico_d4 = EFUSE_RD_CHIP_VER_PKG_ESP32PICOD4;
#endif
  static constexpr unsigned package_classic_picov3 = 6;
  static constexpr unsigned package_s3_qfn56 = 0;
  static constexpr unsigned package_s3_lga56 = 1;
  static constexpr unsigned package_c6_qfn40 = 0;
  static constexpr unsigned package_c6_qfn32 = 1;
  static constexpr unsigned package_p4 = 0;
  __attribute__((always_inline)) inline unsigned package_version() { return lgfx::get_pkg_ver(); }
  __attribute__((always_inline)) inline bool psram_ready()
  {
#if defined (CONFIG_IDF_TARGET_ESP32S3) && defined (CONFIG_SPIRAM_MODE_OCT)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    return esp_psram_is_initialized();
#else
    return esp_spiram_is_initialized();
#endif
#else
    return false;
#endif
  }
  __attribute__((always_inline)) inline unsigned chip_revision()
  {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
    esp_chip_info_t info;
    esp_chip_info(&info);
    return info.revision;
#else
    return 0;
#endif
  }
  __attribute__((always_inline)) inline unsigned flash_capacity_code()
  {
#if defined (CONFIG_IDF_TARGET_ESP32C6)
    return REG_GET_FIELD(EFUSE_RD_MAC_SPI_SYS_4_REG, EFUSE_FLASH_CAP);
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
    return (REG_READ(EFUSE_RD_MAC_SYS2_REG) >> EFUSE_FLASH_CAP_S) & EFUSE_FLASH_CAP_V;
#else
    return 0;
#endif
  }
  __attribute__((always_inline)) inline void module_memory_capacity(unsigned* flash, unsigned* psram)
  {
#if defined (CONFIG_IDF_TARGET_ESP32C5)
    const auto sys2 = REG_READ(EFUSE_RD_MAC_SYS2_REG);
    *flash = (sys2 >> EFUSE_FLASH_CAP_S) & EFUSE_FLASH_CAP_V;
    *psram = (sys2 >> EFUSE_PSRAM_CAP_S) & EFUSE_PSRAM_CAP_V;
#else
    *flash = *psram = 0;
#endif
  }
#if defined (CONFIG_IDF_TARGET_ESP32P4)
  __attribute__((always_inline)) inline std::uint8_t st_touch_address() { return lgfx::Touch_ST7123::default_addr; }
  __attribute__((always_inline)) inline std::uint8_t gt911_touch_address() { return lgfx::Touch_GT911::default_addr_1; }
#endif
#if defined (CONFIG_IDF_TARGET_ESP32C3)
  __attribute__((always_inline)) inline unsigned usb_serial_jtag_frame()
  { return REG_GET_FIELD(USB_SERIAL_JTAG_FRAM_NUM_REG, USB_SERIAL_JTAG_SOF_FRAME_INDEX); }
  __attribute__((always_inline)) inline bool uart_rx_held_high()
  {
    const auto mux = REG_READ(IO_MUX_GPIO20_REG);
    pin_mode(20, pin_mode_t::input_pulldown);
    const bool high = pin_read(20);
    REG_WRITE(IO_MUX_GPIO20_REG, mux);
    return high;
  }
#endif
#if defined (M5GFX_HAS_S3_DEDICATED_GPIO_RELEASE_PROBE)
  static constexpr bool dedicated_gpio_supported = true;
  __attribute__((always_inline)) inline std::uint32_t dedicated_release_read_in()
  {
#if defined (M5GFX_S3_DEDICATED_GPIO_IDF44_CPU_LL)
    return cpu_ll_read_dedic_gpio_in();
#else
    return dedic_gpio_cpu_ll_read_in();
#endif
  }
  __attribute__((always_inline)) inline void dedicated_release_write_mask(std::uint32_t mask, std::uint32_t value)
  {
#if defined (M5GFX_S3_DEDICATED_GPIO_IDF44_CPU_LL)
    cpu_ll_write_dedic_gpio_mask(mask, value);
#else
    dedic_gpio_cpu_ll_write_mask(mask, value);
#endif
  }
  static inline void dedicated_release_channel_masks(dedicated_gpio_handle_t handle,
    std::uint8_t channel_count, std::uint32_t* in_mask, std::uint32_t* out_mask,
    int* in_error, int* out_error)
  {
    const auto bundle = static_cast<dedic_gpio_bundle_handle_t>(handle.value);
#if defined (M5GFX_S3_DEDICATED_GPIO_IDF44_CPU_LL)
    *in_error = dedic_gpio_get_in_mask(bundle, in_mask);
    *out_error = dedic_gpio_get_out_mask(bundle, out_mask);
#else
    std::uint32_t in_offset = 0;
    std::uint32_t out_offset = 0;
    *in_error = dedic_gpio_get_in_offset(bundle, &in_offset);
    *out_error = dedic_gpio_get_out_offset(bundle, &out_offset);
    const std::uint32_t channel_mask = (1u << channel_count) - 1u;
    *in_mask = channel_mask << in_offset;
    *out_mask = channel_mask << out_offset;
#endif
  }
  __attribute__((always_inline)) inline int dedicated_release_core_id()
  {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    return esp_cpu_get_core_id();
#else
    return xPortGetCoreID();
#endif
  }
  __attribute__((always_inline)) inline std::uint32_t dedicated_release_cycle_count() { return cpu_cycles(); }
  __attribute__((always_inline)) inline int dedicated_release_task_core()
  {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 2, 0)
    return xTaskGetCoreID(nullptr);
#else
    return xTaskGetAffinity(nullptr);
#endif
  }
  __attribute__((always_inline)) inline int no_core_affinity() { return static_cast<int>(tskNO_AFFINITY); }
  __attribute__((always_inline)) inline bool dedicated_gpio_in_use()
  { return REG_GET_BIT(SYSTEM_CPU_PERI_CLK_EN_REG, SYSTEM_CLK_EN_DEDICATED_GPIO); }
  __attribute__((always_inline)) inline void pin_weak_pullup_open_drain(int pin)
  {
    pin_mode(pin, pin_mode_t::input_pullup);
    const auto mux = GPIO_PIN_MUX_REG[pin];
    REG_WRITE(mux, (REG_READ(mux) & ~(FUN_PU | FUN_PD | FUN_DRV_M)) | FUN_PU);
    gpio_ll_od_enable(&GPIO, static_cast<gpio_num_t>(pin));
  }
  __attribute__((always_inline)) inline int dedicated_gpio_open(const int* pins, std::uint8_t count, dedicated_gpio_handle_t* handle)
  {
    dedic_gpio_bundle_handle_t bundle = nullptr;
    dedic_gpio_bundle_config_t config = {};
    config.gpio_array = pins;
    config.array_size = count;
    config.flags.in_en = 1;
    config.flags.out_en = 1;
    const auto error = dedic_gpio_new_bundle(&config, &bundle);
    handle->value = bundle;
    return error;
  }
  __attribute__((always_inline)) inline int dedicated_gpio_close(dedicated_gpio_handle_t handle)
  { return dedic_gpio_del_bundle(static_cast<dedic_gpio_bundle_handle_t>(handle.value)); }
  class critical_section_t
  {
    portMUX_TYPE native_ = portMUX_INITIALIZER_UNLOCKED;
  public:
    __attribute__((always_inline)) void enter() { taskENTER_CRITICAL(&native_); }
    __attribute__((always_inline)) void exit() { taskEXIT_CRITICAL(&native_); }
  };
#else
  static constexpr bool dedicated_gpio_supported = false;
  __attribute__((always_inline)) inline std::uint32_t dedicated_release_read_in() { return 0; }
  __attribute__((always_inline)) inline void dedicated_release_write_mask(std::uint32_t, std::uint32_t) {}
  static inline void dedicated_release_channel_masks(dedicated_gpio_handle_t, std::uint8_t,
    std::uint32_t*, std::uint32_t*, int*, int*) {}
  __attribute__((always_inline)) inline int dedicated_release_core_id() { return 0; }
  __attribute__((always_inline)) inline std::uint32_t dedicated_release_cycle_count() { return 0; }
  __attribute__((always_inline)) inline int dedicated_release_task_core() { return 0; }
  __attribute__((always_inline)) inline int no_core_affinity() { return -1; }
  __attribute__((always_inline)) inline bool dedicated_gpio_in_use() { return false; }
  __attribute__((always_inline)) inline void pin_weak_pullup_open_drain(int) {}
  __attribute__((always_inline)) inline int dedicated_gpio_open(const int*, std::uint8_t, dedicated_gpio_handle_t*) { return -1; }
  __attribute__((always_inline)) inline int dedicated_gpio_close(dedicated_gpio_handle_t) { return -1; }
  class critical_section_t { public: void enter() {} void exit() {} };
#endif

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
  __attribute__((always_inline)) inline volatile std::uint32_t& reg(std::uint32_t addr)
  { return *reinterpret_cast<volatile std::uint32_t*>(addr); }

  template <int channel_a, int pin_a, int channel_b, int pin_b, std::uint32_t timeout_us>
  __attribute__((always_inline)) inline bool classic_touch_measure(std::uint16_t* t4, std::uint16_t* t7)
  {
    (void)pin_a;
    (void)pin_b;
    constexpr int channels[] = { channel_a, channel_b };
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
    // DREFH=3, DRANGE=0 caps the charge at 1.2 V. A 2.7 V scan can light the WS2812 array white.
    reg(RTC_IO_TOUCH_CFG_REG) = (cfg & ~((RTC_IO_TOUCH_DREFH << RTC_IO_TOUCH_DREFH_S)
                             | (RTC_IO_TOUCH_DREFL << RTC_IO_TOUCH_DREFL_S)
                             | (RTC_IO_TOUCH_DRANGE << RTC_IO_TOUCH_DRANGE_S)))
                             | (3u << RTC_IO_TOUCH_DREFH_S);
    for (int i = 0; i < 2; ++i)
    { reg(RTC_IO_TOUCH_PAD0_REG + channels[i] * 4) = (pads[i] & ~clear) | set; }
    reg(SENS_SAR_TOUCH_ENABLE_REG) = (enable & ~(SENS_TOUCH_PAD_WORKEN << SENS_TOUCH_PAD_WORKEN_S))
                                   | (((1u << channel_a) | (1u << channel_b)) << SENS_TOUCH_PAD_WORKEN_S);

    reg(SENS_SAR_TOUCH_CTRL2_REG) = reg(SENS_SAR_TOUCH_CTRL2_REG) & ~SENS_TOUCH_START_EN;
    reg(SENS_SAR_TOUCH_CTRL2_REG) = reg(SENS_SAR_TOUCH_CTRL2_REG) | SENS_TOUCH_START_EN;
    const auto started = esp_timer_get_time();
    while (!(reg(SENS_SAR_TOUCH_CTRL2_REG) & SENS_TOUCH_MEAS_DONE))
    {
      if (esp_timer_get_time() - started > timeout_us) { break; }
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
    return ok;
  }

  static IRAM_ATTR std::uint32_t classic_led_cycles() { return cpu_cycles(); }

  // Send black GRB pixels. Flash stalls must not extend a high pulse.
  template <int led_gpio, std::uint16_t pixels>
  static IRAM_ATTR void send_black(std::uint32_t mhz)
  {
    // WS2812C-2020 T0H = 0.22-0.38 us. Aim at the middle: loop overhead adds to the set value,
    // and values near the upper edge lit single LEDs mid-chain depending on the LED supply voltage.
    const std::uint32_t high = mhz * 28 / 100; // 0.28 us
    const std::uint32_t low = mhz * 90 / 100;  // 0.9 us
    for (int bit = 0; bit < pixels * 24; ++bit)
    {
      auto at = classic_led_cycles();
      GPIO.out_w1ts = 1u << led_gpio;
      while (classic_led_cycles() - at < high) {}
      at = classic_led_cycles();
      GPIO.out_w1tc = 1u << led_gpio;
      while (classic_led_cycles() - at < low) {}
    }
  }

  template <int led_gpio, std::uint16_t pixels>
  __attribute__((always_inline)) inline void classic_led_clear()
  {
    gpio_set_pull_mode(static_cast<gpio_num_t>(led_gpio), GPIO_FLOATING);
    GPIO.out_w1tc = 1u << led_gpio; // Set low before enabling output: no high spike on DIN.
    gpio_set_direction(static_cast<gpio_num_t>(led_gpio), GPIO_MODE_OUTPUT);
    rtc_cpu_freq_config_t cpu_freq;
    rtc_clk_cpu_freq_get_config(&cpu_freq); // Same cross-IDF API as lgfx::getApbFrequency.
    const auto mhz = cpu_freq.freq_mhz;
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    esp_rom_delay_us(300);
    taskENTER_CRITICAL(&mux);
    send_black<led_gpio, pixels>(mhz);
    taskEXIT_CRITICAL(&mux);
    esp_rom_delay_us(300);
  }
#endif
}}}
