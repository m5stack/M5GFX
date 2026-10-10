// Generated from detector_order.json and board catalog.
#pragma once

static const board_detector_t* const esp32s3_detectors_qfn56[] = {
  &dial_detector,
  &paper_family_detector,
  &cores3_family_detector,
  &spi_id_detector,
  &capsule_detector,
  &pm1_family_detector,
  &pm1_ext_family_detector,
  &airq_detector,
  &cardputer_family_detector,
  &stamplc_detector,
  &powerhub_detector,
  &dualkey_detector,
  nullptr,
};
static_assert(sizeof(esp32s3_detectors_qfn56) / sizeof(esp32s3_detectors_qfn56[0]) - 1 <= max_detector_families,
              "esp32s3_detectors_qfn56 exceeds the detection session family limit");

static const board_detector_t* const esp32s3_detectors_lga56[] = {
  &atomvoices3r_detector,
  &atoms3r_detector,
  &pmic_id_detector,
  nullptr,
};
static_assert(sizeof(esp32s3_detectors_lga56) / sizeof(esp32s3_detectors_lga56[0]) - 1 <= max_detector_families,
              "esp32s3_detectors_lga56 exceeds the detection session family limit");

