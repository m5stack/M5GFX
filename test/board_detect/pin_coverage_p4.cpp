#define CONFIG_IDF_TARGET_ESP32P4 1
#define CONFIG_IDF_TARGET "esp32p4"
#define COVERAGE_BOARD_HEADER "board_detect/m5/esp32p4.inl"
#define COVERAGE_CONDITIONAL_PINS no_pins()
#define COVERAGE_FAMILIES cover(esp32p4_detectors);
#include "support/pin_coverage.hpp"
