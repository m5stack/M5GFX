// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "M5GFX.h"

#if defined ( ESP_PLATFORM )

#include <cstdint>
#include <sdkconfig.h>
#include <soc/soc.h>
#include <nvs.h>
#include <esp_log.h>
#if __has_include(<driver/i2c_master.h>)
 #include <driver/i2c_master.h>
#else
 #include <driver/i2c.h>
#endif
#include <soc/efuse_reg.h>
#include <soc/gpio_reg.h>

#include "lgfx/v1/panel/Panel_CO5300.hpp"
#include "lgfx/v1/panel/Panel_ILI9342.hpp"
#include "lgfx/v1/panel/Panel_SSD1306.hpp"
#include "lgfx/v1/panel/Panel_SSD1677.hpp"
#include "lgfx/v1/panel/Panel_TM1680.hpp"
#include "lgfx/v1/panel/Panel_ST7735.hpp"
#include "lgfx/v1/panel/Panel_ST7789.hpp"
#include "lgfx/v1/panel/Panel_GC9A01.hpp"
#include "lgfx/v1/panel/Panel_JD9853.hpp"
#include "lgfx/v1/panel/Panel_GDEW0154M09.hpp"
#include "lgfx/v1/panel/Panel_GDEW0154D67.hpp"
#include "lgfx/v1/panel/Panel_IT8951.hpp"
#include "lgfx/v1/touch/Touch_CHSC6540.hpp"
#include "lgfx/v1/touch/Touch_CSTxxx.hpp"
#include "lgfx/v1/touch/Touch_FT5x06.hpp"
#include "lgfx/v1/touch/Touch_GT911.hpp"

#include "board_detect/m5/setup_sentinels.hpp"

#undef M5GFX_HAS_BOARD_DETECT
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32) || defined (CONFIG_IDF_TARGET_ESP32S3) || defined (CONFIG_IDF_TARGET_ESP32C3) || defined (CONFIG_IDF_TARGET_ESP32C5) || defined (CONFIG_IDF_TARGET_ESP32C6) || defined (CONFIG_IDF_TARGET_ESP32C61) || defined (CONFIG_IDF_TARGET_ESP32P4) || defined (CONFIG_IDF_TARGET_ESP32H2)
#define M5GFX_HAS_BOARD_DETECT
#include "board_detect/board_detect.inl"
#include "board_detect/detect_session.hpp"
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
#include "board_detect/m5/esp32_d0wdq6.inl"
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
#include "board_detect/m5/esp32s3.inl"
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
#include "board_detect/m5/esp32c3.inl"
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
#include "board_detect/m5/esp32c5.inl"
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
#include "board_detect/m5/esp32c6.inl"
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
#include "board_detect/m5/esp32c61.inl"
#elif defined (CONFIG_IDF_TARGET_ESP32H2)
#include "board_detect/m5/esp32h2.inl"
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
#include "board_detect/m5/esp32p4.inl"
#endif

#endif

#if defined ( CONFIG_IDF_TARGET_ESP32P4 )

#include "lgfx/v1/platforms/esp32p4/Bus_DSI.hpp"
#include "lgfx/v1/platforms/esp32p4/Panel_ILI9881C.hpp"
#include "lgfx/v1/platforms/esp32p4/Panel_ST7102.hpp"
#include "lgfx/v1/platforms/esp32p4/Panel_ST7121.hpp"
#include "lgfx/v1/platforms/esp32p4/Panel_ST7123.hpp"
#include "lgfx/v1/platforms/esp32p4/Touch_ST7123.hpp"

#elif defined ( CONFIG_IDF_TARGET_ESP32S3 )

#include "lgfx/v1/panel/Panel_ED2208.hpp"

#if defined (CONFIG_SPIRAM_MODE_OCT)
 #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  #include <esp_psram.h>
 #else
  #include <esp32s3/spiram.h>
 #endif
#endif

// for M5PaperS3
#if defined (CONFIG_ESP32S3_SPIRAM_SUPPORT) && defined (CONFIG_SPIRAM_MODE_OCT)

#include <lgfx/v1/platforms/esp32/Panel_EPD.hpp>

#endif

#elif defined ( CONFIG_IDF_TARGET_ESP32C61 )

#include "lgfx/v1/platforms/esp32/Bus_I2C.hpp"

#endif

#else

#include "lgfx/v1/platforms/sdl/Panel_sdl.hpp"
#include "picture_frame/picture_frame.h"

#endif

namespace m5gfx
{
  static constexpr char LIBRARY_NAME[] = "M5GFX";

  M5GFX* M5GFX::_instance = nullptr;

  M5GFX::M5GFX(void) : LGFX_Device()
  {
    if (_instance == nullptr) _instance = this;
  }

  void M5GFX::setDetectConfig(const detect_config_t& config)
  {
    const bool changed = _detect_config.fallback_board != config.fallback_board
                      || _detect_config.fixed_board != config.fixed_board;
    _detect_config = config;
#if defined (ESP_PLATFORM)
    if (_detect_started && changed)
    { ESP_LOGW(LIBRARY_NAME, "[Autodetect] configuration stored after init; applies to the next detection"); }
#else
    (void)changed;
#endif
  }

#if defined ( ESP_PLATFORM )

  // Candidate I2C probes during detection use a software port.
  static constexpr int_fast16_t probe_i2c_port = -1;

  static constexpr std::uint32_t m5pm1_i2c_freq = 100000;
  static constexpr std::uint8_t m5pm1_i2c_addr = 0x6E; // M5PM1 device i2c address

#if defined (CONFIG_IDF_TARGET_ESP32P4)
  struct Light_M5CoreP4X : public lgfx::ILight
  {
    Light_M5CoreP4X(int port, std::uint8_t addr, std::uint32_t freq)
    : _port(port), _addr(addr), _freq(freq) {}

    bool init(uint8_t brightness) override
    {
      static constexpr uint16_t pwm_freq = 1000;
      const uint8_t freq_data[] = {
        0x25, static_cast<uint8_t>(pwm_freq), static_cast<uint8_t>(pwm_freq >> 8)
      };
      lgfx::i2c::transactionWrite(_port, _addr, freq_data, sizeof(freq_data), _freq);
      lgfx::i2c::bitOn(_port, _addr, 0x06, 1u << 0, _freq);
      setBrightness(brightness);
      return true;
    }

