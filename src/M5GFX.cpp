#if defined (ESP32) || defined (CONFIG_IDF_TARGET_ESP32) || defined (CONFIG_IDF_TARGET_ESP32S2) || defined (ESP_PLATFORM)

#include "M5GFX.h"

#include "lgfx/v1/panel/Panel_ILI9342.hpp"
#include "lgfx/v1/panel/Panel_ST7735.hpp"
#include "lgfx/v1/panel/Panel_ST7789.hpp"
#include "lgfx/v1/panel/Panel_GDEW0154M09.hpp"
#include "lgfx/v1/panel/Panel_IT8951.hpp"
#include "lgfx/v1/touch/Touch_FT5x06.hpp"
#include "lgfx/v1/touch/Touch_GT911.hpp"

#include <nvs.h>
#include <esp_log.h>
#include <driver/i2c.h>

namespace m5gfx
{
  static constexpr char LIBRARY_NAME[] = "M5GFX";

  static constexpr std::int32_t axp_i2c_freq = 400000;
  static constexpr std::uint_fast8_t axp_i2c_addr = 0x34;
  static constexpr std::int_fast16_t axp_i2c_port = I2C_NUM_1;
  static constexpr std::int_fast16_t axp_i2c_sda = 21;
  static constexpr std::int_fast16_t axp_i2c_scl = 22;

  M5GFX* M5GFX::_instance = nullptr;

  struct Panel_M5Stack : public lgfx::Panel_ILI9342
  {
    Panel_M5Stack(void)
    {
      _cfg.pin_cs  = 14;
      _cfg.pin_rst = 33;
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
      _cfg.pin_cs = 5;
      _cfg.invert = true;
      _cfg.offset_rotation = 3;

      _rotation = 1; // default rotation
    }

    void reset(void) override
    {
      // AXP192 reg 0x96 = GPIO3&4 control
      lgfx::i2c::registerWrite8(axp_i2c_port, axp_i2c_addr, 0x96, 0, ~0x02, axp_i2c_freq); // GPIO4 LOW (LCD RST)
      lgfx::delay(4);
      lgfx::i2c::registerWrite8(axp_i2c_port, axp_i2c_addr, 0x96, 2, ~0x00, axp_i2c_freq); // GPIO4 HIGH (LCD RST)
    }
  };

  struct Light_M5StackCore2 : public lgfx::ILight
  {
    void init(std::uint8_t brightness) override
    {
      setBrightness(brightness);
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
      lgfx::i2c::registerWrite8(axp_i2c_port, axp_i2c_addr, 0x27, brightness, 0x80, axp_i2c_freq);
    }
  };

  struct Panel_M5StickC : public lgfx::Panel_ST7735S
  {
    Panel_M5StickC(void)
    {
      _cfg.invert = true;
      _cfg.pin_cs  = 5;
      _cfg.pin_rst = 18;
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
    void init(std::uint8_t brightness) override
    {
      lgfx::i2c::init(axp_i2c_port, axp_i2c_sda, axp_i2c_scl);
      lgfx::i2c::registerWrite8(axp_i2c_port, axp_i2c_addr, 0x12, 0x4D, ~0, axp_i2c_freq);
      setBrightness(brightness);
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
      lgfx::i2c::registerWrite8(axp_i2c_port, axp_i2c_addr, 0x28, brightness << 4, 0x0F, axp_i2c_freq);
    }
  };

  struct Panel_M5StickCPlus : public lgfx::Panel_ST7789
  {
    Panel_M5StickCPlus(void)
    {
      _cfg.invert = true;
      _cfg.pin_cs  = 5;
      _cfg.pin_rst = 18;
      _cfg.panel_width  = 135;
      _cfg.panel_height = 240;
      _cfg.offset_x     = 52;
      _cfg.offset_y     = 40;
    }
  };

  M5GFX::M5GFX(void) : LGFX_Device()
  {
    if (_instance == nullptr) _instance = this;
  }

