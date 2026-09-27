// ESP32 PICO-D4 / PICOV3_02 construction. Included inside board_detect::m5.

static constexpr spi_bus_desc_t bus_stickc =
  spi_bus(static_cast<spi_host_device_t>(specs::stickc::bus_host))
    .with_freq(specs::stickc::bus_freq_write, specs::stickc::bus_freq_read)
    .with_pins(desc_stickc.display.sclk, desc_stickc.display.mosi,
               desc_stickc.display.miso, desc_stickc.display.dc)
    .with_three_wire(specs::stickc::bus_three_wire);
static constexpr spi_bus_desc_t bus_stickcplus =
  spi_bus(static_cast<spi_host_device_t>(specs::stickcplus::bus_host))
    .with_freq(specs::stickcplus::bus_freq_write, specs::stickcplus::bus_freq_read)
    .with_pins(desc_stickcplus.display.sclk, desc_stickcplus.display.mosi,
               desc_stickcplus.display.miso, desc_stickcplus.display.dc)
    .with_three_wire(specs::stickcplus::bus_three_wire);
static constexpr spi_bus_desc_t bus_coreink =
  spi_bus(static_cast<spi_host_device_t>(specs::coreink::bus_host))
    .with_freq(specs::coreink::bus_freq_write, specs::coreink::bus_freq_read)
    .with_pins(desc_coreink.display.sclk, desc_coreink.display.mosi,
               desc_coreink.display.miso, desc_coreink.display.dc)
    .with_three_wire(specs::coreink::bus_three_wire);
static constexpr spi_bus_desc_t bus_stickcplus2 =
  spi_bus(static_cast<spi_host_device_t>(specs::stickcplus2::bus_host))
    .with_freq(specs::stickcplus2::bus_freq_write, specs::stickcplus2::bus_freq_read)
    .with_pins(desc_stickcplus2.display.sclk, desc_stickcplus2.display.mosi,
               desc_stickcplus2.display.miso, desc_stickcplus2.display.dc)
    .with_three_wire(specs::stickcplus2::bus_three_wire);

static constexpr panel_desc_t panel_stickc = panel();
static constexpr panel_desc_t panel_stickcplus = panel();
static constexpr panel_desc_t panel_coreink =
  panel().with_size(200, 200)
         .with_pins(desc_coreink.display.cs, desc_coreink.display.rst,
                    desc_coreink.display.busy);
static constexpr panel_desc_t panel_stickcplus2 =
  panel().with_rst(desc_stickcplus2.display.rst);
static constexpr pwm_light_desc_t light_stickcplus2 =
  pwm_light(wiring::stickcplus2::backlight_gpio,
            specs::stickcplus2::backlight::freq,
            specs::stickcplus2::backlight::channel)
    .with_invert(specs::stickcplus2::backlight::invert)
    .with_offset(specs::stickcplus2::backlight::offset);

construct_status_t construct_stickc(const board_result_t&, display_parts_t* parts)
{
  display_parts_owner_t out;
  out.bus.reset(make_spi_bus(bus_stickc));
  out.panel.reset(make_panel<Panel_M5StickC>(panel_stickc, out.bus.get()));
  out.light.reset(new Light_M5StickC(desc_stickc.internal_i2c.hw_port,
                                     desc_stickc.internal_i2c.sda,
                                     desc_stickc.internal_i2c.scl,
                                     specs::stickc::backlight_i2c::i2c_addr,
                                     specs::stickc::pmic::i2c_freq));
  return construct_status(out.release_to(parts));
}

construct_status_t construct_stickcplus(const board_result_t&, display_parts_t* parts)
{
  display_parts_owner_t out;
  out.bus.reset(make_spi_bus(bus_stickcplus));
  out.panel.reset(make_panel<Panel_M5StickCPlus>(panel_stickcplus, out.bus.get()));
  out.light.reset(new Light_M5StickC(desc_stickcplus.internal_i2c.hw_port,
                                     desc_stickcplus.internal_i2c.sda,
                                     desc_stickcplus.internal_i2c.scl,
                                     specs::stickcplus::backlight_i2c::i2c_addr,
                                     specs::stickcplus::pmic::i2c_freq));
  return construct_status(out.release_to(parts));
}

construct_status_t construct_coreink(const board_result_t& result, display_parts_t* parts)
{
  display_parts_owner_t out;
  out.bus.reset(make_spi_bus(bus_coreink));
  if (result.option & generated_options::coreink::m09)
  { out.panel.reset(make_panel<lgfx::Panel_GDEW0154M09>(panel_coreink, out.bus.get())); }
  else
  { out.panel.reset(make_panel<lgfx::Panel_GDEW0154D67>(panel_coreink, out.bus.get())); }
  return construct_status(out.release_to(parts));
}

construct_status_t construct_stickcplus2(const board_result_t&, display_parts_t* parts)
{
  display_parts_owner_t out;
  out.bus.reset(make_spi_bus(bus_stickcplus2));
  out.panel.reset(make_panel<Panel_M5StickCPlus>(panel_stickcplus2, out.bus.get()));
  out.light.reset(make_pwm_light(light_stickcplus2));
  return construct_status(out.release_to(parts));
}

construct_status_t construct_atompsram(const board_result_t&, display_parts_t*)
{
  return construct_status_t::no_display;
}
