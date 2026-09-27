// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

// Included from M5GFX.cpp after the board-specific panel and light types are defined.
namespace board_detect
{
namespace m5
{
  struct display_parts_t
  {
    lgfx::IBus* bus = nullptr;
    lgfx::Panel_Device* panel = nullptr;
    lgfx::ILight* light = nullptr;
    lgfx::ITouch* touch = nullptr;
  };

  struct spi_bus_desc_t
  {
    spi_host_device_t host;
    std::uint32_t freq_write;
    std::uint32_t freq_read;
    std::int8_t sclk;
    std::int8_t mosi;
    std::int8_t miso;
    std::int8_t dc;
    bool three_wire;

    constexpr spi_bus_desc_t(spi_host_device_t host_)
    : host(host_), freq_write(8000000), freq_read(8000000),
      sclk(-1), mosi(-1), miso(-1), dc(-1), three_wire(true) {}

    constexpr spi_bus_desc_t(spi_host_device_t host_, std::uint32_t freq_write_,
                             std::uint32_t freq_read_, std::int8_t sclk_,
                             std::int8_t mosi_, std::int8_t miso_, std::int8_t dc_,
                             bool three_wire_)
    : host(host_), freq_write(freq_write_), freq_read(freq_read_),
      sclk(sclk_), mosi(mosi_), miso(miso_), dc(dc_), three_wire(three_wire_) {}

    constexpr spi_bus_desc_t with_freq(std::uint32_t write, std::uint32_t read) const
    {
      return spi_bus_desc_t(host, write, read, sclk, mosi, miso, dc, three_wire);
    }

    constexpr spi_bus_desc_t with_pins(int sclk_, int mosi_, int miso_, int dc_) const
    {
      return spi_bus_desc_t(host, freq_write, freq_read,
                            static_cast<std::int8_t>(sclk_),
                            static_cast<std::int8_t>(mosi_),
                            static_cast<std::int8_t>(miso_),
                            static_cast<std::int8_t>(dc_), three_wire);
    }

    constexpr spi_bus_desc_t with_three_wire(bool enabled) const
    {
      return spi_bus_desc_t(host, freq_write, freq_read, sclk, mosi, miso, dc, enabled);
    }
  };

  struct panel_desc_t
  {
    std::int16_t width;
    std::int16_t height;
    std::int8_t pin_cs;
    std::int8_t pin_rst;
    std::int8_t pin_busy;
    std::uint8_t offset_rotation;
    std::int8_t initial_rotation;

    constexpr panel_desc_t()
    : width(0), height(0), pin_cs(-1), pin_rst(-1), pin_busy(-1),
      offset_rotation(0xFF), initial_rotation(-1) {}

    constexpr panel_desc_t(std::int16_t width_, std::int16_t height_, std::int8_t pin_cs_,
                           std::int8_t pin_rst_, std::int8_t pin_busy_,
                           std::uint8_t offset_rotation_, std::int8_t initial_rotation_)
    : width(width_), height(height_), pin_cs(pin_cs_), pin_rst(pin_rst_),
      pin_busy(pin_busy_), offset_rotation(offset_rotation_),
      initial_rotation(initial_rotation_) {}

    constexpr panel_desc_t with_size(std::int16_t width_, std::int16_t height_) const
    {
      return panel_desc_t(width_, height_, pin_cs, pin_rst, pin_busy,
                          offset_rotation, initial_rotation);
    }

    constexpr panel_desc_t with_pins(int cs, int rst, int busy) const
    {
      return panel_desc_t(width, height, static_cast<std::int8_t>(cs),
                          static_cast<std::int8_t>(rst), static_cast<std::int8_t>(busy),
                          offset_rotation, initial_rotation);
    }

    constexpr panel_desc_t with_rst(int rst) const
    {
      return panel_desc_t(width, height, pin_cs, static_cast<std::int8_t>(rst), pin_busy,
                          offset_rotation, initial_rotation);
    }

    constexpr panel_desc_t with_rotation(std::uint8_t offset, int initial) const
    {
      return panel_desc_t(width, height, pin_cs, pin_rst, pin_busy, offset,
                          static_cast<std::int8_t>(initial));
    }
  };

  struct i2c_touch_desc_t
  {
    std::uint8_t addr;
    std::int8_t port;
    std::int8_t sda;
    std::int8_t scl;
    std::int8_t pin_int;
    std::uint32_t freq;
    std::int16_t x_min;
    std::int16_t x_max;
    std::int16_t y_min;
    std::int16_t y_max;
    std::uint8_t offset_rotation;

