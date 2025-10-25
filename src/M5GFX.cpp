// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "M5GFX.h"

#if defined ( ESP_PLATFORM )

#include <cstdint>
#include <sdkconfig.h>
#include <nvs.h>
#include <esp_log.h>
#include <driver/i2c.h>
#include <soc/efuse_reg.h>
#include <soc/gpio_reg.h>

#include "lgfx/v1/panel/Panel_ILI9342.hpp"
#include "lgfx/v1/panel/Panel_SSD1306.hpp"
#include "lgfx/v1/panel/Panel_ST7735.hpp"
#include "lgfx/v1/panel/Panel_ST7789.hpp"
#include "lgfx/v1/panel/Panel_GC9A01.hpp"
#include "lgfx/v1/panel/Panel_GDEW0154M09.hpp"
#include "lgfx/v1/panel/Panel_GDEW0154D67.hpp"
#include "lgfx/v1/panel/Panel_IT8951.hpp"
#include "lgfx/v1/touch/Touch_CST816S.hpp"
#include "lgfx/v1/touch/Touch_FT5x06.hpp"
#include "lgfx/v1/touch/Touch_GT911.hpp"

#if defined ( CONFIG_IDF_TARGET_ESP32P4 )

#include "lgfx/v1/platforms/esp32p4/Bus_DSI.hpp"
#include "lgfx/v1/platforms/esp32p4/Panel_ILI9881C.hpp"
#include "lgfx/v1/platforms/esp32p4/Panel_ST7123.hpp"
#include "lgfx/v1/platforms/esp32p4/Touch_ST7123.hpp"

static constexpr int_fast16_t in_i2c_port = I2C_NUM_1;

#elif defined ( CONFIG_IDF_TARGET_ESP32S3 )

// for M5PaperS3
#if defined (CONFIG_ESP32S3_SPIRAM_SUPPORT) && defined (CONFIG_SPIRAM_MODE_OCT)

#include <lgfx/v1/platforms/esp32/Panel_EPD.hpp>

#endif
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

#if defined ( ESP_PLATFORM )

  void i2c_write_register8_array(int_fast16_t i2c_port, uint_fast8_t i2c_addr, const uint8_t* reg_data_mask, uint32_t freq)
  {
    while (reg_data_mask[0] != 0xFF || reg_data_mask[1] != 0xFF || reg_data_mask[2] != 0xFF)
    {
      lgfx::i2c::writeRegister8(i2c_port, i2c_addr, reg_data_mask[0], reg_data_mask[1], reg_data_mask[2], freq);
      reg_data_mask += 3;
    }
  }

  static constexpr std::uint_fast8_t pi4io1_i2c_addr = 0x43;
  static constexpr std::uint_fast8_t pi4io2_i2c_addr = 0x44;
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
      _cfg.offset_rotation = 3;

      _rotation = 1;
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

  struct Panel_M5StackCore2 : public lgfx::Panel_ILI9342
  {
    Panel_M5StackCore2(void)
    {
      _cfg.pin_cs = GPIO_NUM_5;
      _cfg.invert = true;
      _cfg.offset_rotation = 3;

      _rotation = 1; // default rotation
    }

    void rst_control(bool level) override
    {
      uint8_t bits = level ? 2 : 0;
      uint8_t mask = level ? ~0 : ~2;
      // AXP192 reg 0x96 = GPIO3&4 control
      lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x96, bits, mask, axp_i2c_freq);
    }
  };

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
        if (brightness > 4)
        {
          brightness = (brightness / 24) + 5;
        }
        lgfx::i2c::bitOn(axp_i2c_port, axp_i2c_addr, 0x12, 0x08, axp_i2c_freq); // LDO3 enable
      }
      else
      {
        lgfx::i2c::bitOff(axp_i2c_port, axp_i2c_addr, 0x12, 0x08, axp_i2c_freq); // LDO3 disable
      }
      lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x28, brightness, 0xF0, axp_i2c_freq);
    }
  };

  struct Touch_M5Tough : public lgfx::ITouch
  {
    void wakeup(void) override {}
    void sleep(void) override {}

    bool init(void) override
    {
      _inited = false;
      if (isSPI()) return false;

      if (_cfg.pin_int >= 0)
      {
        lgfx::pinMode(_cfg.pin_int, lgfx::pin_mode_t::input);
      }
      _inited = lgfx::i2c::init(_cfg.i2c_port, _cfg.pin_sda, _cfg.pin_scl).has_value();
      static constexpr uint8_t irq_modechange_cmd[] = { 0x5a, 0x5a };  /// (INT mode change)
      lgfx::i2c::transactionWrite(_cfg.i2c_port, _cfg.i2c_addr, irq_modechange_cmd, 2);

      return _inited;
    }

    std::uint_fast8_t getTouchRaw(touch_point_t *tp, std::uint_fast8_t count) override
    {
      if (tp) tp->size = 0;
      if (!_inited || count == 0) return 0;
      if (count > 2) count = 2; // max 2 point.
      if (_cfg.pin_int >= 0)
      {
        if (gpio_in(_cfg.pin_int)) return 0;
      }

      std::size_t len = 3 + count * 6;
      std::uint8_t buf[2][len];
      std::int32_t retry = 5;
      bool flip = false;
      std::uint8_t* tmp;
      for (;;)
      {
        tmp = buf[flip];
        memset(tmp, 0, len);
        if (lgfx::i2c::beginTransaction(_cfg.i2c_port, _cfg.i2c_addr, _cfg.freq, false))
        {
          static constexpr std::uint8_t reg_number = 2;
          if (lgfx::i2c::writeBytes(_cfg.i2c_port, &reg_number, 1)
          && lgfx::i2c::restart(_cfg.i2c_port, _cfg.i2c_addr, _cfg.freq, true)
          && lgfx::i2c::readBytes(_cfg.i2c_port, tmp, 1)
          && (tmp[0] != 0))
          {
            flip = !flip;
            std::size_t points = std::min<std::uint_fast8_t>(count, tmp[0]);
            if (points && lgfx::i2c::readBytes(_cfg.i2c_port, &tmp[1], points * 6 - 2))
            {}
          }
          if (lgfx::i2c::endTransaction(_cfg.i2c_port)) {}
          if (tmp[0] == 0 || memcmp(buf[0], buf[1], len) == 0) break;
        }
        if (0 == --retry) return 0;
      }
      if (count > tmp[0]) count = tmp[0];
    
      for (std::size_t idx = 0; idx < count; ++idx)
      {
        auto data = &tmp[1 + idx * 6];
        tp[idx].size = 1;
        tp[idx].x = (data[0] & 0x0F) << 8 | data[1];
        tp[idx].y = (data[2] & 0x0F) << 8 | data[3];
        tp[idx].id = idx;
      }
      return count;
    }
  };

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
    bool init(std::uint8_t brightness) override
    {
      lgfx::i2c::init(axp_i2c_port, axp_i2c_sda, axp_i2c_scl);
      lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x12, 0x4D, ~0, axp_i2c_freq);
      setBrightness(brightness);
      return true;
    }

    void setBrightness(std::uint8_t brightness) override
    {
      if (brightness)
      {
        brightness = (((brightness >> 1) + 8) / 13) + 5;
        lgfx::i2c::bitOn(axp_i2c_port, axp_i2c_addr, 0x12, 1 << 2, axp_i2c_freq);
      }
      else
      {
        lgfx::i2c::bitOff(axp_i2c_port, axp_i2c_addr, 0x12, 1 << 2, axp_i2c_freq);
      }
      lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x28, brightness << 4, 0x0F, axp_i2c_freq);
    }
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
  static constexpr int_fast16_t aw9523_i2c_addr = 0x58;  // AW9523B
  static constexpr int_fast16_t axp_i2c_addr = 0x34;     // AXP2101
  static constexpr int_fast16_t gc0308_i2c_addr = 0x21;  // GC0308
  static constexpr int_fast16_t i2c_port = I2C_NUM_1;
  static constexpr int_fast16_t i2c_sda = GPIO_NUM_12;
  static constexpr int_fast16_t i2c_scl = GPIO_NUM_11;

  struct Panel_M5StackCoreS3 : public lgfx::Panel_ILI9342
  {
    Panel_M5StackCoreS3(void)
    {
      _cfg.pin_cs = GPIO_NUM_3;
      _cfg.invert = true;
      _cfg.offset_rotation = 3;

      _rotation = 1; // default rotation
    }

    void rst_control(bool level) override
    {
      uint8_t bits = level ? (1<<5) : 0;
      uint8_t mask = level ? ~0 : ~(1<<5);
      // LCD_RST
      lgfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x03, bits, mask, i2c_freq);
    }

    void cs_control(bool flg) override
    {
      lgfx::Panel_ILI9342::cs_control(flg);
      // CS操作時にGPIO35の役割を切り替える (MISO or D/C);

      // FSPIQ_IN_IDX==FSPI MISO / SIG_GPIO_OUT_IDX==GPIO OUT
      // *(volatile uint32_t*)GPIO_FUNC35_OUT_SEL_CFG_REG = flg ? FSPIQ_OUT_IDX : SIG_GPIO_OUT_IDX;

      // CS HIGHの場合はGPIO出力を無効化し、MISO入力として機能させる。
      // CS LOW の場合はGPIO出力を有効化し、D/Cとして機能させる。
      *(volatile uint32_t*)( flg
                             ? GPIO_ENABLE1_W1TC_REG
                             : GPIO_ENABLE1_W1TS_REG
                           ) = 1u << (GPIO_NUM_35 & 31);
    }
  };

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
    bool init(uint8_t brightness) override
    {
      lgfx::i2c::init(i2c_port, GPIO_NUM_45, GPIO_NUM_0);
      lgfx::i2c::writeRegister8(i2c_port, 48, 0x00, 0b01000000, 0, i2c_freq);
      lgfx::delay(1);
      lgfx::i2c::writeRegister8(i2c_port, 48, 0x08, 0b00000001, 0, i2c_freq);
      lgfx::i2c::writeRegister8(i2c_port, 48, 0x70, 0b00000000, 0, i2c_freq);

      setBrightness(brightness);
      return true;
    }

    void setBrightness(uint8_t brightness) override
    {
      lgfx::i2c::writeRegister8(i2c_port, 48, 0x0e, brightness, 0, i2c_freq);
    }
  };

  struct Light_M5StackStampPLC : public lgfx::ILight
  {
    bool _is_backlight_inited = false;

    bool init(uint8_t brightness) override
    {
      lgfx::i2c::init(i2c_port, GPIO_NUM_13, GPIO_NUM_15);

      // set direction: output
      lgfx::i2c::bitOn(i2c_port, pi4io1_i2c_addr, 0x03, 1 << 7, i2c_freq);

      // set pull mode: down
      lgfx::i2c::bitOff(i2c_port, pi4io1_i2c_addr, 0x0D, 1 << 7, i2c_freq);

      // set high impedance: off
      lgfx::i2c::bitOff(i2c_port, pi4io1_i2c_addr, 0x07, 1 << 7, i2c_freq);

      _is_backlight_inited = true;
      setBrightness(brightness);
      return true;
    }

    void setBrightness(uint8_t brightness) override
    {
      if (!_is_backlight_inited) init(127);

      if (brightness == 0) {
        lgfx::i2c::bitOn(i2c_port, pi4io1_i2c_addr, 0x05, 1 << 7, i2c_freq);
      } else {
        lgfx::i2c::bitOff(i2c_port, pi4io1_i2c_addr, 0x05, 1 << 7, i2c_freq);
      }
    }
  };

