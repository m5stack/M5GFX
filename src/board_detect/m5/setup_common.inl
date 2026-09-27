// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

// Included from M5GFX.cpp after the board-specific panel, light, and touch types are defined.
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
    std::int8_t dma_channel;

    constexpr spi_bus_desc_t(spi_host_device_t host_)
    : host(host_), freq_write(8000000), freq_read(8000000),
      sclk(setup_sentinel::no_pin), mosi(setup_sentinel::no_pin),
      miso(setup_sentinel::no_pin), dc(setup_sentinel::no_pin),
      three_wire(true), dma_channel(1) {}

    constexpr spi_bus_desc_t(spi_host_device_t host_, std::uint32_t freq_write_,
                             std::uint32_t freq_read_, std::int8_t sclk_,
                             std::int8_t mosi_, std::int8_t miso_, std::int8_t dc_,
                             bool three_wire_, std::int8_t dma_channel_)
    : host(host_), freq_write(freq_write_), freq_read(freq_read_),
      sclk(sclk_), mosi(mosi_), miso(miso_), dc(dc_), three_wire(three_wire_),
      dma_channel(dma_channel_) {}

    constexpr spi_bus_desc_t with_freq(std::uint32_t write, std::uint32_t read) const
    { return { host, write, read, sclk, mosi, miso, dc, three_wire, dma_channel }; }

    constexpr spi_bus_desc_t with_pins(int sclk_, int mosi_, int miso_, int dc_) const
    {
      return { host, freq_write, freq_read, static_cast<std::int8_t>(sclk_),
               static_cast<std::int8_t>(mosi_), static_cast<std::int8_t>(miso_),
               static_cast<std::int8_t>(dc_), three_wire, dma_channel };
    }

    constexpr spi_bus_desc_t with_three_wire(bool enabled) const
    { return { host, freq_write, freq_read, sclk, mosi, miso, dc, enabled, dma_channel }; }

    constexpr spi_bus_desc_t with_dma_channel(int channel) const
    { return { host, freq_write, freq_read, sclk, mosi, miso, dc, three_wire, static_cast<std::int8_t>(channel) }; }
  };

  struct panel_desc_t
  {
    std::int16_t width;
    std::int16_t height;
    std::int16_t memory_width;
    std::int16_t memory_height;
    std::int16_t offset_x;
    std::int16_t offset_y;
    std::int8_t pin_cs;
    std::int8_t pin_rst;
    std::int8_t pin_busy;
    std::uint8_t offset_rotation;
    std::int8_t initial_rotation;
    std::int8_t invert;
    std::int8_t readable;
    std::int8_t bus_shared;

    constexpr panel_desc_t()
    : width(setup_sentinel::keep_dimension), height(setup_sentinel::keep_dimension),
      memory_width(setup_sentinel::keep_dimension), memory_height(setup_sentinel::keep_dimension),
      offset_x(setup_sentinel::keep_offset), offset_y(setup_sentinel::keep_offset),
      pin_cs(setup_sentinel::keep_i8), pin_rst(setup_sentinel::keep_i8),
      pin_busy(setup_sentinel::keep_i8), offset_rotation(setup_sentinel::keep_u8),
      initial_rotation(setup_sentinel::keep_i8), invert(setup_sentinel::keep_i8),
      readable(setup_sentinel::keep_i8), bus_shared(setup_sentinel::keep_i8) {}

    constexpr panel_desc_t(std::int16_t width_, std::int16_t height_,
                           std::int16_t memory_width_, std::int16_t memory_height_,
                           std::int16_t offset_x_, std::int16_t offset_y_,
                           std::int8_t pin_cs_, std::int8_t pin_rst_, std::int8_t pin_busy_,
                           std::uint8_t offset_rotation_, std::int8_t initial_rotation_,
                           std::int8_t invert_, std::int8_t readable_,
                           std::int8_t bus_shared_)
    : width(width_), height(height_), memory_width(memory_width_), memory_height(memory_height_),
      offset_x(offset_x_), offset_y(offset_y_), pin_cs(pin_cs_), pin_rst(pin_rst_),
      pin_busy(pin_busy_), offset_rotation(offset_rotation_), initial_rotation(initial_rotation_),
      invert(invert_), readable(readable_), bus_shared(bus_shared_) {}

    constexpr panel_desc_t with_size(std::int16_t width_, std::int16_t height_) const
    { return { width_, height_, memory_width, memory_height, offset_x, offset_y, pin_cs, pin_rst, pin_busy, offset_rotation, initial_rotation, invert, readable, bus_shared }; }

    constexpr panel_desc_t with_memory(std::int16_t width_, std::int16_t height_) const
    { return { width, height, width_, height_, offset_x, offset_y, pin_cs, pin_rst, pin_busy, offset_rotation, initial_rotation, invert, readable, bus_shared }; }

    constexpr panel_desc_t with_offset(std::int16_t x, std::int16_t y) const
    { return { width, height, memory_width, memory_height, x, y, pin_cs, pin_rst, pin_busy, offset_rotation, initial_rotation, invert, readable, bus_shared }; }

    constexpr panel_desc_t with_pins(int cs, int rst, int busy = setup_sentinel::keep_i8) const
    { return { width, height, memory_width, memory_height, offset_x, offset_y, static_cast<std::int8_t>(cs), static_cast<std::int8_t>(rst), static_cast<std::int8_t>(busy), offset_rotation, initial_rotation, invert, readable, bus_shared }; }

    constexpr panel_desc_t with_rst(int rst) const
    { return { width, height, memory_width, memory_height, offset_x, offset_y, pin_cs, static_cast<std::int8_t>(rst), pin_busy, offset_rotation, initial_rotation, invert, readable, bus_shared }; }

    constexpr panel_desc_t with_rotation(std::uint8_t offset, int initial = setup_sentinel::keep_i8) const
    { return { width, height, memory_width, memory_height, offset_x, offset_y, pin_cs, pin_rst, pin_busy, offset, static_cast<std::int8_t>(initial), invert, readable, bus_shared }; }

    constexpr panel_desc_t with_invert(int enabled) const
    { return { width, height, memory_width, memory_height, offset_x, offset_y, pin_cs, pin_rst, pin_busy, offset_rotation, initial_rotation, static_cast<std::int8_t>(enabled), readable, bus_shared }; }

    constexpr panel_desc_t with_readable(int enabled) const
    { return { width, height, memory_width, memory_height, offset_x, offset_y, pin_cs, pin_rst, pin_busy, offset_rotation, initial_rotation, invert, static_cast<std::int8_t>(enabled), bus_shared }; }

    constexpr panel_desc_t with_bus_shared(bool enabled) const
    { return { width, height, memory_width, memory_height, offset_x, offset_y, pin_cs, pin_rst, pin_busy, offset_rotation, initial_rotation, invert, readable, enabled }; }
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
    : addr(addr_), port(setup_sentinel::no_pin), sda(setup_sentinel::no_pin),
      scl(setup_sentinel::no_pin), pin_int(setup_sentinel::no_pin), freq(400000),
      x_min(0), x_max(0), y_min(0), y_max(0),
      offset_rotation(setup_sentinel::keep_u8) {}

    constexpr i2c_touch_desc_t(std::uint8_t addr_, std::int8_t port_, std::int8_t sda_,
                               std::int8_t scl_, std::int8_t pin_int_, std::uint32_t freq_,
                               std::int16_t x_min_, std::int16_t x_max_,
                               std::int16_t y_min_, std::int16_t y_max_,
                               std::uint8_t offset_rotation_)
    : addr(addr_), port(port_), sda(sda_), scl(scl_), pin_int(pin_int_), freq(freq_),
      x_min(x_min_), x_max(x_max_), y_min(y_min_), y_max(y_max_),
      offset_rotation(offset_rotation_) {}

    constexpr i2c_touch_desc_t with_port(int value) const
    { return { addr, static_cast<std::int8_t>(value), sda, scl, pin_int, freq, x_min, x_max, y_min, y_max, offset_rotation }; }

    constexpr i2c_touch_desc_t with_pins(int sda_, int scl_, int interrupt) const
    { return { addr, port, static_cast<std::int8_t>(sda_), static_cast<std::int8_t>(scl_), static_cast<std::int8_t>(interrupt), freq, x_min, x_max, y_min, y_max, offset_rotation }; }

    constexpr i2c_touch_desc_t with_freq(std::uint32_t value) const
    { return { addr, port, sda, scl, pin_int, value, x_min, x_max, y_min, y_max, offset_rotation }; }

    constexpr i2c_touch_desc_t with_range(std::int16_t x_min_, std::int16_t x_max_,
                                          std::int16_t y_min_, std::int16_t y_max_) const
    { return { addr, port, sda, scl, pin_int, freq, x_min_, x_max_, y_min_, y_max_, offset_rotation }; }

    constexpr i2c_touch_desc_t with_rotation(std::uint8_t value) const
    { return { addr, port, sda, scl, pin_int, freq, x_min, x_max, y_min, y_max, value }; }
  };

  struct pwm_light_desc_t
  {
    std::int8_t pin;
    std::uint32_t freq;
    std::uint8_t channel;
    bool invert;
    std::uint8_t offset;

    constexpr pwm_light_desc_t with_invert(bool enabled) const
    { return { pin, freq, channel, enabled, offset }; }

    constexpr pwm_light_desc_t with_offset(std::uint8_t value) const
    { return { pin, freq, channel, invert, value }; }
  };

  constexpr spi_bus_desc_t spi_bus(spi_host_device_t host) { return spi_bus_desc_t(host); }
  constexpr panel_desc_t panel() { return panel_desc_t(); }
  constexpr i2c_touch_desc_t i2c_touch(std::uint8_t addr) { return i2c_touch_desc_t(addr); }
  constexpr pwm_light_desc_t pwm_light(int pin, std::uint32_t freq, std::uint8_t channel)
  { return { static_cast<std::int8_t>(pin), freq, channel, false, 0 }; }

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
    cfg.dma_channel = desc.dma_channel;
    cfg.pin_sclk = desc.sclk;
    cfg.pin_mosi = desc.mosi;
    cfg.pin_miso = desc.miso;
    cfg.pin_dc = desc.dc;
    bus->config(cfg);
    return bus;
  }

  lgfx::Panel_Device* apply_panel_desc(lgfx::Panel_Device* target,
                                       const panel_desc_t& desc, lgfx::IBus* bus)
  {
    if (desc.width != setup_sentinel::keep_dimension
     || desc.height != setup_sentinel::keep_dimension
     || desc.memory_width != setup_sentinel::keep_dimension
     || desc.memory_height != setup_sentinel::keep_dimension
     || desc.offset_x != setup_sentinel::keep_offset
     || desc.offset_y != setup_sentinel::keep_offset
     || desc.pin_cs != setup_sentinel::keep_i8
     || desc.pin_rst != setup_sentinel::keep_i8
     || desc.pin_busy != setup_sentinel::keep_i8
     || desc.offset_rotation != setup_sentinel::keep_u8
     || desc.invert != setup_sentinel::keep_i8
     || desc.readable != setup_sentinel::keep_i8
     || desc.bus_shared != setup_sentinel::keep_i8)
    {
      auto cfg = target->config();
      if (desc.width != setup_sentinel::keep_dimension) { cfg.panel_width = desc.width; }
      if (desc.height != setup_sentinel::keep_dimension) { cfg.panel_height = desc.height; }
      if (desc.memory_width != setup_sentinel::keep_dimension) { cfg.memory_width = desc.memory_width; }
      if (desc.memory_height != setup_sentinel::keep_dimension) { cfg.memory_height = desc.memory_height; }
      if (desc.offset_x != setup_sentinel::keep_offset) { cfg.offset_x = desc.offset_x; }
      if (desc.offset_y != setup_sentinel::keep_offset) { cfg.offset_y = desc.offset_y; }
      if (desc.pin_cs != setup_sentinel::keep_i8) { cfg.pin_cs = desc.pin_cs; }
      if (desc.pin_rst != setup_sentinel::keep_i8) { cfg.pin_rst = desc.pin_rst; }
      if (desc.pin_busy != setup_sentinel::keep_i8) { cfg.pin_busy = desc.pin_busy; }
      if (desc.offset_rotation != setup_sentinel::keep_u8) { cfg.offset_rotation = desc.offset_rotation; }
      if (desc.invert != setup_sentinel::keep_i8) { cfg.invert = desc.invert; }
      if (desc.readable != setup_sentinel::keep_i8) { cfg.readable = desc.readable; }
      if (desc.bus_shared != setup_sentinel::keep_i8) { cfg.bus_shared = desc.bus_shared; }
      target->config(cfg);
    }
    if (desc.initial_rotation != setup_sentinel::keep_i8) { target->setRotation(desc.initial_rotation); }
    // Keep rotation before bus attachment: update_madctl writes when a bus is connected.
    target->bus(bus);
    return target;
  }

  template <class PanelT>
  lgfx::Panel_Device* make_panel(const panel_desc_t& desc, lgfx::IBus* bus)
  { return apply_panel_desc(new PanelT(), desc, bus); }

  lgfx::ITouch* apply_i2c_touch_desc(lgfx::ITouch* target, const i2c_touch_desc_t& desc)
  {
    auto cfg = target->config();
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
    if (desc.offset_rotation != setup_sentinel::keep_u8) { cfg.offset_rotation = desc.offset_rotation; }
    cfg.bus_shared = false;
    target->config(cfg);
    return target;
  }

  template <class TouchT>
  lgfx::ITouch* make_i2c_touch(const i2c_touch_desc_t& desc)
  { return apply_i2c_touch_desc(new TouchT(), desc); }

  lgfx::ILight* make_pwm_light(const pwm_light_desc_t& desc)
  {
    auto target = new lgfx::Light_PWM();
    auto cfg = target->config();
    cfg.pin_bl = desc.pin;
    cfg.freq = desc.freq;
    cfg.pwm_channel = desc.channel;
    cfg.invert = desc.invert;
    cfg.offset = desc.offset;
    target->config(cfg);
    return target;
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
}
}
