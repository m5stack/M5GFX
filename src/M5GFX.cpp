// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "M5GFX.h"

#if defined ( ESP_PLATFORM )

#include <sdkconfig.h>
#include <nvs.h>
#include <esp_log.h>
#include <driver/i2c.h>
#include <soc/efuse_reg.h>
#include <soc/gpio_reg.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "lgfx/v1/panel/Panel_ILI9342.hpp"
#include "lgfx/v1/panel/Panel_ST7735.hpp"
#include "lgfx/v1/panel/Panel_ST7789.hpp"
#include "lgfx/v1/panel/Panel_GC9A01.hpp"
#include "lgfx/v1/panel/Panel_GDEW0154M09.hpp"
#include "lgfx/v1/panel/Panel_IT8951.hpp"
#include "lgfx/v1/touch/Touch_FT5x06.hpp"
#include "lgfx/v1/touch/Touch_GT911.hpp"

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
      lgfx::gpio_hi(_cfg.pin_rst);
      lgfx::pinMode(_cfg.pin_rst, lgfx::pin_mode_t::input_pulldown);
      _cfg.invert = lgfx::gpio_in(_cfg.pin_rst);       // get panel type (IPS or TN)
      lgfx::pinMode(_cfg.pin_rst, lgfx::pin_mode_t::output);

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

    void reset(void) override
    {
      // AXP192 reg 0x96 = GPIO3&4 control
      lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x96, 0, ~0x02, axp_i2c_freq); // GPIO4 LOW (LCD RST)
      lgfx::delay(4);
      lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x96, 2, ~0x00, axp_i2c_freq); // GPIO4 HIGH (LCD RST)
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

    void reset(void) override
    {
      lgfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x03, 0, ~(1<<5), i2c_freq);  // LCD_RST
      lgfx::delay(4);
      lgfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x03, (1<<5), ~0, i2c_freq);  // LCD_RST
    }

    void cs_control(bool flg) override
    {
      lgfx::Panel_ILI9342::cs_control(flg);
      // CS操作時にGPIO35の役割を切り替える (MISO or D/C);
      // 0x43==FSPI MISO / 0x100==GPIO OUT
      *(volatile uint32_t*)GPIO_FUNC35_OUT_SEL_CFG_REG = flg ? 0x43 : 0x100;
      if (flg == false)
      { // CS low の場合はD/Cとして扱うためGPIO出力を有効にする;
        *(volatile uint32_t*)GPIO_ENABLE1_W1TS_REG = (0x1 << (GPIO_NUM_35 & 31));
      }
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
        brightness = (brightness / 25) + 18;
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

  struct Light_M5AtomS3 : public lgfx::Light_PWM
  {
    Light_M5AtomS3(void)
    {
      auto cfg = config();
      /// The backlight of AtomS3 does not light up if the PWM cycle is too fast.
      cfg.freq = 240;
      cfg.pin_bl = GPIO_NUM_16;
      cfg.pwm_channel = 7;
      config(cfg);
    }

    void setBrightness(uint8_t brightness) override
    {
      if (brightness) 
      {
        brightness = brightness - (brightness >> 3) + 31;
      }
      Light_PWM::setBrightness(brightness);
    }
  };


#endif

  static void _pin_level(std::int_fast16_t pin, bool level)
  {
    lgfx::pinMode(pin, lgfx::pin_mode_t::output);
    if (level) lgfx::gpio_hi(pin);
    else       lgfx::gpio_lo(pin);
  }

  static void _pin_reset(std::int_fast16_t pin, bool use_reset)
  {
    lgfx::gpio_hi(pin);
    lgfx::pinMode(pin, lgfx::pin_mode_t::output);
    if (!use_reset) return;
    lgfx::gpio_lo(pin);
    auto time = lgfx::millis();
    do
    {
      lgfx::delay(1);
    } while (lgfx::millis() - time < 2);
    lgfx::gpio_hi(pin);
    time = lgfx::millis();
    do
    {
      lgfx::delay(1);
    } while (lgfx::millis() - time < 10);
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

    ESP_LOGD(LIBRARY_NAME, "[Autodetect] read cmd:%02" PRIx32 " = %08" PRIx32, cmd, res);
    return res;
  }

  void M5GFX::_set_backlight(lgfx::ILight* bl)
  {
//  if (_light_last) { delete _light_last; }
    _light_last.reset(bl);
    _panel_last->setLight(bl);
  }

  void M5GFX::_set_pwm_backlight(std::int16_t pin, std::uint8_t ch, std::uint32_t freq, bool invert)
  {
    auto bl = new lgfx::Light_PWM();
    auto cfg = bl->config();
    cfg.pin_bl = pin;
    cfg.freq   = freq;
    cfg.pwm_channel = ch;
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
      ESP_LOGI(LIBRARY_NAME, "[Autodetect] load from NVS : board:%" PRIu32, nvs_board);
    }

    if (0 == nvs_board)
    {
#if defined ( ARDUINO_M5STACK_CORE_ESP32 ) || defined ( ARDUINO_M5STACK_FIRE ) || defined ( ARDUINO_M5Stack_Core_ESP32 )

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
      //ESP_LOGD(LIBRARY_NAME,"autodetect board:%" PRIu32, board);
    } while (board_t::board_unknown == board && --retry >= 0);
    _board = board;

#if defined ( ARDUINO_M5STACK_ATOM ) || defined ( ARDUINO_M5Stack_ATOM )

    if (board == board_t::board_unknown || board == board_t::board_M5Atom)
    {
      return false;
    }

#endif

    /// autodetectの際にreset済みなのでここではuse_resetをfalseで呼び出す。;
    /// M5Paperはreset後の復帰に800msec程度掛かるのでreset省略は起動時間短縮に有効;
    bool res = LGFX_Device::init_impl(false, use_clear);

    if (nvs_board != board) {
      if (0 == nvs_open(LIBRARY_NAME, NVS_READWRITE, &nvs_handle)) {
        ESP_LOGI(LIBRARY_NAME, "[Autodetect] save to NVS : board:%d", (int)board);
        nvs_set_u32(nvs_handle, NVS_KEY, board);
        nvs_close(nvs_handle);
      }
    }
    return res;
  }

  board_t M5GFX::autodetect(bool use_reset, board_t board)
  {
    auto bus_spi = new Bus_SPI();
    _bus_last.reset(bus_spi);

    panel(nullptr);

    auto bus_cfg = bus_spi->config();
    bus_cfg.freq_write = 8000000;
    bus_cfg.freq_read  = 8000000;
    bus_cfg.spi_mode = 0;
    bus_cfg.use_lock = true;

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)

    bus_cfg.spi_host = VSPI_HOST;
    bus_cfg.dma_channel = 1;

    std::uint32_t id;

    std::uint32_t pkg_ver = m5gfx::get_pkg_ver();
