// Generated from detector_order.json and board catalog.
#pragma once

static const board_detector_t* const esp32s3_detectors_qfn56[] = {
  &dial_detector, // board_M5Dial
  &paper_family_detector, // board_M5PaperDIY, board_M5PaperS3
  &cores3_family_detector, // board_M5StackCoreS3, board_M5StackCoreS3SE, board_M5StackChan
  &spi_id_detector, // board_M5AtomS3, board_M5DinMeter, board_M5AtomS3Lite, board_M5AtomS3U
  &capsule_detector, // board_M5Capsule
  &pm1_family_detector, // board_M5StopWatch, board_M5PaperMono
  &pm1_ext_family_detector, // board_M5ChainCaptain, board_M5PaperColor
  &airq_detector, // board_M5AirQ
  &cardputer_family_detector, // board_M5Cardputer, board_M5CardputerADV, board_M5VAMeter
  &stamplc_detector, // board_M5StamPLC
  &powerhub_detector, // board_M5PowerHub
  &dualkey_detector, // board_M5DualKey
  nullptr,
};
static_assert(sizeof(esp32s3_detectors_qfn56) / sizeof(esp32s3_detectors_qfn56[0]) - 1 <= max_detector_families,
              "esp32s3_detectors_qfn56 exceeds the detection session family limit");

static const board_detector_t* const esp32s3_detectors_lga56[] = {
  &atomvoices3r_detector, // board_M5AtomVoiceS3R
  &atoms3r_detector, // board_M5AtomS3R, board_M5AtomS3RCam, board_M5AtomS3RExt
  &pmic_id_detector, // board_M5StickS3, board_M5StampS3Bat
  nullptr,
};
static_assert(sizeof(esp32s3_detectors_lga56) / sizeof(esp32s3_detectors_lga56[0]) - 1 <= max_detector_families,
              "esp32s3_detectors_lga56 exceeds the detection session family limit");

