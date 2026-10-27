  // run_detection_attempt: detection_transaction_t -> detect_board -> prepare
  // -> finalize_prepared_result -> construct_and_adopt.
  // detection_gpio_snapshot_t::capture reclaims GPIO before probing.
  // detect_board owns family order/stops; run_detection_session owns retries/candidates.
  // finalize_prepared_result classifies identity; construct_and_adopt sets adopted.
  // should_persist_detection gates the NVS write in M5GFX.cpp.
  template <class Adopt>
  static board_detect::detect_outcome_t construct_and_adopt(
    board_detect::board_result_t& result, board_detect::detect_outcome_t outcome,
    board_detect::detection_transaction_t& transaction,
    board_t representative_board, Adopt adopt)
  {
    board_detect::m5::display_parts_t parts;
    const auto construct_result = board_detect::m5::construct_detected_board(result, &parts);
    if (construct_result == board_detect::m5::construct_status_t::failed)
    {
      ESP_LOGW(LIBRARY_NAME, "[Autodetect] setup failed for detected board:%u",
               static_cast<unsigned>(result.def->id));
      outcome.result = result;
      outcome.reason = board_detect::fail_reason_t::construct_failed;
      transaction.rollback();
      return outcome;
    }
    if (construct_result == board_detect::m5::construct_status_t::no_display)
    {
      ESP_LOGI(LIBRARY_NAME, "[Autodetect] display is unavailable for detected board:%u",
               static_cast<unsigned>(result.def->id));
      transaction.restore_to_snapshot(result.desc->hold_high_pins.data,
                                result.desc->hold_high_pins.size);
    }
    if (!adopt(parts, representative_board))
    {
      // A rejecting adopter must not retain any of these raw pointers. Release
      // the bus before rollback restores its GPIO routing, then destroy every
      // constructed part so repeated failure injection cannot leak them.
      board_detect::m5::destroy_display_parts(&parts);
      ESP_LOGW(LIBRARY_NAME, "[Autodetect] setup failed for detected board:%u",
               static_cast<unsigned>(result.def->id));
      outcome.result = result;
      outcome.reason = board_detect::fail_reason_t::adopt_failed;
      transaction.rollback();
      return outcome;
    }
    transaction.commit();
    outcome.result = result;
    outcome.adopted = true;
    const auto log = board_detect::m5::success_log(result);
    if (log.name != nullptr && log.name[0] != '\0')
    { ESP_LOGI(LIBRARY_NAME, "[Autodetect] %s%s", log.name, log.annotation); }
    return outcome;
  }

  template <class Adopt>
  static board_detect::detect_outcome_t run_detection_attempt(
    const board_detect::board_detector_t* const* detectors,
    const board_detect::detect_request_t& request, bool final_attempt,
    Adopt adopt)
  {
    board_detect::detect_outcome_t outcome;
    if (detectors == nullptr) { return outcome; }
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32) || defined (CONFIG_IDF_TARGET_ESP32C3) || defined (CONFIG_IDF_TARGET_ESP32C5) || defined (CONFIG_IDF_TARGET_ESP32C6) || defined (CONFIG_IDF_TARGET_ESP32C61) || defined (CONFIG_IDF_TARGET_ESP32P4) || defined (CONFIG_IDF_TARGET_ESP32H2)
    board_detect::detection_transaction_t transaction(
      board_detect::pins(board_detect::m5::wiring::detection::unconditional_pins),
      board_detect::no_pins(), false);
#else
    const bool conditional_pins_unavailable = request.conditional_pins_unavailable;
    board_detect::detection_transaction_t transaction(
      board_detect::pins(board_detect::m5::wiring::detection::unconditional_pins),
      board_detect::pins(board_detect::m5::wiring::detection::opi_pins),
      !conditional_pins_unavailable);
#endif
    if (!transaction.valid())
    {
      ESP_LOGW(LIBRARY_NAME, "[Autodetect] detection transaction could not capture GPIO state");
      return outcome;
    }
    board_detect::probe_ctx_t probe;
    probe.allow_reset = request.allow_reset;
    probe.fallback = request.fallback;
    probe.attempt = request.attempt;
    probe.final_attempt = final_attempt;
    probe.probe_port = detection_i2c_port;
    probe.transaction = &transaction;
    probe.enabled_ids = request.target_set;
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    probe.conditional_pins_unavailable = conditional_pins_unavailable;
#if defined (M5GFX_AUTODETECT_TEST_EXCLUDE_ATOMS3)
    // Test-only failure injection for verifying excluded-result rollback.
    static const board_detect::board_id_t enabled_ids[] = {
      static_cast<board_detect::board_id_t>(board_t::board_M5Dial),
      static_cast<board_detect::board_id_t>(board_t::board_M5DinMeter),
      static_cast<board_detect::board_id_t>(board_t::board_M5StickS3),
      board_detect::board_id_unknown,
    };
    probe.enabled_ids = enabled_ids;
