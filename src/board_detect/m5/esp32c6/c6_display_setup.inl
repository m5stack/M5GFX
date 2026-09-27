static constexpr spi_bus_desc_t bus_unitc6l =
  spi_bus(static_cast<spi_host_device_t>(specs::unitc6l::bus_host))
    .with_freq(specs::unitc6l::bus_freq_write, specs::unitc6l::bus_freq_read)
    .with_pins(desc_unitc6l.display.sclk, desc_unitc6l.display.mosi,
               desc_unitc6l.display.miso, desc_unitc6l.display.dc)
    .with_three_wire(specs::unitc6l::bus_three_wire)
    .with_dma_channel(SPI_DMA_CH_AUTO);
static constexpr panel_desc_t panel_unitc6l =
  panel().with_size(specs::unitc6l::panel_ssd1306::width,
                    specs::unitc6l::panel_ssd1306::height)
    .with_offset(specs::unitc6l::panel_ssd1306::offset_x,
                 specs::unitc6l::panel_ssd1306::offset_y)
    .with_pins(desc_unitc6l.display.cs, desc_unitc6l.display.rst)
    .with_rotation(specs::unitc6l::panel_ssd1306::rotation_offset, 0)
    .with_readable(specs::unitc6l::panel_ssd1306::readable)
    .with_bus_shared(false);

static constexpr spi_bus_desc_t bus_nesson1 =
  spi_bus(static_cast<spi_host_device_t>(specs::nesson1::bus_host))
    .with_freq(specs::nesson1::bus_freq_write, specs::nesson1::bus_freq_read)
    .with_pins(desc_nesson1.display.sclk, desc_nesson1.display.mosi,
               desc_nesson1.display.miso, desc_nesson1.display.dc)
    .with_three_wire(specs::nesson1::bus_three_wire)
    .with_dma_channel(SPI_DMA_CH_AUTO);
static constexpr spi_bus_desc_t bus_nesson1_panel_id =
  bus_nesson1.with_freq(8000000, 8000000);
static constexpr panel_desc_t panel_nesson1 =
  panel().with_size(specs::nesson1::panel_st7789v2::width,
                    specs::nesson1::panel_st7789v2::height)
    .with_offset(specs::nesson1::panel_st7789v2::offset_x,
                 specs::nesson1::panel_st7789v2::offset_y)
    .with_pins(desc_nesson1.display.cs, GPIO_NUM_NC)
    .with_rotation(specs::nesson1::panel_st7789v2::rotation_offset, 0)
    .with_invert(specs::nesson1::panel_st7789v2::invert)
    .with_readable(specs::nesson1::panel_st7789v2::readable)
    .with_bus_shared(true);
static constexpr i2c_touch_desc_t touch_nesson1 =
  i2c_touch(specs::nesson1::touch::i2c_addr)
    .with_port(c6_display_detail::i2c_port)
    .with_pins(c6_display_detail::sda, c6_display_detail::scl,
               wiring::nesson1::touch_int)
    .with_freq(specs::nesson1::touch::i2c_freq)
    .with_range(specs::nesson1::touch::x_min, specs::nesson1::touch::x_max,
                specs::nesson1::touch::y_min, specs::nesson1::touch::y_max)
    .with_rotation(specs::nesson1::touch::rotation_offset)
    .with_bus_shared(true);

construct_status_t construct_unitc6l(const board_result_t&, display_parts_t* parts)
{
  display_parts_owner_t out;
  out.bus.reset(make_spi_bus(bus_unitc6l));
  out.panel.reset(make_panel<lgfx::Panel_SSD1306>(panel_unitc6l, out.bus.get()));
  return construct_status(out.release_to(parts));
}

construct_status_t construct_nesson1(const board_result_t&, display_parts_t* parts)
{
  display_parts_owner_t out;
  out.bus.reset(make_spi_bus(bus_nesson1_panel_id));
  auto spi = static_cast<lgfx::Bus_SPI*>(out.bus.get());
  if (!spi->init()) { return construct_status_t::failed; }
  const auto panel_id = _read_panel_id(spi, desc_nesson1.display.cs,
                                       specs::nesson1::probe_st7789v2::cmd, 1);
  if ((panel_id & specs::nesson1::probe_st7789v2::mask)
      != specs::nesson1::probe_st7789v2::values[0])
  {
    ESP_LOGW(LIBRARY_NAME, "[Autodetect] ArduinoNessoN1 panel ID mismatch: 0x%08x",
             static_cast<unsigned>(panel_id));
  }
  spi->release();
  auto cfg = spi->config();
  cfg.freq_write = bus_nesson1.freq_write;
  cfg.freq_read = bus_nesson1.freq_read;
  spi->config(cfg);
  out.panel.reset(make_panel<lgfx::Panel_ST7789>(panel_nesson1, out.bus.get()));
  out.touch.reset(make_i2c_touch<lgfx::Touch_FT5x06>(touch_nesson1));
  out.light.reset(new Light_ArduinoNessoN1(
    c6_display_detail::i2c_port, specs::nesson1::backlight_i2c::i2c_addr,
    specs::nesson1::backlight_i2c::i2c_freq));
  out.panel->touch(out.touch.get());
  return construct_status(out.release_to(parts));
}
