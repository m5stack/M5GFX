// Generated from extras/board_spec; do not edit by hand.
// Regenerate in extras/board_spec: node cli/spec.js emit-wiring
#pragma once

static const board_detector_t* const esp32_d0wdq6_detectors[] = {
  &paper_family_detector, // board_M5Paper
  &axp_family_detector, // board_M5Station, board_M5StackCore2, board_M5Tough
  &stack_family_detector, // board_M5Stack
  &timercam_detector, // board_M5TimerCam
  nullptr,
};
static_assert(sizeof(esp32_d0wdq6_detectors) / sizeof(esp32_d0wdq6_detectors[0]) - 1 <= max_detector_families,
              "esp32_d0wdq6_detectors exceeds the detection session family limit");

