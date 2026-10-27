// Generated from extras/board_spec; do not edit by hand.
// Regenerate in extras/board_spec: node cli/spec.js emit-wiring
#pragma once

static const board_detector_t* const esp32c5_detectors[] = {
  &stampc5_detector, // board_M5StampC5
  &toughc5_detector, // board_M5ToughC5
  nullptr,
};
static_assert(sizeof(esp32c5_detectors) / sizeof(esp32c5_detectors[0]) - 1 <= max_detector_families,
              "esp32c5_detectors exceeds the detection session family limit");

