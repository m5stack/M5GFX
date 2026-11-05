// Generated from extras/board_spec; do not edit by hand.
// Regenerate in extras/board_spec: node cli/spec.js emit-wiring
#pragma once

static const board_detector_t* const esp32c61_detectors[] = {
  &corematrix_detector, // board_M5CoreMatrix
  nullptr,
};
static_assert(sizeof(esp32c61_detectors) / sizeof(esp32c61_detectors[0]) - 1 <= max_detector_families,
              "esp32c61_detectors exceeds the detection session family limit");

