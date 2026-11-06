// Copyright (c) M5Stack. All rights reserved.
#pragma once

#include "construct_common.inl"

namespace board_detect
{
namespace m5
{
  static constexpr dsi_bus_desc_t bus_corep4x =
    dsi_bus(specs::corep4x::bus_id, specs::corep4x::bus_lane_num,
            specs::corep4x::bus_lane_mbps, specs::corep4x::bus_ldo_chan_id,
            specs::corep4x::bus_ldo_voltage_mv);
  static constexpr panel_desc_t panel_corep4x_base =
    panel().with_size(specs::corep4x::panel_st7102::width,
                      specs::corep4x::panel_st7102::height)
      .with_memory(specs::corep4x::panel_st7102::memory_width,
                   specs::corep4x::panel_st7102::memory_height)
      .with_offset(0, 0).with_pins(GPIO_NUM_NC, GPIO_NUM_NC)
      .with_rotation(specs::corep4x::panel_st7102::rotation_offset)
      .with_readable(specs::corep4x::panel_st7102::readable)
      .with_bus_shared(false)
      .with_rgb_order(specs::corep4x::panel_st7102::rgb_order);
  static constexpr dsi_panel_desc_t panel_corep4x =
    dsi_panel(panel_corep4x_base,
              specs::corep4x::panel_st7102::dpi_freq_mhz,
              specs::corep4x::panel_st7102::hsync_back_porch,
              specs::corep4x::panel_st7102::hsync_pulse_width,
              specs::corep4x::panel_st7102::hsync_front_porch,
              specs::corep4x::panel_st7102::vsync_back_porch,
              specs::corep4x::panel_st7102::vsync_pulse_width,
              specs::corep4x::panel_st7102::vsync_front_porch);
  static constexpr i2c_touch_desc_t touch_corep4x =
    i2c_touch(specs::corep4x::touch::i2c_addr)
      .with_port(corep4x_detail::i2c_port)
      .with_pins(corep4x_detail::sda, corep4x_detail::scl, specs::corep4x::touch::int_pin)
      .with_freq(specs::corep4x::touch::i2c_freq)
      .with_range(specs::corep4x::touch::x_min, specs::corep4x::touch::x_max,
                  specs::corep4x::touch::y_min, specs::corep4x::touch::y_max)
      .with_rotation(specs::corep4x::touch::rotation_offset);

  construct_status_t construct_corep4x(const board_result_t&, display_parts_t* parts)
  {
#if !CONFIG_SPIRAM
    ESP_LOGE(LIBRARY_NAME, "M5CoreP4X needs PSRAM enabled");
    return construct_status_t::no_display;
#else
    display_parts_owner_t out;
    out.bus.reset(make_dsi_bus(bus_corep4x));
    if (!out.bus->init()) { return construct_status_t::no_display; }
    lgfx::delay(50);
    out.panel.reset(make_dsi_panel<lgfx::Panel_ST7102>(panel_corep4x, out.bus.get()));
    out.touch.reset(make_i2c_touch<lgfx::Touch_CST3530>(touch_corep4x));
    out.panel->setTouch(out.touch.get());
    out.light.reset(new Light_M5CoreP4X(corep4x_detail::i2c_port,
                                        corep4x_detail::sda, corep4x_detail::scl,
                                        pmic_ops::ioe1_i2c_addr,
                                        corep4x_detail::i2c_freq));
    return construct_status(out.release_to(parts));
#endif
  }

  construct_status_t construct_detected_board(const board_result_t& result, display_parts_t* parts)
  {
    return construct_board(esp32p4_boards, result, parts);
  }
}
}
#include "construct_common.inl"

namespace board_detect
{
namespace m5
{
  // The catalog emitter supports one board-level DSI bus; ST7121 units
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

  construct_status_t construct_tab5(const board_result_t& result, display_parts_t* parts)
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

    const bool hit_st7121 = result.prepared & tab5_detail::touch_st7121;
    const bool hit_st7123 = result.prepared & tab5_detail::touch_st7123;
    const bool read_st_touch_fw = result.prepared & tab5_detail::read_st_touch_fw;
    const bool found_gt911 = result.prepared & tab5_detail::touch_gt911;