//  ESP_LOGD(LIBRARY_NAME, "pkg_ver : %02" PRIx32, pkg_ver);

    if (pkg_ver == EFUSE_RD_CHIP_VER_PKG_ESP32PICOD4)  /// check PICO-D4 (M5StickC,CPlus,T,T2 / CoreInk / ATOM )
    {
      if (board == 0 || board == board_t::board_M5StickC || board == board_t::board_M5StickCPlus)
      {
        bus_cfg.pin_mosi = GPIO_NUM_15;
        bus_cfg.pin_miso = GPIO_NUM_14;
        bus_cfg.pin_sclk = GPIO_NUM_13;
        bus_cfg.pin_dc   = GPIO_NUM_23;
        bus_cfg.spi_3wire = true;
        bus_spi->config(bus_cfg);
        bus_spi->init();
        _pin_reset(GPIO_NUM_18, use_reset); // LCD RST
        id = _read_panel_id(bus_spi, GPIO_NUM_5);
        if ((id & 0xFF) == 0x85)
        {  //  check panel (ST7789)
          board = board_t::board_M5StickCPlus;
          ESP_LOGI(LIBRARY_NAME, "[Autodetect] M5StickCPlus");
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
          bus_cfg.freq_write = 27000000;
          bus_cfg.freq_read  = 14000000;
          bus_spi->config(bus_cfg);
          auto p = new Panel_M5StickC();
          p->bus(bus_spi);
          _panel_last.reset(p);
          _set_backlight(new Light_M5StickC());
          goto init_clear;
        }
        lgfx::pinMode(GPIO_NUM_18, lgfx::pin_mode_t::input); // LCD RST
        lgfx::pinMode(GPIO_NUM_5 , lgfx::pin_mode_t::input); // LCD CS
        bus_spi->release();
      }

      if (board == 0 || board == board_t::board_M5StackCoreInk)
      {
        _pin_reset( GPIO_NUM_0, true); // EPDがDeepSleepしている場合は自動認識に失敗する。そのためRST制御を必ず行う。;
        bus_cfg.pin_mosi = GPIO_NUM_23;
        bus_cfg.pin_miso = GPIO_NUM_34;
        bus_cfg.pin_sclk = GPIO_NUM_18;
        bus_cfg.pin_dc   = GPIO_NUM_15;
        bus_cfg.spi_3wire = true;
        bus_spi->config(bus_cfg);
        bus_spi->init();
        id = _read_panel_id(bus_spi, GPIO_NUM_9, 0x70, 0);
        if (id == 0x00F00000)
        {  //  check panel (e-paper GDEW0154M09)
          _pin_level(GPIO_NUM_12, true);  // POWER_HOLD_PIN 12
          board = board_t::board_M5StackCoreInk;
          ESP_LOGI(LIBRARY_NAME, "[Autodetect] M5StackCoreInk");
          bus_cfg.freq_write = 40000000;
          bus_cfg.freq_read  = 16000000;
          bus_spi->config(bus_cfg);
          auto p = new lgfx::Panel_GDEW0154M09();
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
        lgfx::pinMode(GPIO_NUM_0, lgfx::pin_mode_t::input); // RST
        lgfx::pinMode(GPIO_NUM_9, lgfx::pin_mode_t::input); // CS
        bus_spi->release();
      }

/// LCD / EPD 検出失敗の場合はATOM 判定;

    }
    else if (pkg_ver == 6)  // PICOV3_02 (ATOM PSRAM)
    {
      board = board_t::board_M5AtomPsram;
      goto init_clear;
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
        // I2C addr 0x34 = AXP192
        lgfx::i2c::init(axp_i2c_port, axp_i2c_sda, axp_i2c_scl);
        if (lgfx::i2c::readRegister8(axp_i2c_port, axp_i2c_addr, 0x03, 400000) == 0x03) // AXP192 found
        {
          ESP_LOGD(LIBRARY_NAME, "AXP192 found");
          if (board == 0 || board == board_t::board_M5Station)
          {
            bus_cfg.pin_mosi = GPIO_NUM_23;
            bus_cfg.pin_miso = -1;
            bus_cfg.pin_sclk = GPIO_NUM_18;
            bus_cfg.pin_dc   = GPIO_NUM_19;
            bus_cfg.spi_3wire = true;
            bus_spi->config(bus_cfg);
            bus_spi->init();
            _pin_reset(GPIO_NUM_15, use_reset); // LCD RST;

            id = _read_panel_id(bus_spi, GPIO_NUM_5);
            if ((id & 0xFF) == 0x85)
            {   // ST7789
              ESP_LOGI(LIBRARY_NAME, "[Autodetect] M5Station");
              board = board_t::board_M5Station;

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
            lgfx::pinMode(GPIO_NUM_5 , lgfx::pin_mode_t::input); // LCD CS
            lgfx::pinMode(GPIO_NUM_15, lgfx::pin_mode_t::input); // LCD RST
          }

          if (board == 0 || board == board_t::board_M5StackCore2 || board == board_t::board_M5Tough)
          {
            if (lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x95, 0x84, 0x72, axp_i2c_freq)) // GPIO4 enable
            {
              // AXP192_LDO2 = LCD PWR
              // AXP192_IO4  = LCD RST
              // AXP192_DC3  = LCD BL (Core2)
              // AXP192_LDO3 = LCD BL (Tough)
              // AXP192_IO1  = TP RST (Tough)
              lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x28, 0xF0, ~0, axp_i2c_freq);   // set LDO2 3300mv // LCD PWR
              lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x12, 0x04, ~0, axp_i2c_freq);   // LDO2 enable
              lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x92, 0x00, 0xF8, axp_i2c_freq); // GPIO1 OpenDrain (M5Tough TOUCH)
              lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x95, 0x84, 0x72, axp_i2c_freq); // GPIO4 enable
              if (use_reset)
              {
                lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x96, 0, ~0x02, axp_i2c_freq); // GPIO4 LOW (LCD RST)
                lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x94, 0, ~0x02, axp_i2c_freq); // GPIO1 LOW (M5Tough TOUCH RST)
                lgfx::delay(1);
              }
              lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x96, 0x02, ~0, axp_i2c_freq);   // GPIO4 HIGH (LCD RST)
              lgfx::i2c::writeRegister8(axp_i2c_port, axp_i2c_addr, 0x94, 0x02, ~0, axp_i2c_freq);   // GPIO1 HIGH (M5Tough TOUCH RST)

              _pin_level(GPIO_NUM_5, true);
              // ets_delay_us(128); // AXP 起動後、LCDがアクセス可能になるまで少し待機;

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

                  _set_backlight(new Light_M5StackCore2());

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
              lgfx::pinMode(GPIO_NUM_4, lgfx::pin_mode_t::input); // TF card CS
              lgfx::pinMode(GPIO_NUM_5, lgfx::pin_mode_t::input); // LCD CS
              bus_spi->release();
            }
          }
        }
      }

      if (board == 0 || board == board_t::board_M5Stack)
      {
        bus_cfg.pin_mosi = GPIO_NUM_23;
        bus_cfg.pin_miso = GPIO_NUM_19;
        bus_cfg.pin_sclk = GPIO_NUM_18;
        bus_cfg.pin_dc   = GPIO_NUM_27;

        bus_cfg.spi_3wire = true;
        bus_spi->config(bus_cfg);
        bus_spi->init();
        _pin_reset(GPIO_NUM_33, use_reset); // LCD RST;

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
        lgfx::pinMode(GPIO_NUM_4 , lgfx::pin_mode_t::input); // TF card CS
        lgfx::pinMode(GPIO_NUM_14, lgfx::pin_mode_t::input); // LCD CS
        lgfx::pinMode(GPIO_NUM_33, lgfx::pin_mode_t::input); // LCD RST
      }


      if (board == 0 || board == board_t::board_M5Paper)
      {
        _pin_reset(GPIO_NUM_23, true);
        lgfx::pinMode(GPIO_NUM_27, lgfx::pin_mode_t::input_pullup); // M5Paper EPD busy pin
        if (!lgfx::gpio_in(GPIO_NUM_27))
        {
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
          lgfx::pinMode(GPIO_NUM_15, lgfx::pin_mode_t::input); // EPD CS
          lgfx::pinMode(GPIO_NUM_4, lgfx::pin_mode_t::input); // M5Paper TF card CS
          lgfx::pinMode(GPIO_NUM_2, lgfx::pin_mode_t::input); // M5EPD_MAIN_PWR_PIN 2
        }
        lgfx::pinMode(GPIO_NUM_27, lgfx::pin_mode_t::input); // BUSY
        lgfx::pinMode(GPIO_NUM_23, lgfx::pin_mode_t::input); // RST
      }
    }

