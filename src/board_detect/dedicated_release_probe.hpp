// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

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
