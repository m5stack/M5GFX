// Generated from detector_order.json and board catalog.
#pragma once

static const board_detector_t* const esp32h2_detectors[] = {
  &nanoh2_detector,
  nullptr,
};
static_assert(sizeof(esp32h2_detectors) / sizeof(esp32h2_detectors[0]) - 1 <= max_detector_families,
              "esp32h2_detectors exceeds the detection session family limit");

