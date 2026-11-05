#define CONFIG_IDF_TARGET_ESP32 1
#define CONFIG_IDF_TARGET "esp32"
#define COVERAGE_BOARD_HEADER "board_detect/m5/esp32_d0wdq6.inl"
#define COVERAGE_CONDITIONAL_PINS no_pins()
#define COVERAGE_FAMILIES cover(esp32_d0wdq6_detectors); cover(esp32_pico_d4_detectors); cover(esp32_picov3_detectors);
#include "support/pin_coverage.hpp"
