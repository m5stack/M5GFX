#pragma once

// If you want to use a set of functions to handle SD/SPIFFS/HTTP,
//  please include <SD.h>,<SPIFFS.h>,<HTTPClient.h> before <M5GFX.h>
// #include <SD.h>
// #include <SPIFFS.h>
// #include <HTTPClient.h>
#if defined (ARDUINO)
#include <FS.h>
#endif

/// Use of std::min and std::max instead of macro definitions is recommended.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#ifdef setFont
#undef setFont
#endif

#define LGFX_USE_V1
#include "lgfx/v1/gitTagVersion.h"
#include "lgfx/v1/platforms/device.hpp"
#include "lgfx/v1/platforms/common.hpp"
#include "lgfx/v1/lgfx_filesystem_support.hpp"
#include "lgfx/v1/LGFXBase.hpp"
#include "lgfx/v1/LGFX_Sprite.hpp"

#include <vector>

namespace m5gfx
{
  using namespace lgfx;
//----------------------------------------------------------------------------

  namespace ili9341_colors  // Color definitions for backwards compatibility with old sketches
  {
    #ifdef ILI9341_BLACK
    #undef ILI9341_BLACK
    #undef ILI9341_NAVY
    #undef ILI9341_DARKGREEN
    #undef ILI9341_DARKCYAN
    #undef ILI9341_MAROON
    #undef ILI9341_PURPLE
    #undef ILI9341_OLIVE
    #undef ILI9341_LIGHTGREY
    #undef ILI9341_DARKGREY
    #undef ILI9341_BLUE
    #undef ILI9341_GREEN
    #undef ILI9341_CYAN
    #undef ILI9341_RED
    #undef ILI9341_MAGENTA
    #undef ILI9341_YELLOW
    #undef ILI9341_WHITE
    #undef ILI9341_ORANGE
    #undef ILI9341_GREENYELLOW
    #undef ILI9341_PINK
    #endif

    #ifdef BLACK
    #undef BLACK
    #undef NAVY
    #undef DARKGREEN
    #undef DARKCYAN
    #undef MAROON
    #undef PURPLE
    #undef OLIVE
    #undef LIGHTGREY
    #undef DARKGREY
    #undef BLUE
    #undef GREEN
    #undef CYAN
    #undef RED
    #undef MAGENTA
    #undef YELLOW
    #undef WHITE
    #undef ORANGE
    #undef GREENYELLOW
    #undef PINK
    #endif

    static constexpr int ILI9341_BLACK       = 0x0000;      /*   0,   0,   0 */
    static constexpr int ILI9341_NAVY        = 0x000F;      /*   0,   0, 128 */
    static constexpr int ILI9341_DARKGREEN   = 0x03E0;      /*   0, 128,   0 */
    static constexpr int ILI9341_DARKCYAN    = 0x03EF;      /*   0, 128, 128 */
    static constexpr int ILI9341_MAROON      = 0x7800;      /* 128,   0,   0 */
    static constexpr int ILI9341_PURPLE      = 0x780F;      /* 128,   0, 128 */
    static constexpr int ILI9341_OLIVE       = 0x7BE0;      /* 128, 128,   0 */
    static constexpr int ILI9341_LIGHTGREY   = 0xC618;      /* 192, 192, 192 */
    static constexpr int ILI9341_DARKGREY    = 0x7BEF;      /* 128, 128, 128 */
    static constexpr int ILI9341_BLUE        = 0x001F;      /*   0,   0, 255 */
    static constexpr int ILI9341_GREEN       = 0x07E0;      /*   0, 255,   0 */
    static constexpr int ILI9341_CYAN        = 0x07FF;      /*   0, 255, 255 */
    static constexpr int ILI9341_RED         = 0xF800;      /* 255,   0,   0 */
    static constexpr int ILI9341_MAGENTA     = 0xF81F;      /* 255,   0, 255 */
    static constexpr int ILI9341_YELLOW      = 0xFFE0;      /* 255, 255,   0 */
    static constexpr int ILI9341_WHITE       = 0xFFFF;      /* 255, 255, 255 */
    static constexpr int ILI9341_ORANGE      = 0xFD20;      /* 255, 165,   0 */
    static constexpr int ILI9341_GREENYELLOW = 0xAFE5;      /* 173, 255,  47 */
    static constexpr int ILI9341_PINK        = 0xF81F;