    constexpr i2c_touch_desc_t(std::uint8_t addr_)
    : addr(addr_), port(-1), sda(-1), scl(-1), pin_int(-1), freq(400000),
      x_min(0), x_max(0), y_min(0), y_max(0), offset_rotation(0xFF) {}

    constexpr i2c_touch_desc_t(std::uint8_t addr_, std::int8_t port_, std::int8_t sda_,
                               std::int8_t scl_, std::int8_t pin_int_, std::uint32_t freq_,
                               std::int16_t x_min_, std::int16_t x_max_,
                               std::int16_t y_min_, std::int16_t y_max_,
                               std::uint8_t offset_rotation_)
    : addr(addr_), port(port_), sda(sda_), scl(scl_), pin_int(pin_int_), freq(freq_),
      x_min(x_min_), x_max(x_max_), y_min(y_min_), y_max(y_max_),
      offset_rotation(offset_rotation_) {}

    constexpr i2c_touch_desc_t with_port(int port_) const
    {
      return i2c_touch_desc_t(addr, static_cast<std::int8_t>(port_), sda, scl, pin_int,
                              freq, x_min, x_max, y_min, y_max, offset_rotation);
    }

    constexpr i2c_touch_desc_t with_pins(int sda_, int scl_, int interrupt) const
    {
      return i2c_touch_desc_t(addr, port, static_cast<std::int8_t>(sda_),
                              static_cast<std::int8_t>(scl_),
                              static_cast<std::int8_t>(interrupt), freq,
                              x_min, x_max, y_min, y_max, offset_rotation);
    }

    constexpr i2c_touch_desc_t with_freq(std::uint32_t freq_) const
    {
      return i2c_touch_desc_t(addr, port, sda, scl, pin_int, freq_,
                              x_min, x_max, y_min, y_max, offset_rotation);
    }

    constexpr i2c_touch_desc_t with_range(std::int16_t x_min_, std::int16_t x_max_,
                                          std::int16_t y_min_, std::int16_t y_max_) const
    {
      return i2c_touch_desc_t(addr, port, sda, scl, pin_int, freq,
                              x_min_, x_max_, y_min_, y_max_, offset_rotation);
    }

    constexpr i2c_touch_desc_t with_rotation(std::uint8_t offset) const
    {
      return i2c_touch_desc_t(addr, port, sda, scl, pin_int, freq,
                              x_min, x_max, y_min, y_max, offset);
    }
  };

  struct pwm_light_desc_t
  {
    std::int8_t pin;
    std::uint32_t freq;
    std::uint8_t channel;
  };

  constexpr spi_bus_desc_t spi_bus(spi_host_device_t host)
  {
    return spi_bus_desc_t(host);
  }

  constexpr panel_desc_t panel()
  {
    return panel_desc_t();
  }

  constexpr i2c_touch_desc_t i2c_touch(std::uint8_t addr)
  {
    return i2c_touch_desc_t(addr);
  }

  constexpr pwm_light_desc_t pwm_light(int pin, std::uint32_t freq, std::uint8_t channel)
  {
    return { static_cast<std::int8_t>(pin), freq, channel };
  }

  lgfx::Bus_SPI* make_spi_bus(const spi_bus_desc_t& desc)
  {
    auto bus = new lgfx::Bus_SPI();
    auto cfg = bus->config();
    cfg.freq_write = desc.freq_write;
    cfg.freq_read = desc.freq_read;
    cfg.spi_mode = 0;
    cfg.spi_3wire = desc.three_wire;
    cfg.use_lock = true;
    cfg.spi_host = desc.host;
    cfg.dma_channel = 1;
    cfg.pin_sclk = desc.sclk;
    cfg.pin_mosi = desc.mosi;
    cfg.pin_miso = desc.miso;
    cfg.pin_dc = desc.dc;
    bus->config(cfg);
    return bus;
  }

  lgfx::Panel_Device* apply_panel_desc(lgfx::Panel_Device* panel,
                                       const panel_desc_t& desc, lgfx::IBus* bus)
  {
    if (desc.width || desc.height || desc.pin_cs >= 0 || desc.pin_rst >= 0
     || desc.pin_busy >= 0 || desc.offset_rotation != 0xFF)
    {
      auto cfg = panel->config();
      if (desc.width) { cfg.panel_width = desc.width; }
      if (desc.height) { cfg.panel_height = desc.height; }
      if (desc.pin_cs >= 0) { cfg.pin_cs = desc.pin_cs; }
      if (desc.pin_rst >= 0) { cfg.pin_rst = desc.pin_rst; }
      if (desc.pin_busy >= 0) { cfg.pin_busy = desc.pin_busy; }
      if (desc.offset_rotation != 0xFF) { cfg.offset_rotation = desc.offset_rotation; }
      panel->config(cfg);
    }
    if (desc.initial_rotation >= 0) { panel->setRotation(desc.initial_rotation); }
    // Keep rotation before bus attachment: update_madctl writes when a bus is connected.
    panel->bus(bus);
    return panel;
  }