    void setBrightness(uint8_t brightness) override
    {
      uint16_t duty = (brightness << 4) | (brightness >> 4);
      const uint8_t duty_data[] = {
        0x1B, static_cast<uint8_t>(duty), static_cast<uint8_t>(0x80 | (duty >> 8))
      };
      lgfx::i2c::transactionWrite(_port, _addr, duty_data, sizeof(duty_data), _freq);
    }
  private:
    int _port;
    std::uint8_t _addr;
    std::uint32_t _freq;
  };
#endif

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
  static constexpr std::int32_t axp_i2c_freq = 400000;
  static constexpr std::uint_fast8_t axp_i2c_addr = 0x34;
  static constexpr std::int_fast16_t axp_i2c_port = I2C_NUM_1;
  static constexpr std::int_fast16_t axp_i2c_sda = GPIO_NUM_21;
  static constexpr std::int_fast16_t axp_i2c_scl = GPIO_NUM_22;

  struct Panel_M5Stack : public lgfx::Panel_ILI9342
  {
    Panel_M5Stack(void)
    {
      _cfg.pin_cs  = GPIO_NUM_14;
      _cfg.pin_rst = GPIO_NUM_33;
      _cfg.offset_rotation = 0;

      _rotation = 0;
    }

    bool init(bool use_reset) override
    {
      _cfg.invert = lgfx::gpio::command(
        (const uint8_t[]) {
        lgfx::gpio::command_mode_output        , GPIO_NUM_33,
        lgfx::gpio::command_write_low          , GPIO_NUM_33,
        lgfx::gpio::command_mode_input_pulldown, GPIO_NUM_33,
        lgfx::gpio::command_write_high         , GPIO_NUM_33,
        lgfx::gpio::command_read               , GPIO_NUM_33,
        lgfx::gpio::command_mode_output        , GPIO_NUM_33,
        lgfx::gpio::command_end
        });
      return lgfx::Panel_ILI9342::init(use_reset);
    }
  };

  /// The Core2 has shipped with an ILI9342C and, later, an ILI9342E. The two need
  /// different init lists, so the board-specific part is a template over the panel.
  template <class Base>
  struct Panel_M5StackCore2_T : public Base
  {
    Panel_M5StackCore2_T(void)
    {
      this->_cfg.pin_cs = GPIO_NUM_5;
      this->_cfg.invert = true;
      this->_cfg.offset_rotation = 0;

      this->_rotation = 0; // default rotation
    }

    /// LCD reset line on the PMIC: AXP192 reg 0x96 bit1 (GPIO4), AXP2101 reg 0x90 bit1 (ALDO2 enable).
    /// On the AXP2101, reg 0x96 is the BLDO1 (backlight) voltage, so it must not be used there.
    void setResetReg(uint8_t reg) { _rst_reg = reg; }

    void rst_control(bool level) override
    {
      uint8_t bits = level ? 2 : 0;
      uint8_t mask = level ? ~0 : ~2;
      lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, _rst_reg, bits, mask, axp_i2c_freq);
    }

  private:
    uint8_t _rst_reg = 0x96;
  };
  using Panel_M5StackCore2  = Panel_M5StackCore2_T<lgfx::Panel_ILI9342>;
  using Panel_M5StackCore2E = Panel_M5StackCore2_T<lgfx::Panel_ILI9342E>;

  struct Light_M5StackCore2 : public lgfx::ILight
  {
    bool init(std::uint8_t brightness) override
    {
      setBrightness(brightness);
      return true;
    }

    void setBrightness(std::uint8_t brightness) override
    {
      if (brightness)
      {
        brightness = (brightness >> 3) + 72;
        lgfx::i2c::bitOn(axp_i2c_port, axp_i2c_addr, 0x12, 0x02, axp_i2c_freq); // DC3 enable
      }
      else
      {
        lgfx::i2c::bitOff(axp_i2c_port, axp_i2c_addr, 0x12, 0x02, axp_i2c_freq); // DC3 disable
      }
    // AXP192 reg 0x27 = DC3
      lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x27, brightness, 0x80, axp_i2c_freq);
    }
  };

  struct Light_M5StackCore2_AXP2101 : public lgfx::ILight
  {
    bool init(std::uint8_t brightness) override
    {
      setBrightness(brightness);
      return true;
    }

    void setBrightness(std::uint8_t brightness) override
    {
      // BLDO1
      if (brightness)
      {
        brightness = ((brightness + 641) >> 5);
        lgfx::i2c::bitOn(axp_i2c_port, axp_i2c_addr, 0x90, 0x10, axp_i2c_freq); // BLDO1 enable
      }
      else
      {
        lgfx::i2c::bitOff(axp_i2c_port, axp_i2c_addr, 0x90, 0x10, axp_i2c_freq); // BLDO1 disable
      }
    // AXP192 reg 0x96 = BLO1 voltage setting (0.5v ~ 3.5v  100mv/step)
      lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x96, brightness, 0, axp_i2c_freq);
    }
  };

  struct Light_M5Tough : public lgfx::ILight
  {
    bool init(std::uint8_t brightness) override
    {
      setBrightness(brightness);
      return true;
    }

    void setBrightness(std::uint8_t brightness) override
    {
      if (brightness)
      {
        // LDO3 = 1.8V + 0.1V * N (N = 0..15) drives the backlight directly; below 2.5V
        // (N = 7) it does not light. Map 1..255 onto N = 7..15 so that every non-zero
        // value is visible.
        brightness = 7 + (brightness * 9) / 256;
        lgfx::i2c::bitOn(axp_i2c_port, axp_i2c_addr, 0x12, 0x08, axp_i2c_freq); // LDO3 enable
      }
      else
      {
        lgfx::i2c::bitOff(axp_i2c_port, axp_i2c_addr, 0x12, 0x08, axp_i2c_freq); // LDO3 disable
      }
      lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x28, brightness, 0xF0, axp_i2c_freq);
    }
  };

  // Touch_M5Tough は lgfx::Touch_CHSC6540 に統合済み

  struct Panel_M5StickC : public lgfx::Panel_ST7735S
  {
    Panel_M5StickC(void)
    {
      _cfg.invert = true;
      _cfg.pin_cs  = GPIO_NUM_5;
      _cfg.pin_rst = GPIO_NUM_18;
      _cfg.panel_width  = 80;
      _cfg.panel_height = 160;
      _cfg.offset_x     = 26;
      _cfg.offset_y     = 1;
      _cfg.offset_rotation = 2;
    }

  protected:

    const std::uint8_t* getInitCommands(std::uint8_t listno) const override
    {
      static constexpr std::uint8_t list[] = {
          CMD_GAMMASET, 1, 0x08,  // Gamma set, curve 4
          0xFF,0xFF, // end
      };
      if (listno == 2)  return list;
      return Panel_ST7735S::getInitCommands(listno);
    }
  };

  struct Light_M5StickC : public lgfx::ILight
  {
    Light_M5StickC(std::int_fast16_t port = axp_i2c_port,
                   std::int_fast16_t sda = axp_i2c_sda,
                   std::int_fast16_t scl = axp_i2c_scl,
                   std::uint_fast8_t addr = axp_i2c_addr,
                   std::int32_t freq = axp_i2c_freq)
    : _port(port), _sda(sda), _scl(scl), _addr(addr), _freq(freq) {}

    bool init(std::uint8_t brightness) override
    {
      lgfx::i2c::init(_port, _sda, _scl);
      lgfx::i2c::writeRegister8(_port, _addr, 0x12, 0x4D, ~0, _freq);
      setBrightness(brightness);
      return true;
    }

    void setBrightness(std::uint8_t brightness) override
    {
      if (brightness)
      {
        brightness = (((brightness >> 1) + 8) / 13) + 5;
        lgfx::i2c::bitOn(_port, _addr, 0x12, 1 << 2, _freq);
      }
      else
      {
        lgfx::i2c::bitOff(_port, _addr, 0x12, 1 << 2, _freq);
      }
      lgfx::i2c::writeRegister8(_port, _addr, 0x28, brightness << 4, 0x0F, _freq);
    }

  private:
    std::int_fast16_t _port;
    std::int_fast16_t _sda;
    std::int_fast16_t _scl;
    std::uint_fast8_t _addr;
    std::int32_t _freq;
  };

  struct Panel_M5StickCPlus : public lgfx::Panel_ST7789
  {
    Panel_M5StickCPlus(void)
    {
      _cfg.invert = true;
      _cfg.pin_cs  = GPIO_NUM_5;
      _cfg.pin_rst = GPIO_NUM_18;
      _cfg.panel_width  = 135;
      _cfg.panel_height = 240;
      _cfg.offset_x     = 52;
      _cfg.offset_y     = 40;
    }
  };