#elif defined (CONFIG_IDF_TARGET_ESP32C6)

  static constexpr int32_t i2c_freq = 400000;
  static constexpr int_fast16_t i2c_port = I2C_NUM_0;
  
  struct Light_ArduinoNessoN1 : public lgfx::ILight
  {
    // static constexpr int_fast16_t i2c_sda = GPIO_NUM_10;
    // static constexpr int_fast16_t i2c_scl = GPIO_NUM_8;
    bool init(uint8_t brightness) override
    {
      setBrightness(brightness);
      return true;
    }

    void setBrightness(uint8_t brightness) override
    {
      if (brightness) {
        lgfx::i2c::bitOn(i2c_port, pi4io2_i2c_addr, 0x05, 1 << 6, i2c_freq);
      } else {
        lgfx::i2c::bitOff(i2c_port, pi4io2_i2c_addr, 0x05, 1 << 6, i2c_freq);
      }
    }
  };

#endif

  __attribute__ ((unused))
  static void _pin_level(std::int_fast16_t pin, bool level)
  {
    lgfx::pinMode(pin, lgfx::pin_mode_t::output);
    if (level) lgfx::gpio_hi(pin);
    else       lgfx::gpio_lo(pin);
  }

  __attribute__ ((unused))
  static void _pin_reset(std::int_fast16_t pin, bool use_reset)
  {
    lgfx::gpio_hi(pin);
    lgfx::pinMode(pin, lgfx::pin_mode_t::output);
    lgfx::delay(1);
    if (!use_reset) return;
    lgfx::gpio_lo(pin);
    lgfx::delay(2);
    lgfx::gpio_hi(pin);
    lgfx::delay(10);
  }

  /// TF card dummy clock送信 ;
  static void _send_sd_dummy_clock(int spi_host, int_fast16_t pin_cs)
  {
    static constexpr uint32_t dummy_clock[] = { ~0u, ~0u, ~0u, ~0u };
    _pin_level(pin_cs, true);
    m5gfx::spi::writeBytes(spi_host, (const uint8_t*)dummy_clock, sizeof(dummy_clock));
    _pin_level(pin_cs, false);
  }

  /// TF card をSPIモードに移行する ;
  __attribute__ ((unused))
  static void _set_sd_spimode(int spi_host, int_fast16_t pin_cs)
  {
    m5gfx::spi::beginTransaction(spi_host, 400000, 0);
    _send_sd_dummy_clock(spi_host, pin_cs);

    uint8_t sd_cmd58[] = { 0x7A, 0, 0, 0, 0, 0xFD, 0xFF, 0xFF }; // READ_OCR command.
    m5gfx::spi::readBytes(spi_host, sd_cmd58, sizeof(sd_cmd58));

    if (sd_cmd58[6] == sd_cmd58[7])  // not SPI mode
    {
      _send_sd_dummy_clock(spi_host, pin_cs);

      static constexpr uint8_t sd_cmd0[] = { 0x40, 0, 0, 0, 0, 0x95, 0xFF, 0xFF }; // GO_IDLE_STATE command.
      m5gfx::spi::writeBytes(spi_host, sd_cmd0, sizeof(sd_cmd0));
    }
    _pin_level(pin_cs, true);
    m5gfx::spi::endTransaction(spi_host);
  }

  __attribute__ ((unused))
  static std::uint32_t _read_panel_id(lgfx::Bus_SPI* bus, std::int32_t pin_cs, std::uint32_t cmd = 0x04, std::uint8_t dummy_read_bit = 1) // 0x04 = RDDID command
  {
    bus->beginTransaction();
    _pin_level(pin_cs, true);
    bus->writeCommand(0, 8);
    bus->wait();
    _pin_level(pin_cs, false);
    bus->writeCommand(cmd, 8);
    bus->beginRead(dummy_read_bit);
    std::uint32_t res = bus->readData(32);
    bus->endTransaction();
    _pin_level(pin_cs, true);

    ESP_LOGD(LIBRARY_NAME, "[Autodetect] read cmd:%02x = %08x", (int)cmd, (int)res);
    return res;
  }

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
    if (getBoard() != board_t::board_unknown)
    {
      return true;
    }

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

    auto board = (board_t)nvs_board;

    int retry = 4;
    do
    {
      if (retry == 1) use_reset = true;
      board = autodetect(use_reset, board);
      //ESP_LOGD(LIBRARY_NAME,"autodetect board:%d", (int)board);
    } while (board_t::board_unknown == board && --retry >= 0);
    _board = board;

#if defined ( ARDUINO_M5STACK_ATOM ) || defined ( ARDUINO_M5Stack_ATOM )

    if (board == board_t::board_unknown || board == board_t::board_M5Atom)
    {
      return false;
    }

#endif

    if (nvs_board != board) {
      if (0 == nvs_open(LIBRARY_NAME, NVS_READWRITE, &nvs_handle)) {
        ESP_LOGI(LIBRARY_NAME, "[Autodetect] save to NVS : board:%d", (int)board);
        nvs_set_u32(nvs_handle, NVS_KEY, board);
        nvs_close(nvs_handle);
      }
    }

    /// autodetectの際にreset済みなのでここではuse_resetをfalseで呼び出す。;
    /// M5Paperはreset後の復帰に800msec程度掛かるのでreset省略は起動時間短縮に有効;
    return LGFX_Device::init_impl(false, use_clear);
  }

  board_t M5GFX::autodetect(bool use_reset, board_t board)
  {
    auto bus_spi = new Bus_SPI();
    _bus_last.reset(bus_spi);

    panel(nullptr);

    auto bus_cfg = bus_spi->config();
    (void)bus_cfg; // prevent compiler warning.
    bus_cfg.freq_write = 8000000;
    bus_cfg.freq_read  = 8000000;
    bus_cfg.spi_mode = 0;
    bus_cfg.use_lock = true;

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)

    bus_cfg.spi_host = VSPI_HOST;
    bus_cfg.dma_channel = 1;

    std::uint32_t id;

    std::uint32_t pkg_ver = m5gfx::get_pkg_ver();
