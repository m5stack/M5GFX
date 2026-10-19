#pragma once
#include <cassert>
#include <initializer_list>
#include "runtime.hpp"
#include "probes.hpp"
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#include "board_detect/m5/esp32s3.inl"
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
#include "board_detect/m5/esp32c6.inl"
#else
#include "board_detect/m5/esp32p4.inl"
#endif
#include "display_backend.hpp"
static unsigned package_id;
static bool psram_active;

namespace m5gfx {

static const char LIBRARY_NAME[] = "host";
using lgfx::board_t;
#include "board_detect/detection_package.inl"
}
int main() {
  using namespace m5gfx::board_detect::m5;
  unsigned cases = 0;
  for (package_id = 0; package_id < 3; ++package_id)
    for (bool active : {false, true})
      for (bool legacy : {false, true}) {
        host_hardware::reset();
        psram_active = active;
        host_hardware::psram_active = active;
        host_hardware::package = package_id;
        const auto out = m5gfx::select_detection_package(legacy);
#if defined(CONFIG_IDF_TARGET_ESP32S3)
        const auto expected = package_id == 0 ? esp32s3_detectors_qfn56 :
                              package_id == 1 ? esp32s3_detectors_lga56 : nullptr;
        assert(out.conditional_pins_unavailable == active);
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
        const auto expected = package_id == 0 ? esp32c6_detectors_qfn40 :
                              package_id == 1 ? esp32c6_detectors_qfn32 : nullptr;
        assert(!out.conditional_pins_unavailable);
#else
        const auto expected = package_id == 0 ? esp32p4_detectors : nullptr;
        assert(!out.conditional_pins_unavailable);
#endif
        assert(out.detectors == expected && !out.direct_setup);
        assert(host_hardware::events.empty() && host_hardware::ports.empty());
        ++cases;
      }
  std::printf("Package selection: %u cases passed\n", cases);
}
