// Generated from detector_order.json and board catalog.
#pragma once

static const board_detector_t* const esp32_pico_d4_detectors[] = {
  &stickc_family_detector,
  &coreink_detector,
  &atom_family_detector,
  nullptr,
};
static_assert(sizeof(esp32_pico_d4_detectors) / sizeof(esp32_pico_d4_detectors[0]) - 1 <= max_detector_families,
              "esp32_pico_d4_detectors exceeds the detection session family limit");

static const board_detector_t* const esp32_picov3_detectors[] = {
  &stickcplus2_detector,
  nullptr,
};
static_assert(sizeof(esp32_picov3_detectors) / sizeof(esp32_picov3_detectors[0]) - 1 <= max_detector_families,
              "esp32_picov3_detectors exceeds the detection session family limit");

