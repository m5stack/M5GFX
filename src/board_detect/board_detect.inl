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
  __attribute__((unused))
  static bool select_provisional_member(const prepare_ctx_t& ctx, board_result_t* result,
                                const board_desc_t* preferred_if_possible,
                                const board_desc_t* hinted_if_possible,
                                const board_desc_t* family_default, const char* why)
  {
    if (!ctx.final_attempt || result == nullptr) { return false; }
    const auto* chosen = preferred_if_possible ? preferred_if_possible
                       : hinted_if_possible ? hinted_if_possible : family_default;
    if (chosen == nullptr) { return false; }
    result->assign(chosen);
    result->provisional = true;
    ESP_LOGW("board_detect_m5", "%s; using %s for this boot", why, chosen->def.name);
    return true;
  }

  static void restore_if_changed(lgfx::gpio::pin_backup_t& saved)
  {
    // restore() briefly disables output driving. Skip it when every saved
    // register already matches so an unchanged output cannot momentarily float.
    if (!saved.matches_current()) { saved.restore(); }
  }

  bool detection_gpio_snapshot_t::capture(pin_list_t unconditional,
                                          pin_list_t conditional,
                                          bool include_conditional)
  {
    const std::size_t total = unconditional.size
                            + (include_conditional ? conditional.size : 0);
    if ((unconditional.data == nullptr && unconditional.size != 0)
     || (conditional.data == nullptr && conditional.size != 0)
     || total > max_detection_pins)
    {
      return false;
    }
    const pin_list_t lists[] = { unconditional, include_conditional ? conditional : pin_list_t { nullptr, 0 } };
    for (const auto& list : lists)
    {
      for (std::size_t index = 0; index < list.size; ++index)
      {
        const auto pin = list.data[index];
        if (pin < 0 || pin >= GPIO_NUM_MAX || !GPIO_IS_VALID_GPIO(pin)) { return false; }
        for (std::size_t prior = 0; prior < count_; ++prior)
        {
          if (saved_[prior].getPin() == pin) { return false; }
        }
        saved_[count_].setPin(pin);
        saved_[count_].backup();
        ++count_;
      }
    }
    // LP-I2C routing can survive a system reset and make ordinary GPIO reads
    // observe a controller-held line instead of the pad.  Reclaim every pin
    // only after the complete snapshot succeeds.  Like the legacy soft-I2C
    // probe, rollback restores normal GPIO state but deliberately does not
    // return pads to stale low-power ownership.
    for (std::size_t index = 0; index < count_; ++index)
    {
      lgfx::gpio::release_lp_pad(saved_[index].getPin());
    }
    return true;
  }

  void detection_gpio_snapshot_t::rollback()
  {
    for (std::size_t index = count_; index != 0; --index) { restore_if_changed(saved_[index - 1]); }
  }

  void detection_gpio_snapshot_t::restore_start(const std::int8_t* pins,
                                                 std::size_t count)
  {
    if (pins == nullptr && count != 0)
    {
      ESP_LOGE("board_detect", "restore_start missing pin list");
      return;
    }
    bool missing = false;
    int missing_pin = -1;
    for (std::size_t requested = 0; requested < count; ++requested)
    {
      const int pin = pins[requested];
      if (pin < 0) { continue; }  // an unused optional signal
      bool found = false;
      if (pin >= 0 && pin < GPIO_NUM_MAX && GPIO_IS_VALID_GPIO(pin))
      {
        for (std::size_t index = count_; index != 0; --index)
        {
          if (saved_[index - 1].getPin() == pin) { found = true; break; }
        }
      }
      if (!found && !missing) { missing = true; missing_pin = pin; }
    }
    // Match full rollback ordering while restoring each captured pin at most once.
    for (std::size_t index = count_; index != 0; --index)
    {
      const auto pin = saved_[index - 1].getPin();
      for (std::size_t requested = 0; requested < count; ++requested)
      {
        if (pins[requested] == pin)
        {
          restore_if_changed(saved_[index - 1]);
          break;
        }
      }
    }
    if (missing)
    {
      ESP_LOGE("board_detect", "restore_start pin=%d is absent from snapshot", missing_pin);
    }
  }

  void detection_bus_journal_t::opened_i2c(int port)
  {
    if (port < 0) { return; }
    for (std::size_t index = 0; index < i2c_count_; ++index)
    {
      if (i2c_ports_[index] == port) { return; }
    }
    if (i2c_count_ < sizeof(i2c_ports_) / sizeof(i2c_ports_[0]))
    {
      i2c_ports_[i2c_count_++] = static_cast<std::int8_t>(port);
    }
  }

  void detection_bus_journal_t::rollback()
  {
    while (i2c_count_ != 0) { lgfx::i2c::release(i2c_ports_[--i2c_count_]); }
  }

  void detection_bus_journal_t::commit()
  {
    i2c_count_ = 0;
  }

  detection_transaction_t::detection_transaction_t(
    pin_list_t unconditional, pin_list_t conditional, bool include_conditional)
  : valid_ { gpio_.capture(unconditional, conditional, include_conditional) }
  {
  }

  detection_transaction_t::~detection_transaction_t()
  {
    rollback();
  }

  void detection_transaction_t::restore_start(std::initializer_list<int> pins)
  {
    std::int8_t values[max_detection_pins];
    std::size_t count = 0;
    for (auto pin : pins)
    {
      if (count == max_detection_pins) { break; }
      // Anything outside int8_t is not a GPIO; keep it absent instead of aliasing.
      values[count++] = (pin < -1 || pin > INT8_MAX) ? INT8_MAX : static_cast<std::int8_t>(pin);
    }
    gpio_.restore_start(values, count);
  }

  void detection_transaction_t::rollback()
  {
    if (!valid_ || committed_) { return; }
    buses_.rollback();
    gpio_.rollback();
  }

  void detection_transaction_t::commit()
  {
    if (!valid_ || committed_) { return; }
    buses_.commit();
    committed_ = true;
  }

  bool board_detector_t::has_member(board_id_t id) const
  {
    if (members == nullptr) { return false; }
    for (auto p = members; *p != nullptr; ++p)
    {
      if ((*p)->id == id) { return true; }
    }
    return false;
  }

  namespace
  {
    static constexpr char tag[] = "board_detect";

    bool enabled(const probe_ctx_t& ctx, board_id_t id)
    {
      if (ctx.enabled_ids == nullptr) { return true; }
      for (auto p = ctx.enabled_ids; *p != board_id_unknown; ++p)
      {
        if (*p == id) { return true; }
      }
      return false;
    }

    bool fallback_only(const board_detector_t* detector)
    {
      if (detector->members == nullptr || detector->members[0] == nullptr) { return false; }
      for (auto p = detector->members; *p != nullptr; ++p)
      {
        if (!((*p)->flags & def_flag_fallback)) { return false; }
      }
      return true;
    }

    bool gpio_power_hold_family(const board_detector_t* detector)
    {
      for (const auto id : gpio_power_hold_board_ids)
      {
        if (detector->has_member(id)) { return true; }
      }
      return false;
    }

    bool run_detector(const board_detector_t* detector, probe_ctx_t& ctx, board_result_t* result)
    {
      if (ctx.transaction == nullptr)
      {
        ESP_LOGE(tag, "detector requires a detection transaction");
        return false;
      }
      const char* family = (detector->members && detector->members[0])
                         ? detector->members[0]->name : "empty";
      const bool signature = detector->signature(ctx);
      ESP_LOGD(tag, "stage=1 family=%s detector=%p match=%d", family,
               static_cast<const void*>(detector), signature);
      if (!signature)
      {
        ctx.transaction->rollback();
        return false;
      }

      ctx.confirm_attempted = true;

      board_result_t candidate;
      const bool confirmed = detector->confirm(ctx, &candidate);
      ESP_LOGD(tag, "stage=2 family=%s detector=%p match=%d board=%u name=%s", family,
               static_cast<const void*>(detector), confirmed,
               static_cast<unsigned>(candidate.def ? candidate.def->id : board_id_unknown),
               candidate.def ? candidate.def->name : "invalid");
      if (!confirmed)
      {
        ctx.transaction->rollback();
        return false;
      }

      if (candidate.desc == nullptr || candidate.def == nullptr
       || candidate.def != &candidate.desc->def
       || candidate.def->id == board_id_unknown)
      {
        ESP_LOGW(tag, "detector=%p returned a match without a consistent board description",
                 static_cast<const void*>(detector));
        ctx.transaction->rollback();
        return false;
      }
      candidate.status = enabled(ctx, candidate.def->id)
                       ? detect_status_t::matched
                       : detect_status_t::excluded;
      const auto& desc = *candidate.desc;
      char option_text[80] = {};
      std::uint32_t named_bits = 0;
      if (desc.option_names.data != nullptr)
      {
        for (std::uint_fast8_t bit = 0; bit < 32 && bit < desc.option_names.size; ++bit)
        {
          if (!(candidate.option & (std::uint32_t(1) << bit))) { continue; }
          const auto used = std::strlen(option_text);
          std::snprintf(option_text + used, sizeof(option_text) - used, "%s%s",
                        used ? "," : "", desc.option_names.data[bit]);
          named_bits |= std::uint32_t(1) << bit;
        }
      }
      const auto unnamed = candidate.option & ~named_bits;
      if (unnamed)
      {
        const auto used = std::strlen(option_text);
        std::snprintf(option_text + used, sizeof(option_text) - used, "%sunknown:%08x",
                      used ? "," : "",
                      static_cast<unsigned>(unnamed));
      }
      if (option_text[0] == '\0') { std::snprintf(option_text, sizeof(option_text), "none"); }
      ESP_LOGD(tag, "detected board=%u name=%s opt=0x%08x",
               static_cast<unsigned>(candidate.def->id), candidate.def->name,
               static_cast<unsigned>(candidate.option));
      ESP_LOGD(tag, "detected option names board=%u opt=0x%08x(%s)",
               static_cast<unsigned>(candidate.def->id),
               static_cast<unsigned>(candidate.option), option_text);
      *result = candidate;
      return true;
    }
  }

  board_result_t detect_board(const board_detector_t* const* list, board_id_t hint, probe_ctx_t& ctx)
  {
    board_result_t result;
    ctx.confirm_attempted = false;
    if (list == nullptr) { return result; }
    ctx.hint = hint;

    const board_detector_t* hinted = nullptr;
    if (hint != board_id_unknown)
    {
      bool hint_found = false;
      for (auto p = list; *p != nullptr; ++p)
      {
        if ((*p)->has_member(hint))
        {
          hint_found = true;
          if (!fallback_only(*p)) { hinted = *p; }
          else
          {
            ESP_LOGD(tag, "hint=%u belongs to a fallback-only detector",
                     static_cast<unsigned>(hint));
          }
          break;
        }
      }
      if (!hint_found)
      {
        // A hint belonging to a detector outside this package-specific list is normal.
        ESP_LOGD(tag, "hint=%u is not present in detector list", static_cast<unsigned>(hint));
      }
    }

    unsigned count = 0;
    while (list[count] != nullptr && count <= max_detector_families) { ++count; }
    if (count > max_detector_families) { return result; }
    bool tried[max_detector_families] = {};
    for (unsigned pass = 0; pass < count; ++pass)
    {
      unsigned chosen = count;
      unsigned best = 4;
      for (unsigned i = 0; i < count; ++i)
      {
        if (tried[i]) { continue; }
        bool blocked = false;
        for (const auto& edge : detector_order_edges)
        {
          if (!list[i]->has_member(edge.after)) { continue; }
          for (unsigned j = 0; j < count; ++j)
          {
            if (!tried[j] && list[j]->has_member(edge.before)) { blocked = true; break; }
          }
          if (blocked) { break; }
        }
        if (blocked) { continue; }
        const bool hold = gpio_power_hold_family(list[i]);
        const unsigned rank = hold ? (list[i] == hinted ? 0 : 1)
                                   : (list[i] == hinted ? 2 : 3);
        if (rank < best) { chosen = i; best = rank; }
      }
      if (chosen == count) { break; } // Generated constraints must be acyclic.
      tried[chosen] = true;
      if (run_detector(list[chosen], ctx, &result)) { return result; }
      if (list[chosen] == hinted)
      {
        ESP_LOGD(tag, "hint=%u did not match its detector family", static_cast<unsigned>(hint));
      }
    }
    result.candidate = ctx.candidate;
    return result;
  }

  bool wait_i2c_scl_high(int pin_scl)
  {
    lgfx::gpio_hi(pin_scl);
    lgfx::delayMicroseconds(5);
    if (lgfx::gpio_in(pin_scl)) { return true; }
    const auto started = lgfx::micros();
    while (lgfx::micros() - started < 25000)
    {
      if (lgfx::gpio_in(pin_scl)) { return true; }
      lgfx::delayMicroseconds(1);
    }
    return false;
  }

  bool release_held_sda(int pin_sda, int pin_scl)
  {
    lgfx::pinMode(pin_sda, lgfx::pin_mode_t::input);
    lgfx::pinMode(pin_scl, lgfx::pin_mode_t::input);
    lgfx::delayMicroseconds(10);

    // A reset of this MCU during a transfer can leave a peripheral holding
    // SDA low while it waits for more clocks. Clock it out (up to 9 bits) and
    // finish with STOP. Input mode is open drain with the output latch high.
    bool bus_released = lgfx::gpio_in(pin_scl);
    if (bus_released && !lgfx::gpio_in(pin_sda))
    {
      for (int i = 0; i < 9 && !lgfx::gpio_in(pin_sda); ++i)
      {
        lgfx::gpio_lo(pin_scl);
        lgfx::delayMicroseconds(5);
        if (!wait_i2c_scl_high(pin_scl)) { bus_released = false; break; }
      }
      if (bus_released)
      {
        lgfx::gpio_lo(pin_scl);
        lgfx::gpio_lo(pin_sda);
        lgfx::delayMicroseconds(5);
        bus_released = wait_i2c_scl_high(pin_scl);
        if (bus_released)
        {
          lgfx::gpio_hi(pin_sda);
          lgfx::delayMicroseconds(10);
        }
      }
    }
    const bool released = bus_released && lgfx::gpio_in(pin_sda) && lgfx::gpio_in(pin_scl);
    return released;
  }

  pin_pull_result_t recover_held_sda_and_resample(
    probe_ctx_t& ctx, pin_pull_result_t pulls, std::uint64_t pin_mask,
    int pin_sda, int pin_scl)
  {
    const auto high = pulls.pulldown_high;
    if ((high & (std::uint64_t(1) << pin_scl))
     && !(high & (std::uint64_t(1) << pin_sda)))
    {
      release_held_sda(pin_sda, pin_scl);
      pulls = probe_pin_pulls(ctx, pin_mask);
      ctx.transaction->restore_start({ pin_sda, pin_scl });
    }
    return pulls;
  }

  bool probe_i2c_bus_present(probe_ctx_t& ctx, int pin_sda, int pin_scl)
  {
    const std::uint64_t sda_bit = std::uint64_t(1) << pin_sda;
    const std::uint64_t scl_bit = std::uint64_t(1) << pin_scl;
    const std::uint64_t mask = sda_bit | scl_bit;
    auto sample = [&]() -> i2c_bus_probe_detail::line_state_t
    {
      const auto pulls = probe_pin_pulls(ctx, mask);
      return { bool(pulls.pulldown_high & sda_bit),
               bool(pulls.pulldown_high & scl_bit),
               !(pulls.pullup_high & scl_bit) };
    };
    auto wait_scl = [&]()
    {
      // A floating pin is raised by the internal pull-up during sample() and
      // never reaches this path. Only an externally held-low SCL receives the
      // legacy soft-I2C clock-stretch allowance.
      lgfx::pinMode(pin_sda, lgfx::pin_mode_t::input);
      lgfx::pinMode(pin_scl, lgfx::pin_mode_t::input);
      wait_i2c_scl_high(pin_scl);
      ctx.transaction->restore_start({ pin_sda, pin_scl });
    };
    auto recover = [&]()
    {
      // A reset during a transaction can leave a slave holding SDA forever.
      // The legacy soft-I2C startup recovered it before attempting detection.
      ESP_LOGD("M5GFX", "[Autodetect] recovering held SDA on I2C pins SDA=%d SCL=%d",
               pin_sda, pin_scl);
      release_held_sda(pin_sda, pin_scl);
      ctx.transaction->restore_start({ pin_sda, pin_scl });
    };
    return i2c_bus_probe_detail::probe_i2c_bus_present(sample, wait_scl, recover);
  }

  bool probe_pin_pullup_low(probe_ctx_t& ctx, int pin)
  {
    lgfx::pinMode(pin, lgfx::pin_mode_t::input_pullup);
    lgfx::delayMicroseconds(10);
    const bool held_low = !lgfx::gpio_in(pin);
    ctx.transaction->restore_start({ pin });
    return held_low;
  }

  bool probe_i2c_ack(probe_ctx_t& ctx, int pin_sda, int pin_scl, std::uint8_t addr)
  {
    // Reserved addresses are never touched, even if requested accidentally.
    if (addr < 0x08 || addr > 0x77) { return false; }
    auto& cache = ctx.i2c_cache;
    if (cache.pin_sda != pin_sda || cache.pin_scl != pin_scl)
    {
      cache = {};
      cache.pin_sda = pin_sda;
      cache.pin_scl = pin_scl;

      lgfx::pinMode(pin_sda, lgfx::pin_mode_t::input_pulldown);
      lgfx::pinMode(pin_scl, lgfx::pin_mode_t::input_pulldown);
      lgfx::delayMicroseconds(10);
      lgfx::pinMode(pin_sda, lgfx::pin_mode_t::input);
      lgfx::pinMode(pin_scl, lgfx::pin_mode_t::input);
      lgfx::delayMicroseconds(10);
      // The first check does not wait: a pin pair without pull-ups reads SCL
      // low here, and waiting for it would delay every such detection. Only a
      // bus that has pull-ups and a held SDA is clocked, and only those clocks
      // wait for a stretching peripheral.
      bool bus_released = lgfx::gpio_in(pin_scl);
      if (bus_released && !lgfx::gpio_in(pin_sda))
      {
        bus_released = release_held_sda(pin_sda, pin_scl);
      }
      cache.pullup_ok = bus_released && lgfx::gpio_in(pin_sda) && lgfx::gpio_in(pin_scl);
      ctx.transaction->restore_start({ pin_sda, pin_scl });
    }

    const std::uint32_t bit = 1u << (addr & 31);
    if (!(cache.checked[addr >> 5] & bit))
    {
      cache.checked[addr >> 5] |= bit;
      if (cache.pullup_ok)
      {
        if (lgfx::i2c::init(ctx.i2c_port_probe, pin_sda, pin_scl).has_value())
        {
          const bool hit = lgfx::i2c::beginTransaction(ctx.i2c_port_probe, addr, 100000, false).has_value()
                        && lgfx::i2c::endTransaction(ctx.i2c_port_probe).has_value();
          if (hit) { cache.ack[addr >> 5] |= bit; }
          lgfx::i2c::release(ctx.i2c_port_probe);
        }
        ctx.transaction->restore_start({ pin_sda, pin_scl });
      }
    }
    return cache.pullup_ok && (cache.ack[addr >> 5] & bit);
  }

  bool probe_i2c_read(probe_ctx_t& ctx, int pin_sda, int pin_scl, std::uint8_t addr,
                      std::uint16_t reg, std::uint8_t* data, std::size_t length,
                      std::uint32_t freq, std::uint32_t poll_ms, bool reg16)
  {
    if (data == nullptr || length == 0 || addr < 0x08 || addr > 0x77) { return false; }
    if (!lgfx::i2c::init(ctx.i2c_port_probe, pin_sda, pin_scl).has_value()) { return false; }
    const std::uint8_t reg_bytes[] = {
      static_cast<std::uint8_t>(reg >> 8), static_cast<std::uint8_t>(reg),
    };
    const auto started = lgfx::millis();
    bool success = false;
    do
    {
      success = lgfx::i2c::transactionWriteRead(
        ctx.i2c_port_probe, addr, reg_bytes + !reg16, 1 + reg16,
        data, length, freq).has_value();
      if (success) { break; }
      lgfx::delay(1);
    } while (lgfx::millis() - started < poll_ms);
    lgfx::i2c::release(ctx.i2c_port_probe);
    ctx.transaction->restore_start({ pin_sda, pin_scl });
    return success;
  }

  pin_pull_result_t probe_pin_pulls(probe_ctx_t& ctx, std::uint64_t pin_mask)
  {
    static constexpr std::size_t max_pins = 64;
    pin_pull_result_t result;
    for (std::size_t pin = 0; pin < max_pins; ++pin)
    {
      const std::uint64_t bit = std::uint64_t(1) << pin;
      if (!(pin_mask & bit)) { continue; }
      // Measure one pin completely before touching the next. On a native-mode
      // SD bus this avoids raising CLK while CMD is temporarily pulled low.
      lgfx::pinMode(pin, lgfx::pin_mode_t::input_pulldown);
      lgfx::delayMicroseconds(10);
      if (lgfx::gpio_in(pin)) { result.pulldown_high |= bit; }
      lgfx::pinMode(pin, lgfx::pin_mode_t::input_pullup);
      lgfx::delayMicroseconds(10);
      if (lgfx::gpio_in(pin)) { result.pullup_high |= bit; }
      ctx.transaction->restore_start(static_cast<std::int8_t>(pin));
    }
    return result;
  }

  bool probe_pin_floating(probe_ctx_t& ctx, std::int8_t pin, std::uint32_t release_us)
  {
    lgfx::pinMode(pin, lgfx::pin_mode_t::input_pullup);
    lgfx::delayMicroseconds(10);
    const bool charged_high = lgfx::gpio_in(pin);
    lgfx::pinMode(pin, lgfx::pin_mode_t::input);
    lgfx::delayMicroseconds(release_us);
    const bool held_high = lgfx::gpio_in(pin);

    lgfx::pinMode(pin, lgfx::pin_mode_t::input_pulldown);
    lgfx::delayMicroseconds(10);
    const bool charged_low = !lgfx::gpio_in(pin);
    lgfx::pinMode(pin, lgfx::pin_mode_t::input);
    lgfx::delayMicroseconds(release_us);
    const bool held_low = !lgfx::gpio_in(pin);
    ctx.transaction->restore_start(pin);
    return charged_high && held_high && charged_low && held_low;
  }

  dedicated_release_result_t probe_dedicated_pin_release(
    probe_ctx_t& ctx, const std::int8_t* pins, std::uint8_t pin_count,
    std::uint16_t reads, std::uint8_t samples, std::uint32_t settle_us)
  {
    dedicated_release_result_t result;
    result.pin_count = pin_count;
    result.requested_samples = samples;
    result.reads = reads;
    for (auto& value : result.median_first_high) { value = dedicated_release_no_high; }
#if defined (M5GFX_HAS_S3_DEDICATED_GPIO_RELEASE_PROBE)
    if (ctx.transaction == nullptr || pins == nullptr || pin_count == 0
     || pin_count > max_dedicated_release_pins || reads == 0
     || reads > max_dedicated_release_reads || samples == 0
     || samples > max_dedicated_release_samples || settle_us == 0)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe has invalid parameters");
      return result;
    }
    const int pinned_core = dedicated_release_task_core();
    if (pinned_core == static_cast<int>(tskNO_AFFINITY)
     || dedicated_release_core_id() != pinned_core)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe requires a core-pinned task");
      return result;
    }
    // The S3 dedicated-GPIO clock/reset bit is shared by both CPU cores.
    // IDF 4.4-5.3 reaches it through periph_module's reference count, while
    // IDF 5.5+ manipulates it directly, making this preflight check necessary
    // to avoid resetting or stopping another live bundle. The check and
    // new_bundle are not atomic: applications that initialize dedicated GPIO
    // concurrently on another core during this brief M5.begin probe are not
    // supported.
    if (REG_GET_BIT(SYSTEM_CPU_PERI_CLK_EN_REG, SYSTEM_CLK_EN_DEDICATED_GPIO))
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO in use; skipping release probe");
      return result;
    }
    int bundle_pins[max_dedicated_release_pins];
    for (std::uint8_t index = 0; index < pin_count; ++index)
    {
      const int pin = pins[index];
      if (pin < 0 || pin >= GPIO_NUM_MAX || !GPIO_IS_VALID_GPIO(pin))
      {
        ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe has invalid pin (%d)", pin);
        return result;
      }
      bundle_pins[index] = pin;
    }
    for (std::uint8_t index = 0; index < pin_count; ++index)
    {
      const int pin = bundle_pins[index];
      // input_pullup retains output-enable but sets OD with a High latch, so
      // the pad is Hi-Z. Prepare the weakest drive before new_bundle connects
      // its active output route to avoid a transient push-pull/strong drive.
      lgfx::pinMode(pin, lgfx::pin_mode_t::input_pullup);
      const auto io_mux = GPIO_PIN_MUX_REG[pin];
      REG_WRITE(io_mux, (REG_READ(io_mux) & ~(FUN_PU | FUN_PD | FUN_DRV_M)) | FUN_PU);
      gpio_ll_od_enable(&GPIO, static_cast<gpio_num_t>(pin));
    }

    dedic_gpio_bundle_handle_t bundle = nullptr;
    dedic_gpio_bundle_config_t config = {};
    config.gpio_array = bundle_pins;
    config.array_size = pin_count;
    config.flags.in_en = 1;
    config.flags.out_en = 1;
    const int creation_core = dedicated_release_core_id();
    if (creation_core != pinned_core)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe task migrated before bundle creation");
      ctx.transaction->restore_start(pins, pin_count);
      return result;
    }
    const esp_err_t bundle_error = dedic_gpio_new_bundle(&config, &bundle);
    if (bundle_error != ESP_OK)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO bundle creation failed (%d)", int(bundle_error));
      ctx.transaction->restore_start(pins, pin_count);
      return result;
    }
    if (dedicated_release_core_id() != pinned_core)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe task migrated after bundle creation");
      delete_dedicated_release_bundle(bundle);
      ctx.transaction->restore_start(pins, pin_count);
      return result;
    }
    std::uint32_t input_mask = 0;
    std::uint32_t output_mask = 0;
    esp_err_t input_mask_error = ESP_OK;
    esp_err_t output_mask_error = ESP_OK;
    dedicated_release_channel_masks(bundle, pin_count, &input_mask, &output_mask,
                                    &input_mask_error, &output_mask_error);
    if (input_mask_error != ESP_OK || output_mask_error != ESP_OK)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO channel lookup failed (in=%d out=%d)",
               int(input_mask_error), int(output_mask_error));
      delete_dedicated_release_bundle(bundle);
      ctx.transaction->restore_start(pins, pin_count);
      return result;
    }
    const std::uint32_t channel_mask = (1u << pin_count) - 1u;
    const std::uint32_t input_first_bit = input_mask & (~input_mask + 1u);
    if (input_first_bit == 0 || input_mask != input_first_bit * channel_mask
     || output_mask == 0)
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO channel allocation is not contiguous");
      delete_dedicated_release_bundle(bundle);
      ctx.transaction->restore_start(pins, pin_count);
      return result;
    }
    static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    const auto measure = [&](std::uint16_t* first_high, std::uint32_t* cycles)
    {
      volatile std::uint32_t raw[max_dedicated_release_reads];
      taskENTER_CRITICAL(&mux);
      // A dedicated bundle belongs to its creation core. Check immediately
      // after scheduling is stopped and reject the measurement if it migrated.
      if (dedicated_release_core_id() != creation_core)
      {
        taskEXIT_CRITICAL(&mux);
        return false;
      }
      dedicated_release_write_mask(output_mask, 0);
      lgfx::delayMicroseconds(settle_us);
      const std::uint32_t started = dedicated_release_cycle_count();
      dedicated_release_write_mask(output_mask, output_mask);
      for (std::uint16_t read = 0; read < reads; ++read)
      { raw[read] = dedicated_release_read_in(); }
      const std::uint32_t stopped = dedicated_release_cycle_count();
      taskEXIT_CRITICAL(&mux);
      *cycles = stopped - started;
      for (std::uint8_t index = 0; index < pin_count; ++index)
      {
        first_high[index] = dedicated_release_no_high;
        const std::uint32_t bit = input_first_bit << index;
        for (std::uint16_t read = 0; read < reads; ++read)
        {
          if (raw[read] & bit) { first_high[index] = read; break; }
        }
      }
      return true;
    };

    bool complete = true;
    std::uint16_t discarded[max_dedicated_release_pins];
    std::uint32_t cycles = 0;
    complete = measure(discarded, &cycles);  // Discard the first, cache-warming pass.
    std::uint16_t measured[max_dedicated_release_pins][max_dedicated_release_samples];
    for (std::uint8_t sample = 0; complete && sample < samples; ++sample)
    {
      std::uint16_t current[max_dedicated_release_pins];
      complete = measure(current, &cycles);
      if (!complete) { break; }
      result.total_cycles += cycles;
      for (std::uint8_t index = 0; index < pin_count; ++index)
      { measured[index][sample] = current[index]; }
    }
    if (complete)
    {
      result.cpu_hz = static_cast<std::uint32_t>(esp_clk_cpu_freq());
      result.available = result.cpu_hz != 0 && result.total_cycles != 0;
      if (result.cpu_hz == 0)
      {
        ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe CPU frequency is unavailable");
      }
      else if (result.total_cycles == 0)
      {
        ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe cycle counter did not advance");
      }
      std::uint8_t missing_transitions = 0;
      for (std::uint8_t index = 0; index < pin_count; ++index)
      {
        result.median_first_high[index] = median_dedicated_release_samples(
          measured[index], samples, reads, &result.valid_samples[index]);
        missing_transitions += result.valid_samples[index] != samples;
      }
      if (missing_transitions != 0)
      {
        ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO release transition missing on %u/%u pins",
                 unsigned(missing_transitions), unsigned(pin_count));
      }
    }
    else
    {
      ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO probe task migrated during capture");
    }

    if (!delete_dedicated_release_bundle(bundle)) { result.available = false; }
    // del_bundle pin cleanup differs by IDF version (4.4/5.2/5.3 leave the
    // pads alone; newer versions disable output), and out_sel is not restored.
    // Dedicated input routing is peripheral-side. In every version,
    // The transaction's restore_start (pin_backup_t::restore) restores out_sel,
    // GPIO_PINn, output-enable, latch, and the complete IO_MUX register
    // (including temporary FUN_DRV).
    ctx.transaction->restore_start(pins, pin_count);
    return result;
