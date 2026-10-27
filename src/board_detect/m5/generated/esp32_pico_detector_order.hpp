// Generated from extras/board_spec; do not edit by hand.
// Regenerate in extras/board_spec: node cli/spec.js emit-wiring
#pragma once

static const board_detector_t* const esp32_pico_d4_detectors[] = {
  &stickc_family_detector, // board_M5StickCPlus, board_M5StickC
  &coreink_detector, // board_M5StackCoreInk
  &atom_family_detector, // board_M5AtomVoice, board_M5AtomMatrix, board_M5AtomLite, board_M5AtomU
  nullptr,
};
static_assert(sizeof(esp32_pico_d4_detectors) / sizeof(esp32_pico_d4_detectors[0]) - 1 <= max_detector_families,
              "esp32_pico_d4_detectors exceeds the detection session family limit");

static const board_detector_t* const esp32_picov3_detectors[] = {
  &stickcplus2_detector, // board_M5StickCPlus2
  nullptr,
};
static_assert(sizeof(esp32_picov3_detectors) / sizeof(esp32_picov3_detectors[0]) - 1 <= max_detector_families,
              "esp32_picov3_detectors exceeds the detection session family limit");