#endif
#endif
    auto result = board_detect::detect_board(
      detectors, request.hint, probe);
    outcome.result = result;
    // No confirm() was entered in this attempt; a weak candidate may still exist.
    if (!probe.confirm_attempted) { outcome.reason = board_detect::fail_reason_t::no_signature; }
    if (result.candidate != nullptr)
    { outcome.verdict = board_detect::verdict_t::candidate; outcome.candidate_kind = board_detect::candidate_kind_t::weak; }
    if (result.status == board_detect::detect_status_t::excluded)
    {
      ESP_LOGW(LIBRARY_NAME, "[Autodetect] detected board:%u is excluded",
               static_cast<unsigned>(result.def->id));
      outcome.reason = board_detect::fail_reason_t::excluded_by_set;
      transaction.rollback();
      return outcome;
    }
    if (result.status != board_detect::detect_status_t::matched)
    {
      transaction.rollback();
      return outcome;
    }
    // Failure injection uses the family representative captured before refine.
    const auto representative_board = static_cast<board_t>(result.def->id);

    const board_detect::prepare_ctx_t& prepare_ctx = probe;
    const bool prepared = result.desc != nullptr
                       && board_detect::prepare(*result.desc, result, prepare_ctx);
    outcome.result = result;
    const bool accepted = board_detect::finalize_prepared_result(outcome, request.fallback);
    if (!prepared)
    {
      // Preserve the matched family's identification; adoption still failed.
      ESP_LOGW(LIBRARY_NAME, "[Autodetect] prepare failed for detected board:%u",
               static_cast<unsigned>(result.def->id));
      outcome.reason = board_detect::fail_reason_t::prepare_failed;
      transaction.rollback();
      return outcome;
    }
    if (!accepted)
    {
      transaction.rollback();
      return outcome;
    }
    return construct_and_adopt(result, outcome, transaction, representative_board, adopt);
  }

  // Fixed startup honors constructor/adopter hooks and observe_* injections (CORES3_FORCE_VBUS), but bypasses detector-only hooks.
  template <class Adopt>
  static board_detect::detect_outcome_t run_fixed_startup(
    const board_detect::m5::board_entry_t& entry, bool allow_reset, Adopt adopt)
  {
    using namespace board_detect;
    detect_outcome_t outcome;
    const auto& desc = *entry.desc;
    if (!prepare_detail::description_valid(desc))
    {
      ESP_LOGW(LIBRARY_NAME, "Fixed board:%u has an invalid startup description", unsigned(desc.def.id));
      return outcome;
    }
    std::int8_t owned[max_detection_pins];
    std::size_t count = 0;
    bool complete = true;
    const auto add_pin = [&](int pin)
    {
      if (pin < 0) { return; }
      for (std::size_t i = 0; i < count; ++i) { if (owned[i] == pin) { return; } }
      if (count == max_detection_pins) { complete = false; return; }
      owned[count++] = static_cast<std::int8_t>(pin);
    };
    add_pin(desc.power.hold_pin); add_pin(desc.reset.pin);
    add_pin(desc.display.sclk); add_pin(desc.display.mosi); add_pin(desc.display.miso);
    add_pin(desc.display.dc); add_pin(desc.display.cs); add_pin(desc.display.rst);
    add_pin(desc.display.busy);
    add_pin(desc.sd.sclk); add_pin(desc.sd.mosi); add_pin(desc.sd.miso);
    add_pin(desc.sd.sd_cs); add_pin(desc.sd.other_cs);
    add_pin(desc.internal_i2c.sda); add_pin(desc.internal_i2c.scl);
    for (std::size_t i = 0; i < desc.hold_high_pins.size; ++i) { add_pin(desc.hold_high_pins.data[i]); }
    for (std::size_t i = 0; i < desc.op_gpio_pins.size; ++i) { add_pin(desc.op_gpio_pins.data[i]); }
    if (!complete)
    {
      ESP_LOGW(LIBRARY_NAME, "Fixed board:%u exceeds the startup pin limit", unsigned(desc.def.id));
      return outcome;
    }
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    if (conditional_detection_pins_unavailable())
    {
      for (std::size_t i = 0; i < count; ++i)
      {
        for (const auto reserved : m5::wiring::detection::opi_pins)
        {
          if (owned[i] == reserved)
          {
            // Acceptance is chip-based; active memory ownership can prevent startup.
            ESP_LOGW(LIBRARY_NAME, "Fixed board:%u needs active OPI PSRAM pin:%d",
                     unsigned(desc.def.id), int(reserved));
            return outcome;
          }
        }
      }
    }
#endif
    detection_transaction_t transaction({ owned, static_cast<std::uint8_t>(count) }, no_pins(), false);
    if (!transaction.valid())
    {
      ESP_LOGW(LIBRARY_NAME, "Fixed board:%u could not capture startup GPIO state", unsigned(desc.def.id));
      return outcome;
    }
    // Fixed startup must not inherit a detected member's state or refinement.
    board_result_t result{};
    result.assign(&desc);
    prepare_ctx_t ctx;
    ctx.allow_reset = allow_reset;
    ctx.probe_port = detection_i2c_port;
    ctx.transaction = &transaction;
    const bool started = entry.fixed_prepare ? entry.fixed_prepare(result, ctx)
                                          : prepare(desc, result, ctx);
    outcome.result = result;
    outcome.verdict = verdict_t::confirmed;
    outcome.attempts = 1;
    if (!started)
    {
      outcome.reason = fail_reason_t::prepare_failed;
      transaction.rollback();
      return outcome;
    }
    return construct_and_adopt(result, outcome, transaction,
                                  static_cast<board_t>(desc.def.id), adopt);
  }

  template <class Adopt>
  static board_detect::detect_outcome_t run_fixed_startup_session(
    const board_detect::m5::board_entry_t& entry, bool allow_reset, Adopt adopt)
  {
    board_detect::detect_request_t request;
    request.allow_reset = allow_reset;
    request.max_attempts = 5;
    // Fixed startup has no no_signature result or candidate selection. The
    // shared session retries every failure. If reset was disabled, the fourth
    // and fifth attempts enable it (attempt indices 3 and 4).
    return board_detect::run_detection_session(request,
      [&entry, &adopt](const board_detect::detect_request_t& attempt, bool)
      {
        return run_fixed_startup(entry, attempt.allow_reset, adopt);
      });
  }