//  ESP_LOGD(LIBRARY_NAME, "pkg_ver : %02x", (int)pkg_ver);

    if (pkg_ver == EFUSE_RD_CHIP_VER_PKG_ESP32PICOD4)  /// check PICO-D4 (M5StickC,CPlus,T,T2 / CoreInk / ATOM )
    {
      if (board == 0 || board == board_t::board_M5StickC || board == board_t::board_M5StickCPlus)
      {
        gpio::pin_backup_t backup_pins[] = { GPIO_NUM_5, GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_18, GPIO_NUM_23 };
        _pin_reset(GPIO_NUM_18, use_reset); // LCD RST
        bus_cfg.pin_mosi = GPIO_NUM_15;
        bus_cfg.pin_miso = GPIO_NUM_14;
        bus_cfg.pin_sclk = GPIO_NUM_13;
        bus_cfg.pin_dc   = GPIO_NUM_23;
        bus_cfg.spi_3wire = true;
        bus_spi->config(bus_cfg);
        bus_spi->init();
        id = _read_panel_id(bus_spi, GPIO_NUM_5);
        if ((id & 0xFB) == 0x81) // 0x81 or 0x85
        {  //  check panel (ST7789)
          board = board_t::board_M5StickCPlus;
          ESP_LOGI(LIBRARY_NAME, "[Autodetect] M5StickCPlus");
          bus_spi->release();
          bus_cfg.spi_host = HSPI_HOST;
          bus_cfg.freq_write = 40000000;
          bus_cfg.freq_read  = 15000000;
          bus_spi->config(bus_cfg);
          auto p = new Panel_M5StickCPlus();
          p->bus(bus_spi);
          _panel_last.reset(p);
          _set_backlight(new Light_M5StickC());
          goto init_clear;
        }
        if ((id & 0xFF) == 0x7C)
        {  //  check panel (ST7735)
          board = board_t::board_M5StickC;
          ESP_LOGI(LIBRARY_NAME, "[Autodetect] M5StickC");
          bus_spi->release();
          bus_cfg.spi_host = HSPI_HOST;
          bus_cfg.freq_write = 27000000;
          bus_cfg.freq_read  = 14000000;
          bus_spi->config(bus_cfg);
          auto p = new Panel_M5StickC();
          p->bus(bus_spi);
          _panel_last.reset(p);
          _set_backlight(new Light_M5StickC());
          goto init_clear;
        }
        bus_spi->release();
        for (auto pin: backup_pins) { pin.restore(); }
      }

      if (board == 0 || board == board_t::board_M5StackCoreInk)
      {
        gpio::pin_backup_t backup_pins[] = { GPIO_NUM_0, GPIO_NUM_9, GPIO_NUM_15, GPIO_NUM_18, GPIO_NUM_23, GPIO_NUM_34 };
        _pin_reset( GPIO_NUM_0, true); // EPDがDeepSleepしている場合は自動認識に失敗する。そのためRST制御を必ず行う。;
        bus_cfg.pin_mosi = GPIO_NUM_23;
        bus_cfg.pin_miso = GPIO_NUM_34;
        bus_cfg.pin_sclk = GPIO_NUM_18;
        bus_cfg.pin_dc   = GPIO_NUM_15;
        bus_cfg.spi_3wire = true;
        bus_spi->config(bus_cfg);
        bus_spi->init();
        lgfx::Panel_HasBuffer* p = nullptr;
        id = _read_panel_id(bus_spi, GPIO_NUM_9, 0x2f ,0);
        if (id == 0x00010001)
        { //  check panel (e-paper GDEW0154D67)
          p = new lgfx::Panel_GDEW0154D67();
        } else {
          id = _read_panel_id(bus_spi, GPIO_NUM_9, 0x70, 0);
          if ((id & 0xFFFF00FFu) == 0x00F00000u)
          { //  check panel (e-paper GDEW0154M09)
            // ID of first lot  : 0x00F00000u
            // ID of 2023/11/17 : 0x00F01600u
            p = new lgfx::Panel_GDEW0154M09();
          }
        }
        if (p != nullptr)
        {
          _pin_level(GPIO_NUM_12, true);  // POWER_HOLD_PIN 12
          board = board_t::board_M5StackCoreInk;
          ESP_LOGI(LIBRARY_NAME, "[Autodetect] M5StackCoreInk");
          bus_cfg.freq_write = 40000000;
          bus_cfg.freq_read  = 16000000;
          bus_spi->config(bus_cfg);
          p->bus(bus_spi);
          _panel_last.reset(p);
          auto cfg = p->config();
          cfg.panel_height = 200;
          cfg.panel_width  = 200;
          cfg.pin_cs   = GPIO_NUM_9;
          cfg.pin_rst  = GPIO_NUM_0;
          cfg.pin_busy = GPIO_NUM_4;
          p->config(cfg);
          goto init_clear;
        }
        bus_spi->release();
        for (auto pin: backup_pins) { pin.restore(); }
      }

/// LCD / EPD 検出失敗の場合はATOM 判定;

    }
    else if (pkg_ver == 6)  // PICOV3_02 (StickCPlus2 / ATOM PSRAM)
    {
      if (board == 0 || board == board_t::board_M5StickCPlus2)
      {
        gpio::pin_backup_t backup_pins[] = { GPIO_NUM_5, GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15 };
        _pin_reset(GPIO_NUM_12, use_reset); // LCD RST
        bus_cfg.pin_mosi = GPIO_NUM_15;
        bus_cfg.pin_miso = (gpio_num_t)-1; //GPIO_NUM_NC;
        bus_cfg.pin_sclk = GPIO_NUM_13;
        bus_cfg.pin_dc   = GPIO_NUM_14;
        bus_cfg.spi_3wire = true;
        bus_spi->config(bus_cfg);
        bus_spi->init();
        id = _read_panel_id(bus_spi, GPIO_NUM_5);
        if ((id & 0xFB) == 0x81) // 0x81 or 0x85
        {  //  check panel (ST7789)
          _pin_level(GPIO_NUM_4, true);  // POWER_HOLD_PIN 4
          board = board_t::board_M5StickCPlus2;
          ESP_LOGI(LIBRARY_NAME, "[Autodetect] M5StickCPlus2");
          bus_spi->release();
          bus_cfg.spi_host = HSPI_HOST;
          bus_cfg.freq_write = 40000000;
          bus_cfg.freq_read  = 15000000;
          bus_spi->config(bus_cfg);
          auto p = new Panel_M5StickCPlus();
          {
            auto cfg = p->config();
            cfg.pin_rst = GPIO_NUM_12;
            p->config(cfg);
          }
          p->bus(bus_spi);
          _panel_last.reset(p);
          _set_pwm_backlight(GPIO_NUM_27, 7, 256, false, 40);
          goto init_clear;
        }
        bus_spi->release();
        for (auto pin: backup_pins) { pin.restore(); }
      }

      if (board == 0)
      {
        board = board_t::board_M5AtomPsram;
        goto init_clear;
      }
    }
    else
    if (pkg_ver == EFUSE_RD_CHIP_VER_PKG_ESP32D0WDQ6)
    {
      /// AXP192の有無を最初に判定し、分岐する。;
      if (board == 0
      || board == board_t::board_M5Station
      || board == board_t::board_M5StackCore2
      || board == board_t::board_M5Tough)
      {
        gpio::pin_backup_t backup_pins[] = { axp_i2c_sda, axp_i2c_scl };
        // I2C addr 0x34 = AXP192
        lgfx::i2c::init(axp_i2c_port, axp_i2c_sda, axp_i2c_scl);

        auto chk_axp = lgfx::i2c::readRegister8(axp_i2c_port, axp_i2c_addr, 0x03, 400000);
        if (chk_axp.has_value())
        {
          uint_fast16_t axp_exists = 0;
          if (chk_axp.value() == 0x03) { // AXP192 found
            axp_exists = 192;
            ESP_LOGD(LIBRARY_NAME, "AXP192 found");
          }
          else if (chk_axp.value() == 0x4A) { // AXP2101 found
            axp_exists = 2101;
            ESP_LOGD(LIBRARY_NAME, "AXP2101 found");
          }
  
          if (axp_exists == 192 && (board == 0 || board == board_t::board_M5Station))
          {
            gpio::pin_backup_t backup_pins2[] = { GPIO_NUM_5, GPIO_NUM_15, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_23 };
            _pin_reset(GPIO_NUM_15, use_reset); // LCD RST;
            bus_cfg.pin_mosi = GPIO_NUM_23;
            bus_cfg.pin_miso = -1;
            bus_cfg.pin_sclk = GPIO_NUM_18;
            bus_cfg.pin_dc   = GPIO_NUM_19;
            bus_cfg.spi_3wire = true;
            bus_spi->config(bus_cfg);
            bus_spi->init();

            id = _read_panel_id(bus_spi, GPIO_NUM_5);
            if ((id & 0xFB) == 0x81) // 0x81 or 0x85
            {  //  check panel (ST7789)
              ESP_LOGI(LIBRARY_NAME, "[Autodetect] M5Station");
              board = board_t::board_M5Station;

              bus_spi->release();
              bus_cfg.spi_host = HSPI_HOST;
              bus_cfg.freq_write = 40000000;
              bus_cfg.freq_read  = 15000000;
              bus_spi->config(bus_cfg);

              auto p = new Panel_M5StickCPlus();
              {
                auto cfg = p->config();
                cfg.pin_rst = 15;
                p->config(cfg);
                p->setRotation(1);
              }
              p->bus(bus_spi);
              _panel_last.reset(p);
              /// M5StationのバックライトはM5Toughと同じ;
              _set_backlight(new Light_M5Tough());
              goto init_clear;
            }
            bus_spi->release();
            for (auto pin: backup_pins2) { pin.restore(); }
          }

          if (axp_exists && (board == 0 || board == board_t::board_M5StackCore2 || board == board_t::board_M5Tough))
          {
            // fore Core2 1st gen (AXP192)
              // AXP192_LDO2 = LCD PWR
              // AXP192_IO4  = LCD RST
              // AXP192_DC3  = LCD BL (Core2)
              // AXP192_LDO3 = LCD BL (Tough)
              // AXP192_IO1  = TP RST (Tough)
            static constexpr uint8_t reg_data_axp192_first[] = {
              0x95, 0x84, 0x72,   // GPIO4 enable
              0x28, 0xF0, 0xFF,   // set LDO2 3300mv // LCD PWR
              0x12, 0x04, 0xFF,   // LDO2 enable
              0x92, 0x00, 0xF8,   // GPIO1 OpenDrain (M5Tough TOUCH)
              0xFF, 0xFF, 0xFF,
            };
            static constexpr uint8_t reg_data_axp192_reset[] = {
              0x96, 0x00, 0xFD,   // GPIO4 LOW (LCD RST)
              0x94, 0x00, 0xFD,   // GPIO1 LOW (M5Tough TOUCH RST)
              0xFF, 0xFF, 0xFF,
            };
            static constexpr uint8_t reg_data_axp192_second[] = {
              0x96, 0x02, 0xFF,   // GPIO4 HIGH (LCD RST)
              0x94, 0x02, 0xFF,   // GPIO1 HIGH (M5Tough TOUCH RST)
              0xFF, 0xFF, 0xFF,
            };

            // for Core2 v1.1 (AXP2101)
              // ALDO2 == LCD+TOUCH RST
              // ALDO3 == SPK EN
              // ALDO4 == TF, TP, LCD PWR
              // BLDO1 == LCD BL
              // BLDO2 == Boost EN
              // DLDO1 == Vibration Motor
            static constexpr uint8_t reg_data_axp2101_first[] = {
              0x90, 0x08, 0x7B,   // ALDO4 ON / ALDO3 OFF, DLDO1 OFF
              0x80, 0x05, 0xFF,   // DCDC1 + DCDC3 ON
              0x82, 0x12, 0x00,   // DCDC1 3.3V
              0x84, 0x6A, 0x00,   // DCDC3 3.3V
              0xFF, 0xFF, 0xFF,
            };
            static constexpr uint8_t reg_data_axp2101_reset[] = {
              0x90, 0x00, 0xFD,   // ALDO2 OFF
              0xFF, 0xFF, 0xFF,
            };
            static constexpr uint8_t reg_data_axp2101_second[] = {
              0x90, 0x02, 0xFF,   // ALDO2 ON
              0xFF, 0xFF, 0xFF,
            };

            _pin_level(GPIO_NUM_5, true);

            bool isAxp192 = axp_exists == 192;

            i2c_write_register8_array(axp_i2c_port, axp_i2c_addr, isAxp192 ? reg_data_axp192_first : reg_data_axp2101_first, axp_i2c_freq);
            if (use_reset) {
              i2c_write_register8_array(axp_i2c_port, axp_i2c_addr, isAxp192 ? reg_data_axp192_reset : reg_data_axp2101_reset, axp_i2c_freq);
              lgfx::delay(1);
            }
            i2c_write_register8_array(axp_i2c_port, axp_i2c_addr, isAxp192 ? reg_data_axp192_second : reg_data_axp2101_second, axp_i2c_freq);
            lgfx::delay(1);

            {
              gpio::pin_backup_t backup_pins2[] = { GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_15, GPIO_NUM_18, GPIO_NUM_23, GPIO_NUM_38 };
              bus_cfg.pin_mosi = GPIO_NUM_23;
              bus_cfg.pin_miso = GPIO_NUM_38;
              bus_cfg.pin_sclk = GPIO_NUM_18;
              bus_cfg.pin_dc   = GPIO_NUM_15;
              bus_cfg.spi_3wire = true;
              bus_spi->config(bus_cfg);
              bus_spi->init();

              _set_sd_spimode(bus_cfg.spi_host, GPIO_NUM_4);

              id = _read_panel_id(bus_spi, GPIO_NUM_5);
              if ((id & 0xFF) == 0xE3)
              {   // ILI9342c
                bus_cfg.freq_write = 40000000;
                bus_cfg.freq_read  = 16000000;
                bus_spi->config(bus_cfg);

                auto p = new Panel_M5StackCore2();
                p->bus(bus_spi);
                _panel_last.reset(p);

                // Tough のタッチコントローラ有無をチェックする;
                // Core2/Tough 判別条件としてCore2のTP(0x38)の有無を用いた場合、以下の問題が生じる;
                // ・Core2のTPがスリープしている場合は反応が得られない;
                // ・ToughにGoPlus2を組み合わせると0x38に反応がある;
                // 上記のことから、ここではToughのTP(0x2E)の有無によって判定する;
                if ( ! lgfx::i2c::readRegister8(axp_i2c_port, 0x2E, 0, 400000).has_value()) // 0x2E:M5Tough TOUCH
                {
                  ESP_LOGI(LIBRARY_NAME, "[Autodetect] M5StackCore2");
                  board = board_t::board_M5StackCore2;

                  ILight* light = nullptr;
                  if (isAxp192) {
                    light = new Light_M5StackCore2();
                  } else {
                    light = new Light_M5StackCore2_AXP2101();
                  }
                  _set_backlight(light);

                  auto t = new lgfx::Touch_FT5x06();
                  _touch_last.reset(t);
                  auto cfg = t->config();
                  cfg.pin_int  = GPIO_NUM_39;
                  cfg.pin_sda  = GPIO_NUM_21;
                  cfg.pin_scl  = GPIO_NUM_22;
                  cfg.i2c_addr = 0x38;
                  cfg.i2c_port = I2C_NUM_1;
                  cfg.freq = 400000;
                  cfg.x_min = 0;
                  cfg.x_max = 319;
                  cfg.y_min = 0;
                  cfg.y_max = 279;
                  cfg.bus_shared = false;
                  t->config(cfg);
                  p->touch(t);
                  float affine[6] = { 1, 0, 0, 0, 1, 0 };
                  p->setCalibrateAffine(affine);
                }
                else
                {
                  ESP_LOGI(LIBRARY_NAME, "[Autodetect] M5Tough");
                  board = board_t::board_M5Tough;

                  _set_backlight(new Light_M5Tough());

                  auto t = new Touch_M5Tough();
                  _touch_last.reset(t);
                  auto cfg = t->config();
                  cfg.pin_int  = GPIO_NUM_39;
                  cfg.pin_sda  = GPIO_NUM_21;
                  cfg.pin_scl  = GPIO_NUM_22;
                  cfg.i2c_addr = 0x2E;
                  cfg.i2c_port = I2C_NUM_1;
                  cfg.freq = 400000;
                  cfg.x_min = 0;
                  cfg.x_max = 319;
                  cfg.y_min = 0;
                  cfg.y_max = 239;
                  cfg.bus_shared = false;
                  t->config(cfg);
                  p->touch(t);
                }

                goto init_clear;
              }
              bus_spi->release();
              for (auto pin: backup_pins2) { pin.restore(); }
            }
          }
        }
        for (auto pin: backup_pins) { pin.restore(); }
      }

      if (board == 0 || board == board_t::board_M5Stack)
      {
        gpio::pin_backup_t backup_pins[] = { GPIO_NUM_4, GPIO_NUM_14, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_23, GPIO_NUM_27, GPIO_NUM_33 };
        _pin_reset(GPIO_NUM_33, use_reset); // LCD RST;
        bus_cfg.pin_mosi = GPIO_NUM_23;
        bus_cfg.pin_miso = GPIO_NUM_19;
        bus_cfg.pin_sclk = GPIO_NUM_18;
        bus_cfg.pin_dc   = GPIO_NUM_27;

        bus_cfg.spi_3wire = true;
        bus_spi->config(bus_cfg);
        bus_spi->init();

        _set_sd_spimode(bus_cfg.spi_host, GPIO_NUM_4);

        id = _read_panel_id(bus_spi, GPIO_NUM_14);
        if ((id & 0xFF) == 0xE3)
        {   // ILI9342c
          ESP_LOGI(LIBRARY_NAME, "[Autodetect] M5Stack");
          board = board_t::board_M5Stack;

          bus_cfg.freq_write = 40000000;
          bus_cfg.freq_read  = 16000000;
          bus_spi->config(bus_cfg);

          auto p = new Panel_M5Stack();
          p->bus(bus_spi);
          _panel_last.reset(p);
          _set_pwm_backlight(GPIO_NUM_32, 7, 44100);
          goto init_clear;
        }
        bus_spi->release();
        for (auto pin: backup_pins) { pin.restore(); }
      }


      if (board == 0 || board == board_t::board_M5Paper)
      {
        gpio::pin_backup_t backup_pins[] = { GPIO_NUM_23, GPIO_NUM_27 };
        _pin_reset(GPIO_NUM_23, true);
        lgfx::pinMode(GPIO_NUM_27, lgfx::pin_mode_t::input_pullup); // M5Paper EPD busy pin
        if (!lgfx::gpio_in(GPIO_NUM_27))
        {
          gpio::pin_backup_t backup_pins2[] = { GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_27 };
          _pin_level(GPIO_NUM_2, true);  // M5EPD_MAIN_PWR_PIN 2
          lgfx::pinMode(GPIO_NUM_27, lgfx::pin_mode_t::input);
          bus_cfg.pin_mosi = GPIO_NUM_12;
          bus_cfg.pin_miso = GPIO_NUM_13;
          bus_cfg.pin_sclk = GPIO_NUM_14;
          bus_cfg.pin_dc   = -1;
          bus_cfg.spi_3wire = false;
          bus_spi->config(bus_cfg);
          id = lgfx::millis();

          _pin_level(GPIO_NUM_15, true); // M5Paper CS;
          bus_spi->init();
          _set_sd_spimode(bus_cfg.spi_host, GPIO_NUM_4);
          do
          {
            vTaskDelay(1);
            if (lgfx::millis() - id > 1024) { id = 0; break; }
          } while (!lgfx::gpio_in(GPIO_NUM_27));
          if (id)
          {
            bus_spi->beginTransaction();
            lgfx::gpio_lo(GPIO_NUM_15);
            bus_spi->writeData(__builtin_bswap16(0x6000), 16);
            bus_spi->writeData(__builtin_bswap16(0x0302), 16);  // read DevInfo
            id = lgfx::millis();
            bus_spi->wait();
            lgfx::gpio_hi(GPIO_NUM_15);
            do
            {
              vTaskDelay(1);
              if (lgfx::millis() - id > 192) { break; }
            } while (!lgfx::gpio_in(GPIO_NUM_27));
            lgfx::gpio_lo(GPIO_NUM_15);
            bus_spi->writeData(__builtin_bswap16(0x1000), 16);
            bus_spi->writeData(__builtin_bswap16(0x0000), 16);
            std::uint8_t buf[40];
            bus_spi->beginRead();
            bus_spi->readBytes(buf, 40, false);
            bus_spi->endRead();
            bus_spi->endTransaction();
            lgfx::gpio_hi(GPIO_NUM_15);
            id = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
            // ESP_LOGI(LIBRARY_NAME, "[Autodetect] panel size :%08x", (int)id);
            if (id == 0x03C0021C)
            {  //  check panel ( panel size 960(0x03C0) x 540(0x021C) )
              board = board_t::board_M5Paper;
              ESP_LOGI(LIBRARY_NAME, "[Autodetect] M5Paper");
              bus_cfg.freq_write = 40000000;
              bus_cfg.freq_read  = 20000000;
              bus_spi->config(bus_cfg);
              {
                auto p = new lgfx::Panel_IT8951();
                p->bus(bus_spi);
                _panel_last.reset(p);
                auto cfg = p->config();
                cfg.panel_height = 540;
                cfg.panel_width  = 960;
                cfg.pin_cs   = GPIO_NUM_15;
                cfg.pin_rst  = GPIO_NUM_23;
                cfg.pin_busy = GPIO_NUM_27;
                cfg.offset_rotation = 3;
                p->config(cfg);
              }
              {
                auto t = new lgfx::Touch_GT911();
                _touch_last.reset(t);
                auto cfg = t->config();
                cfg.pin_int  = GPIO_NUM_36;
                cfg.pin_sda  = GPIO_NUM_21;
                cfg.pin_scl  = GPIO_NUM_22;
#ifdef _M5EPD_H_
                cfg.i2c_port = I2C_NUM_0;
#else
                cfg.i2c_port = I2C_NUM_1;
#endif
                cfg.freq = 400000;
                cfg.x_min = 0;
                cfg.x_max = 539;
                cfg.y_min = 0;
                cfg.y_max = 959;
                cfg.offset_rotation = 1;
                cfg.bus_shared = false;
                t->config(cfg);
                _panel_last->touch(t);
              }
              goto init_clear;
            }
          }
          bus_spi->release();
          for (auto pin: backup_pins2) { pin.restore(); }
        }
        for (auto pin: backup_pins) { pin.restore(); }
      }
    }

