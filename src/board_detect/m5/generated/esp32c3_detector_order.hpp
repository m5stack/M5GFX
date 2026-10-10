// Generated from detector_order.json and board catalog.
#pragma once

static const board_detector_t* const esp32c3_detectors[] = {
  &c3_family_detector,
  nullptr,
};
static_assert(sizeof(esp32c3_detectors) / sizeof(esp32c3_detectors[0]) - 1 <= max_detector_families,
              "esp32c3_detectors exceeds the detection session family limit");

