#define CONFIG_IDF_TARGET_ESP32C6 1
#define CONFIG_IDF_TARGET "esp32c6"
#define COVERAGE_BOARD_HEADER "board_detect/m5/esp32c6.inl"
#define COVERAGE_CONDITIONAL_PINS no_pins()
#define COVERAGE_FAMILIES cover(esp32c6_detectors_qfn40); cover(esp32c6_detectors_qfn32);
#include "support/pin_coverage.hpp"
