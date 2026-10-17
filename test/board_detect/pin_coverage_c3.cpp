#define CONFIG_IDF_TARGET_ESP32C3 1
#define CONFIG_IDF_TARGET "esp32c3"
#define COVERAGE_BOARD_HEADER "board_detect/m5/esp32c3.inl"
#define COVERAGE_CONDITIONAL_PINS no_pins()
#define COVERAGE_FAMILIES cover(esp32c3_detectors);
#include "support/pin_coverage.hpp"
