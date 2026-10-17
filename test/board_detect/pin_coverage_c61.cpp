#define CONFIG_IDF_TARGET_ESP32C61 1
#define CONFIG_IDF_TARGET "esp32c61"
#define COVERAGE_BOARD_HEADER "board_detect/m5/esp32c61.inl"
#define COVERAGE_CONDITIONAL_PINS no_pins()
#define COVERAGE_FAMILIES cover(esp32c61_detectors);
#include "support/pin_coverage.hpp"
