// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once
#include <cstdint>

namespace m5gfx { namespace board_detect {
  struct pin_pull_result_t
  {
    std::uint64_t pulldown_high = 0;
    std::uint64_t pullup_high = 0;
    std::uint64_t pulldown_release_high = 0;
    std::uint64_t pullup_release_high = 0;
    bool release_sampled = false;
  };

  // Catalog detect_class floating follows internal pulls after 10 us only.
  // This floating class additionally requires both biased levels to survive release.
  enum class pull_class_t : std::uint8_t
  {
    up, down, weak_up, weak_down, floating, conflict, pull_dependent,
  };

  // Requires a measured pin in 0..63. Pull-dependent pins without release
  // samples cannot be distinguished as floating or weakly biased.
  inline pull_class_t classify_pin_pull(const pin_pull_result_t& measured, int pin)
  {
    const auto bit = std::uint64_t(1) << pin;
    const bool pd = measured.pulldown_high & bit;
    const bool pu = measured.pullup_high & bit;
    if (pd && pu) { return pull_class_t::up; }
    if (!pd && !pu) { return pull_class_t::down; }
    if (pd && !pu) { return pull_class_t::conflict; }
    if (!measured.release_sampled) { return pull_class_t::pull_dependent; }
    const bool pd_release = measured.pulldown_release_high & bit;
    const bool pu_release = measured.pullup_release_high & bit;
    if (!pd_release && pu_release) { return pull_class_t::floating; }
    if (!pd_release && !pu_release) { return pull_class_t::weak_down; }
    if (pd_release && pu_release) { return pull_class_t::weak_up; }
    return pull_class_t::conflict;
  }

  struct detect_class_expected_t
  {
    std::uint64_t mask;
    std::uint64_t up;
    std::uint64_t down;
    std::uint64_t floating;
    std::uint64_t fixed;
  };

  // Catalog detect_class only on pins whose classification is invariant across
  // cold boots, software resets, wake from sleep and interrupted detection.
  // Driven pins may use fixed only when the driver is always powered. Do not
  // use PMIC-switched pulls or pins without internal pulls. This is a gate
  // before driven probes: board pins exclude candidates; choice soc_pins may
  // select revisions (revision selection is not implemented yet).
  inline bool match_detect_class(const detect_class_expected_t& expected,
                                 const pin_pull_result_t& measured)
  {
    const auto up = measured.pulldown_high & measured.pullup_high;
    const auto down = ~(measured.pulldown_high | measured.pullup_high);
    const auto floating = measured.pullup_high & ~measured.pulldown_high;
    const auto matched = (expected.up & up) | (expected.down & down)
                       | (expected.floating & floating) | (expected.fixed & (up | down));
    return (matched & expected.mask) == expected.mask;
  }
} }


#include <cstddef>
#include <cstdint>

namespace m5gfx
{
namespace board_detect
{
  static constexpr std::uint8_t max_dedicated_release_pins = 8;
  static constexpr std::uint8_t max_dedicated_release_samples = 15;
  static constexpr std::uint16_t max_dedicated_release_reads = 256;
  static constexpr std::uint16_t dedicated_release_no_high = 0xFFFF;

  enum class pin_release_band_t : std::uint8_t
  {
    short_release,
    ambiguous,
    long_release,
  };

  struct dedicated_release_result_t
  {
    std::uint16_t median_first_high[max_dedicated_release_pins] = {};
    std::uint8_t valid_samples[max_dedicated_release_pins] = {};
    std::uint8_t pin_count = 0;
    std::uint8_t requested_samples = 0;
    std::uint16_t reads = 0;
    std::uint32_t cpu_hz = 0;
    std::uint32_t total_cycles = 0;
    // Hardware setup, all timed passes, and timing metadata succeeded. Per-pin
    // transitions are validated separately by valid() and the summarizer.
    bool available = false;

    bool valid(std::uint8_t index) const
    {
      return available && index < pin_count
          && requested_samples != 0 && valid_samples[index] == requested_samples
          && median_first_high[index] < reads;
    }
  };

