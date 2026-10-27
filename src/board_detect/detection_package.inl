#if defined (CONFIG_IDF_TARGET_ESP32S3)
  static bool conditional_detection_pins_unavailable()
  {
#if defined (CONFIG_SPIRAM_MODE_OCT)
    return board_detect::io::psram_ready();
#else
    return false;
#endif
  }
#endif

  struct detection_package_t
  {
    const board_detect::board_detector_t* const* detectors = nullptr;
    bool conditional_pins_unavailable = false;
    bool direct_setup = false;
  };

  __attribute__((always_inline)) static inline detection_package_t select_detection_package()
  {
    detection_package_t package;
    // Selection does not probe or configure any GPIO.
#if !defined(CONFIG_IDF_TARGET) || defined(CONFIG_IDF_TARGET_ESP32)
    package.direct_setup = true;
    const auto pkg = board_detect::io::package_version();
    if (pkg == board_detect::io::package_classic_q6)
    { package.detectors = board_detect::m5::esp32_d0wdq6_detectors; }
    else if (pkg == board_detect::io::package_classic_pico_d4)
    { package.detectors = board_detect::m5::esp32_pico_d4_detectors; }
    else if (pkg == 6)
    { package.detectors = board_detect::m5::esp32_picov3_detectors; }
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    package.conditional_pins_unavailable = conditional_detection_pins_unavailable();
    switch (board_detect::io::package_version())
    {
    case 0: package.detectors = board_detect::m5::esp32s3_detectors_qfn56; break;
    case 1: package.detectors = board_detect::m5::esp32s3_detectors_lga56; break;
    default: break;
    }
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
    package.detectors = board_detect::m5::esp32c3_detectors;
#elif defined(CONFIG_IDF_TARGET_ESP32C5)
    package.detectors = board_detect::m5::esp32c5_detectors;
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
    const auto pkg = board_detect::io::package_version();
    ESP_LOGD(LIBRARY_NAME, "pkg_ver : %02x", (int)pkg);
    // Package version: 1 = QFN32 (ESP32-C6FH4/FH8), 0 = QFN40.
    if (pkg == 1) { package.detectors = board_detect::m5::esp32c6_detectors_qfn32; }
    else if (pkg == 0)
    {
      package.detectors = board_detect::m5::esp32c6_detectors_qfn40;
    }
#elif defined(CONFIG_IDF_TARGET_ESP32C61)
    package.detectors = board_detect::m5::esp32c61_detectors;
#elif defined(CONFIG_IDF_TARGET_ESP32H2)
    package.detectors = board_detect::m5::esp32h2_detectors;
#elif defined(CONFIG_IDF_TARGET_ESP32P4)
    const auto pkg = board_detect::io::package_version();
    ESP_LOGD(LIBRARY_NAME, "pkg_ver : %02x", (int)pkg);
    if (pkg == 0) { package.detectors = board_detect::m5::esp32p4_detectors; }
#endif
    return package;
  }

  static const board_detect::m5::board_entry_t* select_fixed_board(board_t board)
  {
    using namespace board_detect::m5;
#if !defined(CONFIG_IDF_TARGET) || defined(CONFIG_IDF_TARGET_ESP32)
    const auto& boards = esp32_d0wdq6_boards;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    const auto& boards = esp32s3_boards;
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
    const auto& boards = esp32c3_boards;
#elif defined(CONFIG_IDF_TARGET_ESP32C5)
    const auto& boards = esp32c5_boards;
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
    const auto& boards = esp32c6_boards;
#elif defined(CONFIG_IDF_TARGET_ESP32C61)
    const auto& boards = esp32c61_boards;
#elif defined(CONFIG_IDF_TARGET_ESP32H2)
    const auto& boards = esp32h2_boards;
#elif defined(CONFIG_IDF_TARGET_ESP32P4)
    const auto& boards = esp32p4_boards;
#endif
    const auto id = static_cast<board_detect::board_id_t>(board);
    return static_cast<std::uint32_t>(board) == id ? find_board(boards, id) : nullptr;
  }

  static bool reject_detected_setup(board_t setup_board)
  {
#if defined (M5GFX_AUTODETECT_TEST_FAIL_SETUP_BOARDS)
    static constexpr board_t rejected[] = { M5GFX_AUTODETECT_TEST_FAIL_SETUP_BOARDS };
    for (const auto board : rejected)
    {
      if (setup_board == board) { return true; }
    }
#else
    (void)setup_board;
#endif
    return false;
  }
