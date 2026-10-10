// Generated from detector_order.json and board catalog.
#pragma once

static const board_detector_t* const esp32c61_detectors[] = {
  &corematrix_detector,
  nullptr,
};
static_assert(sizeof(esp32c61_detectors) / sizeof(esp32c61_detectors[0]) - 1 <= max_detector_families,
              "esp32c61_detectors exceeds the detection session family limit");