#elif defined (CONFIG_IDF_TARGET_ESP32S3)

    bus_cfg.spi_host = SPI2_HOST;
    bus_cfg.dma_channel = SPI_DMA_CH_AUTO;

    std::uint32_t id;

    std::uint32_t pkg_ver = m5gfx::get_pkg_ver();
// ESP_LOGD(LIBRARY_NAME, "pkg_ver : %02x  /  board:%d", (int)pkg_ver, (int)board);
    switch (pkg_ver) {
    case 0: // EFUSE_PKG_VERSION_ESP32S3:     // QFN56

      if (board == 0 || board == board_t::board_M5StackCoreS3 || board == board_t::board_M5StackCoreS3SE)
      {
        lgfx::i2c::init(i2c_port, i2c_sda, i2c_scl);

// ESP_LOGI("DEBUG","AW 0x10 :%02x", (int)lgfx::i2c::readRegister8(i2c_port, aw9523_i2c_addr, 0x10, 400000).value());
// ESP_LOGI("DEBUG","AXP0x03 :%02x", (int)lgfx::i2c::readRegister8(i2c_port, axp_i2c_addr, 0x03, 400000).value());

        auto chk_axp = lgfx::i2c::readRegister8(i2c_port, axp_i2c_addr, 0x03, i2c_freq);
        if (chk_axp.has_value())
        {
          auto chk_aw  = lgfx::i2c::readRegister8(i2c_port, aw9523_i2c_addr, 0x10, i2c_freq);
          if (chk_aw .has_value() && chk_aw .value() == 0x23)
          {
            auto result = lgfx::gpio::command(
              (const uint8_t[]) {
              lgfx::gpio::command_mode_input_pullup, GPIO_NUM_35,
              lgfx::gpio::command_mode_input_pullup, GPIO_NUM_36,
              lgfx::gpio::command_mode_input_pullup, GPIO_NUM_37,
              lgfx::gpio::command_read             , GPIO_NUM_35,
              lgfx::gpio::command_read             , GPIO_NUM_36,
              lgfx::gpio::command_read             , GPIO_NUM_37,
              lgfx::gpio::command_end
              }
            );
            /// SPIバスのプルアップが効いていない場合はVBUS 5V出力を有効化する。
            /// (USBホストモジュール等、5Vが出ていないと信号線の電気を吸い込む組合せがあるため)
            uint8_t reg0x02 = (result == 0) ? 0b00000111 : 0b00000101;
            uint8_t reg0x03 = (result == 0) ? 0b10000011 : 0b00000011;
            m5gfx::i2c::bitOn(i2c_port, aw9523_i2c_addr, 0x02, reg0x02); //port0 output ctrl
            m5gfx::i2c::bitOn(i2c_port, aw9523_i2c_addr, 0x03, reg0x03); //port1 output ctrl
            m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x04, 0b00011000);  // CONFIG_P0
            m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x05, 0b00001100);  // CONFIG_P1
            m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x11, 0b00010000);  // GCR P0 port is Push-Pull mode.
            m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x12, 0b11111111);  // LEDMODE_P0
            m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x13, 0b11111111);  // LEDMODE_P1
            m5gfx::i2c::writeRegister8(i2c_port, axp_i2c_addr, 0x90, 0xBF); // LDOS ON/OFF control 0
            m5gfx::i2c::writeRegister8(i2c_port, axp_i2c_addr, 0x94, 33 - 5); // ALDO3 set to 3.3v // for GC0308 Camera
            m5gfx::i2c::writeRegister8(i2c_port, axp_i2c_addr, 0x95, 33 - 5); // ALDO4 set to 3.3v // for TF card slot

            bus_cfg.pin_mosi = GPIO_NUM_37;
            bus_cfg.pin_miso = GPIO_NUM_35;
            bus_cfg.pin_sclk = GPIO_NUM_36;
            bus_cfg.pin_dc   = GPIO_NUM_35;// MISOとLCD D/CをGPIO35でシェアしている;
            bus_cfg.spi_mode = 0;
            bus_cfg.spi_3wire = true;
            bus_spi->config(bus_cfg);
            bus_spi->init();

            _set_sd_spimode(bus_cfg.spi_host, GPIO_NUM_4);

            id = _read_panel_id(bus_spi, GPIO_NUM_3);
            if ((id & 0xFF) == 0xE3)
            {  //  check panel (ILI9342)
              board = board_t::board_M5StackCoreS3;
              // Camera GC0308 check (not found == M5StackCoreS3SE)
              auto chk_gc  = lgfx::i2c::readRegister8(i2c_port, gc0308_i2c_addr, 0x00, i2c_freq);
              if (chk_gc .has_value() && chk_gc .value() == 0x9b) {
                ESP_LOGI(LIBRARY_NAME, "[Autodetect] board_M5StackCoreS3");
              } else {
                board = board_M5StackCoreS3SE;
                ESP_LOGI(LIBRARY_NAME, "[Autodetect] board_M5StackCoreS3SE");
              }
              bus_cfg.freq_write = 40000000;
              bus_cfg.freq_read  = 16000000;
              bus_spi->config(bus_cfg);
              auto p = new Panel_M5StackCoreS3();
              p->bus(bus_spi);
              _panel_last.reset(p);

              _set_backlight(new Light_M5StackCoreS3());

              {
                auto t = new Touch_M5StackCoreS3();
                _touch_last.reset(t);
                _panel_last->touch(t);
              }

              goto init_clear;
            }
            bus_spi->release();
            lgfx::pinMode(GPIO_NUM_4, lgfx::pin_mode_t::input); // TF card CS
            lgfx::pinMode(GPIO_NUM_3, lgfx::pin_mode_t::input); // LCD CS
          }
        }
        lgfx::i2c::release(i2c_port);
      }

      if (board == 0 || board == board_t::board_M5Dial)
      {
        gpio::pin_backup_t backup_pins[] = { GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7, GPIO_NUM_8 };
        _pin_reset(GPIO_NUM_8, use_reset); // LCD RST
        bus_cfg.pin_mosi = GPIO_NUM_5;
        bus_cfg.pin_miso = (gpio_num_t)-1; //GPIO_NUM_NC;
        bus_cfg.pin_sclk = GPIO_NUM_6;
        bus_cfg.pin_dc   = GPIO_NUM_4;
        bus_cfg.spi_mode = 0;
        bus_cfg.spi_3wire = true;
        bus_spi->config(bus_cfg);
        bus_spi->init();
        id = _read_panel_id(bus_spi, GPIO_NUM_7);
        if ((id & 0xFFFFFF) == 0x019a00)
        {  //  check panel (GC9A01)
          board = board_t::board_M5Dial;
          ESP_LOGI(LIBRARY_NAME, "[Autodetect] board_M5Dial");
          bus_spi->release();
          bus_cfg.freq_write = 80000000;
          bus_cfg.freq_read  = 16000000;
          bus_spi->config(bus_cfg);
          bus_spi->init();
          auto p = new Panel_GC9A01();
          p->bus(bus_spi);
          {
            auto cfg = p->config();
            cfg.pin_cs  = GPIO_NUM_7;
            cfg.pin_rst = GPIO_NUM_8;
            cfg.panel_width = 240;
            cfg.panel_height = 240;
            cfg.readable = false;
            cfg.invert = true;
            p->config(cfg);
          }
          _panel_last.reset(p);
          _set_pwm_backlight(GPIO_NUM_9, 7, 44100);

          {
            auto t = new m5gfx::Touch_FT5x06();
            if (t) {
              _touch_last.reset(t);

              auto cfg = t->config();

              cfg.x_min = 0;
              cfg.x_max = 239;
              cfg.y_min = 0;
              cfg.y_max = 239;
              cfg.pin_int = GPIO_NUM_14;
              cfg.bus_shared = false;
              cfg.offset_rotation = 0;

              cfg.i2c_port = 1;
              cfg.i2c_addr = 0x38;
              cfg.pin_sda = GPIO_NUM_11;
              cfg.pin_scl = GPIO_NUM_12;
              cfg.freq = 400000;

              t->config(cfg);

              _panel_last->touch(t);
            }
          }

          goto init_clear;
        }
        bus_spi->release();
        for (auto pin: backup_pins) { pin.restore(); }
      }

      if (board == 0 || board == board_t::board_M5PaperS3)
      {
        static constexpr int_fast16_t papers3_i2c_sda = GPIO_NUM_41;
        static constexpr int_fast16_t papers3_i2c_scl = GPIO_NUM_42;
        static constexpr const uint8_t gt911_i2c_addr[] = { 0x14, 0x5D };
        gpio::pin_backup_t backup_pins[] = { papers3_i2c_sda, papers3_i2c_scl };
        auto result = lgfx::gpio::command(
          (const uint8_t[]) {
          lgfx::gpio::command_mode_output        , papers3_i2c_scl,
          lgfx::gpio::command_write_low          , papers3_i2c_scl,
          lgfx::gpio::command_mode_output        , papers3_i2c_sda,
          lgfx::gpio::command_write_low          , papers3_i2c_sda,
          lgfx::gpio::command_write_high         , papers3_i2c_scl,
          lgfx::gpio::command_write_high         , papers3_i2c_sda,
          lgfx::gpio::command_mode_input_pulldown, papers3_i2c_scl,
          lgfx::gpio::command_mode_input_pulldown, papers3_i2c_sda,
          lgfx::gpio::command_delay              , 1,
          lgfx::gpio::command_read               , papers3_i2c_scl,
          lgfx::gpio::command_read               , papers3_i2c_sda,
          lgfx::gpio::command_end
          }
        );
        // Check G41,G42 HIGH
        if (result == 0x03) {
          lgfx::i2c::init(i2c_port, papers3_i2c_sda, papers3_i2c_scl);
          {
            bool gt911_found = false;
            for (auto addr: gt911_i2c_addr) {
              if (lgfx::i2c::beginTransaction(i2c_port, addr, 400000).has_value()) {
                gt911_found = lgfx::i2c::endTransaction(i2c_port).has_value();
                if (gt911_found) {
                  break;
                }
              }
            }
            if (gt911_found) {
              board = board_t::board_M5PaperS3;
              ESP_LOGI(LIBRARY_NAME, "[Autodetect] board_M5PaperS3");
              // PWROFF_PULSE_PIN
              lgfx::pinMode(GPIO_NUM_44, lgfx::pin_mode_t::output);
              lgfx::gpio_lo(GPIO_NUM_44);

#if !(defined(CONFIG_ESP32S3_SPIRAM_SUPPORT))
              ESP_LOGE(LIBRARY_NAME, "M5PaperS3 need OPI-PSRAM enabled");
#elif !defined (CONFIG_SPIRAM_MODE_OCT)
              ESP_LOGE(LIBRARY_NAME, "M5PaperS3 need OPI-PSRAM enabled");
#else
              auto bus_epd = new Bus_EPD();
              _bus_last.reset(bus_epd);
              auto p = new lgfx::Panel_EPD();
              _panel_last.reset(p);

              {
                auto bus_cfg = bus_epd->config();
                bus_cfg.bus_speed = 16000000;
                bus_cfg.pin_data[0] = GPIO_NUM_6;
                bus_cfg.pin_data[1] = GPIO_NUM_14;
                bus_cfg.pin_data[2] = GPIO_NUM_7;
                bus_cfg.pin_data[3] = GPIO_NUM_12;
                bus_cfg.pin_data[4] = GPIO_NUM_9;
                bus_cfg.pin_data[5] = GPIO_NUM_11;
                bus_cfg.pin_data[6] = GPIO_NUM_8;
                bus_cfg.pin_data[7] = GPIO_NUM_10;
                bus_cfg.pin_pwr = GPIO_NUM_46;
                bus_cfg.pin_spv = GPIO_NUM_17;
                bus_cfg.pin_ckv = GPIO_NUM_18;
                bus_cfg.pin_sph = GPIO_NUM_13;
                bus_cfg.pin_oe = GPIO_NUM_45;
                bus_cfg.pin_le = GPIO_NUM_15;
                bus_cfg.pin_cl = GPIO_NUM_16;
                bus_cfg.bus_width = 8;
                bus_epd->config(bus_cfg);
                p->setBus(bus_epd);
              }
              {
                auto cfg_detail = p->config_detail();
                cfg_detail.line_padding = 8;
                p->config_detail(cfg_detail);
              }
              {
                auto cfg = p->config();
                cfg.memory_width = 960;
                cfg.panel_width = 960;
                cfg.memory_height = 540;
                cfg.panel_height = 540;
                cfg.offset_rotation = 3;
                cfg.offset_x = 0;
                cfg.offset_y = 0;
                cfg.bus_shared = false;
                p->config(cfg);
              }

              {
                auto t = new lgfx::Touch_GT911();
                _touch_last.reset(t);
                auto cfg = t->config();
                cfg.pin_int = GPIO_NUM_48;
                cfg.pin_sda = GPIO_NUM_41;
                cfg.pin_scl = GPIO_NUM_42;
                cfg.freq = 400000;
                cfg.i2c_port = I2C_NUM_1;
                cfg.x_min = 0;
                cfg.x_max = 539;
                cfg.y_min = 0;
                cfg.y_max = 959;
                cfg.offset_rotation = 1;
                cfg.bus_shared = false;
                t->config(cfg);
                _panel_last->touch(t);
                p->touch(t);
              }
              goto init_clear;
#endif
            }
          }
          lgfx::i2c::release(i2c_port);
        }
        for (auto &bup : backup_pins) { bup.restore(); }
      }

      if (board == 0 || board == board_t::board_M5AtomS3)
      {
        gpio::pin_backup_t backup_pins[] = { GPIO_NUM_15, GPIO_NUM_17, GPIO_NUM_21, GPIO_NUM_33, GPIO_NUM_34 };
        _pin_reset(GPIO_NUM_34, use_reset); // LCD RST
        bus_cfg.pin_mosi = GPIO_NUM_21;
        bus_cfg.pin_miso = (gpio_num_t)-1; //GPIO_NUM_NC;
        bus_cfg.pin_sclk = GPIO_NUM_17;
        bus_cfg.pin_dc   = GPIO_NUM_33;
        bus_cfg.spi_mode = 0;
        bus_cfg.spi_3wire = true;
        bus_spi->config(bus_cfg);
        bus_spi->init();
        id = _read_panel_id(bus_spi, GPIO_NUM_15);
        if ((id & 0xFFFFFF) == 0x079100)
        {  //  check panel (GC9107)
          board = board_t::board_M5AtomS3;
          ESP_LOGI(LIBRARY_NAME, "[Autodetect] board_M5AtomS3");
          bus_spi->release();
          bus_cfg.spi_host = SPI3_HOST;
          bus_cfg.freq_write = 40000000;
          bus_cfg.freq_read  = 16000000;
          bus_spi->config(bus_cfg);
          bus_spi->init();
          auto p = new Panel_GC9107();
          p->bus(bus_spi);
          {
            auto cfg = p->config();
            cfg.pin_cs  = GPIO_NUM_15;
            cfg.pin_rst = GPIO_NUM_34;
            cfg.panel_width = 128;
            cfg.panel_height = 128;
            cfg.offset_y = 32;
            cfg.readable = false;
            p->config(cfg);
          }
          _panel_last.reset(p);
          _set_pwm_backlight(GPIO_NUM_16, 7, 256, false, 48);

          goto init_clear;
        }
        bus_spi->release();
        for (auto pin: backup_pins) { pin.restore(); }
      }

      if (board == 0 || board == board_t::board_M5DinMeter)
      {
        gpio::pin_backup_t backup_pins[] = { GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7, GPIO_NUM_8 };
        _pin_reset(GPIO_NUM_8, use_reset); // LCD RST
        bus_cfg.pin_mosi = GPIO_NUM_5;
        bus_cfg.pin_miso = (gpio_num_t)-1; //GPIO_NUM_NC;
        bus_cfg.pin_sclk = GPIO_NUM_6;
        bus_cfg.pin_dc   = GPIO_NUM_4;
        bus_cfg.spi_mode = 0;
        bus_cfg.spi_3wire = true;
        bus_spi->config(bus_cfg);
        bus_spi->init();
        id = _read_panel_id(bus_spi, GPIO_NUM_7);
        if ((id & 0xFB) == 0x81) // 0x81 or 0x85
        {  //  check panel (ST7789)
          board = board_t::board_M5DinMeter;
          ESP_LOGI(LIBRARY_NAME, "[Autodetect] board_M5DinMeter");
          bus_spi->release();
          bus_cfg.freq_write = 40000000;
          bus_cfg.freq_read  = 16000000;
          bus_spi->config(bus_cfg);
          bus_spi->init();
          auto p = new Panel_ST7789();
          p->bus(bus_spi);
          {
            auto cfg = p->config();
            cfg.pin_cs  = GPIO_NUM_7;
            cfg.pin_rst = GPIO_NUM_8;
            cfg.panel_width = 135;
            cfg.panel_height = 240;
            cfg.offset_x     = 52;
            cfg.offset_y     = 40;
            cfg.offset_rotation = 2;
            cfg.readable = true;
            cfg.invert = true;
            p->config(cfg);
          }
          _panel_last.reset(p);
          _set_pwm_backlight(GPIO_NUM_9, 7, 256, false, 16);

          goto init_clear;
        }
        bus_spi->release();
        for (auto pin: backup_pins) { pin.restore(); }
      }

      if (board == 0
       || board == board_t::board_M5Cardputer
       || board == board_t::board_M5CardputerADV
       || board == board_t::board_M5VAMeter)
      {
        gpio::pin_backup_t backup_pins[] = { GPIO_NUM_33, GPIO_NUM_34, GPIO_NUM_35, GPIO_NUM_36, GPIO_NUM_37 };
        _pin_reset(GPIO_NUM_33, use_reset); // LCD RST
        bus_cfg.pin_mosi = GPIO_NUM_35;
        bus_cfg.pin_miso = (gpio_num_t)-1; //GPIO_NUM_NC;
        bus_cfg.pin_sclk = GPIO_NUM_36;
        bus_cfg.pin_dc   = GPIO_NUM_34;
        bus_cfg.spi_mode = 0;
        bus_cfg.spi_3wire = true;
        bus_spi->config(bus_cfg);
        bus_spi->init();
        id = _read_panel_id(bus_spi, GPIO_NUM_37);
        //  check panel (ST7789)
        if ((id & 0xFB) == 0x81) // 0x81 or 0x85
        {
/*
Here, VAMeter/Cardputer/CardputerADV will be automatically recognized.
The usage of each pin is as follows.
|    |  VAMeter  | Cardputer  |CardputerADV|
|:--:+:---------:+:----------:+:----------:|
| G5 | SYS_SDA   | KEY_MATRIX |  External  |
| G6 | SYS_SCL   | KEY_MATRIX |  External  |
| G7 |  NC       | KEY_MATRIX |Internal FPC|
| G8 | External  |  74HC138   |  SYS_SDA   |
| G9 | External  |  74HC138   |  SYS_SCL   |
*/
          board = board_t::board_M5Cardputer;
          gpio::pin_backup_t backup_pins2[] = { GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_8, GPIO_NUM_9 };
          auto result = lgfx::gpio::command(
            (const uint8_t[]) {
            lgfx::gpio::command_mode_input_pulldown, GPIO_NUM_9,
            lgfx::gpio::command_mode_input_pulldown, GPIO_NUM_8,
            lgfx::gpio::command_mode_input_pulldown, GPIO_NUM_6,
            lgfx::gpio::command_mode_input_pulldown, GPIO_NUM_5,
            lgfx::gpio::command_read               , GPIO_NUM_9,
            lgfx::gpio::command_read               , GPIO_NUM_8,
            lgfx::gpio::command_read               , GPIO_NUM_6,
            lgfx::gpio::command_read               , GPIO_NUM_5,
            lgfx::gpio::command_end
            }
          );
          for (auto &bup : backup_pins2) { bup.restore(); }
          if ((result & 3) == 3) {
            m5gfx::i2c::i2c_temporary_switcher_t backup_i2c_setting(1, GPIO_NUM_5, GPIO_NUM_6);
            result = (m5gfx::i2c::transactionWrite(1, 0x40, nullptr, 0).has_value()
                    && m5gfx::i2c::transactionWrite(1, 0x41, nullptr, 0).has_value());
            backup_i2c_setting.restore();
            if (result) {
              board = board_t::board_M5VAMeter;
            }
          }
          if (board == board_t::board_M5Cardputer) {
            if ((result & 0x0C) == 0x0C) {
              board = board_t::board_M5CardputerADV;
            }
          }
          bus_spi->release();
          bus_cfg.spi_host = SPI3_HOST;
          bus_cfg.freq_write = 40000000;
          bus_cfg.freq_read  = 16000000;
          bus_spi->config(bus_cfg);
          bus_spi->init();
          auto p = new Panel_ST7789();
          p->bus(bus_spi);
          {
            auto cfg = p->config();
            cfg.pin_cs  = GPIO_NUM_37;
            cfg.pin_rst = GPIO_NUM_33;
            cfg.panel_height = 240;
            cfg.offset_rotation = 0;
            cfg.readable = true;
            cfg.invert = true;
            int rotation = 0;
            int bl_freq = 256;
            int bl_offset = 16;
            if (board == board_t::board_M5VAMeter) {
              ESP_LOGI(LIBRARY_NAME, "[Autodetect] board_M5VAMeter");
              cfg.panel_width = 240;
              cfg.offset_x     = 0;
              cfg.offset_y     = 0;
              bl_freq = 512;
              bl_offset = 64;
            } else {
              cfg.panel_width = 135;
              cfg.offset_x     = 52;
              cfg.offset_y     = 40;
              rotation = 1;
              if (board == board_t::board_M5Cardputer) {
                ESP_LOGI(LIBRARY_NAME, "[Autodetect] board_M5Cardputer");
              } else {
                ESP_LOGI(LIBRARY_NAME, "[Autodetect] board_M5CardputerADV");
              }
            }
            p->config(cfg);
            p->setRotation(rotation);
            _panel_last.reset(p);
            _set_pwm_backlight(GPIO_NUM_38, 7, bl_freq, false, bl_offset);
          }

          goto init_clear;
        }
        bus_spi->release();
        for (auto pin: backup_pins) { pin.restore(); }
      }

      if (board == 0 || board == board_t::board_M5AirQ)
      {
        gpio::pin_backup_t backup_pins[] = { GPIO_NUM_2, GPIO_NUM_3, GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_6 };
        _pin_reset( GPIO_NUM_2, true); // EPDがDeepSleepしている場合は自動認識に失敗する。そのためRST制御を必ず行う。;
        bus_cfg.pin_mosi = GPIO_NUM_6;
        bus_cfg.pin_miso = (gpio_num_t)-1; //GPIO_NUM_NC;
        bus_cfg.pin_sclk = GPIO_NUM_5;
        bus_cfg.pin_dc   = GPIO_NUM_3;
        bus_cfg.spi_3wire = true;
        bus_spi->config(bus_cfg);
        bus_spi->init();
        lgfx::Panel_HasBuffer* p = nullptr;
        id = _read_panel_id(bus_spi, GPIO_NUM_4, 0x2f ,0);
        if (id == 0x00010001)
        { //  check panel (e-paper GDEW0154D67)
          p = new lgfx::Panel_GDEW0154D67();
        } else {
          id = _read_panel_id(bus_spi, GPIO_NUM_4, 0x70, 0);
          if ((id & 0xFFFF00FFu) == 0x00F00000u)
          { //  check panel (e-paper GDEW0154M09)
            // ID of first lot  : 0x00F00000u
            // ID of 2023/11/17 : 0x00F01600u
            p = new lgfx::Panel_GDEW0154M09();
          }
        }
        if (p != nullptr)
        {
          _pin_level(GPIO_NUM_46, true);  // POWER_HOLD_PIN 46
          board = board_t::board_M5AirQ;
          ESP_LOGI(LIBRARY_NAME, "[Autodetect] M5AirQ");
          bus_cfg.freq_write = 40000000;
          bus_cfg.freq_read  = 16000000;
          bus_spi->config(bus_cfg);
          p->bus(bus_spi);
          _panel_last.reset(p);
          auto cfg = p->config();
          cfg.panel_height = 200;
          cfg.panel_width  = 200;
          cfg.pin_cs   = GPIO_NUM_4;
          cfg.pin_rst  = GPIO_NUM_2;
          cfg.pin_busy = GPIO_NUM_1;
          p->config(cfg);
          goto init_clear;
        }
        bus_spi->release();
        for (auto pin: backup_pins) { pin.restore(); }
      }

      if (board == 0 || board == board_t::board_M5StampPLC)
      {
        gpio::pin_backup_t backup_pins[] = { GPIO_NUM_3, GPIO_NUM_6, GPIO_NUM_7, GPIO_NUM_8, GPIO_NUM_9, GPIO_NUM_10, GPIO_NUM_12 };
        _pin_reset(GPIO_NUM_3, use_reset); // LCD RST
        bus_cfg.pin_mosi = GPIO_NUM_8;
        bus_cfg.pin_miso = GPIO_NUM_9;
        bus_cfg.pin_sclk = GPIO_NUM_7;
        bus_cfg.pin_dc   = GPIO_NUM_6;
        bus_cfg.spi_mode = 0;
        bus_cfg.spi_3wire = true;
        bus_spi->config(bus_cfg);
        bus_spi->init();

        _set_sd_spimode(bus_cfg.spi_host, GPIO_NUM_10);

        id = _read_panel_id(bus_spi, GPIO_NUM_12);
        //  check panel (ST7789)
        if ((id & 0xFB) == 0x81) // 0x81 or 0x85
        {
          board = board_t::board_M5StampPLC;
          bus_spi->release();
          bus_cfg.freq_write = 40000000;
          bus_cfg.freq_read  = 16000000;
          bus_cfg.spi_3wire = true;
          bus_spi->config(bus_cfg);
          bus_spi->init();
          auto p = new Panel_ST7789();
          p->bus(bus_spi);
          {
            auto cfg = p->config();
            cfg.pin_cs  = GPIO_NUM_12;
            cfg.pin_rst = GPIO_NUM_3;
            cfg.panel_width = 135;
            cfg.panel_height = 240;
            cfg.offset_x     = 52;
            cfg.offset_y     = 40;
            cfg.offset_rotation = 0;
            cfg.readable = true;
            cfg.invert = true;
            cfg.bus_shared = true;
            p->config(cfg);
            p->setRotation(1);
          }
          _panel_last.reset(p);
          _set_backlight(new Light_M5StackStampPLC());
          goto init_clear;
        }
        bus_spi->release();
        for (auto pin: backup_pins) { pin.restore(); }
      }
 
      break;
    case 1: // EFUSE_PKG_VERSION_ESP32S3PICO: // LGA56

      if (board == 0 || board == board_t::board_M5AtomS3R)
      {
        gpio::pin_backup_t backup_pins[] = { GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_21, GPIO_NUM_42, GPIO_NUM_48 };
        _pin_reset(GPIO_NUM_48, use_reset); // LCD RST
        bus_cfg.pin_mosi = GPIO_NUM_21;
        bus_cfg.pin_miso = (gpio_num_t)-1; //GPIO_NUM_NC;
        bus_cfg.pin_sclk = GPIO_NUM_15;
        bus_cfg.pin_dc   = GPIO_NUM_42;
        bus_cfg.spi_mode = 0;
        bus_cfg.spi_3wire = true;
        bus_spi->config(bus_cfg);
        bus_spi->init();
        id = _read_panel_id(bus_spi, GPIO_NUM_14);
        if ((id & 0xFFFFFF) == 0x079100)
        {  //  check panel (GC9107)
          board = board_t::board_M5AtomS3R;
          ESP_LOGI(LIBRARY_NAME, "[Autodetect] board_M5AtomS3R");
          bus_spi->release();
          bus_cfg.spi_host = SPI3_HOST;
          bus_cfg.freq_write = 40000000;
          bus_cfg.freq_read  = 16000000;
          bus_spi->config(bus_cfg);
          bus_spi->init();
          auto p = new Panel_GC9107();
          p->bus(bus_spi);
          {
            auto cfg = p->config();
            cfg.pin_cs  = GPIO_NUM_14;
            cfg.pin_rst = GPIO_NUM_48;
            cfg.panel_width = 128;
            cfg.panel_height = 128;
            cfg.offset_y = 32;
            cfg.readable = false;
            cfg.bus_shared = false;
            p->config(cfg);
          }
          _panel_last.reset(p);
          _set_backlight(new Light_M5StackAtomS3R());

          goto init_clear;
        }
        bus_spi->release();
        for (auto pin: backup_pins) { pin.restore(); }
      }

      break;

    default: break;
    }

