#ifndef __LGFX_BOARDS_HPP__
#define __LGFX_BOARDS_HPP__

namespace lgfx // これはm5gfxに変更してはならない;
{
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
    , board_M5Station
    , board_M5StackCoreS3
    , board_M5AtomS3

/// non display boards 
    , board_M5Atom
    , board_M5ATOM = board_M5Atom
    , board_M5AtomPsram
    , board_M5AtomU
    , board_M5Camera
    , board_M5TimerCam
    , board_M5StampPico
    , board_M5StampC3
    , board_M5StampC3U
    , board_M5StampS3
    , board_M5AtomS3Lite
    , board_M5AtomS3U

/// external displays
    , board_M5AtomDisplay
    , board_M5ATOMDisplay = board_M5AtomDisplay
    , board_M5UnitLCD
    , board_M5UnitOLED
    , board_M5UnitGLASS
    , board_M5UnitRCA
    , board_M5ModuleDisplay
    , board_M5ModuleRCA
    };
  }
  using namespace boards;
}

#endif
