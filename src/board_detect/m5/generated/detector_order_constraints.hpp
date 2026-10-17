// Generated from detector_order.json and board catalog.
#pragma once

namespace m5gfx { namespace board_detect {
static constexpr unsigned max_detector_families = 12;
struct detector_order_edge_t { board_id_t before; board_id_t after; };
// Each representative board ID orders the entire family, including all listed members.
static constexpr detector_order_edge_t detector_order_edges[] = {
  { 9, 132 }, // axp_family_detector (board_M5Station, board_M5StackCore2, board_M5Tough) before timercam_detector (board_M5TimerCam): TimerCam RTC probe on G12/G14 runs after the AXP display family
  { 1, 132 }, // stack_family_detector (board_M5Stack) before timercam_detector (board_M5TimerCam): TimerCam RTC probe on G12/G14 runs after Stack display probing
  { 7, 132 }, // paper_family_detector (board_M5Paper) before timercam_detector (board_M5TimerCam): TimerCam RTC probe on G12/G14 runs after Paper display probing
  { 4, 6 }, // stickc_family_detector (board_M5StickCPlus, board_M5StickC) before coreink_detector (board_M5StackCoreInk): CoreInk SCLK G18 is StickC and StickC-Plus LCD RST
  { 4, 142 }, // stickc_family_detector (board_M5StickCPlus, board_M5StickC) before atom_family_detector (board_M5AtomVoice, board_M5AtomMatrix, board_M5AtomLite, board_M5AtomU): Atom touch probing on G13 runs after StickC LCD SCLK
  { 6, 142 }, // coreink_detector (board_M5StackCoreInk) before atom_family_detector (board_M5AtomVoice, board_M5AtomMatrix, board_M5AtomLite, board_M5AtomU): Atom touch probing runs after CoreInk display probing
  { 32, 15 }, // pm1_ext_family_detector (board_M5ChainCaptain, board_M5PaperColor) before airq_detector (board_M5AirQ): AirQ RST G2 and DC G3 overlap ChainCaptain and PaperColor internal I2C SCL and SDA
  { 10, 11 }, // cores3_family_detector (board_M5StackCoreS3, board_M5StackCoreS3SE, board_M5StackChan) before spi_id_detector (board_M5AtomS3, board_M5DinMeter, board_M5AtomS3Lite, board_M5AtomS3U): AtomS3 MOSI G21 overlaps CoreS3 touch INT; probe CoreS3 first to avoid output contention
  { 12, 139 }, // dial_detector (board_M5Dial) before capsule_detector (board_M5Capsule): Capsule SDA G8 is Dial LCD reset; probe Dial before toggling G8
  { 11, 139 }, // spi_id_detector (board_M5AtomS3, board_M5DinMeter, board_M5AtomS3Lite, board_M5AtomS3U) before capsule_detector (board_M5Capsule): Capsule SDA G8 is DinMeter LCD reset; probe DinMeter before toggling G8
  { 10, 147 }, // cores3_family_detector (board_M5StackCoreS3, board_M5StackCoreS3SE, board_M5StackChan) before dualkey_detector (board_M5DualKey): Probe displayed QFN56 boards before DualKey NC pins
  { 12, 147 }, // dial_detector (board_M5Dial) before dualkey_detector (board_M5DualKey): Probe displayed QFN56 boards before DualKey NC pins
  { 30, 147 }, // pm1_family_detector (board_M5StopWatch, board_M5PaperMono) before dualkey_detector (board_M5DualKey): Probe displayed QFN56 boards before DualKey NC pins
  { 32, 147 }, // pm1_ext_family_detector (board_M5ChainCaptain, board_M5PaperColor) before dualkey_detector (board_M5DualKey): Probe displayed QFN56 boards before DualKey NC pins
  { 34, 147 }, // paper_family_detector (board_M5PaperDIY, board_M5PaperS3) before dualkey_detector (board_M5DualKey): Probe displayed QFN56 boards before DualKey NC pins
  { 11, 147 }, // spi_id_detector (board_M5AtomS3, board_M5DinMeter, board_M5AtomS3Lite, board_M5AtomS3U) before dualkey_detector (board_M5DualKey): Probe displayed QFN56 boards before DualKey NC pins
  { 14, 147 }, // cardputer_family_detector (board_M5Cardputer, board_M5CardputerADV, board_M5VAMeter) before dualkey_detector (board_M5DualKey): Probe displayed QFN56 boards before DualKey NC pins
  { 15, 147 }, // airq_detector (board_M5AirQ) before dualkey_detector (board_M5DualKey): Probe displayed QFN56 boards before DualKey NC pins
  { 21, 147 }, // stamplc_detector (board_M5StamPLC) before dualkey_detector (board_M5DualKey): Probe displayed QFN56 boards before DualKey NC pins
  { 145, 18 }, // atomvoices3r_detector (board_M5AtomVoiceS3R) before atoms3r_detector (board_M5AtomS3R, board_M5AtomS3RCam, board_M5AtomS3RExt): Voice codec on G45/G0 is read before AtomS3R display probing can drive Voice I2S DOUT G48
  { 145, 26 }, // atomvoices3r_detector (board_M5AtomVoiceS3R) before pmic_id_detector (board_M5StickS3, board_M5StampS3Bat): Voice codec is identified before the G47/G48 PM1 bus can touch Voice I2S DOUT G48
  { 18, 26 }, // atoms3r_detector (board_M5AtomS3R, board_M5AtomS3RCam, board_M5AtomS3RExt) before pmic_id_detector (board_M5StickS3, board_M5StampS3Bat): AtomS3R display reset G48 must not receive the PM1 I2C probe first
  { 153, 33 }, // stampc5_detector (board_M5StampC5) before toughc5_detector (board_M5ToughC5): StampC5 eFuse flash and PSRAM gate must precede ToughC5 I2C GPIO probing even with an NVS hint
  { 154, 140 }, // stampc6_detector (board_M5StampC6) before nanoc6_detector (board_M5NanoC6): StampC6 eFuse capacity must precede NanoC6 G3 probing because G3 is a StampC6 user pin
  { 31, 22 }, // corep4x_detector (board_M5CoreP4X) before tab5_family_detector (board_M5Tab5, board_M5Tab5X): Probe the CoreP4X internal devices before Tab5
  { 22, 148 }, // tab5_family_detector (board_M5Tab5, board_M5Tab5X) before unitpoep4_detector (board_M5UnitPoEP4): Probe Tab5 internal devices before driving Ethernet management pins
};
} } // namespace m5gfx::board_detect
