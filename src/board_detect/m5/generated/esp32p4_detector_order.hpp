// Generated from detector_order.json and board catalog.
#pragma once

static const board_detector_t* const esp32p4_detectors[] = {
  &corep4x_detector,
  &tab5_family_detector,
  &unitpoep4_detector,
  &stampp4_detector,
  nullptr,
};
static_assert(sizeof(esp32p4_detectors) / sizeof(esp32p4_detectors[0]) - 1 <= max_detector_families,
              "esp32p4_detectors exceeds the detection session family limit");