    const dsi_bus_desc_t bus_tab5 =
      dsi_bus(specs::tab5::bus_id, specs::tab5::bus_lane_num,
              hit_st7121 ? tab5_st7121_lane_mbps : specs::tab5::bus_lane_mbps,
              specs::tab5::bus_ldo_chan_id, specs::tab5::bus_ldo_voltage_mv);
    display_parts_owner_t out;
    auto* bus_dsi = make_dsi_bus(bus_tab5);
    out.bus.reset(bus_dsi);
    if (!bus_dsi->init()) { return construct_status_t::no_display; }

    bool hit_ili9881 = false;
    if (!read_st_touch_fw && !found_gt911)
    {
      // A temporarily silent touch must not hide an answering ILI9881C.
      lgfx::delay(80);
      for (int i = 0; i < 3 && !hit_ili9881; ++i)
      {
        std::uint8_t id[3] = {};
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
    }
    if (found_gt911 || hit_ili9881)
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

  // ST7121 units need 900 Mbps on the same two lanes; the catalog value is
  // what the ST7123 (and the fallback) uses.
  static constexpr std::uint16_t stampp4xhub_st7121_lane_mbps = 900;
  static constexpr std::uint8_t stampp4xhub_touch_addr = 0x55;

  static constexpr panel_desc_t panel_stampp4xhub_base =
    panel().with_size(specs::stampp4xhub::panel_st7123::width,
                      specs::stampp4xhub::panel_st7123::height)
      .with_memory(specs::stampp4xhub::panel_st7123::memory_width,
                   specs::stampp4xhub::panel_st7123::memory_height)
      // LCD_RST is shared with TP_RST and driven once before construction;
      // letting the panel reset again would restart the touch controller.
      .with_offset(0, 0).with_pins(GPIO_NUM_NC, GPIO_NUM_NC)
      .with_rotation(specs::stampp4xhub::panel_st7123::rotation_offset)
      .with_readable(specs::stampp4xhub::panel_st7123::readable)
      .with_bus_shared(false)
      .with_rgb_order(specs::stampp4xhub::panel_st7123::rgb_order);

  static constexpr dsi_panel_desc_t panel_stampp4xhub_st7121 =
    dsi_panel(panel_stampp4xhub_base,
              specs::stampp4xhub::panel_st7121::dpi_freq_mhz,
              specs::stampp4xhub::panel_st7121::hsync_back_porch,
              specs::stampp4xhub::panel_st7121::hsync_pulse_width,
              specs::stampp4xhub::panel_st7121::hsync_front_porch,
              specs::stampp4xhub::panel_st7121::vsync_back_porch,
              specs::stampp4xhub::panel_st7121::vsync_pulse_width,
              specs::stampp4xhub::panel_st7121::vsync_front_porch);
  static constexpr dsi_panel_desc_t panel_stampp4xhub_st7123 =
    dsi_panel(panel_stampp4xhub_base,
              specs::stampp4xhub::panel_st7123::dpi_freq_mhz,
              specs::stampp4xhub::panel_st7123::hsync_back_porch,
              specs::stampp4xhub::panel_st7123::hsync_pulse_width,
              specs::stampp4xhub::panel_st7123::hsync_front_porch,
              specs::stampp4xhub::panel_st7123::vsync_back_porch,
              specs::stampp4xhub::panel_st7123::vsync_pulse_width,
              specs::stampp4xhub::panel_st7123::vsync_front_porch);

  static constexpr i2c_touch_desc_t touch_stampp4xhub =
    i2c_touch(specs::stampp4xhub::touch::i2c_addr)
      .with_port(stampp4xhub_detail::i2c_port)
      .with_pins(stampp4xhub_detail::sda, stampp4xhub_detail::scl,
                 specs::stampp4xhub::touch::int_pin)
      .with_freq(specs::stampp4xhub::touch::i2c_freq)
      .with_range(specs::stampp4xhub::touch::x_min, specs::stampp4xhub::touch::x_max,
                  specs::stampp4xhub::touch::y_min, specs::stampp4xhub::touch::y_max)
      .with_rotation(specs::stampp4xhub::touch::rotation_offset);

#if CONFIG_SPIRAM
  // Releases the shared LCD_RST / TP_RST line and returns the touch firmware
  // version, which selects the display controller (-1 when it stays silent).
  // The panel stays powered across a warm reset (its 3.3 V rail is VSYS_3V3),
  // so this pulse is the only thing that returns it to a known state; the
  // factory test holds it low for 100 ms, and a shorter pulse leaves a
  // warm-booted panel uninitialised.
  static int stampp4xhub_reset_and_read_touch(void)
  {
    constexpr int reset = stampp4xhub_detail::reset;
    lgfx::pinMode(reset, lgfx::pin_mode_t::output);
    lgfx::gpio_lo(reset);
    lgfx::delay(100);
    lgfx::gpio_hi(reset);

    if (!lgfx::i2c::init(stampp4xhub_detail::i2c_port,
                         stampp4xhub_detail::sda,
                         stampp4xhub_detail::scl).has_value())
    {
      return -1;
    }
    // The touch controller needs a few hundred milliseconds after a shared
    // reset before its firmware registers answer.
    for (int retry = 0; retry < 50; ++retry)
    {
      lgfx::delay(20);
      static constexpr std::uint8_t fw_reg[2] = { 0, 0 };
      std::uint8_t version = 0;
      if (lgfx::i2c::transactionWriteRead(stampp4xhub_detail::i2c_port,
                                          stampp4xhub_touch_addr,
                                          fw_reg, sizeof(fw_reg), &version, 1,
                                          specs::stampp4xhub::touch::i2c_freq).has_value())
      {
        // The display controller is slower than the touch controller to come
        // out of reset. The factory test waits 500 ms after releasing the
        // shared line before it starts the display; with a warm-booted panel,
        // a shorter wait drops the DCS init list and the panel stays dark
        // while the backlight is on.
        lgfx::delay(500);
        return version;
      }
    }
    return -1;
  }
#endif

  construct_status_t construct_stampp4xhub(const board_result_t&, display_parts_t* parts)
  {
#if !CONFIG_SPIRAM
    ESP_LOGE(LIBRARY_NAME, "M5StamP4XHub needs PSRAM enabled");
    return construct_status_t::no_display;
#else
    const int touch_fw_version = stampp4xhub_reset_and_read_touch();
    if (touch_fw_version < 0)
    {
      // No answer from the panel's touch controller: there is no way to tell an
      // ST7121 from an ST7123, and starting DSI at the wrong rate leaves the host
      // waiting on a panel that is not driving the bus (a detached screen hangs
      // the boot). Adopt the identity only, so Power / Grove / LED still come up
      // on a headless board.
      ESP_LOGW(LIBRARY_NAME, "M5StamP4XHub touch firmware version read failed; no display");
      return construct_status_t::no_display;
    }
    const bool hit_st7121 = (touch_fw_version == 1);
    ESP_LOGI(LIBRARY_NAME, "M5StamP4XHub touch firmware version %d", touch_fw_version);

    const dsi_bus_desc_t bus_stampp4xhub =
      dsi_bus(specs::stampp4xhub::bus_id, specs::stampp4xhub::bus_lane_num,
              hit_st7121 ? stampp4xhub_st7121_lane_mbps : specs::stampp4xhub::bus_lane_mbps,
              specs::stampp4xhub::bus_ldo_chan_id, specs::stampp4xhub::bus_ldo_voltage_mv);
    display_parts_owner_t out;
    auto* bus_dsi = make_dsi_bus(bus_stampp4xhub);
    out.bus.reset(bus_dsi);
    if (!bus_dsi->init()) { return construct_status_t::no_display; }

    out.touch.reset(make_i2c_touch<lgfx::Touch_ST7123>(touch_stampp4xhub));
    if (hit_st7121)
    {
      out.panel.reset(make_dsi_panel<lgfx::Panel_ST7121>(panel_stampp4xhub_st7121, bus_dsi));
    }
    else
    {
      out.panel.reset(make_dsi_panel<lgfx::Panel_ST7123>(panel_stampp4xhub_st7123, bus_dsi));
    }
    out.panel->setTouch(out.touch.get());
    out.light.reset(new Light_M5StamP4XHub(stampp4xhub_detail::i2c_port,
                                           stampp4xhub_detail::sda,
                                           stampp4xhub_detail::scl,
                                           pmic_ops::pm1_i2c_addr,
                                           stampp4xhub_detail::i2c_freq));
    return construct_status(out.release_to(parts));
#endif
  }
}
}
