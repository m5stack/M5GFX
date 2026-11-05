// Generated from extras/board_spec; do not edit by hand.
// Regenerate in extras/board_spec: node cli/spec.js emit-wiring
#pragma once

static const board_detector_t* const esp32c3_detectors[] = {
  &c3_family_detector, // board_M5StampC3, board_M5StampC3U
  nullptr,
};
static_assert(sizeof(esp32c3_detectors) / sizeof(esp32c3_detectors[0]) - 1 <= max_detector_families,
              "esp32c3_detectors exceeds the detection session family limit");

