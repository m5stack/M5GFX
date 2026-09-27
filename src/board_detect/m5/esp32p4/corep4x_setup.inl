#include "../setup_common.inl"

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
                                        pmic_ops::ioe1_i2c_addr,
                                        corep4x_detail::i2c_freq));
    return construct_status(out.release_to(parts));
#endif
  }

  construct_status_t setup_detected_board(const board_result_t& result, display_parts_t* parts)
  {
    return setup_board(esp32p4_boards, result, parts);
  }
}
}
