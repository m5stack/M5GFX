// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "board_detect.hpp"
#include "i2c_bus_probe.hpp"
#include "ops.inl"
#include "m5/generated/gpio_power_hold_board_ids.hpp"
#include "m5/generated/detector_order_constraints.hpp"

#include <cstdio>
#include <cstring>
#include <driver/gpio.h>
#include <soc/gpio_reg.h>
#include <esp_log.h>
#if defined (CONFIG_IDF_TARGET_ESP32S3) \
 && __has_include(<driver/dedic_gpio.h>) \
 && (__has_include(<hal/dedic_gpio_cpu_ll.h>) || __has_include(<hal/cpu_ll.h>)) \
 && (__has_include(<esp_private/esp_clk.h>) || __has_include(<esp_clk.h>))
 #define M5GFX_HAS_S3_DEDICATED_GPIO_RELEASE_PROBE
 #include <driver/dedic_gpio.h>
 #include <esp_cpu.h>
 #include <esp_idf_version.h>
 #if __has_include(<esp_private/esp_clk.h>)
  #include <esp_private/esp_clk.h>
 #else
  #include <esp_clk.h>
 #endif
 #include <freertos/FreeRTOS.h>
 #if __has_include(<hal/dedic_gpio_cpu_ll.h>)
  #include <hal/dedic_gpio_cpu_ll.h>
 #else
  // IDF 4.4 exposes the same S3 instructions through cpu_ll.h and reports
  // allocated channels as masks rather than offsets.
  #define M5GFX_S3_DEDICATED_GPIO_LEGACY_CPU_LL
  #include <hal/cpu_ll.h>
 #endif
 #include <hal/gpio_ll.h>
 #include <soc/gpio_periph.h>
 #include <soc/gpio_reg.h>
 #include <soc/io_mux_reg.h>
 #include <soc/system_reg.h>
#endif

namespace m5gfx
{
namespace board_detect
{
#if defined (M5GFX_HAS_S3_DEDICATED_GPIO_RELEASE_PROBE)
  static inline std::uint32_t dedicated_release_read_in()
  {
#if defined (M5GFX_S3_DEDICATED_GPIO_LEGACY_CPU_LL)
    return cpu_ll_read_dedic_gpio_in();
#else
    return dedic_gpio_cpu_ll_read_in();
#endif
  }

  static inline void dedicated_release_write_mask(std::uint32_t mask, std::uint32_t value)
  {
#if defined (M5GFX_S3_DEDICATED_GPIO_LEGACY_CPU_LL)
    cpu_ll_write_dedic_gpio_mask(mask, value);
#else
    dedic_gpio_cpu_ll_write_mask(mask, value);
#endif
  }

  static void dedicated_release_channel_masks(
    dedic_gpio_bundle_handle_t bundle, std::uint8_t pin_count,
    std::uint32_t* in_mask, std::uint32_t* out_mask,
    esp_err_t* in_error, esp_err_t* out_error)
  {
#if defined (M5GFX_S3_DEDICATED_GPIO_LEGACY_CPU_LL)
    *in_error = dedic_gpio_get_in_mask(bundle, in_mask);
    *out_error = dedic_gpio_get_out_mask(bundle, out_mask);
#else
    std::uint32_t in_offset = 0;
    std::uint32_t out_offset = 0;
    *in_error = dedic_gpio_get_in_offset(bundle, &in_offset);
    *out_error = dedic_gpio_get_out_offset(bundle, &out_offset);
    const std::uint32_t channel_mask = (1u << pin_count) - 1u;
    *in_mask = channel_mask << in_offset;
    *out_mask = channel_mask << out_offset;
#endif
  }

  static inline int dedicated_release_core_id()
  {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    return esp_cpu_get_core_id();
#else
    return xPortGetCoreID();
#endif
  }

  static inline std::uint32_t dedicated_release_cycle_count()
  {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    return esp_cpu_get_cycle_count();
#else
    return esp_cpu_get_ccount();
#endif
  }

  static inline int dedicated_release_task_core()
  {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 2, 0)
    return xTaskGetCoreID(nullptr);
#else
    return xTaskGetAffinity(nullptr);
#endif
  }

  static bool delete_dedicated_release_bundle(dedic_gpio_bundle_handle_t bundle)
  {
    const esp_err_t error = dedic_gpio_del_bundle(bundle);
    if (error != ESP_OK)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO bundle cleanup failed (%d)", int(error));
      return false;
    }
    return true;
  }
#endif

  // Call after family-specific evidence gates; a hint is eligible only while
  // that member remains possible. Defaults may be absent for open families.
  // Only some chips' detectors use it.
#include "member_selection.inl"

#include "transaction.inl"

#include "detector_runner.inl"

#include "probe_hardware.inl"

#include "startup.inl"

#include "spi_detector.inl"

#include "prepare.inl"
}
}
