#define CONFIG_IDF_TARGET_ESP32C5 1
#define CONFIG_IDF_TARGET "esp32c5"
#define COVERAGE_BOARD_HEADER "board_detect/m5/esp32c5.inl"
#define COVERAGE_CONDITIONAL_PINS no_pins()
#define COVERAGE_FAMILIES cover(esp32c5_detectors);
#include "support/pin_coverage.hpp"