#elif defined (CONFIG_IDF_TARGET_ESP32S3)

  static constexpr int32_t i2c_freq = 400000;
  static constexpr int_fast16_t aw9523_i2c_addr = 0x58; // AW9523B
  static constexpr int_fast16_t axp_i2c_addr = 0x34;    // AXP2101
  static constexpr int_fast16_t i2c_port = I2C_NUM_1;
  static constexpr int_fast16_t i2c_sda = GPIO_NUM_12;
  static constexpr int_fast16_t i2c_scl = GPIO_NUM_11;

  /// The CoreS3 family has shipped with an ILI9342C and, later, an ILI9342E (see Panel_M5StackCore2_T).
  template <class Base>
  struct Panel_M5StackCoreS3_T : public Base
  {
    Panel_M5StackCoreS3_T(void)
    {
      this->_cfg.pin_cs = GPIO_NUM_3;
      this->_cfg.invert = true;
      this->_cfg.offset_rotation = 0;

      this->_rotation = 0; // default rotation
    }

    void rst_control(bool level) override
    {
      static constexpr uint8_t lcd_rst_bit = 1 << 1; // AW9523B P1_1
      uint8_t bits = level ? lcd_rst_bit : 0;
      uint8_t mask = level ? ~0 : ~ lcd_rst_bit;
      // LCD_RST
      lgfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x03, bits, mask, i2c_freq);
    }

    void cs_control(bool flg) override
    {
      Base::cs_control(flg);
      // CS操作時にGPIO35の役割を切り替える (MISO or D/C);

      // FSPIQ_IN_IDX==FSPI MISO / SIG_GPIO_OUT_IDX==GPIO OUT
      *(volatile uint32_t*)GPIO_FUNC35_OUT_SEL_CFG_REG = flg ? FSPIQ_OUT_IDX : SIG_GPIO_OUT_IDX;

      // CS HIGHの場合はGPIO出力を無効化し、MISO入力として機能させる。
      // CS LOW の場合はGPIO出力を有効化し、D/Cとして機能させる。
      *(volatile uint32_t*)( flg
                             ? GPIO_ENABLE1_W1TC_REG
                             : GPIO_ENABLE1_W1TS_REG
                           ) = 1u << (GPIO_NUM_35 & 31);
    }
  };
  using Panel_M5StackCoreS3  = Panel_M5StackCoreS3_T<lgfx::Panel_ILI9342>;
  using Panel_M5StackCoreS3E = Panel_M5StackCoreS3_T<lgfx::Panel_ILI9342E>;

  struct Touch_M5StackCoreS3 : public lgfx::Touch_FT5x06
  {
    Touch_M5StackCoreS3(void)
    {
      _cfg.pin_int  = GPIO_NUM_21;
      _cfg.pin_sda  = i2c_sda;
      _cfg.pin_scl  = i2c_scl;
      _cfg.i2c_addr = 0x38;
      _cfg.i2c_port = i2c_port;
      _cfg.freq = i2c_freq;
      _cfg.x_min = 0;
      _cfg.x_max = 319;
      _cfg.y_min = 0;
      _cfg.y_max = 239;
      _cfg.bus_shared = false;
    }

    uint_fast8_t getTouchRaw(touch_point_t* tp, uint_fast8_t count) override
    {
      uint_fast8_t res = 0;
      if (!gpio_in(_cfg.pin_int))
      {
        res = lgfx::Touch_FT5x06::getTouchRaw(tp, count);
        if (res == 0)
        { /// clear INT.
          // レジスタ 0x00を読み出すとPort0のINTがクリアされ、レジスタ 0x01を読み出すとPort1のINTがクリアされる。
          lgfx::i2c::readRegister8(i2c_port, aw9523_i2c_addr, 0x00, i2c_freq);
          lgfx::i2c::readRegister8(i2c_port, aw9523_i2c_addr, 0x01, i2c_freq);
        }
      }
      return res;
    }
  };

  struct Light_M5StackCoreS3 : public lgfx::ILight
  {
    bool init(uint8_t brightness) override
    {
      setBrightness(brightness);
      return true;
    }

    void setBrightness(uint8_t brightness) override
    {
      if (brightness)
      {
        brightness = ((brightness + 641) >> 5);
    // AXP2101 reg 0x90 = LDOS ON/OFF control
        lgfx::i2c::bitOn(i2c_port, axp_i2c_addr, 0x90, 0x80, i2c_freq); // DLDO1 enable
      }
      else
      {
        lgfx::i2c::bitOff(i2c_port, axp_i2c_addr, 0x90, 0x80, i2c_freq); // DLDO1 disable
      }
    // AXP2101 reg 0x99 = DLDO1 voltage setting
      lgfx::i2c::writeRegister8(i2c_port, axp_i2c_addr, 0x99, brightness, 0, i2c_freq);
    }
  };

  struct Light_M5StackAtomS3R : public lgfx::ILight
  {
    Light_M5StackAtomS3R(int port, int sda, int scl, std::uint8_t i2c_addr,
                         std::uint32_t freq)
    : _i2c_port(port), _sda(sda), _scl(scl), _i2c_addr(i2c_addr),
      _i2c_freq(freq) {}

    bool init(uint8_t brightness) override
    {
      lgfx::i2c::init(_i2c_port, _sda, _scl);
      lgfx::i2c::writeRegister8(_i2c_port, _i2c_addr, 0x00, 0b01000000, 0, _i2c_freq);
      lgfx::delay(1);
      lgfx::i2c::writeRegister8(_i2c_port, _i2c_addr, 0x08, 0b00000001, 0, _i2c_freq);
      lgfx::i2c::writeRegister8(_i2c_port, _i2c_addr, 0x70, 0b00000000, 0, _i2c_freq);

      setBrightness(brightness);
      return true;
    }

    void setBrightness(uint8_t brightness) override
    {
      lgfx::i2c::writeRegister8(_i2c_port, _i2c_addr, 0x0e, brightness, 0, _i2c_freq);
    }

  private:
    int _i2c_port;
    int _sda;
    int _scl;
    std::uint8_t _i2c_addr;
    std::uint32_t _i2c_freq;
  };

  struct Light_M5StackStamPLC : public lgfx::ILight
  {
    Light_M5StackStamPLC(int port, int sda, int scl, std::uint8_t i2c_addr)
    : _i2c_port(port), _sda(sda), _scl(scl), _i2c_addr(i2c_addr) {}

    bool _is_backlight_inited = false;

    bool init(uint8_t brightness) override
    {
      lgfx::i2c::init(_i2c_port, _sda, _scl);

      // set direction: output
      lgfx::i2c::bitOn(_i2c_port, _i2c_addr, 0x03, 1 << 7, i2c_freq);

      // set pull mode: down
      lgfx::i2c::bitOff(_i2c_port, _i2c_addr, 0x0D, 1 << 7, i2c_freq);

      // set high impedance: off
      lgfx::i2c::bitOff(_i2c_port, _i2c_addr, 0x07, 1 << 7, i2c_freq);

      _is_backlight_inited = true;
      setBrightness(brightness);
      return true;
    }

    void setBrightness(uint8_t brightness) override
    {
      if (!_is_backlight_inited) init(127);

      if (brightness == 0) {
        lgfx::i2c::bitOn(_i2c_port, _i2c_addr, 0x05, 1 << 7, i2c_freq);
      } else {
        lgfx::i2c::bitOff(_i2c_port, _i2c_addr, 0x05, 1 << 7, i2c_freq);
      }
    }

  private:
    int _i2c_port;
    int _sda;
    int _scl;
    std::uint8_t _i2c_addr;
  };

  struct Light_M5PaperMono : public lgfx::ILight
  {
    Light_M5PaperMono(int port = i2c_port, int sda = GPIO_NUM_47, int scl = GPIO_NUM_48,
                      std::uint8_t addr = m5pm1_i2c_addr,
                      std::uint32_t freq = m5pm1_i2c_freq)
    : _port(port), _sda(sda), _scl(scl), _addr(addr), _freq(freq) {}

    bool init(uint8_t brightness) override
    {
      lgfx::i2c::init(_port, _sda, _scl);

      // IO3 push_pull
      lgfx::i2c::bitOff(_port, _addr, 0x13, 1<<3, _freq);

      // IO3 PWM
      lgfx::i2c::bitOn(_port, _addr, 0x16, 0xC0, _freq);

      uint16_t bl_freq = 5000;
      uint8_t write_buf[3];
      write_buf[0] = 0x34; // PWM_FREQ_L addr
      write_buf[1] = bl_freq & 0xFF; // freq_low
      write_buf[2] = (bl_freq >> 8) & 0xFF; // freq_high
      lgfx::i2c::transactionWrite(_port, _addr, write_buf, sizeof(write_buf), _freq);

      setBrightness(brightness);
      return true;
    }

    void setBrightness(uint8_t brightness) override
    {
      if (brightness == 0) {
        lgfx::i2c::writeRegister8(_port, _addr, 0x31, 0, 0, _freq);
      } else {
        // lgfx::i2c::writeRegister8(i2c_port, m5pm1_i2c_addr, 0x31, 0x10 | (brightness >> 4), 0, m5pm1_i2c_freq);
        uint8_t write_buf[3];
        uint32_t br = brightness * brightness;
        write_buf[0] = 0x30; // PWM0_L addr
        write_buf[1] = (br >> 4) & 0xFF;
        write_buf[2] = (br >> 12) | 0x10;
        lgfx::i2c::transactionWrite(_port, _addr, write_buf, sizeof(write_buf), _freq);
      }
    }

  private:
    int _port;
    int _sda;
    int _scl;
    std::uint8_t _addr;
    std::uint32_t _freq;
  };

  struct Light_M5ChainCaptain : public lgfx::ILight
  {
    Light_M5ChainCaptain(int port, int sda, int scl, std::uint8_t addr, std::uint32_t freq)
    : _port(port), _sda(sda), _scl(scl), _addr(addr), _freq(freq) {}

    bool init(uint8_t brightness) override
    {
      lgfx::i2c::init(_port, _sda, _scl);

      // Disable M5IOE1 I2C idle sleep and configure IO11/PWM_CH3 for the backlight.
      lgfx::i2c::writeRegister8(_port, _addr, 0x23, 0x00, 0, _freq);
      lgfx::i2c::bitOff(_port, _addr, 0x14, 1u << 2, _freq);
      lgfx::i2c::bitOn( _port, _addr, 0x04, 1u << 2, _freq);

      const uint16_t pwm_freq = 1000;
      uint8_t freq_buf[] = { 0x25, (uint8_t)pwm_freq, (uint8_t)(pwm_freq >> 8) };
      lgfx::i2c::transactionWrite(_port, _addr, freq_buf, sizeof(freq_buf), _freq);

      setBrightness(brightness);
      return true;
    }

    void setBrightness(uint8_t brightness) override
    {
      const uint32_t squared = (uint32_t)brightness * brightness;
      const uint16_t duty = (squared * 4095u + 32512u) / 65025u;
      uint8_t duty_buf[] = {
        0x1F, // PWM_CH3 duty low register
        (uint8_t)duty,
        (uint8_t)((duty >> 8) | 0x80), // normal polarity, PWM enabled
      };
      lgfx::i2c::transactionWrite(_port, _addr, duty_buf, sizeof(duty_buf), _freq);
    }

  private:
    int _port;
    int _sda;
    int _scl;
    std::uint8_t _addr;
    std::uint32_t _freq;
  };

  struct Panel_StopWatch : public lgfx::Panel_CO5300
  {
    Panel_StopWatch(void)
    {
      _cfg.memory_width  = _cfg.panel_width  = 480;
      _cfg.memory_height = _cfg.panel_height = 480;
    }

    const uint8_t* getInitCommands(uint8_t listno) const override
    {
      static constexpr uint8_t list0[] = {
        0x11, 0+CMD_INIT_DELAY, 150, // Sleep out
        0xC4, 1, 0x80,
        0x35, 1, 0x80,
        0x44, 2, 0x01, 0xD2, // Tear Effect Line = 0x1D2 == 466
        0x53, 1, 0x20,
        0x20, 0,
        0x36, 1, 0,
        0x51, 1, 0xA0,
        0x29, 0,
        0xff, 0xff // end
      };
      switch (listno) {
        case 0: return list0;
        default: return nullptr;
      }
    }
  };