#elif defined (CONFIG_IDF_TARGET_ESP32P4)

    std::uint32_t id;

    std::uint32_t pkg_ver = m5gfx::get_pkg_ver();
    ESP_LOGD(LIBRARY_NAME, "pkg_ver : %02x", (int)pkg_ver);

    if (pkg_ver == 0) // pkg_ver == EFUSE_RD_CHIP_VER_PKG_
    {
      if (board == 0 || board == board_t::board_M5Tab5)
      {
        // SDA = GPIO_NUM_31
        // SCL = GPIO_NUM_32
        // TP INT = GPIO_NUM_23
        lgfx::pinMode(GPIO_NUM_23, lgfx::pin_mode_t::output); // TP INT
        lgfx::gpio_hi(GPIO_NUM_23); // select I2C Addr (high=0x14 / low=0x5D)
        lgfx::i2c::init(in_i2c_port, GPIO_NUM_31, GPIO_NUM_32);

        id = lgfx::i2c::readRegister8(in_i2c_port, pi4io1_i2c_addr, 0x01).has_value()
          && lgfx::i2c::readRegister8(in_i2c_port, pi4io2_i2c_addr, 0x01).has_value();
        if (id != 0) {
          board = board_t::board_M5Tab5;
          ESP_LOGI(LIBRARY_NAME, "[Autodetect] board_M5Tab5");

          static constexpr const uint8_t reg_data_io1_1[] = {
            0x03, 0b01111111, 0,   // PI4IO_REG_IO_DIR
            0x05, 0b01000110, 0,   // PI4IO_REG_OUT_SET (bit4=LCD Reset,bit5=GT911 TouchReset  LOW)
            0x07, 0b00000000, 0,   // PI4IO_REG_OUT_H_IM
            0x0D, 0b01111111, 0,   // PI4IO_REG_PULL_SEL
            0x0B, 0b01111111, 0,   // PI4IO_REG_PULL_EN
            0xFF,0xFF,0xFF,
          };
          static constexpr const uint8_t reg_data_io1_2[] = {
            0x05, 0b01110110, 0,   // PI4IO_REG_OUT_SET (bit4=LCD Reset,bit5=GT911 TouchReset  HIGH)
            0xFF,0xFF,0xFF,
          };

          static constexpr const uint8_t reg_data_io2[] = {
            0x03, 0b10111001, 0,   // PI4IO_REG_IO_DIR
            0x07, 0b00000110, 0,   // PI4IO_REG_OUT_H_IM
            0x0D, 0b10111001, 0,   // PI4IO_REG_PULL_SEL
            0x0B, 0b11111001, 0,   // PI4IO_REG_PULL_EN
            0x09, 0b01000000, 0,   // PI4IO_REG_IN_DEF_STA
            0x11, 0b10111111, 0,   // PI4IO_REG_INT_MASK
            0x05, 0b10001001, 0,   // PI4IO_REG_OUT_SET
            0xFF,0xFF,0xFF,
          };

          i2c_write_register8_array(in_i2c_port, pi4io1_i2c_addr, reg_data_io1_1, 100000);
          i2c_write_register8_array(in_i2c_port, pi4io2_i2c_addr, reg_data_io2, 100000);
          lgfx::delay(10);
          i2c_write_register8_array(in_i2c_port, pi4io1_i2c_addr, reg_data_io1_2, 100000);

#if !CONFIG_SPIRAM
          ESP_LOGE(LIBRARY_NAME, "M5Tab5 need PSRAM enabled");
          if (false)
#elif CONFIG_SPIRAM_SPEED <= 80
          ESP_LOGE(LIBRARY_NAME, "M5Tab5 need PSRAM SPEED 200MHz");
 #if defined (ESP_ARDUINO_VERSION)
  #if ESP_ARDUINO_VERSION == ESP_ARDUINO_VERSION_VAL(3,3,0)
   #warning "The Arduino-ESP32 v3.3.0 has a problem that PSRAM does not work at 200MHz. Please use v3.3.1 or later or v3.2.x."
  #endif
 #endif
#endif

          {
            auto bus_dsi = new Bus_DSI();
            _bus_last.reset(bus_dsi);
            auto bus_cfg = bus_dsi->config();
            bus_cfg.bus_id = 0;
            bus_cfg.lane_num = 2;
            bus_cfg.lane_mbps = 960;
            bus_cfg.ldo_chan_id = 3;
            bus_cfg.ldo_voltage_mv = 2500;
            bus_dsi->config(bus_cfg);
            if (bus_dsi->init()) {
              bool hit_ili9881 = false;
              bool hit_st7123 = false;
              lgfx::delay(80);
              for (int i = 0; i < 3; ++i) {
                uint8_t id[3] = { 0, };
                bus_dsi->readParams( 0xF4, id, 2 );
                ESP_LOGD(LIBRARY_NAME, "ST ID %02x %02x", id[0], id[1]);
                if (id[0] == 0x71 && id[1] == 0x23) {
                  hit_st7123 = true;
                  break;
                }
                static constexpr uint8_t params_page1[] = { 0x98, 0x81, 0x01 };
                bus_dsi->writeParams( 0xFF, params_page1, 3);
                bus_dsi->readParams( 0x00, &id[0], 1 );
                bus_dsi->readParams( 0x01, &id[1], 1 );
                bus_dsi->readParams( 0x02, &id[2], 1 );
                ESP_LOGD(LIBRARY_NAME, "ILI ID %02x %02x %02x", id[0], id[1], id[2]);
                if (id[0] == 0x98 && id[1] == 0x81) {
                  static constexpr uint8_t params_page0[] = { 0x98, 0x81, 0x00 };
                  bus_dsi->writeParams( 0xFF, params_page0, 3);
                  hit_ili9881 = true;
                  break;
                }
              }
              if (hit_ili9881) {
                _touch_last.reset(new Touch_GT911());
                auto p = new Panel_ILI9881C();
                _panel_last.reset(p);
                auto det = p->config_detail();
                det.dpi_freq_mhz = 60;
                det.hsync_back_porch = 140;
                det.hsync_pulse_width = 40;
                det.hsync_front_porch = 40;
                det.vsync_back_porch = 20;
                det.vsync_pulse_width = 4;
                det.vsync_front_porch = 20;
                p->config_detail(det);
              } else if (hit_st7123) {
                _touch_last.reset(new Touch_ST7123());
                auto p = new Panel_ST7123();
                _panel_last.reset(p);
                auto det = p->config_detail();

                det.dpi_freq_mhz = 80;
                det.hsync_back_porch = 40;
                det.hsync_pulse_width = 2;
                det.hsync_front_porch = 40;
                // note: back + pulse == 10. If it is out of sync, the display position will shift vertically.
                det.vsync_back_porch = 8;
                det.vsync_pulse_width = 2;
                // note: reducing the front porch will cause the touch panel to stop working.
                det.vsync_front_porch = 220;
                p->config_detail(det);
              }
              {
                auto p = _panel_last.get();
                auto cfg = p->config();
                cfg.memory_width = 720;
                cfg.memory_height = 1280;
                cfg.panel_width = 720;
                cfg.panel_height = 1280;
                cfg.readable = true;
                cfg.rgb_order = true;
                cfg.bus_shared = false;
                cfg.offset_x = 0;
                cfg.offset_y = 0;
                cfg.offset_rotation = 0;
                cfg.pin_cs = GPIO_NUM_NC;
                cfg.pin_rst = GPIO_NUM_NC;
                p->config(cfg);
                p->setBus(bus_dsi);
              }
              auto t = _touch_last.get();
              if (t) {
                auto cfg = t->config();

                cfg.pin_rst = -1;
                cfg.pin_sda = GPIO_NUM_31;
                cfg.pin_scl = GPIO_NUM_32;
                cfg.pin_int = GPIO_NUM_23;
                cfg.freq = 400000;
                cfg.x_min = 0;
                cfg.x_max = 719;
                cfg.y_min = 0;
                cfg.y_max = 1279;
                cfg.i2c_port = 1;
                cfg.bus_shared = false;
                cfg.offset_rotation = 0;
                t->config(cfg);
                _panel_last->setTouch(t);
              }
            }
            _set_pwm_backlight(GPIO_NUM_22, 7, 44100);
          }
          goto init_clear;
        }
      }
    }

