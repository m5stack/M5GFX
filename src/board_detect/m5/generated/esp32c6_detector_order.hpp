// Generated from detector_order.json and board catalog.
#pragma once

static const board_detector_t* const esp32c6_detectors_qfn40[] = {
  &c6_display_family_detector,
  nullptr,
};
static_assert(sizeof(esp32c6_detectors_qfn40) / sizeof(esp32c6_detectors_qfn40[0]) - 1 <= max_detector_families,
              "esp32c6_detectors_qfn40 exceeds the detection session family limit");

static const board_detector_t* const esp32c6_detectors_qfn32[] = {
  &stampc6_detector,
  &nanoc6_detector,
  nullptr,
};
static_assert(sizeof(esp32c6_detectors_qfn32) / sizeof(esp32c6_detectors_qfn32[0]) - 1 <= max_detector_families,
              "esp32c6_detectors_qfn32 exceeds the detection session family limit");