#elif defined (CONFIG_IDF_TARGET_ESP32C5)

  struct Light_M5ToughC5 : public lgfx::ILight
  {
    Light_M5ToughC5(int_fast16_t port, std::uint8_t addr, std::uint32_t freq)
    : _port(port), _addr(addr), _freq(freq) {}
    // LCD backlight = M5IOE1 PIN10, driven by the expander's PWM channel 4.
    // Registers: 0x21/0x22 = PWM4 duty (12bit, H[7]=enable), 0x25/0x26 = shared PWM frequency (Hz).
    bool init(uint8_t brightness) override
    {
      static constexpr uint8_t freq_1khz[] = { 0x25, 0xE8, 0x03 };
      lgfx::i2c::transactionWrite(_port, _addr, freq_1khz, sizeof(freq_1khz), _freq);
      lgfx::i2c::bitOn(_port, _addr, 0x04, 0x02, _freq); // PIN10 output mode
      setBrightness(brightness);
      return true;
    }

    void writeDuty(uint_fast16_t duty12)
    {
      uint8_t buf[] = { 0x21, (uint8_t)duty12, (uint8_t)(0x80 | (duty12 >> 8)) };
      lgfx::i2c::transactionWrite(_port, _addr, buf, sizeof(buf), _freq);
    }

    void setBrightness(uint8_t brightness) override
    {
      // gamma 2.0: perceived brightness tracks the setting instead of the raw duty
      uint_fast16_t duty12 = ((uint32_t)brightness * brightness * 4095u + 32512u) / 65025u;
      if (brightness && duty12 == 0) { duty12 = 1; }
      writeDuty(duty12);
    }
  private:
    int_fast16_t _port;
    std::uint8_t _addr;
    std::uint32_t _freq;
  };

