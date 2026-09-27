#include "../setup_common.inl"

namespace board_detect
{
namespace m5
{
  static constexpr spi_bus_desc_t bus_toughc5 =
    spi_bus(SPI2_HOST).with_freq(specs::toughc5::bus_freq_write,
                                 specs::toughc5::bus_freq_read)
      .with_pins(wiring::toughc5::display_sclk, wiring::toughc5::display_mosi,
                 wiring::toughc5::display_miso, wiring::toughc5::display_dc)
      .with_three_wire(specs::toughc5::bus_three_wire)
      .with_dma_channel(SPI_DMA_CH_AUTO);
  static constexpr spi_bus_desc_t bus_toughc5_panel_id =
    bus_toughc5.with_freq(8000000, 8000000);
  static constexpr panel_desc_t panel_toughc5 =
    panel().with_size(specs::toughc5::panel_ili9342c::width,
                      specs::toughc5::panel_ili9342c::height)
      .with_pins(wiring::toughc5::display_cs, GPIO_NUM_NC)
      .with_rotation(specs::toughc5::panel_ili9342c::rotation_offset, 0)
      .with_invert(specs::toughc5::panel_ili9342c::invert)
      .with_readable(specs::toughc5::panel_ili9342c::readable).with_bus_shared(true);
  static constexpr i2c_touch_desc_t touch_toughc5 =
    i2c_touch(specs::toughc5::touch::i2c_addr)
      .with_port(toughc5_detail::i2c_port)
      .with_pins(toughc5_detail::sda, toughc5_detail::scl, GPIO_NUM_NC)
      .with_freq(specs::toughc5::touch::i2c_freq)
      .with_range(specs::toughc5::touch::x_min, specs::toughc5::touch::x_max,
                  specs::toughc5::touch::y_min, specs::toughc5::touch::y_max);

  construct_status_t construct_toughc5(const board_result_t&, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_toughc5_panel_id));
    auto spi = static_cast<lgfx::Bus_SPI*>(out.bus.get());
    if (!spi->init()) { return construct_status_t::failed; }
    auto panel_id = _read_panel_id(spi, wiring::toughc5::display_cs);
    if ((panel_id & specs::toughc5::probe_ili9342c::mask)
        != specs::toughc5::probe_ili9342c::values[0])
    {
      lgfx::delay(2);
      panel_id = _read_panel_id(spi, wiring::toughc5::display_cs);
      if ((panel_id & specs::toughc5::probe_ili9342c::mask)
          != specs::toughc5::probe_ili9342c::values[0])
      {
        ESP_LOGW(LIBRARY_NAME, "[Autodetect] ToughC5 panel ID mismatch: 0x%08x",
                 static_cast<unsigned>(panel_id));
      }
    }
    spi->release();
    auto cfg = spi->config();
    cfg.freq_write = bus_toughc5.freq_write;
    cfg.freq_read = bus_toughc5.freq_read;
    spi->config(cfg);
    out.panel.reset(make_panel<lgfx::Panel_ILI9342>(panel_toughc5, out.bus.get()));
    out.light.reset(make_default_part<Light_M5ToughC5>(
      toughc5_detail::i2c_port, pmic_ops::ioe1_i2c_addr, toughc5_detail::i2c_freq));
    out.touch.reset(make_i2c_touch<lgfx::Touch_CHSC6540>(touch_toughc5));
    out.panel->touch(out.touch.get());
    return construct_status(out.release_to(parts));
  }

  construct_status_t setup_esp32c5(const board_result_t& result, display_parts_t* parts)
  {
    return setup_board(esp32c5_boards, result, parts);
  }
}
}