    static constexpr int BLACK           = 0x0000;      /*   0,   0,   0 */
    static constexpr int NAVY            = 0x000F;      /*   0,   0, 128 */
    static constexpr int DARKGREEN       = 0x03E0;      /*   0, 128,   0 */
    static constexpr int DARKCYAN        = 0x03EF;      /*   0, 128, 128 */
    static constexpr int MAROON          = 0x7800;      /* 128,   0,   0 */
    static constexpr int PURPLE          = 0x780F;      /* 128,   0, 128 */
    static constexpr int OLIVE           = 0x7BE0;      /* 128, 128,   0 */
    static constexpr int LIGHTGREY       = 0xC618;      /* 192, 192, 192 */
    static constexpr int DARKGREY        = 0x7BEF;      /* 128, 128, 128 */
    static constexpr int BLUE            = 0x001F;      /*   0,   0, 255 */
    static constexpr int GREEN           = 0x07E0;      /*   0, 255,   0 */
    static constexpr int CYAN            = 0x07FF;      /*   0, 255, 255 */
    static constexpr int RED             = 0xF800;      /* 255,   0,   0 */
    static constexpr int MAGENTA         = 0xF81F;      /* 255,   0, 255 */
    static constexpr int YELLOW          = 0xFFE0;      /* 255, 255,   0 */
    static constexpr int WHITE           = 0xFFFF;      /* 255, 255, 255 */
    static constexpr int ORANGE          = 0xFD20;      /* 255, 165,   0 */
    static constexpr int GREENYELLOW     = 0xAFE5;      /* 173, 255,  47 */
    static constexpr int PINK            = 0xF81F;
  }

  namespace tft_command
  {
    static constexpr int TFT_DISPOFF = 0x28;
    static constexpr int TFT_DISPON  = 0x29;
    static constexpr int TFT_SLPIN   = 0x10;
    static constexpr int TFT_SLPOUT  = 0x11;
  }

  namespace boards
  {
    enum board_t
    { board_unknown
    , board_Non_Panel
    , board_M5Stack
    , board_M5StackCore2
    , board_M5StickC
    , board_M5StickCPlus
    , board_M5StackCoreInk
    , board_M5Paper
    , board_M5Tough
    , board_M5ATOM
    , board_M5Camera
    , board_M5TimerCam
    };
  }
  using board_t = boards::board_t;

  class M5GFX : public lgfx::LGFX_Device
  {
    static M5GFX* _instance;

    struct DisplayState
    {
      const lgfx::IFont *gfxFont;
      lgfx::TextStyle style;
      lgfx::FontMetrics metrics;
      int32_t cursor_x, cursor_y;
    };

    lgfx::Bus_SPI _bus_spi;
    lgfx::Panel_Device* _panel_last;
    lgfx::ILight* _light_last;
    lgfx::ITouch* _touch_last;
    board_t _board;
    std::vector<DisplayState> _displayStateStack;

    bool init_impl(bool use_reset, bool use_clear) override;
    board_t autodetect(bool use_reset = false, board_t board = board_t::board_unknown);
    void _set_backlight(lgfx::ILight* bl);
    void _set_pwm_backlight(std::int16_t pin, std::uint8_t ch, std::uint32_t freq = 12000, bool invert = false);

  public:
    M5GFX(void);

    static M5GFX* getInstance(void) { return _instance; }

    inline board_t getBoard(void) const { return _board; }

    void clearDisplay(int32_t color = TFT_BLACK) { fillScreen(color); }
    void progressBar(int x, int y, int w, int h, uint8_t val);
    void pushState(void);
    void popState(void);

    /// draw RGB565 format image.
    void drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const void *data)
    {
      pushImage(x, y, w, h, (const rgb565_t*)data);
    }

    /// draw RGB565 format image, with transparent color.
    void drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const void *data, uint16_t transparent)
    {
      pushImage(x, y, w, h, (const rgb565_t*)data, transparent);
    }
  };


  class M5Canvas : public lgfx::LGFX_Sprite
  {
  public:
    M5Canvas() : LGFX_Sprite() {}
    M5Canvas(LovyanGFX* parent) : LGFX_Sprite(parent) { _psram = true; }

    void* frameBuffer(uint8_t) { return getBuffer(); }
  };

//----------------------------------------------------------------------------
}

using namespace m5gfx::ili9341_colors;
using namespace m5gfx::tft_command;
using M5GFX = m5gfx::M5GFX;
using M5Canvas = m5gfx::M5Canvas;