#elif defined (CONFIG_IDF_TARGET_ESP32C6)

  struct Light_ArduinoNessoN1 : public lgfx::ILight
  {
    Light_ArduinoNessoN1(int_fast16_t port, std::uint8_t addr, std::uint32_t freq)
    : _port(port), _addr(addr), _freq(freq) {}

    bool init(uint8_t brightness) override
    {
      setBrightness(brightness);
      return true;
    }

    void setBrightness(uint8_t brightness) override
    {
      if (brightness) {
        lgfx::i2c::bitOn(_port, _addr, 0x05, 1 << 6, _freq);
      } else {
        lgfx::i2c::bitOff(_port, _addr, 0x05, 1 << 6, _freq);
      }
    }

  private:
    int_fast16_t _port;
    std::uint8_t _addr;
    std::uint32_t _freq;
  };

#endif

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32) || defined (CONFIG_IDF_TARGET_ESP32S3)
  /// The ILI9342E needs about 1 us after RAMRD before the first pixel is valid. At the 16 MHz
  /// read clock used here the eight dummy clocks are not enough and it returns one stale pixel
  /// (24 bits) first, so that pixel is skipped as well; at 8 MHz and below the eight clocks
  /// suffice. Measured on a Core2 and a CoreS3 with the E, not on the C (which keeps 8).
  static void _set_ili9342e_read(lgfx::Panel_ILI9342* p, std::uint32_t freq_read)
  {
    auto cfg = p->config();
    cfg.dummy_read_pixel = (freq_read > 8000000) ? 32 : 8;
    p->config(cfg);
  }


#endif

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
#include "board_detect/m5/esp32_d0wdq6_setup.inl"
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
#include "board_detect/m5/esp32s3_setup.inl"
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
#include "board_detect/m5/esp32c3_setup.inl"
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
#include "board_detect/m5/esp32c5_setup.inl"
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
#include "board_detect/m5/esp32c6_setup.inl"
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
#include "board_detect/m5/esp32c61_setup.inl"
#elif defined (CONFIG_IDF_TARGET_ESP32H2)
#include "board_detect/m5/esp32h2_setup.inl"
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
#include "board_detect/m5/esp32p4_setup.inl"
#endif

