// LovyanGFX v1 is built as a single translation unit: this file includes every
// implementation file (*.inl) under src/lgfx/v1, and those are not compiled on their own.
//
// Why: every implementation file pulls in the same framework headers, and parsing them
// once instead of once per file cuts the library build time several-fold (the Arduino
// build also pre-scans each source file for includes, which doubled that cost).
//
// Maintenance notes:
// - Adding an implementation file: create it as *.inl and add an #include below. Do not
//   add *.cpp files under src/lgfx/v1. LovyanGFX and M5GFX share this source tree but keep
//   their own copy of this file, because their panel sets differ; update both.
// - File-local names are visible to every file that follows: static functions and
//   constants, macros, and anonymous namespaces do not isolate files here. Use a name that
//   identifies the file (or a class member), and #undef helper macros at the end of the
//   file that defines them.
// - Platform directories are self-guarded, so only the active platform contributes code;
//   the desktop back ends (sdl / opencv / framebuffer) follow the selection made in
//   platforms/common.hpp (LGFX_PLATFORM_*).
// - Two files provide helpers for the ones that follow and are marked "must precede".
// - The C decoders in src/lgfx/utility and the font tables in src/lgfx/Fonts stay separate
//   translation units on purpose (third-party file-local names collide; the font tables
//   would make a single 100+ MB translation unit).


#define LGFX_V1_IMPLEMENTATION

#include "LGFXBase.inl"
#include "LGFX_Sprite.inl"
#include "LGFX_Button.inl"
#include "lgfx_fonts.inl"

#include "misc/DividedFrameBuffer.inl"
#include "misc/SpriteBuffer.inl"
#include "misc/common_function.inl"
#include "misc/pixelcopy.inl"

#include "panel/Panel_AMOLED.inl"
#include "panel/Panel_Device.inl"
#include "panel/Panel_ED2208.inl"
#include "panel/Panel_EPDiy.inl"
#include "panel/Panel_FrameBufferBase.inl"   // must precede platforms/esp32/Panel_EPD.inl (provides cacheWriteBack / Cache_WriteBack_Addr)
#include "panel/Panel_GDEW0154D67.inl"
#include "panel/Panel_GDEW0154M09.inl"
#include "panel/Panel_HasBuffer.inl"
#include "panel/Panel_IT8951.inl"
#include "panel/Panel_LCD.inl"
#include "panel/Panel_M5HDMI.inl"
#include "panel/Panel_M5UnitGLASS.inl"
#include "panel/Panel_M5UnitLCD.inl"
#include "panel/Panel_SSD1306.inl"
#include "panel/Panel_SSD1677.inl"
#include "panel/Panel_TM1680.inl"

#include "touch/Touch_CHSC6540.inl"
#include "touch/Touch_CSTxxx.inl"
#include "touch/Touch_FT5x06.inl"
#include "touch/Touch_GT911.inl"

#include "platforms/esp32/common.inl"   // must precede the other esp32* files (provides reg() / writereg())
#include "platforms/esp32/Bus_EPD.inl"
#include "platforms/esp32/Bus_I2C.inl"
#include "platforms/esp32/Bus_Parallel8.inl"
#include "platforms/esp32/Bus_SPI.inl"
#include "platforms/esp32/Light_PWM.inl"
#include "platforms/esp32/Panel_CVBS.inl"
#include "platforms/esp32/Panel_EPD.inl"

#include "platforms/esp32c3/Bus_Parallel8.inl"
#include "platforms/esp32p4/Bus_DSI.inl"
#include "platforms/esp32p4/Panel_DSI.inl"
#include "platforms/esp32p4/Panel_LT8912B.inl"
#include "platforms/esp32p4/Touch_ST7123.inl"
#include "platforms/esp32s2/Bus_Parallel16.inl"
#include "platforms/esp32s2/Bus_Parallel8.inl"
#include "platforms/esp32s3/Bus_Parallel16.inl"
#include "platforms/esp32s3/Bus_Parallel8.inl"

#include "platforms/framebuffer/Panel_fb.inl"
#include "platforms/framebuffer/common.inl"
#include "platforms/sdl/Panel_sdl.inl"
#include "platforms/sdl/common.inl"

#undef LGFX_V1_IMPLEMENTATION