#else
    ESP_LOGW("M5GFX", "[Autodetect] dedicated GPIO release probe is unavailable in this SDK");
    (void)ctx;
    (void)pins;
    (void)settle_us;
    return result;
#endif
  }

  void soft_spi_t::init()
  {
    lgfx::gpio_lo(pin_sclk_);
    lgfx::pinMode(pin_sclk_, lgfx::pin_mode_t::output);
    lgfx::gpio_lo(pin_mosi_);
    lgfx::pinMode(pin_mosi_, lgfx::pin_mode_t::output);
    if (pin_miso_ != pin_mosi_) { lgfx::pinMode(pin_miso_, lgfx::pin_mode_t::input); }
    if (pin_dc_ >= 0) { lgfx::pinMode(pin_dc_, lgfx::pin_mode_t::output); }
  }

  void soft_spi_t::beginTransaction()
  {
    lgfx::gpio_lo(pin_sclk_);
  }

  void soft_spi_t::endTransaction()
  {
    endRead();
    lgfx::gpio_lo(pin_sclk_);
  }

  void soft_spi_t::clock()
  {
    lgfx::delayMicroseconds(half_us_);
    lgfx::gpio_hi(pin_sclk_);
    lgfx::delayMicroseconds(half_us_);
    lgfx::gpio_lo(pin_sclk_);
    lgfx::delayMicroseconds(half_us_);
  }

  void soft_spi_t::send(std::uint32_t data, std::uint_fast8_t bits)
  {
    for (std::uint_fast8_t i = 0; i < bits; ++i)
    {
      if (data & (std::uint32_t(1) << bit_index(i))) { lgfx::gpio_hi(pin_mosi_); }
      else                                           { lgfx::gpio_lo(pin_mosi_); }
      clock();
    }
  }

  void soft_spi_t::writeCommand(std::uint32_t data, std::uint_fast8_t bits)
  {
    if (pin_dc_ >= 0) { lgfx::gpio_lo(pin_dc_); }
    send(data, bits);
  }

  void soft_spi_t::writeData(std::uint32_t data, std::uint_fast8_t bits)
  {
    if (pin_dc_ >= 0) { lgfx::gpio_hi(pin_dc_); }
    send(data, bits);
  }

  std::uint32_t soft_spi_t::transferData(std::uint32_t data, std::uint_fast8_t bits)
  {
    if (pin_dc_ >= 0) { lgfx::gpio_hi(pin_dc_); }
    std::uint32_t value = 0;
    for (std::uint_fast8_t i = 0; i < bits; ++i)
    {
      const auto index = bit_index(i);
      if (data & (std::uint32_t(1) << index)) { lgfx::gpio_hi(pin_mosi_); }
      else                                    { lgfx::gpio_lo(pin_mosi_); }
      lgfx::delayMicroseconds(half_us_);
      if (lgfx::gpio_in(pin_miso_)) { value |= std::uint32_t(1) << index; }
      lgfx::gpio_hi(pin_sclk_);
      lgfx::delayMicroseconds(half_us_);
      lgfx::gpio_lo(pin_sclk_);
    }
    return value;
  }

  void soft_spi_t::beginRead(std::uint_fast8_t dummy_bits)
  {
    if (pin_dc_ >= 0) { lgfx::gpio_hi(pin_dc_); }
    if (pin_miso_ == pin_mosi_) { lgfx::pinMode(pin_mosi_, lgfx::pin_mode_t::input); }
    for (std::uint_fast8_t i = 0; i < dummy_bits; ++i) { clock(); }
  }

  std::uint32_t soft_spi_t::readData(std::uint_fast8_t bits)
  {
    std::uint32_t value = 0;
    for (std::uint_fast8_t i = 0; i < bits; ++i)
    {
      lgfx::delayMicroseconds(half_us_);
      if (lgfx::gpio_in(pin_miso_)) { value |= std::uint32_t(1) << bit_index(i); }
      lgfx::gpio_hi(pin_sclk_);
      lgfx::delayMicroseconds(half_us_);
      lgfx::gpio_lo(pin_sclk_);
    }
    return value;
  }

  void soft_spi_t::readBytes(std::uint8_t* dst, std::size_t length)
  {
    while (length--) { *dst++ = static_cast<std::uint8_t>(readData(8)); }
  }

  void soft_spi_t::endRead()
  {
    if (pin_miso_ == pin_mosi_)
    {
      // Input mode leaves the latch high. Clear it before driving MOSI again
      // to avoid a long high pulse on shared data lines such as WS2812 inputs.
      lgfx::gpio_lo(pin_mosi_);
      lgfx::pinMode(pin_mosi_, lgfx::pin_mode_t::output);
    }
  }

  std::uint32_t soft_spi_read32(probe_ctx_t& ctx, int pin_sclk, int pin_mosi, int pin_miso,
                                int pin_dc, int pin_cs, std::uint8_t cmd, std::uint8_t dummy_bits,
                                std::uint32_t half_us, bool legacy_zero_preamble)
  {
    soft_spi_t bus(pin_sclk, pin_mosi, pin_miso, pin_dc, half_us);
    if (legacy_zero_preamble)
    {
      // Legacy _read_panel_id clocked a zero command while CS was high before
      // selecting the panel. ESP32 PICO display probes retain that exact read.
      bus.init();
      bus.beginTransaction();
      lgfx::gpio_hi(pin_cs);
      lgfx::pinMode(pin_cs, lgfx::pin_mode_t::output);
      bus.writeCommand(0, 8);
      bus.wait();
    }
    else
    {
      lgfx::gpio_hi(pin_cs);
      lgfx::pinMode(pin_cs, lgfx::pin_mode_t::output);
      bus.init();
      bus.beginTransaction();
    }
    lgfx::gpio_lo(pin_cs);
    bus.writeCommand(cmd, 8);
    bus.beginRead(dummy_bits);
    const auto value = bus.readData(32);
    lgfx::gpio_hi(pin_cs);
    bus.endTransaction();

    // CS is owned by the caller. Restore only the shared bus signals that this
    // helper borrowed, using their state at transaction start.
    ctx.transaction->restore_start({ pin_sclk, pin_mosi, pin_miso, pin_dc });
    return value;
  }

  namespace startup_detail
  {
    class retry_budget_t
    {
    public:
      explicit retry_budget_t(std::uint8_t milliseconds)
      : started_(lgfx::millis()), milliseconds_(milliseconds) {}

      bool exhausted() const { return lgfx::millis() - started_ >= milliseconds_; }
      std::uint32_t deadline_ms() const { return started_ + milliseconds_; }

    private:
      std::uint32_t started_;
      std::uint8_t milliseconds_;
    };

    void pin_level(int pin, bool high)
    {
      if (high) { lgfx::gpio_hi(pin); }
      else      { lgfx::gpio_lo(pin); }
      lgfx::pinMode(pin, lgfx::pin_mode_t::output);
    }

    void hold_pin_level(int pin, bool high)
    {
      const auto bit = std::uint32_t(1) << (pin & 31);
#if !defined (SOC_GPIO_PIN_COUNT) || SOC_GPIO_PIN_COUNT > 32
      const bool output = pin & 32 ? REG_READ(GPIO_ENABLE1_REG) & bit : REG_READ(GPIO_ENABLE_REG) & bit;
      const bool level = pin & 32 ? REG_READ(GPIO_OUT1_REG) & bit : REG_READ(GPIO_OUT_REG) & bit;
#else
      const bool output = REG_READ(GPIO_ENABLE_REG) & bit;
      const bool level = REG_READ(GPIO_OUT_REG) & bit;
#endif
      if (output && level == high) { return; }
      // Set the output latch before enabling the driver, including after M5Unified prehold.
      pin_level(pin, high);
    }

    bool read_register(int port, const power_desc_t& power,
                       std::uint8_t addr, std::uint8_t reg,
                       std::uint8_t* value, retry_budget_t& retry_budget)
    {
      for (;;)
      {
        const auto result = lgfx::i2c::readRegister8(port, addr, reg, power.i2c_freq);
        if (result.has_value())
        {
          *value = result.value();
          return true;
        }
        if (retry_budget.exhausted()) { return false; }
        lgfx::delay(1);
      }
    }

    bool read_register(int port, const power_desc_t& power,
                       std::uint8_t addr, std::uint8_t reg,
                       std::uint8_t* value)
    {
      retry_budget_t retry_budget(power.wake_poll_ms);
      return read_register(port, power, addr, reg, value, retry_budget);
    }

    ops::run_result_t run_sequence(int port, const power_desc_t& power,
                                   ops::op_list_t sequence,
                                   pin_list_t gpio_pins = no_pins(),
                                   const ops::retry_policy_t* policy = nullptr)
    {
      const int ports[] = { port };
      ops::lgfx_backend_context_t backend_context { ports, 1 };
      const ops::gpio_scope_t gpio_scope { GPIO_NUM_MAX, gpio_pins.data, gpio_pins.size };
      return ops::run_ops(ops::lgfx_backend(&backend_context),
                          power.devices, power.device_count,
                          sequence.data, sequence.size, gpio_scope, policy,
                          ops::lgfx_wait_ready_finished);
    }

    const pmic_variant_t* read_variant(const power_desc_t& power, int port,
                                       retry_budget_t& retry_budget)
    {
      if (power.variants == nullptr || power.variant_count == 0
       || power.variant_count > 8) { return nullptr; }
      std::uint8_t last_addr = 0xFF;
      std::uint8_t last_reg = 0xFF;
      std::uint8_t id_value = 0;
      bool id_valid = false;
      for (std::uint_fast8_t i = 0; i < power.variant_count; ++i)
      {
        if (power.variants[i].i2c_addr != last_addr || power.variants[i].id_reg != last_reg)
        {
          last_addr = power.variants[i].i2c_addr;
          last_reg = power.variants[i].id_reg;
          id_valid = read_register(port, power, last_addr, last_reg, &id_value, retry_budget);
        }
        const auto mask = power.variants[i].id_mask;
        if (id_valid && (id_value & mask) == (power.variants[i].id_value & mask))
        {
          return &power.variants[i];
        }
      }
      return nullptr;
    }

    const pmic_variant_t* read_variant(const power_desc_t& power, int port)
    {
      retry_budget_t retry_budget(power.wake_poll_ms);
      return read_variant(power, port, retry_budget);
    }

    void set_sd_spi_mode(const shared_sd_desc_t& sd, const prepare_ctx_t& ctx)
    {
      soft_spi_t bus(sd.sclk, sd.mosi, sd.miso, -1, 2);
      bus.init();
      bus.beginTransaction();
      pin_level(sd.sd_cs, true);

      for (int i = 0; i < 10; ++i) { bus.writeData(0xFF, 8); }
      lgfx::gpio_lo(sd.sd_cs);
      static const std::uint8_t cmd58[] = { 0x7A, 0, 0, 0, 0, 0xFD, 0xFF, 0xFF };
      std::uint8_t response[sizeof(cmd58)];
      for (std::size_t i = 0; i < sizeof(cmd58); ++i)
      {
        response[i] = static_cast<std::uint8_t>(bus.transferData(cmd58[i], 8));
      }
      if (response[6] == response[7])
      {
        lgfx::gpio_hi(sd.sd_cs);
        for (int i = 0; i < 10; ++i) { bus.writeData(0xFF, 8); }
        lgfx::gpio_lo(sd.sd_cs);
        static const std::uint8_t cmd0[] = { 0x40, 0, 0, 0, 0, 0x95, 0xFF, 0xFF };
        for (auto value : cmd0) { bus.transferData(value, 8); }
      }
      lgfx::gpio_hi(sd.sd_cs);
      bus.endTransaction();
      ctx.transaction->restore_start({ sd.sclk, sd.mosi, sd.miso });
    }

    void pin_reset(const reset_desc_t& reset, bool active)
    {
      lgfx::gpio_hi(reset.pin);
      lgfx::pinMode(reset.pin, lgfx::pin_mode_t::output);
      lgfx::delay(1);
      if (!active) { return; }
      lgfx::gpio_lo(reset.pin);
      lgfx::delay(reset.low_ms);
      lgfx::gpio_hi(reset.pin);
      lgfx::delay(reset.post_ms);
    }

    class i2c_scope_t
    {
    public:
      i2c_scope_t(detection_transaction_t& transaction_, int port_, const i2c_desc_t& wiring)
      : transaction(transaction_), port(port_), pins { wiring.sda, wiring.scl }
      {
        opened = lgfx::i2c::init(port, wiring.sda, wiring.scl).has_value();
      }

      ~i2c_scope_t()
      {
        if (opened) { lgfx::i2c::release(port); }
        transaction.restore_start(pins);
      }

      detection_transaction_t& transaction;
      int port;
      std::int8_t pins[2];
      bool opened = false;
    };

    // Unchecked primitives; callers must first pass description_valid().
    bool prepare_power(const board_desc_t& desc, board_result_t& result, int i2c_port,
                       bool retain_confirmed_board = false);
    bool prepare_sd_spi(const board_desc_t& desc, board_result_t& result,
                        const prepare_ctx_t& ctx);
  }

  bool startup_detail::prepare_power(const board_desc_t& desc, board_result_t& result,
                                     int i2c_port, bool retain_confirmed_board)
  {
    if (result.prepared & prepared_power) { return true; }
    const auto& power = desc.power;
    if (power.hold_pin >= 0) { startup_detail::hold_pin_level(power.hold_pin, power.hold_high); }
    if (power.variants == nullptr || power.variant_count == 0)
    {
      result.prepared |= prepared_power;
      return true;
    }

    // Wake-up polling and every PMIC transaction share one retry window so a
    // partially responsive controller cannot multiply the configured delay.
    startup_detail::retry_budget_t retry_budget(power.wake_poll_ms);
    const pmic_variant_t* variant = nullptr;
    if (power.variant_confirmed)
    {
      // The detector has already established the controller identity. Select
      // the branch only from recorded option bits; operation lists themselves
      // remain linear and contain no hardware-dependent conditionals.
      for (std::uint_fast8_t i = 0; i < power.variant_count; ++i)
      {
        const auto& candidate = power.variants[i];
        if ((result.option & candidate.option_select_mask) == candidate.option_select_value)
        {
          if (variant != nullptr) { return false; }
          variant = &candidate;
        }
      }
    }
    else
    {
      variant = startup_detail::read_variant(power, i2c_port, retry_budget);
    }
    if (variant == nullptr) { return false; }
    std::uint8_t power_state = variant->power_state.mask;
    if (variant->power_state.mask
     && !startup_detail::read_register(i2c_port, power, variant->i2c_addr,
                                       variant->power_state.reg, &power_state,
                                       retry_budget)) { return false; }
    std::uint8_t reset_state = power_state;
    if (variant->reset_state.mask && variant->reset_state.reg != variant->power_state.reg
     && !startup_detail::read_register(i2c_port, power, variant->i2c_addr,
                                       variant->reset_state.reg, &reset_state,
                                       retry_budget)) { return false; }
    const bool power_was_off = !(power_state & variant->power_state.mask);
    const bool reset_was_low = !(reset_state & variant->reset_state.mask);
    const ops::retry_policy_t retry_policy { retry_budget.deadline_ms(), 1 };
    const auto power_result = startup_detail::run_sequence(i2c_port, power,
                                                            variant->power_on,
                                                            desc.op_gpio_pins,
                                                            &retry_policy);
    if (power_result.status != ops::op_status_t::ok)
    {
      if (!retain_confirmed_board) { return false; }
      // This flag is set-only and has the same lifetime as prepared_power.
      result.prepared |= prepared_power_failed;
      // Detection has already established the board identity. Power/IOE writes
      // are post-identification setup, so a partial hardware failure must not
      // turn the confirmed board into a different model; stop the list, warn,
      // and let construction report whatever hardware remains usable.
      ESP_LOGW("board_detect",
               "power_on stopped after board confirmation: op=%u status=%u native=%d",
               static_cast<unsigned>(power_result.failed_index),
               static_cast<unsigned>(power_result.status), 0);
    }
    if (power_was_off) { result.prepared &= ~prepared_sd_spi; }
    lgfx::delay(power_was_off || reset_was_low ? power.cold_wait_ms : power.warm_wait_ms);
    result.prepared |= prepared_power;
    return true;
  }

  bool startup_detail::prepare_sd_spi(const board_desc_t& desc, board_result_t& result,
                                      const prepare_ctx_t& ctx)
  {
    if (result.prepared & prepared_sd_spi) { return true; }
    const auto& sd = desc.sd;
    if (sd.sd_cs < 0) { return true; }
    // Keep the other CS high; the detection transaction restores it on failure.
    startup_detail::pin_level(sd.other_cs, true);
    startup_detail::set_sd_spi_mode(sd, ctx);
    result.prepared |= prepared_sd_spi;
    return true;
  }

  namespace startup_detail
  {
    template <typename T>
    bool list_valid(const list_desc_t<T>& list)
    {
      return list.data != nullptr || list.size == 0;
    }

    bool list_valid(ops::op_list_t list)
    {
      return list.data != nullptr || list.size == 0;
    }

    bool list_contains(pin_list_t list, int pin)
    {
      for (std::size_t i = 0; i < list.size; ++i)
      {
        if (list.data[i] == pin) { return true; }
      }
      return false;
    }

    bool gpio_valid(int pin)
    {
      return pin >= 0 && pin < GPIO_NUM_MAX && GPIO_IS_VALID_GPIO(pin);
    }

    bool optional_gpio_valid(int pin) { return pin == -1 || gpio_valid(pin); }

    bool pin_list_valid(pin_list_t list)
    {
      if (!list_valid(list)) { return false; }
      for (std::size_t i = 0; i < list.size; ++i)
      {
        if (!gpio_valid(list.data[i])) { return false; }
      }
      return true;
    }

    bool variant_device_valid(const power_desc_t& power, const pmic_variant_t& variant)
    {
      for (std::size_t i = 0; i < power.device_count; ++i)
      {
        const auto& device = power.devices[i];
        if (device.bus_id == 0 && device.addr == variant.i2c_addr)
        {
          return device.freq_hz == power.i2c_freq;
        }
      }
      return false;
    }

    // Checked before any pin or register is touched, on every public entry
    // that acts on a description. Lists are expected to come from ops::list(),
    // registers(), pins() and options(); a hand-written size cannot be checked
    // against its array, but a missing array with a non-zero size is rejected here.
    bool description_valid(const board_desc_t& desc)
    {
      const bool has_variants = desc.power.variants != nullptr;
      bool valid = has_variants == (desc.power.variant_count != 0)
                && desc.power.variant_count <= 8
                && (!has_variants
                 || (desc.power.devices != nullptr && desc.power.device_count != 0))
                && !(has_variants && desc.power.hold_pin >= 0)
                && !(desc.reset.kind == reset_kind_t::i2c_regs && !has_variants)
                // A confirmed multi-variant family cannot safely choose an I2C
                // reset list without repeating the variant-identification read.
                && !(desc.power.variant_confirmed && desc.power.variant_count > 1
                  && desc.reset.kind == reset_kind_t::i2c_regs)
                && !((has_variants || desc.reset.kind == reset_kind_t::i2c_regs)
                     && (desc.internal_i2c.sda < 0 || desc.internal_i2c.scl < 0))
                && !(desc.reset.kind == reset_kind_t::custom && desc.reset.custom == nullptr)
                && pin_list_valid(desc.hold_high_pins)
                && list_valid(desc.option_names)
                && pin_list_valid(desc.op_gpio_pins);
      // no_display_pins(): every display pin is absent and nothing below may
      // refer to one. Any other description needs the SPI display signals and
      // holds its chip select high during detection.
      const bool display_absent = desc.display.sclk < 0 && desc.display.mosi < 0
                               && desc.display.miso < 0 && desc.display.dc < 0
                               && desc.display.cs < 0 && desc.display.rst < 0
                               && desc.display.busy < 0;
      valid = valid
           && (display_absent
            || (desc.display.sclk >= 0 && desc.display.mosi >= 0 && desc.display.cs >= 0
             && list_contains(desc.hold_high_pins, desc.display.cs)))
           && !(display_absent && desc.sd.sd_cs >= 0);
      const std::int8_t described_pins[] = {
        desc.power.hold_pin, desc.reset.pin,
        desc.sd.sclk, desc.sd.mosi, desc.sd.miso, desc.sd.sd_cs, desc.sd.other_cs,
        desc.display.sclk, desc.display.mosi, desc.display.miso, desc.display.dc,
        desc.display.cs, desc.display.rst, desc.display.busy,
        desc.internal_i2c.sda, desc.internal_i2c.scl,
      };
      for (auto pin : described_pins) { valid = valid && optional_gpio_valid(pin); }
      // Shared display/SD wires and exposed reset pins describe the same physical pins.
      // Both chip selects must be held high while probing their shared bus.
      if (valid && desc.sd.sd_cs >= 0)
      {
        valid = desc.sd.sclk >= 0 && desc.sd.mosi >= 0 && desc.sd.miso >= 0
             && desc.sd.other_cs >= 0
             && desc.sd.sclk == desc.display.sclk
             && desc.sd.mosi == desc.display.mosi
             && desc.sd.miso == desc.display.miso
             && desc.sd.other_cs == desc.display.cs
             && list_contains(desc.hold_high_pins, desc.sd.sd_cs);
      }
      if (valid && (desc.reset.kind == reset_kind_t::gpio
                 || desc.reset.kind == reset_kind_t::custom))
      {
        valid = desc.reset.pin >= 0 && desc.reset.pin == desc.display.rst;
      }
      for (std::uint_fast8_t i = 0; valid && i < desc.power.variant_count; ++i)
      {
        const auto& variant = desc.power.variants[i];
        const ops::gpio_scope_t gpio_scope {
          GPIO_NUM_MAX, desc.op_gpio_pins.data, desc.op_gpio_pins.size
        };
        valid = list_valid(variant.power_on)
             && list_valid(variant.reset_assert)
             && list_valid(variant.reset_release)
             && list_valid(variant.restore_registers)
             && variant_device_valid(desc.power, variant)
             && variant.restore_registers.size <= max_pmic_restore_registers
             && (variant.option_select_value & ~variant.option_select_mask) == 0
             && (desc.reset.kind != reset_kind_t::i2c_regs
                 || (variant.reset_assert.size != 0 && variant.reset_release.size != 0))
             // GPIO operations are only safe on pins captured by the detection
             // transaction and passed as the execution scope.
             && ops::validate_ops(desc.power.devices, desc.power.device_count,
                                  variant.power_on.data, variant.power_on.size,
                                  gpio_scope).status == ops::op_status_t::ok
             && ops::validate_ops(desc.power.devices, desc.power.device_count,
                                  variant.reset_assert.data, variant.reset_assert.size,
                                  gpio_scope).status == ops::op_status_t::ok
             && ops::validate_ops(desc.power.devices, desc.power.device_count,
                                  variant.reset_release.data, variant.reset_release.size,
                                  gpio_scope).status == ops::op_status_t::ok;
      }
      if (!valid)
      {
        ESP_LOGW(tag, "invalid board description id=%u", static_cast<unsigned>(desc.def.id));
      }
      return valid;
    }

    void hold_chip_selects(const board_desc_t& desc)
    {
      for (std::size_t i = 0; i < desc.hold_high_pins.size; ++i)
      {
        pin_level(desc.hold_high_pins.data[i], true);
      }
    }
  }

  bool prepare_reset(const board_desc_t& desc, board_result_t& result,
                     const prepare_ctx_t& ctx, int i2c_port,
                     std::uint32_t* detected_option, bool retain_confirmed_board)
  {
    const auto& reset = desc.reset;
    if (!startup_detail::description_valid(desc)) { return false; }
    if (result.prepared & prepared_reset) { return true; }
    if (!(reset.flags & reset_always) && !ctx.allow_reset)
    {
      if ((reset.flags & reset_hold_when_skipped)
       && !(result.prepared & prepared_reset_line))
      {
        if (reset.kind == reset_kind_t::i2c_regs)
        {
          // Some legacy boards always released an externally held reset and waited for
          // stabilization even when the caller skipped the assert pulse.  Extend the
          // GPIO flag's "leave released" meaning to typed I2C reset sequences as well.
          const auto* variant = desc.power.variant_confirmed && desc.power.variant_count == 1
                              ? &desc.power.variants[0]
                              : startup_detail::read_variant(desc.power, i2c_port);
          if (variant == nullptr) { return false; }
          startup_detail::retry_budget_t release_budget(desc.power.wake_poll_ms);
          const ops::retry_policy_t release_policy { release_budget.deadline_ms(), 1 };
          const auto release_result = startup_detail::run_sequence(
            i2c_port, desc.power, variant->reset_release, desc.op_gpio_pins,
            &release_policy);
          if (release_result.status != ops::op_status_t::ok)
          {
            if (!retain_confirmed_board) { return false; }
            // Detection has already established the board identity.  Legacy
            // setup ignored reset-register write failures, so warn with the
            // failed operation and keep the confirmed model instead of probing
            // another board against partially configured hardware.
            ESP_LOGW("board_detect",
                     "reset_release stopped after board confirmation: op=%u status=%u native=%d",
                     static_cast<unsigned>(release_result.failed_index),
                     static_cast<unsigned>(release_result.status),
                     0);
          }
          lgfx::delay(reset.post_ms);
        }
        else
        {
          startup_detail::pin_reset(reset, false);
        }
        result.prepared |= prepared_reset_line;
      }
      return true;
    }

    bool ok = true;
    if (reset.kind == reset_kind_t::i2c_regs)
    {
      // Confirmed single-variant families already performed their full ID read;
      // do not turn a later setup read fault into loss of the detected model.
      const auto* variant = desc.power.variant_confirmed && desc.power.variant_count == 1
                          ? &desc.power.variants[0]
                          : startup_detail::read_variant(desc.power, i2c_port);
      if (variant == nullptr) { return false; }
      // The legacy path gave assert and release independent wake-poll budgets;
      // preserve that retry meaning when executing their typed operation lists.
      startup_detail::retry_budget_t assert_budget(desc.power.wake_poll_ms);
      const ops::retry_policy_t assert_policy { assert_budget.deadline_ms(), 1 };
      const auto assert_result = startup_detail::run_sequence(
        i2c_port, desc.power, variant->reset_assert, desc.op_gpio_pins,
        &assert_policy);
      ok = assert_result.status == ops::op_status_t::ok;
      if (!ok && retain_confirmed_board)
      {
        ESP_LOGW("board_detect",
                 "reset_assert stopped after board confirmation: op=%u status=%u native=%d",
                 static_cast<unsigned>(assert_result.failed_index),
                 static_cast<unsigned>(assert_result.status), 0);
      }
      if (ok || retain_confirmed_board)
      {
        lgfx::delay(reset.low_ms);
        startup_detail::retry_budget_t release_budget(desc.power.wake_poll_ms);
        const ops::retry_policy_t release_policy { release_budget.deadline_ms(), 1 };
        const auto release_result = startup_detail::run_sequence(
          i2c_port, desc.power, variant->reset_release, desc.op_gpio_pins,
          &release_policy);
        ok = release_result.status == ops::op_status_t::ok;
        if (!ok && retain_confirmed_board)
        {
          ESP_LOGW("board_detect",
                   "reset_release stopped after board confirmation: op=%u status=%u native=%d",
                   static_cast<unsigned>(release_result.failed_index),
                   static_cast<unsigned>(release_result.status), 0);
        }
        if (ok || retain_confirmed_board) { lgfx::delay(reset.post_ms); }
      }
      if (retain_confirmed_board) { ok = true; }
    }
    else if (reset.kind == reset_kind_t::custom)
    {
      ok = reset.custom(desc, ctx, detected_option);
    }
    else if (reset.kind == reset_kind_t::gpio)
    {
      startup_detail::pin_reset(reset, true);
      result.prepared |= prepared_reset_line;
    }
    if (ok) { result.prepared |= prepared_reset; }
    return ok;
  }

  bool observe_spi_variant(probe_ctx_t& ctx, const board_desc_t& desc,
                           const spi_id_probe_t* probes, std::size_t probe_count,
                           std::uint32_t* option, bool three_wire,
                           std::uint8_t slow_retry_half_us, bool legacy_zero_preamble)
  {
    const int read_pin = three_wire || desc.display.miso < 0
                       ? desc.display.mosi : desc.display.miso;
    std::uint8_t last_cmd = 0;
    std::uint8_t last_dummy_bits = 0;
    std::uint32_t id = 0;
    bool have_id = false;
    for (std::size_t index = 0; index < probe_count; ++index)
    {
      const auto& probe = probes[index];
      if (!have_id || probe.cmd != last_cmd || probe.dummy_bits != last_dummy_bits)
      {
        id = soft_spi_read32(ctx, desc.display.sclk, desc.display.mosi, read_pin,
                             desc.display.dc, desc.display.cs, probe.cmd, probe.dummy_bits,
                             1, legacy_zero_preamble);
        last_cmd = probe.cmd;
        last_dummy_bits = probe.dummy_bits;
        have_id = true;
      }
      for (std::size_t value = 0; value < probe.value_count; ++value)
      {
        if ((id & probe.mask) != probe.values[value]) { continue; }
        *option = probe.option_bit;
        return true;
      }
    }
    if (slow_retry_half_us > 0)
    {
      // AtomS3R has shipped with GC9107 batches that answer only at a slow
      // clock. Its alternatives share one command, so re-read that command
      // exactly once and compare the result against every matching probe.
      const auto retry_cmd = probes[0].cmd;
      id = soft_spi_read32(ctx, desc.display.sclk, desc.display.mosi, read_pin,
                           desc.display.dc, desc.display.cs, retry_cmd,
                           probes[0].dummy_bits,
                           slow_retry_half_us, legacy_zero_preamble);
      for (std::size_t index = 0; index < probe_count; ++index)
      {
        const auto& probe = probes[index];
        if (probe.cmd != retry_cmd) { continue; }
        for (std::size_t value = 0; value < probe.value_count; ++value)
        {
          if ((id & probe.mask) != probe.values[value]) { continue; }
          *option = probe.option_bit;
          return true;
        }
      }
    }
    return false;
  }

  bool probe_spi_id(probe_ctx_t& ctx, const board_desc_t& desc,
                    const spi_id_probe_t* probes, std::size_t probe_count,
                    board_result_t* result, bool three_wire,
                    std::uint8_t slow_retry_half_us,
                    bool legacy_zero_preamble)
  {
    if (probes == nullptr || probe_count == 0 || result == nullptr
     || !startup_detail::description_valid(desc)
     || desc.display.cs < 0) { return false; }  // no_display_pins() has no SPI ID
    const std::int8_t pins[] = {
      desc.display.cs, desc.display.sclk, desc.display.mosi,
      desc.display.dc, desc.display.rst,
    };
    const std::int8_t shared_sd_pins[] = {
      desc.display.cs, desc.display.sclk, desc.display.mosi,
      desc.display.dc, desc.display.rst,
      desc.sd.sclk, desc.sd.mosi, desc.sd.miso, desc.sd.sd_cs,
    };
    const auto restore_probe_pins = [&]()
    {
      if (desc.sd.sd_cs >= 0) { ctx.transaction->restore_start(shared_sd_pins); }
      else                    { ctx.transaction->restore_start(pins); }
    };
    board_result_t candidate;
    candidate.assign(&desc);
    const prepare_ctx_t& prepare_ctx = ctx;
    if (desc.sd.sd_cs >= 0)
    {
      // Keep both devices deselected while the shared SD bus is switched to
      // SPI mode. Descriptions without shared SD take the original path.
      startup_detail::pin_level(desc.sd.sd_cs, true);
      startup_detail::pin_level(desc.display.cs, true);
      if (!startup_detail::prepare_sd_spi(desc, candidate, prepare_ctx))
      {
        restore_probe_pins();
        return false;
      }
    }
    if (!prepare_reset(desc, candidate, prepare_ctx, ctx.i2c_port_probe))
    {
      restore_probe_pins();
      return false;
    }
    if (observe_spi_variant(ctx, desc, probes, probe_count, &candidate.option,
                            three_wire, slow_retry_half_us, legacy_zero_preamble))
    {
      *result = candidate;
      return true;
    }
    restore_probe_pins();
    return false;
  }

  struct spi_id_member_t
  {
    const board_desc_t* desc;
    const spi_id_probe_t* probes;
    std::uint8_t probe_count;
    bool three_wire;
    bool touches_conditional_pins;
    std::uint8_t slow_retry_half_us;
    bool legacy_zero_preamble;
  };

  bool fixed_start_spi_variant(board_result_t& result, const prepare_ctx_t& ctx,
                               const spi_id_member_t& member)
  {
    if (!prepare(*result.desc, result, ctx)) { return false; }
    probe_ctx_t probe;
    static_cast<prepare_ctx_t&>(probe) = ctx;
    if (!observe_spi_variant(probe, *result.desc, member.probes, member.probe_count,
                             &result.option, member.three_wire,
                             member.slow_retry_half_us, member.legacy_zero_preamble))
    {
      ESP_LOGW("M5GFX", "Fixed board:%u panel variant unreadable; using default",
               static_cast<unsigned>(result.def->id));
    }
    const auto& display = result.desc->display;
    const std::int8_t signals[] = { display.dc, display.sclk, display.mosi, display.miso };
    ctx.transaction->restore_start(signals);
    return true;
  }

  class spi_id_detector_t final : public board_detector_t
  {
  public:
    spi_id_detector_t(const board_def_t* const* board_members, const spi_id_member_t* members_desc,
                      std::uint8_t member_count, bool shared_id_read = false,
                      bool (*signature_probe)(probe_ctx_t&) = nullptr)
    : board_detector_t(board_members),
      members_desc_(members_desc), member_count_(member_count),
      shared_id_read_(shared_id_read), signature_probe_(signature_probe) {}
    bool signature(probe_ctx_t& ctx) const override
    { return signature_probe_ == nullptr || signature_probe_(ctx); }
    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (shared_id_read_) { return probe_family(ctx, result); }
      if (ctx.attempt == 0 && !ctx.final_attempt && ctx.hint != board_id_unknown)
      {
        for (std::uint8_t index = 0; index < member_count_; ++index)
        {
          const auto& member = members_desc_[index];
          if (member.desc->def.id != ctx.hint) { continue; }
          if (probe_member(ctx, member, result)) { return true; }
          return false;
        }
      }
      for (std::uint8_t index = 0; index < member_count_; ++index)
      {
        const auto& member = members_desc_[index];
        if (probe_member(ctx, member, result)) { return true; }
      }
      return false;
    }

  private:
    bool probe_family(probe_ctx_t& ctx, board_result_t* result) const
    {
      if (member_count_ == 0 || result == nullptr) { return false; }
      const auto& first = members_desc_[0];
      if (first.desc == nullptr || first.probes == nullptr || first.probe_count == 0
       || !startup_detail::description_valid(*first.desc)
       || first.desc->display.cs < 0) { return false; }

      const auto& desc = *first.desc;
      const std::int8_t pins[] = {
        desc.display.cs, desc.display.sclk, desc.display.mosi,
        desc.display.dc, desc.display.rst,
      };
      board_result_t candidate;
      candidate.assign(&desc);
      const prepare_ctx_t& prepare_ctx = ctx;
      if (!prepare_reset(desc, candidate, prepare_ctx, ctx.i2c_port_probe))
      {
        ctx.transaction->restore_start(pins);
        return false;
      }

      const auto& read_probe = first.probes[0];
      const int read_pin = first.three_wire || desc.display.miso < 0
                         ? desc.display.mosi : desc.display.miso;
      const std::uint32_t panel_id = soft_spi_read32(
        ctx, desc.display.sclk, desc.display.mosi, read_pin,
        desc.display.dc, desc.display.cs, read_probe.cmd, read_probe.dummy_bits,
        1, first.legacy_zero_preamble);

      // Members of a legacy family share the physical read. Compare the one
      // captured value in legacy priority order instead of resetting the same
      // panel again for each possible member.
      for (std::uint8_t member_index = 0; member_index < member_count_; ++member_index)
      {
        const auto& member = members_desc_[member_index];
        for (std::uint8_t probe_index = 0; probe_index < member.probe_count; ++probe_index)
        {
          const auto& probe = member.probes[probe_index];
          if (probe.cmd != read_probe.cmd || probe.dummy_bits != read_probe.dummy_bits)
          {
            ctx.transaction->restore_start(pins);
            return false;
          }
          for (std::size_t value = 0; value < probe.value_count; ++value)
          {
            if ((panel_id & probe.mask) != probe.values[value]) { continue; }
            candidate.assign(member.desc);
            candidate.option = probe.option_bit;
            *result = candidate;
            return true;
          }
        }
      }
      ctx.transaction->restore_start(pins);
      return false;
    }

    static bool probe_member(probe_ctx_t& ctx, const spi_id_member_t& member,
                             board_result_t* result)
    {
      if (ctx.conditional_pins_unavailable && member.touches_conditional_pins)
      {
        // OPI PSRAM owns GPIO33..37. Skip only the candidate that touches
        // those pins; another member of the same family may remain safe.
        return false;
      }
      return probe_spi_id(ctx, *member.desc, member.probes, member.probe_count, result,
                          member.three_wire, member.slow_retry_half_us,
                          member.legacy_zero_preamble);
    }

    const spi_id_member_t* members_desc_;
    std::uint8_t member_count_;
    bool shared_id_read_;
    bool (*signature_probe_)(probe_ctx_t&);
  };

  bool prepare(const board_desc_t& desc, board_result_t& result, const prepare_ctx_t& ctx)
  {
    if (ctx.transaction == nullptr)
    {
      ESP_LOGE("board_detect", "prepare requires a detection transaction");
      return false;
    }
    if (result.desc != &desc || result.def != &desc.def || result.def->id == board_id_unknown)
    {
      return false;
    }
    if (!startup_detail::description_valid(desc)) { return false; }
    const board_desc_t* current = &desc;
    if (!(result.prepared & prepared_power))
    {
      if (desc.power.variants != nullptr)
      {
        startup_detail::i2c_scope_t i2c(*ctx.transaction, ctx.i2c_port_probe, desc.internal_i2c);
        if (!i2c.opened
         || !startup_detail::prepare_power(desc, result, i2c.port, true)) { return false; }
      }
      else if (!startup_detail::prepare_power(desc, result, ctx.i2c_port_probe, true)) { return false; }
    }
    if (!(result.prepared & prepared_refine) && result.refine != nullptr)
    {
      if (result.prepared & prepared_power_failed)
      {
        // Pending refinement means the member is not yet confirmed. Give a
        // failed power sequence another attempt before retaining it provisionally.
        if (!ctx.final_attempt) { return false; }
        result.provisional = true;
        ESP_LOGW("board_detect",
                 "member refinement skipped after retained power_on failure; board=%u",
                 static_cast<unsigned>(result.def->id));
      }
      else
      {
        if (!result.refine(result, ctx)) { return false; }
        if (result.desc == nullptr || result.def != &result.desc->def
         || result.def->id == board_id_unknown
         || !startup_detail::description_valid(*result.desc)) { return false; }
        current = result.desc;
      }
      result.prepared |= prepared_refine;
    }
    else if (result.desc != nullptr)
    {
      current = result.desc;
    }
    if (!startup_detail::prepare_sd_spi(*current, result, ctx)) { return false; }
    const bool reset_was_prepared = result.prepared & prepared_reset;
    if (!reset_was_prepared)
    {
      if (current->reset.kind == reset_kind_t::i2c_regs)
      {
        startup_detail::i2c_scope_t i2c(*ctx.transaction, ctx.i2c_port_probe, current->internal_i2c);
        if (!i2c.opened || !prepare_reset(*current, result, ctx, i2c.port, nullptr, true))
        {
          return false;
        }
      }
      else if (!prepare_reset(*current, result, ctx, ctx.i2c_port_probe, nullptr, true))
      {
        return false;
      }
    }
    startup_detail::hold_chip_selects(*current);
    // The internal port is taken over here and handed to later users. Opening it
    // on other pins before autodetect is a misuse; it is reported, not restored.
    if (current->internal_i2c.hw_port >= 0
     && lgfx::i2c::isInitialized(current->internal_i2c.hw_port))
    {
      const auto sda = lgfx::i2c::getPinSDA(current->internal_i2c.hw_port);
      const auto scl = lgfx::i2c::getPinSCL(current->internal_i2c.hw_port);
      if (sda.has_value() && scl.has_value()
       && (sda.value() != current->internal_i2c.sda || scl.value() != current->internal_i2c.scl))
      {
        ESP_LOGW("board_detect", "I2C%d was open on SDA=%d SCL=%d; moving it to SDA=%d SCL=%d",
                 current->internal_i2c.hw_port, sda.value(), scl.value(),
                 current->internal_i2c.sda, current->internal_i2c.scl);
      }
    }
    if (current->internal_i2c.hw_port >= 0
     && !lgfx::i2c::init(current->internal_i2c.hw_port, current->internal_i2c.sda,
                         current->internal_i2c.scl).has_value())
    {
      ESP_LOGW("board_detect", "I2C%d could not be opened for SDA=%d SCL=%d",
               current->internal_i2c.hw_port, current->internal_i2c.sda, current->internal_i2c.scl);
    }
    return true;
  }
}
}
