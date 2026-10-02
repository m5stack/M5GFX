// See lgfx_v1.cpp for the translation unit layout and maintenance notes.

#define LGFX_V1_IMPLEMENTATION

#include "panel/Panel_FrameBufferBase.inl"   // must precede platforms/esp32/Panel_EPD.inl (provides cacheWriteBack / Cache_WriteBack_Addr)
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
