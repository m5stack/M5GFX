#include "../setup_common.inl"

namespace board_detect
{
namespace m5
{
  // The catalog emitter supports one board-level DSI bus; legacy ST7121 units
  // alone need 900 Mbps after their panel variant is identified at runtime.
  static constexpr std::uint16_t tab5_st7121_lane_mbps = 900;

  static constexpr panel_desc_t panel_tab5_base =
    panel().with_size(specs::tab5::panel_ili9881c::width,
                      specs::tab5::panel_ili9881c::height)
      .with_memory(specs::tab5::panel_ili9881c::memory_width,
                   specs::tab5::panel_ili9881c::memory_height)
      .with_offset(0, 0).with_pins(GPIO_NUM_NC, GPIO_NUM_NC)
      .with_rotation(specs::tab5::panel_ili9881c::rotation_offset)
      .with_readable(specs::tab5::panel_ili9881c::readable)
      .with_bus_shared(false)
      .with_rgb_order(specs::tab5::panel_ili9881c::rgb_order);

  static constexpr dsi_panel_desc_t panel_tab5_ili9881c =
    dsi_panel(panel_tab5_base,
              specs::tab5::panel_ili9881c::dpi_freq_mhz,
              specs::tab5::panel_ili9881c::hsync_back_porch,
              specs::tab5::panel_ili9881c::hsync_pulse_width,
              specs::tab5::panel_ili9881c::hsync_front_porch,
              specs::tab5::panel_ili9881c::vsync_back_porch,
              specs::tab5::panel_ili9881c::vsync_pulse_width,
              specs::tab5::panel_ili9881c::vsync_front_porch);
  static constexpr dsi_panel_desc_t panel_tab5_st7121 =
    dsi_panel(panel_tab5_base,
              specs::tab5::panel_st7121::dpi_freq_mhz,
              specs::tab5::panel_st7121::hsync_back_porch,
              specs::tab5::panel_st7121::hsync_pulse_width,
              specs::tab5::panel_st7121::hsync_front_porch,
              specs::tab5::panel_st7121::vsync_back_porch,
              specs::tab5::panel_st7121::vsync_pulse_width,
              specs::tab5::panel_st7121::vsync_front_porch);
  static constexpr dsi_panel_desc_t panel_tab5_st7123 =
    dsi_panel(panel_tab5_base,
              specs::tab5::panel_st7123::dpi_freq_mhz,
              specs::tab5::panel_st7123::hsync_back_porch,
              specs::tab5::panel_st7123::hsync_pulse_width,
              specs::tab5::panel_st7123::hsync_front_porch,
              // back + pulse must stay 10 or the display shifts vertically.
              specs::tab5::panel_st7123::vsync_back_porch,
              specs::tab5::panel_st7123::vsync_pulse_width,
              // Reducing the front porch causes the touch panel to stop.
              specs::tab5::panel_st7123::vsync_front_porch);

  static constexpr i2c_touch_desc_t touch_tab5 =
    i2c_touch_default_addr()
      .with_port(tab5_detail::i2c_port)
      .with_pins(tab5_detail::sda, tab5_detail::scl, tab5_detail::touch_int)
      .with_freq(specs::tab5::touch::i2c_freq)
      .with_range(specs::tab5::touch::x_min, specs::tab5::touch::x_max,
                  specs::tab5::touch::y_min, specs::tab5::touch::y_max)
      .with_rotation(specs::tab5::touch::rotation_offset);
  static constexpr pwm_light_desc_t light_tab5 =
    pwm_light(specs::tab5::backlight::pin, specs::tab5::backlight::freq,
              specs::tab5::backlight::channel)
      .with_invert(specs::tab5::backlight::invert)
      .with_offset(specs::tab5::backlight::offset);

