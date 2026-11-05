// Generated from extras/board_spec; do not edit by hand.
// Regenerate in extras/board_spec: node cli/spec.js emit-wiring
#pragma once
#include <set>
#include <vector>

struct expected_detector_pins_t {
  const board_detector_t* detector;
  const char* name;
  std::set<int> pins;
  std::vector<board_id_t> members;
};

#if defined(CONFIG_IDF_TARGET_ESP32)
static const expected_detector_pins_t expected_detector_pins[] = {
  { &axp_family_detector, "axp_family_detector", { 4, 5, 12, 15, 18, 19, 21, 22, 23, 32, 33, 38 }, { 9, 2, 8 } },
  { &stack_family_detector, "stack_family_detector", { 4, 14, 18, 19, 23, 27, 33 }, { 1 } },
  { &paper_family_detector, "paper_family_detector", { 2, 4, 12, 13, 14, 15, 23, 27 }, { 7 } },
  { &timercam_detector, "timercam_detector", { 12, 14, 33 }, { 132 } },
  { &stickc_family_detector, "stickc_family_detector", { 5, 13, 14, 15, 18, 21, 22, 23 }, { 4, 3 } },
  { &coreink_detector, "coreink_detector", { 0, 4, 9, 12, 15, 18, 23, 34 }, { 6 } },
  { &atom_family_detector, "atom_family_detector", { 2, 12, 13, 18, 21, 23, 25, 27, 34 }, { 142, 141, 128, 130 } },
  { &stickcplus2_detector, "stickcplus2_detector", { 4, 5, 12, 13, 14, 15, 27, 34, 35, 37, 38 }, { 5 } },
};
#endif

#if defined(CONFIG_IDF_TARGET_ESP32S3)
static const expected_detector_pins_t expected_detector_pins[] = {
  { &cores3_family_detector, "cores3_family_detector", { 3, 4, 11, 12, 15, 16, 21, 35, 36, 37, 38, 39, 40, 41, 42, 45, 46, 47, 48 }, { 10, 17, 27 } },
  { &dial_detector, "dial_detector", { 4, 5, 6, 7, 8, 9, 11, 12, 14, 46 }, { 12 } },
  { &pm1_family_detector, "pm1_family_detector", { 4, 13, 14, 15, 16, 17, 18, 39, 40, 41, 42, 45, 46, 47, 48 }, { 30, 29 } },
  { &pm1_ext_family_detector, "pm1_ext_family_detector", { 2, 3, 11, 12, 13, 14, 15, 16, 43, 44, 45, 46, 47 }, { 32, 28 } },
  { &paper_family_detector, "paper_family_detector", { 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 41, 42, 44, 45, 46, 48 }, { 34, 19 } },
  { &atoms3_family_detector, "atoms3_family_detector", { 4, 5, 6, 7, 8, 9, 12, 15, 16, 17, 21, 33, 34, 41, 46 }, { 11, 13, 137, 138 } },
  { &cardputer_family_detector, "cardputer_family_detector", { 5, 6, 8, 9, 33, 34, 35, 36, 37, 38 }, { 14, 24, 16 } },
  { &airq_detector, "airq_detector", { 1, 2, 3, 4, 5, 6, 46 }, { 15 } },
  { &stamplc_detector, "stamplc_detector", { 3, 6, 7, 8, 9, 10, 12 }, { 21 } },
  { &capsule_detector, "capsule_detector", { 8, 10, 46 }, { 139 } },
  { &powerhub_detector, "powerhub_detector", { 45, 48 }, { 146 } },
  { &dualkey_detector, "dualkey_detector", { 17, 21, 38, 39 }, { 147 } },
  { &atomvoices3r_detector, "atomvoices3r_detector", { 0, 45 }, { 145 } },
  { &atoms3r_detector, "atoms3r_detector", { 0, 9, 12, 14, 15, 18, 21, 42, 45, 48 }, { 18, 144, 143 } },
  { &pmic_id_detector, "pmic_id_detector", { 21, 38, 39, 40, 41, 45, 47, 48 }, { 26, 149 } },
};
#endif

#if defined(CONFIG_IDF_TARGET_ESP32C3)
static const expected_detector_pins_t expected_detector_pins[] = {
  { &c3_family_detector, "c3_family_detector", { 20 }, { 134, 135 } },
};
#endif

#if defined(CONFIG_IDF_TARGET_ESP32C5)
static const expected_detector_pins_t expected_detector_pins[] = {
  { &stampc5_detector, "stampc5_detector", {  }, { 153 } },
  { &toughc5_detector, "toughc5_detector", { 2, 3, 7, 8, 9, 25, 26 }, { 33 } },
};
#endif

#if defined(CONFIG_IDF_TARGET_ESP32C6)
static const expected_detector_pins_t expected_detector_pins[] = {
  { &c6_display_family_detector, "c6_display_family_detector", { 2, 3, 6, 8, 10, 15, 16, 17, 18, 20, 21, 22 }, { 25, 23 } },
  { &stampc6_detector, "stampc6_detector", {  }, { 154 } },
  { &nanoc6_detector, "nanoc6_detector", { 3 }, { 140 } },
};
#endif

#if defined(CONFIG_IDF_TARGET_ESP32H2)
static const expected_detector_pins_t expected_detector_pins[] = {
  { &nanoh2_detector, "nanoh2_detector", { 3 }, { 151 } },
};
#endif

#if defined(CONFIG_IDF_TARGET_ESP32C61)
static const expected_detector_pins_t expected_detector_pins[] = {
  { &corematrix_detector, "corematrix_detector", { 0, 1 }, { 152 } },
};
#endif

#if defined(CONFIG_IDF_TARGET_ESP32P4)
static const expected_detector_pins_t expected_detector_pins[] = {
  { &corep4x_detector, "corep4x_detector", { 9, 11 }, { 31 } },
  { &tab5_family_detector, "tab5_family_detector", { 22, 23, 31, 32 }, { 22, 35 } },
  { &unitpoep4_detector, "unitpoep4_detector", { 31, 32, 52 }, { 148 } },
};
#endif
