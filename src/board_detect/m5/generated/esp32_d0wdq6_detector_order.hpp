// Generated from detector_order.json and board catalog.
#pragma once

static const board_detector_t* const esp32_d0wdq6_detectors[] = {
  &paper_family_detector,
  &axp_family_detector,
  &stack_family_detector,
  &timercam_detector,
  nullptr,
};
static_assert(sizeof(esp32_d0wdq6_detectors) / sizeof(esp32_d0wdq6_detectors[0]) - 1 <= max_detector_families,
              "esp32_d0wdq6_detectors exceeds the detection session family limit");