#if defined (M5GFX_HAS_BOARD_DETECT)
#include "board_detect/detection_package.inl"


#include "board_detect/detection_integration.inl"

  bool M5GFX::_adopt_detected_parts(lgfx::IBus* bus, lgfx::Panel_Device* panel_part,
                                    lgfx::ILight* light, lgfx::ITouch* touch)
  {
    _bus_last.reset(bus);
    _panel_last.reset(panel_part);
    _light_last.reset(light);
    _touch_last.reset(touch);
    if (light != nullptr && panel_part != nullptr) { panel_part->setLight(light); }
    panel(_panel_last.get());
    return true;
  }
#endif

  void M5GFX::_set_backlight(lgfx::ILight* bl)
  {
//  if (_light_last) { delete _light_last; }
    _light_last.reset(bl);
    _panel_last->setLight(bl);
  }

  void M5GFX::_set_pwm_backlight(std::int16_t pin, std::uint8_t ch, std::uint32_t freq, bool invert, uint8_t offset)
  {
    auto bl = new lgfx::Light_PWM();
    auto cfg = bl->config();
    cfg.pin_bl = pin;
    cfg.freq   = freq;
    cfg.pwm_channel = ch;
    cfg.offset = offset;
    cfg.invert = invert;
    bl->config(cfg);
    _set_backlight(bl);
  }

  bool M5GFX::init_impl(bool use_reset, bool use_clear)
  {
    _detect_started = true;
    _board_candidate = board_t::board_unknown;
    if (getBoard() != board_t::board_unknown)
    {
      return true;
    }

    _fixed_board = board_t::board_unknown;
    if (_detect_config.fixed_board != board_t::board_unknown)
    {
#if defined (M5GFX_HAS_BOARD_DETECT)
      const auto* entry = select_fixed_board(_detect_config.fixed_board);
      if (entry != nullptr)
      {
        // Acceptance is independent of display construction; a failed init may retry.
        _fixed_board = _detect_config.fixed_board;
        const auto outcome = run_fixed_detection_session(*entry, use_reset,
          [this](board_detect::m5::display_parts_t& parts, board_t setup_board)
          {
            return !reject_detected_setup(setup_board)
                && _adopt_detected_parts(parts.bus, parts.panel, parts.light, parts.touch);
          });
        if (!outcome.setup_succeeded) { return false; }
        _board = _fixed_board;
        return _finish_detected_init(use_clear);
      }
#endif
      ESP_LOGW(LIBRARY_NAME, "Fixed board:%u is not supported by this chip",
               static_cast<unsigned>(_detect_config.fixed_board));
      return false;
    }

#if defined (M5GFX_HAS_BOARD_DETECT)
    static constexpr char NVS_KEY[] = "AUTODETECT";
    std::uint32_t nvs_board = 0;
    std::uint32_t nvs_handle = 0;
    if (0 == nvs_open(LIBRARY_NAME, NVS_READONLY, &nvs_handle))
    {
      nvs_get_u32(nvs_handle, NVS_KEY, static_cast<uint32_t*>(&nvs_board));
      nvs_close(nvs_handle);
      ESP_LOGI(LIBRARY_NAME, "[Autodetect] load from NVS : board:%d", (int)nvs_board);
    }

    if (0 == nvs_board)
    {
#if defined ( M5GFX_BOARD )

      nvs_board = M5GFX_BOARD;

#elif defined ( ARDUINO_M5STACK_CORE_ESP32 ) || defined ( ARDUINO_M5STACK_FIRE ) || defined ( ARDUINO_M5Stack_Core_ESP32 )

      nvs_board = board_t::board_M5Stack;

#elif defined ( ARDUINO_M5STACK_CORE2 ) || defined ( ARDUINO_M5STACK_Core2 )

      nvs_board = board_t::board_M5StackCore2;

#elif defined ( ARDUINO_M5STICK_C ) || defined ( ARDUINO_M5Stick_C )

      nvs_board = board_t::board_M5StickC;

#elif defined ( ARDUINO_M5STICK_C_PLUS ) || defined ( ARDUINO_M5Stick_C_Plus )

      nvs_board = board_t::board_M5StickCPlus;

#elif defined ( ARDUINO_M5STACK_COREINK ) || defined ( ARDUINO_M5Stack_CoreInk )

      nvs_board = board_t::board_M5StackCoreInk;

#elif defined ( ARDUINO_M5STACK_PAPER ) || defined ( ARDUINO_M5STACK_Paper )

      nvs_board = board_t::board_M5Paper;

#elif defined ( ARDUINO_M5STACK_TOUGH )

      nvs_board = board_t::board_M5Tough;

#elif defined ( ARDUINO_M5STACK_ATOM ) || defined ( ARDUINO_M5Stack_ATOM )

      nvs_board = board_t::board_M5Atom;

//#elif defined ( ARDUINO_M5STACK_TIMER_CAM ) || defined ( ARDUINO_M5Stack_Timer_CAM )
#endif
    }

    board_detect::detect_request_t request;
    request.hint = nvs_board <= UINT16_MAX
                 ? static_cast<board_detect::board_id_t>(nvs_board) : board_detect::board_id_unknown;
    request.preferred = static_cast<board_detect::board_id_t>(_detect_config.fallback_board);
    request.allow_reset = use_reset;
    request.max_attempts = 5;
    auto package = select_detection_package();
    request.conditional_pins_unavailable = package.conditional_pins_unavailable;
    const auto outcome = board_detect::run_detection_session(
      request,
      [this, &package](const board_detect::detect_request_t& attempt, bool final_attempt)
      {
        if (!package.direct_setup) { panel(nullptr); }
        auto result = run_detection_attempt(package.detectors, attempt, final_attempt,
          [this](board_detect::m5::display_parts_t& parts, board_t setup_board)
          {
            return !reject_detected_setup(setup_board)
                && _adopt_detected_parts(parts.bus, parts.panel, parts.light, parts.touch);
          });
        if (!package.direct_setup || !result.setup_succeeded)
        {
          if (package.direct_setup) { panel(nullptr); }
          panel(_panel_last.get());
        }
        return result;
      });
    const auto board = outcome.setup_succeeded
                     ? static_cast<board_t>(outcome.result.def->id) : board_t::board_unknown;
    if (outcome.result.candidate != nullptr)
    { _board_candidate = static_cast<board_t>(outcome.result.candidate->id); }
    _board = board;
    if (board != board_t::board_unknown) { _board_candidate = board_t::board_unknown; }

#if defined ( ARDUINO_M5STACK_ATOM ) || defined ( ARDUINO_M5Stack_ATOM )

    if (board == board_t::board_unknown || board == board_t::board_M5Atom)
    {
      return false;
    }

#endif

    // Unresolved members and failed adoption must never replace a saved hint.
    if (board_detect::should_persist_detection(outcome, nvs_board)) {
      if (0 == nvs_open(LIBRARY_NAME, NVS_READWRITE, &nvs_handle)) {
        ESP_LOGI(LIBRARY_NAME, "[Autodetect] save to NVS : board:%d", (int)board);
        nvs_set_u32(nvs_handle, NVS_KEY, board);
        nvs_close(nvs_handle);
      }
    }

#else
    // Unsupported chips retain the legacy unknown-board result; build hints are ignored.
    (void)use_reset;
#endif
    return _finish_detected_init(use_clear);
  }

  bool M5GFX::_finish_detected_init(bool use_clear)
  {
    // The new path performs every permitted reset in prepare(). Construction
    // and panel initialisation never pulse reset a second time.
    if (false == LGFX_Device::init_impl(false, use_clear)) {
      return false;
    }

#if defined (CONFIG_IDF_TARGET_ESP32S3)
    switch (_board) {
    default:
      break;

    case board_t::board_M5StopWatch:
      auto p = reinterpret_cast<Panel_StopWatch*>(_panel_last.get());
      if (p->initPanelFb() ) {
        auto fbPanel = p->getPanelFb();
        if( fbPanel ) {
          fbPanel->setBus(_bus_last.get());
          fbPanel->setAutoDisplay(true);
          setPanel(fbPanel);
        }
      }
      break;
    }
#endif

    return true;
  }

  board_t M5GFX::autodetect(bool use_reset, board_t board,
                            bool final_attempt, bool* transient_fallback,
                            bool* no_signature, board_t* candidate_board)
  {
    panel(nullptr);
#if defined (M5GFX_HAS_BOARD_DETECT)
    const auto package = select_detection_package(true);
    board_detect::detect_request_t request;
    request.hint = static_cast<std::uint32_t>(board) <= UINT16_MAX
                 ? static_cast<board_detect::board_id_t>(board) : board_detect::board_id_unknown;
    request.preferred = static_cast<board_detect::board_id_t>(_detect_config.fallback_board);
    request.allow_reset = use_reset;
    request.conditional_pins_unavailable = package.conditional_pins_unavailable;
    const auto outcome = board_detect::run_detection_session(request,
      [this, &package, final_attempt](const board_detect::detect_request_t& attempt, bool)
      {
        return run_detection_attempt(package.detectors, attempt, final_attempt,
          [this](board_detect::m5::display_parts_t& parts, board_t setup_board)
          {
            return !reject_detected_setup(setup_board)
                && _adopt_detected_parts(parts.bus, parts.panel, parts.light, parts.touch);
          });
      });
    if (no_signature != nullptr)
    { *no_signature = outcome.reason == board_detect::fail_reason_t::no_signature; }
    if (candidate_board != nullptr && *candidate_board == board_t::board_unknown
     && outcome.result.candidate != nullptr)
    { *candidate_board = static_cast<board_t>(outcome.result.candidate->id); }
    if (outcome.setup_succeeded && transient_fallback != nullptr)
    { *transient_fallback = outcome.result.provisional; }
    panel(_panel_last.get());
    return outcome.setup_succeeded ? static_cast<board_t>(outcome.result.def->id)
                                   : board_t::board_unknown;
#else
    (void)use_reset;
    (void)board;
    (void)final_attempt;
    (void)transient_fallback;
    (void)no_signature;
    (void)candidate_board;
    panel(_panel_last.get());
    return board_t::board_unknown;
#endif
  }