#elif defined (CONFIG_IDF_TARGET_ESP32C6)

    bus_cfg.spi_host = SPI2_HOST;
    bus_cfg.dma_channel = SPI_DMA_CH_AUTO;

    std::uint32_t id;

    std::uint32_t pkg_ver = m5gfx::get_pkg_ver();
    ESP_LOGD(LIBRARY_NAME, "pkg_ver : %02x", (int)pkg_ver);

    if (pkg_ver == 1)
    { // ESP32C6FH4(QFN32) : NanoC6
      if (board == 0 || board == board_t::board_M5NanoC6)
      {
      }
    } else
    if (pkg_ver == 0)
    { // ESP32C6(QFN40) : NessoN1, UnitC6L
      if (board == 0 || board == board_t::board_ArduinoNessoN1 || board == board_t::board_M5UnitC6L)
      {
        gpio::pin_backup_t backup_pins[] =
        { GPIO_NUM_8
        , GPIO_NUM_10
        , GPIO_NUM_16
        , GPIO_NUM_17
        , GPIO_NUM_18
        , GPIO_NUM_20
        , GPIO_NUM_21
        , GPIO_NUM_22
        };
  /*
  |    | NessoN1 | UnitC6L |
  |:--:+:-------:+:-------:|
  | G8 | SYS_SCL | SYS_SCL |
  | G10| SYS_SDA | SYS_SDA |
  | G18|   GND   |   NC    |
  */
        auto result = lgfx::gpio::command(
          (const uint8_t[]) {
          lgfx::gpio::command_mode_input_pullup  , GPIO_NUM_18,
          lgfx::gpio::command_mode_input_pulldown, GPIO_NUM_8,
          lgfx::gpio::command_mode_input_pulldown, GPIO_NUM_10,
          lgfx::gpio::command_read               , GPIO_NUM_18,
          lgfx::gpio::command_read               , GPIO_NUM_8,
          lgfx::gpio::command_read               , GPIO_NUM_10,
          lgfx::gpio::command_end
          }
        );
        // 3 == NessoN1 / 7 == UnitC6L
        // ESP_LOGE("debug", "\n\nN1/C6L detect %02x\n\n\n", (int)result);
        if (result == 0x07)
        { // UnitC6L ?
          lgfx::i2c::init(i2c_port, GPIO_NUM_10, GPIO_NUM_8);

          _pin_reset(GPIO_NUM_6, use_reset); // LCD RST
          bus_cfg.pin_mosi = GPIO_NUM_21;
          bus_cfg.pin_miso = GPIO_NUM_22; // NC
          bus_cfg.pin_sclk = GPIO_NUM_20;
          bus_cfg.pin_dc   = GPIO_NUM_18;
          bus_cfg.spi_mode = 0;
          bus_cfg.spi_3wire = true;
          bus_spi->config(bus_cfg);
          bus_spi->init();

          // id = _read_panel_id(bus_spi, GPIO_NUM_6);
          // if (id == 0x??) //  check panel (SSD1306)
          {
            board = board_t::board_M5UnitC6L;
            ESP_LOGI(LIBRARY_NAME, "[Autodetect] board_M5UnitC6L");

            bus_spi->release();
            bus_cfg.freq_write = 40000000;
            bus_cfg.freq_read  = 10000000;
            bus_spi->config(bus_cfg);
            bus_spi->init();
            auto p = new Panel_SSD1306();
            p->bus(bus_spi);
            {
              _panel_last.reset(p);
              auto cfg = p->config();
              cfg.pin_cs  = GPIO_NUM_6;  // OLED CS
              cfg.pin_rst = GPIO_NUM_15; // OLED RST
              cfg.panel_width = 64;
              cfg.panel_height = 48;
              cfg.offset_x     = 32;
              cfg.offset_y     = 0;
              cfg.offset_rotation = 0;
              cfg.readable = true;
              cfg.bus_shared = false;
              p->config(cfg);
              p->setRotation(0);
            }
            goto init_clear;
          }
          bus_spi->release();
        } else
        if (result == 0x03)
        { // NessoN1 ?
          lgfx::i2c::init(i2c_port, GPIO_NUM_10, GPIO_NUM_8);
        // PI4IO E0
        //  P0 BTN1
        //  P1 BTN2
        //  P2-P5 NC
        //  P5 LNA Enable
        //  P6 RF Switch
        //  P7 LoRa Reset
          static constexpr const uint8_t reg_data_io1[] = {
            0x03, 0b11100000, 0,   // PI4IO_REG_IO_DIR
            0x05, 0b10000000, 0,   // PI4IO_REG_OUT_SET
            0x07, 0b00011100, 0,   // PI4IO_REG_OUT_H_IM
            0x0D, 0b11000011, 0,   // PI4IO_REG_PULL_SEL
            0x0B, 0b11000011, 0,   // PI4IO_REG_PULL_EN
            0x09, 0b00000011, 0,   // PI4IO_REG_IN_DEF_STA
            0x11, 0b11111100, 0,   // PI4IO_REG_INT_MASK
            0xFF,0xFF,0xFF,
          };
        // PI4IO E1
        // P0 Power OFF system
        // P1 LCD_RST
        // P2 EXT_PWR_EN
        // P3 NC
        // P4 NC
        // P5 VIN_DET
        // P6 LCD_BL
        // P7 SYS_LEDG  Low-level light
          static constexpr const uint8_t reg_data_io2[] = {
            0x03, 0b11000111, 0,   // PI4IO_REG_IO_DIR
            0x07, 0b00011000, 0,   // PI4IO_REG_OUT_H_IM
            0x05, 0b00000000, 0,   // PI4IO_REG_OUT_SET
            0x0D, 0b10000000, 0,   // PI4IO_REG_PULL_SEL
            0x0B, 0b11111111, 0,   // PI4IO_REG_PULL_EN
            0x05, 0b10000010, 0,   // PI4IO_REG_OUT_SET
            0xFF,0xFF,0xFF,
          };
          i2c_write_register8_array(i2c_port, pi4io2_i2c_addr, reg_data_io2, 100000);
          i2c_write_register8_array(i2c_port, pi4io1_i2c_addr, reg_data_io1, 100000);

          bus_cfg.pin_mosi = GPIO_NUM_21;
          bus_cfg.pin_miso = GPIO_NUM_22;
          bus_cfg.pin_sclk = GPIO_NUM_20;
          bus_cfg.pin_dc   = GPIO_NUM_16;
          bus_cfg.spi_mode = 0;
          bus_cfg.spi_3wire = true;
          bus_spi->config(bus_cfg);
          bus_spi->init();

          id = _read_panel_id(bus_spi, GPIO_NUM_17);
          //  check panel (ST7789)
          if ((id & 0xFB) == 0x81) // 0x81 or 0x85
          {
            board = board_t::board_ArduinoNessoN1;
            ESP_LOGI(LIBRARY_NAME, "[Autodetect] board_ArduinoNessoN1");

            bus_spi->release();
            bus_cfg.freq_write = 40000000;
            bus_cfg.freq_read  = 16000000;
            bus_spi->config(bus_cfg);
            bus_spi->init();
            auto p = new Panel_ST7789();
            p->bus(bus_spi);
            {
              _panel_last.reset(p);
              auto cfg = p->config();
              cfg.pin_cs  = GPIO_NUM_17; // LCD CS
              cfg.pin_rst = -1;
              cfg.panel_width = 135;
              cfg.panel_height = 240;
              cfg.offset_x     = 52;
              cfg.offset_y     = 40;
              cfg.offset_rotation = 0;
              cfg.readable = true;
              cfg.invert = true;
              cfg.bus_shared = true;
              p->config(cfg);
              p->setRotation(0);
            }

            {
              auto t = new m5gfx::Touch_FT5x06();
              if (t) {
                _touch_last.reset(t);
                auto cfg = t->config();

                cfg.x_min = 0;
                cfg.x_max = 134;
                cfg.y_min = 0;
                cfg.y_max = 239;
                cfg.bus_shared = true;
                cfg.offset_rotation = 0;
                
                cfg.i2c_port = I2C_NUM_0;
                cfg.i2c_addr = 0x38;
                cfg.pin_rst = -1;
                cfg.pin_int = GPIO_NUM_3;
                cfg.pin_sda = GPIO_NUM_10;
                cfg.pin_scl = GPIO_NUM_8;
                cfg.freq = 400000;

                t->config(cfg);
                p->touch(t);
              }
            }
            _set_backlight(new Light_ArduinoNessoN1());
            goto init_clear;
          }
          bus_spi->release();
        }
        for (auto &bup : backup_pins) { bup.restore(); }
      }
    }