  template <class PanelT>
  lgfx::Panel_Device* make_panel(const panel_desc_t& desc, lgfx::IBus* bus)
  {
    return apply_panel_desc(new PanelT(), desc, bus);
  }

  lgfx::ITouch* apply_i2c_touch_desc(lgfx::ITouch* touch, const i2c_touch_desc_t& desc)
  {
    auto cfg = touch->config();
    cfg.pin_int = desc.pin_int;
    cfg.pin_sda = desc.sda;
    cfg.pin_scl = desc.scl;
    cfg.i2c_addr = desc.addr;
    cfg.i2c_port = desc.port;
    cfg.freq = desc.freq;
    cfg.x_min = desc.x_min;
    cfg.x_max = desc.x_max;
    cfg.y_min = desc.y_min;
    cfg.y_max = desc.y_max;
    if (desc.offset_rotation != 0xFF) { cfg.offset_rotation = desc.offset_rotation; }
    cfg.bus_shared = false;
    touch->config(cfg);
    return touch;
  }

  template <class TouchT>
  lgfx::ITouch* make_i2c_touch(const i2c_touch_desc_t& desc)
  {
    return apply_i2c_touch_desc(new TouchT(), desc);
  }

  lgfx::ILight* make_pwm_light(const pwm_light_desc_t& desc)
  {
    auto light = new lgfx::Light_PWM();
    auto cfg = light->config();
    cfg.pin_bl = desc.pin;
    cfg.freq = desc.freq;
    cfg.pwm_channel = desc.channel;
    light->config(cfg);
    return light;
  }

  template <class PartT>
  PartT* make_default_part() { return new PartT(); }

  struct display_parts_owner_t
  {
    std::unique_ptr<lgfx::IBus> bus;
    std::unique_ptr<lgfx::Panel_Device> panel;
    std::unique_ptr<lgfx::ILight> light;
    std::unique_ptr<lgfx::ITouch> touch;

    ~display_parts_owner_t();
    bool release_to(display_parts_t* parts);
  };

  display_parts_owner_t::~display_parts_owner_t() = default;

  bool display_parts_owner_t::release_to(display_parts_t* parts)
  {
    if (parts == nullptr || !bus || !panel) { return false; }
    parts->bus = bus.release();
    parts->panel = panel.release();
    parts->light = light.release();
    parts->touch = touch.release();
    return true;
  }

  static constexpr spi_bus_desc_t bus_station = spi_bus(SPI2_HOST)
    .with_freq(40000000, 15000000)
    .with_pins(desc_station.display.sclk, desc_station.display.mosi,
               desc_station.display.miso, desc_station.display.dc);
  static constexpr panel_desc_t panel_station = panel()
    .with_rst(desc_station.display.rst)
    .with_rotation(1, 0);

  static constexpr spi_bus_desc_t bus_core2 = spi_bus(SPI3_HOST)
    .with_freq(40000000, 16000000)
    .with_pins(desc_core2.display.sclk, desc_core2.display.mosi,
               desc_core2.display.miso, desc_core2.display.dc);
  static constexpr spi_bus_desc_t bus_tough = spi_bus(SPI3_HOST)
    .with_freq(40000000, 16000000)
    .with_pins(desc_tough.display.sclk, desc_tough.display.mosi,
               desc_tough.display.miso, desc_tough.display.dc);
  static constexpr panel_desc_t panel_core = panel();
  static constexpr i2c_touch_desc_t touch_core2 = i2c_touch(0x38)
    .with_port(desc_core2.internal_i2c.hw_port)
    .with_pins(desc_core2.internal_i2c.sda, desc_core2.internal_i2c.scl, GPIO_NUM_39)
    .with_freq(400000)
    .with_range(0, 319, 0, 279);
  static constexpr i2c_touch_desc_t touch_tough = i2c_touch(0x2E)
    .with_port(desc_tough.internal_i2c.hw_port)
    .with_pins(desc_tough.internal_i2c.sda, desc_tough.internal_i2c.scl, GPIO_NUM_39)
    .with_freq(detail::tough_touch_i2c_frequency)
    .with_range(0, 319, 0, 239);

  static constexpr spi_bus_desc_t bus_stack = spi_bus(SPI3_HOST)
    .with_freq(40000000, 16000000)
    .with_pins(desc_stack.display.sclk, desc_stack.display.mosi,
               desc_stack.display.miso, desc_stack.display.dc);
  static constexpr panel_desc_t panel_stack = panel();
  static constexpr pwm_light_desc_t light_stack = pwm_light(GPIO_NUM_32, 44100, 7);