  construct_status_t construct_tab5(const board_result_t&, display_parts_t* parts)
  {
#if !CONFIG_SPIRAM
    ESP_LOGE(LIBRARY_NAME, "M5Tab5 need PSRAM enabled");
    return construct_status_t::no_display;
#else
#if CONFIG_SPIRAM_SPEED <= 80
    ESP_LOGE(LIBRARY_NAME, "M5Tab5 need PSRAM SPEED 200MHz");
#if defined (ESP_ARDUINO_VERSION)
#if ESP_ARDUINO_VERSION == ESP_ARDUINO_VERSION_VAL(3,3,0)
#warning "The Arduino-ESP32 v3.3.0 has a problem that PSRAM does not work at 200MHz. Please use v3.3.1 or later or v3.2.x."
#endif
#endif
#endif

    bool hit_st7121 = false;
    bool hit_st7123 = false;
    bool read_st_touch_fw = false;
    bool found_gt911 = false;
    int last_logged_fw = -1;
    for (int i = 0; i < 60; ++i)
    {
      std::uint8_t fw_version = 0;
      std::uint8_t fw_reg[2] = { 0, 0 };
      if (lgfx::i2c::transactionWriteRead(tab5_detail::i2c_port,
                                           lgfx::Touch_ST7123::default_addr,
                                           fw_reg, sizeof(fw_reg), &fw_version, 1,
                                           tab5_detail::i2c_freq).has_value())
      {
        read_st_touch_fw = true;
        if (fw_version != last_logged_fw)
        {
          last_logged_fw = fw_version;
          ESP_LOGI(LIBRARY_NAME, "M5Tab5 ST touch FW version %02x", fw_version);
          if (fw_version != 1 && fw_version != 3)
          {
            ESP_LOGW(LIBRARY_NAME, "M5Tab5 unknown ST touch FW version %02x", fw_version);
          }
        }
        if (fw_version == 1) { hit_st7121 = true; break; }
        if (fw_version == 3) { hit_st7123 = true; break; }
      }
      else if (lgfx::i2c::beginTransaction(tab5_detail::i2c_port,
                                            lgfx::Touch_GT911::default_addr_1,
                                            tab5_detail::i2c_freq, false).has_value()
            && lgfx::i2c::endTransaction(tab5_detail::i2c_port).has_value())
      {
        found_gt911 = true;
        ESP_LOGI(LIBRARY_NAME, "M5Tab5 GT911 touch detected");
        break;
      }
      lgfx::delay(10);
    }
    if (!read_st_touch_fw && !found_gt911)
    {
      ESP_LOGW(LIBRARY_NAME, "M5Tab5 ST touch FW version read failed");
    }

    const dsi_bus_desc_t bus_tab5 =
      dsi_bus(specs::tab5::bus_id, specs::tab5::bus_lane_num,
              hit_st7121 ? tab5_st7121_lane_mbps : specs::tab5::bus_lane_mbps,
              specs::tab5::bus_ldo_chan_id, specs::tab5::bus_ldo_voltage_mv);
    display_parts_owner_t out;
    auto* bus_dsi = make_dsi_bus(bus_tab5);
    out.bus.reset(bus_dsi);
    if (!bus_dsi->init()) { return construct_status_t::no_display; }

    bool hit_ili9881 = false;
    lgfx::delay(80);
    for (int i = 0; !hit_st7121 && !hit_st7123 && !hit_ili9881 && i < 3; ++i)
    {
      std::uint8_t id[3] = {};
      bus_dsi->readParams(0xF4, id, 2);
      ESP_LOGD(LIBRARY_NAME, "ST ID %02x %02x", id[0], id[1]);
      if (id[0] == 0x71 && id[1] == 0x23)
      {
        ESP_LOGI(LIBRARY_NAME, "M5Tab5 ST DSI ID matched 71 23");
      }
      static constexpr std::uint8_t params_page1[] = { 0x98, 0x81, 0x01 };
      bus_dsi->writeParams(0xFF, params_page1, 3);
      bus_dsi->readParams(0x00, &id[0], 1);
      bus_dsi->readParams(0x01, &id[1], 1);
      bus_dsi->readParams(0x02, &id[2], 1);
      ESP_LOGD(LIBRARY_NAME, "ILI ID %02x %02x %02x", id[0], id[1], id[2]);
      if (id[0] == 0x98 && id[1] == 0x81)
      {
        static constexpr std::uint8_t params_page0[] = { 0x98, 0x81, 0x00 };
        bus_dsi->writeParams(0xFF, params_page0, 3);
        hit_ili9881 = true;
      }
    }

    if (hit_ili9881)
    {
      out.touch.reset(make_i2c_touch<lgfx::Touch_GT911>(touch_tab5));
      out.panel.reset(make_dsi_panel<lgfx::Panel_ILI9881C>(panel_tab5_ili9881c, bus_dsi));
    }
    else if (hit_st7121)
    {
      ESP_LOGI(LIBRARY_NAME, "M5Tab5 detected ST7121 display");
      out.touch.reset(make_i2c_touch<lgfx::Touch_ST7123>(touch_tab5));
      out.panel.reset(make_dsi_panel<lgfx::Panel_ST7121>(panel_tab5_st7121, bus_dsi));
    }
    else if (hit_st7123)
    {
      ESP_LOGI(LIBRARY_NAME, "M5Tab5 detected ST7123 display");
      out.touch.reset(make_i2c_touch<lgfx::Touch_ST7123>(touch_tab5));
      out.panel.reset(make_dsi_panel<lgfx::Panel_ST7123>(panel_tab5_st7123, bus_dsi));
    }
    else
    {
      ESP_LOGE(LIBRARY_NAME, "M5Tab5 display panel was not detected");
      return construct_status_t::no_display;
    }
    out.panel->setTouch(out.touch.get());
    out.light.reset(make_pwm_light(light_tab5));
    return construct_status(out.release_to(parts));
#endif
  }
}
}
