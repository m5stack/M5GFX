#define CONFIG_IDF_TARGET_ESP32H2 1
#define CONFIG_IDF_TARGET "esp32h2"
#define COVERAGE_BOARD_HEADER "board_detect/m5/esp32h2.inl"
#define COVERAGE_CONDITIONAL_PINS no_pins()
#define COVERAGE_FAMILIES cover(esp32h2_detectors);
#include "support/pin_coverage.hpp"
