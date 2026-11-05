// Generated from extras/board_spec; do not edit by hand.
// Regenerate in extras/board_spec: node cli/spec.js emit-wiring
#pragma once

static const board_detector_t* const esp32h2_detectors[] = {
  &nanoh2_detector, // board_M5NanoH2
  nullptr,
};
static_assert(sizeof(esp32h2_detectors) / sizeof(esp32h2_detectors[0]) - 1 <= max_detector_families,
              "esp32h2_detectors exceeds the detection session family limit");