  inline std::uint16_t median_dedicated_release_samples(const std::uint16_t* values,
                                                         std::uint8_t sample_count,
                                                         std::uint16_t reads,
                                                         std::uint8_t* valid_count)
  {
    if (valid_count != nullptr) { *valid_count = 0; }
    if (values == nullptr || valid_count == nullptr || sample_count == 0
     || sample_count > max_dedicated_release_samples || reads == 0
     || reads > max_dedicated_release_reads)
    {
      return dedicated_release_no_high;
    }
    std::uint16_t sorted[max_dedicated_release_samples];
    for (std::uint8_t index = 0; index < sample_count; ++index)
    {
      if (values[index] < reads) { sorted[(*valid_count)++] = values[index]; }
    }
    // A single missing transition invalidates the pin instead of making its
    // retained median look faster than it was.
    if (*valid_count != sample_count) { return dedicated_release_no_high; }
    for (std::uint8_t index = 1; index < *valid_count; ++index)
    {
      const auto value = sorted[index];
      std::uint8_t insert = index;
      while (insert != 0 && value < sorted[insert - 1])
      {
        sorted[insert] = sorted[insert - 1];
        --insert;
      }
      sorted[insert] = value;
    }
    return sorted[sample_count / 2];
  }

  struct dedicated_release_summary_t
  {
    std::uint32_t average_ns = 0;
    std::uint32_t pin_ns[max_dedicated_release_pins] = {};
    std::uint8_t pin_count = 0;
    std::uint8_t valid_pins = 0;
    pin_release_band_t band = pin_release_band_t::ambiguous;
  };

  inline std::uint32_t dedicated_release_ns(std::uint32_t first_high,
                                             std::uint32_t total_cycles,
                                             std::uint64_t denominator)
  {
    if (denominator == 0) { return 0; }
    const std::uint64_t numerator = std::uint64_t(first_high) * total_cycles * 1000000000ULL;
    return static_cast<std::uint32_t>((numerator + denominator / 2) / denominator);
  }

  inline dedicated_release_summary_t summarize_dedicated_release(
    const dedicated_release_result_t& result,
    std::uint32_t short_max_ns,
    std::uint32_t long_min_ns)
  {
    dedicated_release_summary_t summary;
    summary.pin_count = result.pin_count;
    if (!result.available || result.pin_count == 0
     || result.pin_count > max_dedicated_release_pins
     || result.requested_samples == 0 || result.reads == 0 || result.cpu_hz == 0
     || result.total_cycles == 0 || short_max_ns >= long_min_ns)
    {
      return summary;
    }
    const std::uint64_t denominator64 = std::uint64_t(result.requested_samples)
                                      * result.reads * result.cpu_hz;
    std::uint32_t total_first_high = 0;
    for (std::uint8_t index = 0; index < result.pin_count; ++index)
    {
      if (!result.valid(index)) { continue; }
      ++summary.valid_pins;
      total_first_high += result.median_first_high[index];
      summary.pin_ns[index] = dedicated_release_ns(result.median_first_high[index],
                                                    result.total_cycles, denominator64);
    }
    if (summary.valid_pins != summary.pin_count) { return summary; }
    const std::uint64_t average_denominator64 = denominator64 * result.pin_count;
    summary.average_ns = dedicated_release_ns(total_first_high, result.total_cycles,
                                              average_denominator64);
    summary.band = summary.average_ns <= short_max_ns ? pin_release_band_t::short_release
                 : summary.average_ns >= long_min_ns ? pin_release_band_t::long_release
                                                     : pin_release_band_t::ambiguous;
    return summary;
  }
}
}


namespace m5gfx
{
namespace board_detect
{
namespace i2c_bus_probe_detail
{
  struct line_state_t
  {
    line_state_t(bool sda, bool scl, bool scl_held = false)
    : sda_high(sda), scl_high(scl), scl_held_low(scl_held) {}
    bool sda_high;
    bool scl_high;
    bool scl_held_low;
  };

  template <typename Sample, typename WaitScl, typename Recover>
  bool probe_i2c_bus_present(Sample sample, WaitScl wait_scl, Recover recover)
  {
    line_state_t state = sample();
    if (!state.scl_high && state.scl_held_low)
    {
      wait_scl();
      state = sample();
    }
    if (state.scl_high && !state.sda_high)
    {
      recover();
      state = sample();
    }
    return state.sda_high && state.scl_high;
  }
}
}
}