  static constexpr spi_bus_desc_t bus_paper = spi_bus(SPI3_HOST)
    .with_freq(40000000, 20000000)
    .with_pins(desc_paper.display.sclk, desc_paper.display.mosi,
               desc_paper.display.miso, desc_paper.display.dc)
    .with_three_wire(false);
  static constexpr panel_desc_t panel_paper = panel()
    .with_size(960, 540)
    .with_pins(desc_paper.display.cs, desc_paper.display.rst, desc_paper.display.busy)
    .with_rotation(3, -1);
  static constexpr i2c_touch_desc_t touch_paper = i2c_touch(0x5D)
    .with_port(I2C_NUM_1)
    .with_pins(GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_36)
    .with_freq(400000)
    .with_range(0, 539, 0, 959)
    .with_rotation(1);

  bool construct_station(const board_result_t&, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_station));
    out.panel.reset(make_panel<Panel_M5StickCPlus>(panel_station, out.bus.get()));
    out.light.reset(make_default_part<Light_M5Tough>());
    return out.release_to(parts);
  }

  void construct_core_panel(const board_result_t& result, const spi_bus_desc_t& bus_desc,
                            display_parts_owner_t* out)
  {
    out->bus.reset(make_spi_bus(bus_desc));
    if (result.option & option_lcd_e)
    {
      out->panel.reset(make_panel<Panel_M5StackCore2E>(panel_core, out->bus.get()));
      _set_ili9342e_read(static_cast<lgfx::Panel_ILI9342*>(out->panel.get()),
                        bus_desc.freq_read);
    }
    else { out->panel.reset(make_panel<Panel_M5StackCore2>(panel_core, out->bus.get())); }
  }

  bool construct_core2(const board_result_t& result, display_parts_t* parts)
  {
    display_parts_owner_t out;
    construct_core_panel(result, bus_core2, &out);
    out.light.reset((result.option & option_core2_new_pmic)
                  ? static_cast<lgfx::ILight*>(make_default_part<Light_M5StackCore2_AXP2101>())
                  : static_cast<lgfx::ILight*>(make_default_part<Light_M5StackCore2>()));
    out.touch.reset(make_i2c_touch<lgfx::Touch_FT5x06>(touch_core2));
    out.panel->touch(out.touch.get());
    float affine[6] = { 1, 0, 0, 0, 1, 0 };
    out.panel->setCalibrateAffine(affine);
    return out.release_to(parts);
  }

  bool construct_tough(const board_result_t& result, display_parts_t* parts)
  {
    display_parts_owner_t out;
    construct_core_panel(result, bus_tough, &out);
    out.light.reset(make_default_part<Light_M5Tough>());
    out.touch.reset(make_i2c_touch<lgfx::Touch_CHSC6540>(touch_tough));
    out.panel->touch(out.touch.get());
    return out.release_to(parts);
  }

  bool construct_stack(const board_result_t&, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_stack));
    out.panel.reset(make_panel<Panel_M5Stack>(panel_stack, out.bus.get()));
    out.light.reset(make_pwm_light(light_stack));
    return out.release_to(parts);
  }

  bool construct_paper(const board_result_t&, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_paper));
    out.panel.reset(make_panel<lgfx::Panel_IT8951>(panel_paper, out.bus.get()));
    auto touch_desc = touch_paper;
#ifdef _M5EPD_H_
    touch_desc.port = I2C_NUM_0;
#endif
    out.touch.reset(make_i2c_touch<lgfx::Touch_GT911>(touch_desc));
    out.panel->touch(out.touch.get());
    return out.release_to(parts);
  }

  using construct_fn_t = bool (*)(const board_result_t&, display_parts_t*);
  struct construct_entry_t { board_id_t id; construct_fn_t construct; };
  static const construct_entry_t constructors[] = {
    { desc_station.def.id, construct_station },
    { desc_core2.def.id, construct_core2 },
    { desc_tough.def.id, construct_tough },
    { desc_stack.def.id, construct_stack },
    { desc_paper.def.id, construct_paper },
  };

  bool setup_esp32_d0wdq6(const board_result_t& result, display_parts_t* parts)
  {
    if (parts == nullptr || result.def == nullptr) { return false; }
    for (const auto& entry : constructors)
    {
      if (entry.id == result.def->id) { return entry.construct(result, parts); }
    }
    ESP_LOGE(LIBRARY_NAME, "No display constructor for board id %u",
             static_cast<unsigned>(result.def->id));
    return false;
  }
}
}