#else

  bool M5GFX::init_impl(bool use_reset, bool use_clear)
  {
    _detect_started = true;
    // SDL has no detection pass; a fixed board overrides its build-time model.
    _fixed_board = _detect_config.fixed_board;
    board_t b = _fixed_board;
#if defined (M5GFX_BOARD)
    if (b == board_t::board_unknown) { b = M5GFX_BOARD; }
#endif
    _board = autodetect(use_reset, b);
    return LGFX_Device::init_impl(use_reset, use_clear);
  }

  board_t M5GFX::autodetect(bool use_reset, board_t board,
                            bool final_attempt, bool* transient_fallback,
                            bool* no_signature, board_t* candidate_board)
  {
    (void)use_reset;
    (void)final_attempt;
    (void)transient_fallback;
    (void)no_signature;
    (void)candidate_board;
    auto p = new Panel_sdl();
    _panel_last.reset(p);
    auto pnl_cfg = p->config();

    int_fast16_t w = 320;
    int_fast16_t h = 240;
    int_fast16_t r = 0;
    int scale = 1;
#if defined (M5GFX_SCALE)
  #if M5GFX_SCALE > 1
    scale = M5GFX_SCALE;
#endif
#endif

    if (board == 0) {
      board = board_M5Stack;
    }

    const char* title;
    switch (board) {
    case board_M5Stack:        title = "M5Stack";        break;
    case board_M5StackCore2:   title = "M5StackCore2";   break;
    case board_M5StackCoreS3:  title = "M5StackCoreS3";  break;
    case board_M5StackCoreS3SE:title = "M5StackCoreS3SE";break;
    case board_M5StackChan:    title = "M5StackChan";    break;
    case board_M5StickC:       title = "M5StickC";       break;
    case board_M5StickCPlus:   title = "M5StickCPlus";   break;
    case board_M5StickCPlus2:  title = "M5StickCPlus2";  break;
    case board_M5StickS3:      title = "M5StickS3";      break;
    case board_M5StackCoreInk: title = "M5StackCoreInk"; break;
    case board_M5Paper:        title = "M5Paper";        break;
    case board_M5PaperS3:      title = "M5PaperS3";      break;
    case board_M5PaperDIY:     title = "M5PaperDIY";     break;
    case board_M5PaperColor:   title = "M5PaperColor";   break;
    case board_M5PaperMono:    title = "M5PaperMono";    break;
    case board_M5Tough:        title = "M5Tough";        break;
    case board_M5ToughC5:      title = "M5ToughC5";      break;
    case board_M5StampC5:      title = "M5StampC5";      break;
    case board_M5CoreMatrix:   title = "M5CoreMatrix";   break;
    case board_M5Station:      title = "M5Station";      break;
    case board_M5StopWatch:    title = "M5StopWatch";    break;
    case board_M5ChainCaptain: title = "M5ChainCaptain"; break;
    case board_M5CoreP4X:      title = "M5CoreP4X";      break;
    case board_M5AtomS3:       title = "M5AtomS3";       break;
    case board_M5AtomS3R:      title = "M5AtomS3R";      break;
    case board_M5Dial:         title = "M5Dial";         break;
    case board_M5Cardputer:    title = "M5Cardputer";    break;
    case board_M5CardputerADV: title = "M5CardputerADV"; break;
    case board_M5DinMeter:     title = "M5DinMeter";     break;
    case board_M5AirQ:         title = "M5AirQ";         break;
    case board_M5VAMeter:      title = "M5VAMeter";      break;
    case board_M5StamPLC:      title = "M5StamPLC";      break;
    case board_M5Tab5:         title = "M5Tab5";         break;
    case board_M5Tab5X:        title = "M5Tab5X";        break;
    case board_M5UnitPoEP4:    title = "M5UnitPoEP4";    break;
    case board_ArduinoNessoN1: title = "ArduinoNessoN1"; break;
    default:                   title = "M5GFX";          break;
    }
    p->setWindowTitle(title);

    switch (board) {
    case board_M5AtomS3:
    case board_M5AtomS3R:
      w = 128;
      h = 128;
      break;

    case board_M5Paper:
    case board_M5PaperS3:
    case board_M5PaperDIY:
      w = 960;
      h = 540;
      pnl_cfg.offset_rotation = 3;
      p->setColorDepth(lgfx::color_depth_t::grayscale_8bit);
      r = 0;
      break;

    case board_M5PaperColor:
      w = 400;
      h = 600;
      pnl_cfg.offset_rotation = 0;
      r = 0;
      break;

    case board_M5PaperMono:
      w = 800;
      h = 480;
      pnl_cfg.offset_rotation = 3;
      p->setColorDepth(lgfx::color_depth_t::grayscale_8bit);
      r = 0;
      break;

    case board_M5StackCoreInk:
    case board_M5AirQ:
      w = 200;
      h = 200;
      p->setColorDepth(lgfx::color_depth_t::grayscale_8bit);
      break;

    case board_M5StickC:
      w = 80;
      h = 160;
      break;

    case board_M5Station:
    case board_M5Cardputer:
    case board_M5CardputerADV:
    case board_M5DinMeter:
      w = 240;
      h = 135;
      pnl_cfg.offset_rotation = 0;
      r = 0;
      break;

    case board_M5StickCPlus:
    case board_M5StickCPlus2:
    case board_M5StickS3:
    case board_ArduinoNessoN1:
      w = 135;
      h = 240;
      break;

    case board_M5StamPLC:
      w = 135;
      h = 240;
      pnl_cfg.offset_rotation = 1;
      r = 0;
      break;

    case board_M5StackCore2:
      pnl_cfg.offset_rotation = 0;
      r = 0;
      break;
    case board_M5Stack:
    case board_M5StackCoreS3:
    case board_M5StackCoreS3SE:
    case board_M5StackChan:
      pnl_cfg.offset_rotation = 0;
      r = 0;
      break;

    case board_M5Dial:
    case board_M5ChainCaptain:
      w = 240;
      h = 240;
      break;

      case board_M5VAMeter:
      w = 240;
      h = 240;
      break;

    case board_M5Tab5:
    case board_M5Tab5X:
      w = 720;
      h = 1280;
      break;

    default:
      break;
    }

#if defined (M5GFX_SHORTCUT_MOD)
    p->setShortcutKeymod(M5GFX_SHORTCUT_MOD);
#endif

#if defined (M5GFX_SHOW_FRAME)
    auto pf = getPictureFrame(board);
    if (pf) {
      p->setFrameImage(pf->img, pf->w, pf->h, pf->x, pf->y);
    }
#endif

    pnl_cfg.memory_width = w;
    pnl_cfg.panel_width = w;
    pnl_cfg.memory_height = h;
    pnl_cfg.panel_height = h;
    pnl_cfg.bus_shared = false;
    p->config(pnl_cfg);
    p->setScaling(scale, scale);

#if defined (M5GFX_ROTATION)
    p->setFrameRotation(M5GFX_ROTATION);
#endif

    p->setRotation(r);

    auto t = new lgfx::Touch_sdl();
    _touch_last.reset(t);
    {
      auto cfg = t->config();
      cfg.x_min = 0;
      cfg.x_max = w - 1;
      cfg.y_min = 0;
      cfg.y_max = h - 1;
      cfg.bus_shared = false;
      t->config(cfg);
      p->touch(t);
      //    float affine[6] = { 1, 0, 0, 0, 1, 0 };
      //    p->setCalibrateAffine(affine);
    }

    panel(_panel_last.get());

    return board;
  }


#endif  /// end of if defined (ESP_PLATFORM)

  void M5GFX::progressBar(int x, int y, int w, int h, uint8_t val)
  {
    drawRect(x, y, w, h, 0x09F1);
    fillRect(x + 1, y + 1, w * (((float)val) / 100.0f), h - 1, 0x09F1);
  }

  void M5GFX::pushState(void)
  {
    DisplayState s;
    s.gfxFont = _font;
    s.style = _text_style;
    s.metrics = _font_metrics;
    s.cursor_x = _cursor_x;
    s.cursor_y = _cursor_y;
    _displayStateStack.push_back(s);
  }

  void M5GFX::popState(void)
  {
    if (_displayStateStack.empty()) return;
    DisplayState s = _displayStateStack.back();
    _displayStateStack.pop_back();
    _font = s.gfxFont;
    _text_style = s.style;
    _font_metrics = s.metrics;
    _cursor_x = s.cursor_x;
    _cursor_y = s.cursor_y;
  }
}