  static void _pin_level(std::int_fast16_t pin, bool level)
  {
    lgfx::pinMode(pin, lgfx::pin_mode_t::output); // M5Stack TF card CS
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

  static std::uint32_t _read_panel_id(lgfx::Bus_SPI* bus, std::int32_t pin_cs, std::uint32_t cmd = 0x04, std::uint8_t dummy_read_bit = 1) // 0x04 = RDDID command
  {
    bus->beginTransaction();
    _pin_level(pin_cs, false);
    bus->writeCommand(cmd, 8);
    if (dummy_read_bit) bus->writeData(0, dummy_read_bit);  // dummy read bit
    bus->beginRead();
    std::uint32_t res = bus->readData(32);
    bus->endTransaction();
    _pin_level(pin_cs, true);

    ESP_LOGW(LIBRARY_NAME, "[Autodetect] read cmd:%02x = %08x", cmd, res);
    return res;
  }

  bool M5GFX::init_impl(bool use_reset, bool use_clear)
  {
    static constexpr char NVS_KEY[] = "AUTODETECT";
    std::uint32_t nvs_board = 0;
    std::uint32_t nvs_handle = 0;
    if (0 == nvs_open(LIBRARY_NAME, NVS_READONLY, &nvs_handle))
    {
      nvs_get_u32(nvs_handle, NVS_KEY, static_cast<uint32_t*>(&nvs_board));
      nvs_close(nvs_handle);
      ESP_LOGW(LIBRARY_NAME, "[Autodetect] load from NVS : board:%d", nvs_board);
    }

    if (0 == nvs_board)
    {
#if defined ( ARDUINO_M5Stack_Core_ESP32 ) || defined ( ARDUINO_M5STACK_FIRE )

      nvs_board = board_t::board_M5Stack;

#elif defined ( ARDUINO_M5STACK_Core2 )

      nvs_board = board_t::board_M5StackCore2;

#elif defined ( ARDUINO_M5Stick_C )

      nvs_board = board_t::board_M5StickC;

#elif defined ( ARDUINO_M5Stick_C_Plus )

      nvs_board = board_t::board_M5StickCPlus;

#elif defined ( ARDUINO_M5Stack_CoreInk )

      nvs_board = board_t::board_M5StackCoreInk;

#elif defined ( ARDUINO_M5STACK_Paper )

      nvs_board = board_t::board_M5Paper;

#elif defined ( ARDUINO_M5Stack_ATOM )
#elif defined ( ARDUINO_M5STACK_TOUGH )
#elif defined ( ARDUINO_M5Stack-Timer-CAM )
#endif
    }

    _board = (board_t)nvs_board;

    int retry = 4;
    do
    {
      if (retry == 1) use_reset = true;
      _board = autodetect(use_reset, _board);
      //ESP_LOGI("M5GFX","autodetect _board:%d", _board);
    } while (board_t::board_unknown == _board && --retry >= 0);

    /// autodetectの際にreset済みなのでここではuse_resetをfalseで呼び出す。
    /// M5Paperはreset後の復帰に800msec程度掛かるのでreset省略は起動時間短縮に有効
    bool res = LGFX_Device::init_impl(false, use_clear);

    if (nvs_board != _board) {
      if (0 == nvs_open(LIBRARY_NAME, NVS_READWRITE, &nvs_handle)) {
        ESP_LOGW(LIBRARY_NAME, "[Autodetect] save to NVS : board:%d", _board);
        nvs_set_u32(nvs_handle, NVS_KEY, _board);
        nvs_close(nvs_handle);
      }
    }
    return res;
  }

  board_t M5GFX::autodetect(bool use_reset, board_t board)
  {
    auto bus_cfg = _bus_spi.config();

    panel(nullptr);

    if (_panel_last)
    {
      delete _panel_last;
      _panel_last = nullptr;
    }
    if (_light_last)
    {
      delete _light_last;
      _light_last = nullptr;
    }
    if (_touch_last)
    {
      delete _touch_last;
      _touch_last = nullptr;
    }

    bus_cfg.freq_write = 8000000;
    bus_cfg.freq_read  = 8000000;
    bus_cfg.spi_host = VSPI_HOST;
    bus_cfg.spi_mode = 0;
    bus_cfg.dma_channel = 1;
    bus_cfg.use_lock = true;

    std::uint32_t id;

    if (board == 0 || board == board_t::board_M5Stack)
    {
      _pin_level(14, true);     // LCD CS;
      bus_cfg.pin_mosi = 23;
      bus_cfg.pin_miso = 19;
      bus_cfg.pin_sclk = 18;
      bus_cfg.pin_dc   = 27;

      _pin_level( 4, true);  // M5Stack TF card CS

      bus_cfg.spi_3wire = true;
      _bus_spi.config(bus_cfg);
      _bus_spi.init();
      _pin_reset(33, use_reset); // LCD RST;

      id = _read_panel_id(&_bus_spi, 14);
      if ((id & 0xFF) == 0xE3)
      {   // ILI9342c
        ESP_LOGW(LIBRARY_NAME, "[Autodetect] M5Stack");
        board = board_t::board_M5Stack;

        bus_cfg.freq_write = 40000000;
        bus_cfg.freq_read  = 16000000;
        _bus_spi.config(bus_cfg);

        auto p = new Panel_M5Stack();
        p->bus(&_bus_spi);
        _panel_last = p;

        {
          auto bl = new lgfx::Light_PWM();
          auto cfg = bl->config();
          cfg.pin_bl = 32;
          cfg.freq   = 44100;
          cfg.pwm_channel = 7;
          bl->config(cfg);
          _light_last = bl;
          p->light(bl);
        }

        goto init_clear;
      }
      _bus_spi.release();
      lgfx::pinMode(33, lgfx::pin_mode_t::input); // LCD RST
      lgfx::pinMode(14, lgfx::pin_mode_t::input); // LCD CS
      lgfx::pinMode( 4, lgfx::pin_mode_t::input); // M5Stack TF card CS
    }


    if (board == 0 || board == board_t::board_M5StickC || board == board_t::board_M5StickCPlus)
    {
      bus_cfg.pin_mosi = 15;
      bus_cfg.pin_miso = 14;
      bus_cfg.pin_sclk = 13;
      bus_cfg.pin_dc   = 23;
      bus_cfg.spi_3wire = true;
      _bus_spi.config(bus_cfg);
      _bus_spi.init();
      _pin_reset(18, use_reset); // LCD RST
      id = _read_panel_id(&_bus_spi, 5);
      if ((id & 0xFF) == 0x85)
      {  //  check panel (ST7789)
        board = board_t::board_M5StickCPlus;
        ESP_LOGW(LIBRARY_NAME, "[Autodetect] M5StickCPlus");
        bus_cfg.freq_write = 80000000;
        bus_cfg.freq_read  = 16000000;
        _bus_spi.config(bus_cfg);
        auto p = new Panel_M5StickCPlus();
        p->bus(&_bus_spi);
        _light_last = new Light_M5StickC();
        p->light(_light_last);
        _panel_last = p;
        goto init_clear;
      }
      if ((id & 0xFF) == 0x7C)
      {  //  check panel (ST7735)
        board = board_t::board_M5StickC;
        ESP_LOGW(LIBRARY_NAME, "[Autodetect] M5StickC");
        bus_cfg.freq_write = 27000000;
        bus_cfg.freq_read  = 14000000;
        _bus_spi.config(bus_cfg);
        auto p = new Panel_M5StickC();
        p->bus(&_bus_spi);
        _light_last = new Light_M5StickC();
        p->light(_light_last);
        _panel_last = p;
        goto init_clear;
      }
      lgfx::pinMode(18, lgfx::pin_mode_t::input); // LCD RST
      lgfx::pinMode( 5, lgfx::pin_mode_t::input); // LCD CS
      _bus_spi.release();
    }


    if (board == 0 || board == board_t::board_M5StackCoreInk)
    {
      bus_cfg.pin_mosi = 23;
      bus_cfg.pin_miso = -1;
      bus_cfg.pin_sclk = 18;
      bus_cfg.pin_dc   = 15;
      bus_cfg.spi_3wire = true;
      _bus_spi.config(bus_cfg);
      _bus_spi.init();
      _pin_reset( 0, true); // EPDがDeepSleepしていると自動認識に失敗するためRST制御は必須とする
      id = _read_panel_id(&_bus_spi, 9, 0x70, 0);
      if (id == 0x00F00000)
      {  //  check panel (e-paper GDEW0154M09)
        _pin_level(12, true);  // POWER_HOLD_PIN 12
        board = board_t::board_M5StackCoreInk;
        ESP_LOGW(LIBRARY_NAME, "[Autodetect] M5StackCoreInk");
        bus_cfg.freq_write = 40000000;
        bus_cfg.freq_read  = 16000000;
        _bus_spi.config(bus_cfg);
        auto p = new lgfx::Panel_GDEW0154M09();
        p->bus(&_bus_spi);
        _panel_last = p;
        auto cfg = p->config();
        cfg.panel_height = 200;
        cfg.panel_width  = 200;
        cfg.pin_cs   = 9;
        cfg.pin_rst  = 0;
        cfg.pin_busy = 4;
        p->config(cfg);
        goto init_clear;
      }
      lgfx::pinMode( 0, lgfx::pin_mode_t::input); // RST
      lgfx::pinMode( 9, lgfx::pin_mode_t::input); // CS
      _bus_spi.release();
    }


    if (board == 0 || board == board_t::board_M5Paper)
    {
      _pin_reset(23, true);
      lgfx::pinMode(27, lgfx::pin_mode_t::input_pullup); // M5Paper EPD busy pin
      if (!lgfx::gpio_in(27))
      {
        _pin_level( 2, true);  // M5EPD_MAIN_PWR_PIN 2
        lgfx::pinMode(27, lgfx::pin_mode_t::input);
        bus_cfg.pin_mosi = 12;
        bus_cfg.pin_miso = 13;
        bus_cfg.pin_sclk = 14;
        bus_cfg.pin_dc   = -1;
        bus_cfg.spi_3wire = false;
        _bus_spi.config(bus_cfg);
        id = lgfx::millis();
        do
        {
          if (lgfx::millis() - id > 1000) { id = 0; break; }
          lgfx::delay(1);
        } while (!lgfx::gpio_in(27));
        if (id)
        {
          _pin_level( 4, true);  // M5Paper TF card CS
          _bus_spi.init();
          _bus_spi.beginTransaction();
          _pin_level(15, false); // M5Paper CS;
          _bus_spi.writeData(__builtin_bswap16(0x6000), 16);
          _bus_spi.writeData(__builtin_bswap16(0x0302), 16);  // read DevInfo
          _bus_spi.wait();
          lgfx::gpio_hi(15);
          id = lgfx::millis();
          while (!lgfx::gpio_in(27))
          {
            if (lgfx::millis() - id > 150) { break; }
            lgfx::delay(1);
          };
          lgfx::gpio_lo(15);
          _bus_spi.writeData(__builtin_bswap16(0x1000), 16);
          _bus_spi.writeData(__builtin_bswap16(0x0000), 16);
          std::uint8_t buf[40];
          _bus_spi.beginRead();
          _bus_spi.readBytes(buf, 40, false);
          _bus_spi.endRead();
          _bus_spi.endTransaction();
          lgfx::gpio_hi(15);
          id = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
          ESP_LOGW("LovyanGFX", "[Autodetect] panel size :%08x", id);
          if (id == 0x03C0021C)
          {  //  check panel ( panel size 960(0x03C0) x 540(0x021C) )
            board = board_t::board_M5Paper;
            ESP_LOGW(LIBRARY_NAME, "[Autodetect] M5Paper");
            bus_cfg.freq_write = 40000000;
            bus_cfg.freq_read  = 20000000;
            _bus_spi.config(bus_cfg);
            auto p = new lgfx::Panel_IT8951();
            p->bus(&_bus_spi);
            _panel_last = p;
            auto cfg = p->config();
            cfg.panel_height = 540;
            cfg.panel_width  = 960;
            cfg.pin_cs   = 15;
            cfg.pin_rst  = 23;
            cfg.pin_busy = 27;
            cfg.offset_rotation = 3;
            p->config(cfg);
            {
              auto t = new lgfx::Touch_GT911();
              _touch_last = t;
              auto cfg = t->config();
              cfg.pin_int  = 36;   // INT pin number
              cfg.pin_sda  = 21;   // I2C SDA pin number
              cfg.pin_scl  = 22;   // I2C SCL pin number
              cfg.i2c_addr = 0x14; // I2C device addr
              cfg.i2c_port = I2C_NUM_0;// I2C port number
              cfg.freq = 400000;   // I2C freq
              cfg.x_min = 0;
              cfg.x_max = 539;
              cfg.y_min = 0;
              cfg.y_max = 959;
              cfg.offset_rotation = 1;
              t->config(cfg);
              if (!t->init())
              {
                cfg.i2c_addr = 0x5D; // addr change (0x14 or 0x5D)
                t->config(cfg);
              }
              p->touch(t);
            }
            goto init_clear;
          }
          _bus_spi.release();
          lgfx::pinMode( 4, lgfx::pin_mode_t::input); // M5Paper TF card CS
          lgfx::pinMode(15, lgfx::pin_mode_t::input); // EPD CS
        }
        lgfx::pinMode( 2, lgfx::pin_mode_t::input); // M5EPD_MAIN_PWR_PIN 2
      }
      lgfx::pinMode(27, lgfx::pin_mode_t::input); // BUSY
      lgfx::pinMode(23, lgfx::pin_mode_t::input); // RST
    }


    if (board == 0 || board == board_t::board_M5StackCore2)
    {
      lgfx::i2c::init(axp_i2c_port, axp_i2c_sda, axp_i2c_scl);
      // I2C addr 0x34 = AXP192
      if (lgfx::i2c::registerWrite8(axp_i2c_port, axp_i2c_addr, 0x95, 0x84, 0x72, axp_i2c_freq))
      { // GPIO4 enable
        // AXP192_LDO2 = LCD PWR
        // AXP192_DC3  = LCD BL
        // AXP192_IO4  = LCD RST

        if (use_reset) lgfx::i2c::registerWrite8(axp_i2c_port, axp_i2c_addr, 0x96, 0, ~0x02, axp_i2c_freq); // GPIO4 LOW (LCD RST)
        lgfx::i2c::registerWrite8(axp_i2c_port, axp_i2c_addr, 0x28, 0xF0, ~0, axp_i2c_freq);   // set LDO2 3300mv // LCD PWR
        lgfx::i2c::registerWrite8(axp_i2c_port, axp_i2c_addr, 0x12, 0x06, ~0, axp_i2c_freq);   // LDO2 and DC3 enable (DC3 = LCD BL)
        lgfx::i2c::registerWrite8(axp_i2c_port, axp_i2c_addr, 0x96, 0x02, ~0, axp_i2c_freq);   // GPIO4 HIGH (LCD RST)

        ets_delay_us(128); // AXP 起動後、LCDがアクセス可能になるまで少し待機

        bus_cfg.pin_mosi = 23;
        bus_cfg.pin_miso = 38;
        bus_cfg.pin_sclk = 18;
        bus_cfg.pin_dc   = 15;
        bus_cfg.spi_3wire = true;
        _bus_spi.config(bus_cfg);
        _bus_spi.init();

        _pin_level( 4, true);   // TF card CS
        id = _read_panel_id(&_bus_spi, 5);
        if ((id & 0xFF) == 0xE3)
        {   // ILI9342c
          ESP_LOGW(LIBRARY_NAME, "[Autodetect] M5StackCore2");
          board = board_t::board_M5StackCore2;

          bus_cfg.freq_write = 40000000;
          bus_cfg.freq_read  = 16000000;
          _bus_spi.config(bus_cfg);

          auto p = new Panel_M5StackCore2();
          p->bus(&_bus_spi);
          _panel_last = p;

          _light_last = new Light_M5StackCore2();
          p->light(_light_last);

          {
            auto t = new lgfx::Touch_FT5x06();
            auto cfg = t->config();
            cfg.pin_int  = 39;   // INT pin number
            cfg.pin_sda  = 21;   // I2C SDA pin number
            cfg.pin_scl  = 22;   // I2C SCL pin number
            cfg.i2c_addr = 0x38; // I2C device addr
            cfg.i2c_port = I2C_NUM_1;// I2C port number
            cfg.freq = 400000;   // I2C freq
            cfg.x_min = 0;
            cfg.x_max = 319;
            cfg.y_min = 0;
            cfg.y_max = 279;
            t->config(cfg);
            p->touch(t);
            float affine[6] = { 1, 0, 0, 0, 1, 0 };
            p->setCalibrateAffine(affine);
            _touch_last = t;
          }

          goto init_clear;
        }
        lgfx::pinMode( 4, lgfx::pin_mode_t::input); // TF card CS
        lgfx::pinMode( 5, lgfx::pin_mode_t::input); // LCD CS
        _bus_spi.release();
      }
    }

    board = board_t::board_unknown;

init_clear:

ESP_LOGI("nvs_board","_board:%d", board);

    panel(_panel_last);

    return board;
  }

  void M5GFX::progressBar(int x, int y, int w, int h, uint8_t val)
  {
    drawRect(x, y, w, h, 0x09F1);
    fillRect(x + 1, y + 1, w * (((float)val) / 100.0), h - 1, 0x09F1);
  }

  void M5GFX::pushState(void)
  {
    DisplayState s;
    s.gfxFont = _font;
    s.style = _text_style;
    s.metrics = _font_metrics;
    s.cursor_x = _cursor_x;
    s.cursor_y = _cursor_y;
    s.padX = _padding_x;
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
    _padding_x = s.padX;
    _cursor_x = s.cursor_x;
    _cursor_y = s.cursor_y;
  }

}

#endif