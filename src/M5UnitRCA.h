#ifndef __M5GFX_M5UNITRCA__
#define __M5GFX_M5UNITRCA__

// If you want to use a set of functions to handle SD/SPIFFS/HTTP,
//  please include <SD.h>,<SPIFFS.h>,<HTTPClient.h> before <M5GFX.h>
// #include <SD.h>
// #include <SPIFFS.h>
// #include <HTTPClient.h>

#include "M5GFX.h"
#include "lgfx/v1/platforms/esp32/Panel_CVBS.hpp"

#if defined ( ARDUINO )
 #include <Arduino.h>
#elif __has_include( <sdkconfig.h> )
 #include <sdkconfig.h>
#endif

#ifndef M5UNITRCA_PIN_DAC
#define M5UNITRCA_PIN_DAC GPIO_NUM_26
#endif
#ifndef M5UNITRCA_LOGICAL_WIDTH
#define M5UNITRCA_LOGICAL_WIDTH 216
#endif
#ifndef M5UNITRCA_LOGICAL_HEIGHT
#define M5UNITRCA_LOGICAL_HEIGHT 144
#endif
#ifndef M5UNITRCA_SIGNAL_TYPE
#define M5UNITRCA_SIGNAL_TYPE signal_type_t::PAL
#endif
#ifndef M5UNITRCA_OUTPUT_WIDTH
#define M5UNITRCA_OUTPUT_WIDTH 0
#endif
#ifndef M5UNITRCA_OUTPUT_HEIGHT
#define M5UNITRCA_OUTPUT_HEIGHT 0
#endif

class M5UnitRCA : public lgfx::LGFX_Device
{
  lgfx::Panel_CVBS _panel_instance;

public:

  typedef lgfx::Panel_CVBS::config_detail_t::signal_type_t signal_type_t;

  M5UnitRCA( uint16_t logical_width    = M5UNITRCA_LOGICAL_WIDTH
           , uint16_t logical_height   = M5UNITRCA_LOGICAL_HEIGHT
           , uint16_t output_width     = M5UNITRCA_OUTPUT_WIDTH
           , uint16_t output_height    = M5UNITRCA_OUTPUT_HEIGHT
           , signal_type_t signal_type = M5UNITRCA_SIGNAL_TYPE
           , uint8_t  pin_dac          = M5UNITRCA_PIN_DAC
           )
  {
    setup(logical_width, logical_height, output_width, output_height, signal_type, pin_dac);
  }

  using lgfx::LGFX_Device::init;
  bool init( uint16_t logical_width
           , uint16_t logical_height
           , uint16_t output_width     = M5UNITRCA_OUTPUT_WIDTH
           , uint16_t output_height    = M5UNITRCA_OUTPUT_HEIGHT
           , signal_type_t signal_type = M5UNITRCA_SIGNAL_TYPE
           , uint8_t  pin_dac          = M5UNITRCA_PIN_DAC
           )
  {
    setup(logical_width, logical_height, output_width, output_height, signal_type, pin_dac);
    return init();
  }

  void setup(uint16_t logical_width    = M5UNITRCA_LOGICAL_WIDTH
           , uint16_t logical_height   = M5UNITRCA_LOGICAL_HEIGHT
           , uint16_t output_width     = M5UNITRCA_OUTPUT_WIDTH
           , uint16_t output_height    = M5UNITRCA_OUTPUT_HEIGHT
           , signal_type_t signal_type = M5UNITRCA_SIGNAL_TYPE
           , uint8_t  pin_dac          = M5UNITRCA_PIN_DAC
           )
  {
    if (output_width  < logical_width ) { output_width  = logical_width;  }
    if (output_height < logical_height) { output_height = logical_height; }

    {
      auto cfg = _panel_instance.config();
      cfg.panel_width   = logical_width;
      cfg.panel_height  = logical_height;
      cfg.memory_width  = output_width;
      cfg.memory_height = output_height;
      cfg.offset_x = (output_width  - logical_width ) >> 1;
      cfg.offset_y = (output_height - logical_height) >> 1;

      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
    _board = lgfx::board_t::board_M5UnitRCA;
  }

  // 出力信号の電圧レベルを調整する。 初期値 128
  // GPIOに保護抵抗がついている場合、出力電圧が低下するため、この関数で高い値を設定して調整する。
  // M5Stack Core2 の場合 200を指定。
  void setOutputLevel(uint8_t output_level) {
      auto cfg = _panel_instance.config_detail();
      cfg.output_level = output_level;
      _panel_instance.config_detail(cfg);
  }

  // for M5Stack Core2
  inline void setOutputBoost(bool boost = true) {
      setOutputLevel(boost ? 200 : 128);
  }

  void setOutputPin(uint8_t pin_dac) {
      auto cfg = _panel_instance.config_detail();
      cfg.pin_dac = pin_dac;
      _panel_instance.config_detail(cfg);
  }

  // 出力信号の種類を設定する
  // NTSC,    // black = 7.5IRE
  // NTSC_J,  // black = 0IRE (for Japan)
  // PAL,
  // PAL_M,
  // PAL_N,
  void setSignalType(signal_type_t signal_type) {
      auto cfg = _panel_instance.config_detail();
      cfg.signal_type = signal_type;
      _panel_instance.config_detail(cfg);
  }

  enum use_psram_t {
    psram_no_use = 0,
    psram_half_use = 1,
    psram_use = 2,
  };

  // PSRAMの使用方法を設定する
  // psram_level_t::psram_no_use   = PSRAM不使用
  // psram_level_t::psram_half_use = SRAMとPSRAMを半分ずつ使用
  // psram_level_t::psram_use      = PSRAM使用
  void setPsram(use_psram_t use_psram) {
      auto cfg = _panel_instance.config_detail();
      cfg.use_psram = use_psram;
      _panel_instance.config_detail(cfg);
  }

  // PSRAM使用設定
  // 0 = PSRAM不使用
  // 1 = SRAMとPSRAMを半分ずつ使用
  // 2 = PSRAM使用
  inline void setPsram(uint8_t use_psram) {
    setPsram((use_psram_t)use_psram);
  }
};

#endif
