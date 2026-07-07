#ifndef __LGFX_BOARDS_HPP__
#define __LGFX_BOARDS_HPP__

namespace lgfx // This should not be changed to "m5gfx"
{
  namespace boards
  { // Board IDs are canonically assigned in the m5stack-board-id repository.
    // https://github.com/m5stack/m5stack-board-id/blob/main/board.csv
    // When adding a new board, obtain its BID there first; never renumber existing entries.
    enum board_t
    { board_unknown = 0
    , board_M5Stack = 1
    , board_M5StackCore2 = 2
    , board_M5StickC = 3
    , board_M5StickCPlus = 4
    , board_M5StickCPlus2 = 5
    , board_M5StackCoreInk = 6
    , board_M5Paper = 7
    , board_M5Tough = 8
    , board_M5Station = 9
    , board_M5StackCoreS3 = 10
    , board_M5AtomS3 = 11
    , board_M5Dial = 12
    , board_M5DinMeter = 13
    , board_M5Cardputer = 14
    , board_M5AirQ = 15
    , board_M5VAMeter = 16
    , board_M5StackCoreS3SE = 17
    , board_M5AtomS3R = 18
    , board_M5PaperS3 = 19
    , board_M5CoreMP135 = 20
    , board_M5StampPLC = 21
    , board_M5Tab5 = 22
    , board_ArduinoNessoN1 = 23
    , board_M5CardputerADV = 24
    , board_M5UnitC6L = 25
    , board_M5StickS3 = 26
    , board_M5StackChan = 27
    , board_M5PaperColor = 28
    , board_M5PaperMono = 29
    , board_M5StopWatch = 30
    , board_M5CoreP4X = 31
    , board_M5ChainCaptain = 32
    , board_M5ToughC5 = 33

/// non display boards
    , board_M5AtomLite = 128
    , board_M5ATOM __attribute__ ((deprecated)) = board_M5AtomLite
    , board_M5Atom __attribute__ ((deprecated)) = board_M5AtomLite
    , board_M5AtomPsram = 129
    , board_M5AtomU = 130
    , board_M5Camera = 131
    , board_M5TimerCam = 132
    , board_M5StampPico = 133
    , board_M5StampC3 = 134
    , board_M5StampC3U = 135
    , board_M5StampS3 = 136
    , board_M5AtomS3Lite = 137
    , board_M5AtomS3U = 138
    , board_M5Capsule = 139
    , board_M5NanoC6 = 140
    , board_M5AtomMatrix = 141
    , board_M5AtomVoice = 142
    , board_M5AtomEcho __attribute__ ((deprecated)) = board_M5AtomVoice
    , board_M5AtomS3RExt = 143
    , board_M5AtomS3RCam = 144
    , board_M5AtomVoiceS3R = 145
    , board_M5AtomEchoS3R __attribute__ ((deprecated)) = board_M5AtomVoiceS3R
    , board_M5PowerHub = 146
    , board_M5DualKey = 147
    , board_M5UnitPoEP4 = 148
    , board_M5StampS3Bat = 149
    , board_M5StampP4 = 150
    , board_M5NanoH2 = 151
    , board_M5CoreMatrix = 152
    , board_M5StampC5 = 153
    , board_M5StampC6 = 154
    , board_M5StampS3Mini = 155
    , board_M5StampP4X = 156

/// external displays
    , board_M5AtomDisplay = 192
    , board_M5ATOMDisplay = board_M5AtomDisplay
    , board_M5UnitLCD = 193
    , board_M5UnitOLED = 194
    , board_M5UnitMiniOLED = 195
    , board_M5UnitGLASS = 196
    , board_M5UnitGLASS2 = 197
    , board_M5UnitRCA = 198
    , board_M5ModuleDisplay = 199
    , board_M5ModuleRCA = 200
    , board_M5UnitPoEP4HDMI = 201

    , board_FrameBuffer = 512
    };
  }
  using namespace boards;
}

#endif