#elif defined (CONFIG_IDF_TARGET_ESP32S3)

    bus_cfg.spi_host = SPI2_HOST;
    bus_cfg.dma_channel = SPI_DMA_CH_AUTO;

    std::uint32_t id;

//     std::uint32_t pkg_ver = m5gfx::get_pkg_ver();
// ESP_LOGE("DEBUG","pkg_ver:%02" PRIx32, pkg_ver);

      if (board == 0 || board == board_t::board_M5AtomS3)
      {
        bus_cfg.pin_mosi = GPIO_NUM_21;
        bus_cfg.pin_miso = GPIO_NUM_13;
        bus_cfg.pin_sclk = GPIO_NUM_17;
        bus_cfg.pin_dc   = GPIO_NUM_33;
        bus_cfg.spi_mode = 0;
        bus_cfg.spi_3wire = true;
        bus_spi->config(bus_cfg);
        bus_spi->init();
        _pin_reset(GPIO_NUM_34, use_reset); // LCD RST
        id = _read_panel_id(bus_spi, GPIO_NUM_15);
        if ((id & 0xFFFFFF) == 0x079100)
        {  //  check panel (GC9107)
          board = board_t::board_M5AtomS3;
          ESP_LOGW(LIBRARY_NAME, "[Autodetect] board_M5AtomS3");
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
          _set_backlight(new Light_M5AtomS3());

          goto init_clear;
        }
        lgfx::pinMode(GPIO_NUM_48, lgfx::pin_mode_t::input); // LCD RST
        bus_spi->release();
      }

      if (board == 0 || board == board_t::board_M5StackCoreS3)
      {
        lgfx::i2c::init(i2c_port, i2c_sda, i2c_scl);

// ESP_LOGI("DEBUG","AW 0x10 :%02" PRIx32, lgfx::i2c::readRegister8(i2c_port, aw9523_i2c_addr, 0x10, 400000).value());
// ESP_LOGI("DEBUG","AXP0x03 :%02" PRIx32, lgfx::i2c::readRegister8(i2c_port, axp_i2c_addr, 0x03, 400000).value());

        auto chk_axp = lgfx::i2c::readRegister8(i2c_port, axp_i2c_addr, 0x03, i2c_freq);
        if (chk_axp.has_value() && chk_axp.value() == 0x4A)
        {
          auto chk_aw  = lgfx::i2c::readRegister8(i2c_port, aw9523_i2c_addr, 0x10, i2c_freq);
          if (chk_aw .has_value() && chk_aw .value() == 0x23)
          {
            m5gfx::i2c::writeRegister8(i2c_port, axp_i2c_addr, 0x90, 0xBF); // LDOS ON/OFF control 0
            m5gfx::i2c::writeRegister8(i2c_port, axp_i2c_addr, 0x95, 0x28); // ALDO3 set to 3.3v // for TF card slot

            m5gfx::i2c::bitOn(i2c_port, aw9523_i2c_addr, 0x02, 0b00000101); //port0 output ctrl
            m5gfx::i2c::bitOn(i2c_port, aw9523_i2c_addr, 0x03, 0b00000011); //port1 output ctrl
            m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x04, 0b00011000);  // CONFIG_P0
            m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x05, 0b00001100);  // CONFIG_P1
            m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x11, 0b00010000);  // GCR P0 port is Push-Pull mode.
            m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x12, 0b11111111);  // LEDMODE_P0
            m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x13, 0b11111111);  // LEDMODE_P1

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
              ESP_LOGW(LIBRARY_NAME, "[Autodetect] board_M5StackCoreS3");
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


#endif

    board = board_t::board_unknown;
    goto init_clear;
init_clear:

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