#endif

    board = board_t::board_unknown;
    goto init_clear;
init_clear:

    panel(_panel_last.get());

    return board;
  }

#else

  bool M5GFX::init_impl(bool use_reset, bool use_clear)
  {
    board_t b = board_t::board_unknown;
#if defined (M5GFX_BOARD)
    b = M5GFX_BOARD;
#endif
    _board = autodetect(use_reset, b);
    return LGFX_Device::init_impl(use_reset, use_clear);
  }

  board_t M5GFX::autodetect(bool use_reset, board_t board)
  {
    (void)use_reset;
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
    case board_M5StickC:       title = "M5StickC";       break;
    case board_M5StickCPlus:   title = "M5StickCPlus";   break;
    case board_M5StickCPlus2:  title = "M5StickCPlus2";  break;
    case board_M5StackCoreInk: title = "M5StackCoreInk"; break;
    case board_M5Paper:        title = "M5Paper";        break;
    case board_M5PaperS3:      title = "M5PaperS3";      break;
    case board_M5Tough:        title = "M5Tough";        break;
    case board_M5Station:      title = "M5Station";      break;
    case board_M5AtomS3:       title = "M5AtomS3";       break;
    case board_M5AtomS3R:      title = "M5AtomS3R";      break;
    case board_M5Dial:         title = "M5Dial";         break;
    case board_M5Cardputer:    title = "M5Cardputer";    break;
    case board_M5CardputerADV: title = "M5CardputerADV"; break;
    case board_M5DinMeter:     title = "M5DinMeter";     break;
    case board_M5AirQ:         title = "M5AirQ";         break;
    case board_M5VAMeter:      title = "M5VAMeter";      break;
    case board_M5StampPLC:     title = "M5StampPLC";     break;
    case board_M5Tab5:         title = "M5Tab5";         break;
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
      w = 960;
      h = 540;
      pnl_cfg.offset_rotation = 3;
      p->setColorDepth(lgfx::color_depth_t::grayscale_8bit);
      r = 1;
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
      w = 240;
      h = 135;
      pnl_cfg.offset_rotation = 3;
      r = 1;
      break;

    case board_M5StickCPlus:
    case board_M5StickCPlus2:
    case board_M5DinMeter:
    case board_M5StampPLC:
    case board_ArduinoNessoN1:
      w = 135;
      h = 240;
      break;

    case board_M5StackCore2:
      pnl_cfg.offset_rotation = 3;
      r = 1;
      break;
    case board_M5Stack:
    case board_M5StackCoreS3:
    case board_M5StackCoreS3SE:
      pnl_cfg.offset_rotation = 3;
      r = 1;
      break;

    case board_M5Dial:
      w = 240;
      h = 240;
      break;

      case board_M5VAMeter:
      w = 240;
      h = 240;
      break;

    case board_M5Tab5:
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
