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
    std::int8_t io0;
    std::int8_t io1;
    std::int8_t io2;
    std::int8_t io3;
    bool three_wire;
    std::int8_t dma_channel;

    constexpr spi_bus_desc_t(spi_host_device_t host_)
    : host(host_), freq_write(8000000), freq_read(8000000),
      sclk(setup_sentinel::no_pin), mosi(setup_sentinel::no_pin),
      miso(setup_sentinel::no_pin), dc(setup_sentinel::no_pin),
      io0(setup_sentinel::no_pin), io1(setup_sentinel::no_pin),
      io2(setup_sentinel::no_pin), io3(setup_sentinel::no_pin),
      three_wire(true), dma_channel(1) {}

    constexpr spi_bus_desc_t(spi_host_device_t host_, std::uint32_t freq_write_,
                             std::uint32_t freq_read_, std::int8_t sclk_,
                             std::int8_t mosi_, std::int8_t miso_, std::int8_t dc_,
                             std::int8_t io0_, std::int8_t io1_,
                             std::int8_t io2_, std::int8_t io3_,
                             bool three_wire_, std::int8_t dma_channel_)
    : host(host_), freq_write(freq_write_), freq_read(freq_read_),
      sclk(sclk_), mosi(mosi_), miso(miso_), dc(dc_),
      io0(io0_), io1(io1_), io2(io2_), io3(io3_), three_wire(three_wire_),
      dma_channel(dma_channel_) {}

    constexpr spi_bus_desc_t with_freq(std::uint32_t write, std::uint32_t read) const
    { return { host, write, read, sclk, mosi, miso, dc, io0, io1, io2, io3, three_wire, dma_channel }; }

    constexpr spi_bus_desc_t with_pins(int sclk_, int mosi_, int miso_, int dc_) const
    {
      return { host, freq_write, freq_read, static_cast<std::int8_t>(sclk_),
               static_cast<std::int8_t>(mosi_), static_cast<std::int8_t>(miso_),
               static_cast<std::int8_t>(dc_), io0, io1, io2, io3, three_wire, dma_channel };
    }

    constexpr spi_bus_desc_t with_quad_pins(int io0_, int io1_, int io2_, int io3_) const
    {
      return { host, freq_write, freq_read, sclk, mosi, miso, dc,
               static_cast<std::int8_t>(io0_), static_cast<std::int8_t>(io1_),
               static_cast<std::int8_t>(io2_), static_cast<std::int8_t>(io3_),
               three_wire, dma_channel };
    }

    constexpr spi_bus_desc_t with_three_wire(bool enabled) const
    { return { host, freq_write, freq_read, sclk, mosi, miso, dc, io0, io1, io2, io3, enabled, dma_channel }; }

    constexpr spi_bus_desc_t with_dma_channel(int channel) const
    { return { host, freq_write, freq_read, sclk, mosi, miso, dc, io0, io1, io2, io3, three_wire, static_cast<std::int8_t>(channel) }; }
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
    std::int8_t rgb_order;

    constexpr panel_desc_t()
    : width(setup_sentinel::keep_dimension), height(setup_sentinel::keep_dimension),
      memory_width(setup_sentinel::keep_dimension), memory_height(setup_sentinel::keep_dimension),
      offset_x(setup_sentinel::keep_offset), offset_y(setup_sentinel::keep_offset),
      pin_cs(setup_sentinel::keep_i8), pin_rst(setup_sentinel::keep_i8),
      pin_busy(setup_sentinel::keep_i8), offset_rotation(setup_sentinel::keep_u8),
      initial_rotation(setup_sentinel::keep_i8), invert(setup_sentinel::keep_i8),
      readable(setup_sentinel::keep_i8), bus_shared(setup_sentinel::keep_i8),
      rgb_order(setup_sentinel::keep_i8) {}

    constexpr panel_desc_t(std::int16_t width_, std::int16_t height_,
                           std::int16_t memory_width_, std::int16_t memory_height_,
                           std::int16_t offset_x_, std::int16_t offset_y_,
                           std::int8_t pin_cs_, std::int8_t pin_rst_, std::int8_t pin_busy_,
                           std::uint8_t offset_rotation_, std::int8_t initial_rotation_,
                           std::int8_t invert_, std::int8_t readable_,
                           std::int8_t bus_shared_, std::int8_t rgb_order_)
    : width(width_), height(height_), memory_width(memory_width_), memory_height(memory_height_),
      offset_x(offset_x_), offset_y(offset_y_), pin_cs(pin_cs_), pin_rst(pin_rst_),
      pin_busy(pin_busy_), offset_rotation(offset_rotation_), initial_rotation(initial_rotation_),
      invert(invert_), readable(readable_), bus_shared(bus_shared_), rgb_order(rgb_order_) {}

    constexpr panel_desc_t with_size(std::int16_t width_, std::int16_t height_) const
    { return { width_, height_, memory_width, memory_height, offset_x, offset_y, pin_cs, pin_rst, pin_busy, offset_rotation, initial_rotation, invert, readable, bus_shared, rgb_order }; }

    constexpr panel_desc_t with_memory(std::int16_t width_, std::int16_t height_) const
    { return { width, height, width_, height_, offset_x, offset_y, pin_cs, pin_rst, pin_busy, offset_rotation, initial_rotation, invert, readable, bus_shared, rgb_order }; }

    constexpr panel_desc_t with_offset(std::int16_t x, std::int16_t y) const
    { return { width, height, memory_width, memory_height, x, y, pin_cs, pin_rst, pin_busy, offset_rotation, initial_rotation, invert, readable, bus_shared, rgb_order }; }

    constexpr panel_desc_t with_pins(int cs, int rst, int busy = setup_sentinel::keep_i8) const
    { return { width, height, memory_width, memory_height, offset_x, offset_y, static_cast<std::int8_t>(cs), static_cast<std::int8_t>(rst), static_cast<std::int8_t>(busy), offset_rotation, initial_rotation, invert, readable, bus_shared, rgb_order }; }

    constexpr panel_desc_t with_rst(int rst) const
    { return { width, height, memory_width, memory_height, offset_x, offset_y, pin_cs, static_cast<std::int8_t>(rst), pin_busy, offset_rotation, initial_rotation, invert, readable, bus_shared, rgb_order }; }

    constexpr panel_desc_t with_rotation(std::uint8_t offset, int initial = setup_sentinel::keep_i8) const
    { return { width, height, memory_width, memory_height, offset_x, offset_y, pin_cs, pin_rst, pin_busy, offset, static_cast<std::int8_t>(initial), invert, readable, bus_shared, rgb_order }; }

    constexpr panel_desc_t with_invert(int enabled) const
    { return { width, height, memory_width, memory_height, offset_x, offset_y, pin_cs, pin_rst, pin_busy, offset_rotation, initial_rotation, static_cast<std::int8_t>(enabled), readable, bus_shared, rgb_order }; }

    constexpr panel_desc_t with_readable(int enabled) const
    { return { width, height, memory_width, memory_height, offset_x, offset_y, pin_cs, pin_rst, pin_busy, offset_rotation, initial_rotation, invert, static_cast<std::int8_t>(enabled), bus_shared, rgb_order }; }

    constexpr panel_desc_t with_bus_shared(bool enabled) const
    { return { width, height, memory_width, memory_height, offset_x, offset_y, pin_cs, pin_rst, pin_busy, offset_rotation, initial_rotation, invert, readable, enabled, rgb_order }; }

    constexpr panel_desc_t with_rgb_order(bool enabled) const
    { return { width, height, memory_width, memory_height, offset_x, offset_y, pin_cs, pin_rst, pin_busy, offset_rotation, initial_rotation, invert, readable, bus_shared, enabled }; }
  };

  struct i2c_bus_desc_t
  {
    std::int8_t port;
    std::uint32_t freq_write;
    std::uint32_t freq_read;
    std::int8_t sda;
    std::int8_t scl;
    std::uint8_t addr;
    std::uint8_t prefix_len;
  };

  constexpr i2c_bus_desc_t i2c_bus(int port, std::uint32_t freq_write,
                                   std::uint32_t freq_read, int sda, int scl,
                                   std::uint8_t addr, std::uint8_t prefix_len)
  {
    return { static_cast<std::int8_t>(port), freq_write, freq_read,
             static_cast<std::int8_t>(sda), static_cast<std::int8_t>(scl),
             addr, prefix_len };
  }

#if defined (CONFIG_IDF_TARGET_ESP32P4)
  struct dsi_bus_desc_t
  {
    std::uint8_t bus_id;
    std::uint8_t lane_num;
    std::uint16_t lane_mbps;
    std::uint8_t ldo_chan_id;
    std::uint16_t ldo_voltage_mv;
  };

  constexpr dsi_bus_desc_t dsi_bus(std::uint8_t bus_id, std::uint8_t lane_num,
                                   std::uint16_t lane_mbps, std::uint8_t ldo_chan_id,
                                   std::uint16_t ldo_voltage_mv)
  { return { bus_id, lane_num, lane_mbps, ldo_chan_id, ldo_voltage_mv }; }

  struct dsi_panel_desc_t
  {
    panel_desc_t panel;
    std::uint8_t dpi_freq_mhz;
    std::uint16_t hsync_back_porch;
    std::uint16_t hsync_pulse_width;
    std::uint16_t hsync_front_porch;
    std::uint16_t vsync_back_porch;
    std::uint16_t vsync_pulse_width;
    std::uint16_t vsync_front_porch;
  };

  constexpr dsi_panel_desc_t dsi_panel(
    const panel_desc_t& panel_, std::uint8_t dpi_freq_mhz,
    std::uint16_t hsync_back_porch, std::uint16_t hsync_pulse_width,
    std::uint16_t hsync_front_porch, std::uint16_t vsync_back_porch,
    std::uint16_t vsync_pulse_width, std::uint16_t vsync_front_porch)
  {
    return { panel_, dpi_freq_mhz, hsync_back_porch, hsync_pulse_width,
             hsync_front_porch, vsync_back_porch, vsync_pulse_width,
             vsync_front_porch };
  }

#endif

  // Parallel EPD bus (Bus_EPD). Pins follow the Bus_EPD::config_t names;
  // unused data lines stay at no_pin.
  struct epd_bus_desc_t
  {
    std::uint32_t bus_speed;
    std::uint8_t bus_width;
    std::int8_t data[8];
    std::int8_t pwr;
    std::int8_t spv;
    std::int8_t ckv;
    std::int8_t sph;
    std::int8_t oe;
    std::int8_t le;
    std::int8_t cl;
  };

  constexpr epd_bus_desc_t epd_bus(std::uint32_t bus_speed, std::uint8_t bus_width,
                                   int d0, int d1, int d2, int d3, int d4, int d5, int d6, int d7,
                                   int pwr, int spv, int ckv, int sph, int oe, int le, int cl)
  {
    return { bus_speed, bus_width,
             { static_cast<std::int8_t>(d0), static_cast<std::int8_t>(d1),
               static_cast<std::int8_t>(d2), static_cast<std::int8_t>(d3),
               static_cast<std::int8_t>(d4), static_cast<std::int8_t>(d5),
               static_cast<std::int8_t>(d6), static_cast<std::int8_t>(d7) },
             static_cast<std::int8_t>(pwr), static_cast<std::int8_t>(spv),
             static_cast<std::int8_t>(ckv), static_cast<std::int8_t>(sph),
             static_cast<std::int8_t>(oe), static_cast<std::int8_t>(le),
             static_cast<std::int8_t>(cl) };
  }

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
    bool bus_shared;

    constexpr i2c_touch_desc_t(std::uint8_t addr_)
    : addr(addr_), port(setup_sentinel::no_pin), sda(setup_sentinel::no_pin),
      scl(setup_sentinel::no_pin), pin_int(setup_sentinel::no_pin), freq(400000),
      x_min(0), x_max(0), y_min(0), y_max(0),
      offset_rotation(setup_sentinel::keep_u8), bus_shared(false) {}

    constexpr i2c_touch_desc_t(std::uint8_t addr_, std::int8_t port_, std::int8_t sda_,
                               std::int8_t scl_, std::int8_t pin_int_, std::uint32_t freq_,
                               std::int16_t x_min_, std::int16_t x_max_,
                               std::int16_t y_min_, std::int16_t y_max_,
                               std::uint8_t offset_rotation_, bool bus_shared_)
    : addr(addr_), port(port_), sda(sda_), scl(scl_), pin_int(pin_int_), freq(freq_),
      x_min(x_min_), x_max(x_max_), y_min(y_min_), y_max(y_max_),
      offset_rotation(offset_rotation_), bus_shared(bus_shared_) {}

    constexpr i2c_touch_desc_t with_port(int value) const
    { return { addr, static_cast<std::int8_t>(value), sda, scl, pin_int, freq, x_min, x_max, y_min, y_max, offset_rotation, bus_shared }; }

    constexpr i2c_touch_desc_t with_pins(int sda_, int scl_, int interrupt) const
    { return { addr, port, static_cast<std::int8_t>(sda_), static_cast<std::int8_t>(scl_), static_cast<std::int8_t>(interrupt), freq, x_min, x_max, y_min, y_max, offset_rotation, bus_shared }; }

    constexpr i2c_touch_desc_t with_freq(std::uint32_t value) const
    { return { addr, port, sda, scl, pin_int, value, x_min, x_max, y_min, y_max, offset_rotation, bus_shared }; }

    constexpr i2c_touch_desc_t with_range(std::int16_t x_min_, std::int16_t x_max_,
                                          std::int16_t y_min_, std::int16_t y_max_) const
    { return { addr, port, sda, scl, pin_int, freq, x_min_, x_max_, y_min_, y_max_, offset_rotation, bus_shared }; }

    constexpr i2c_touch_desc_t with_rotation(std::uint8_t value) const
    { return { addr, port, sda, scl, pin_int, freq, x_min, x_max, y_min, y_max, value, bus_shared }; }

    constexpr i2c_touch_desc_t with_bus_shared(bool value) const
    { return { addr, port, sda, scl, pin_int, freq, x_min, x_max, y_min, y_max, offset_rotation, value }; }
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
  // Leave the address to the touch driver's own default (it is not a valid
  // 7-bit address, so apply_i2c_touch_desc keeps the driver value).
  constexpr i2c_touch_desc_t i2c_touch_default_addr() { return i2c_touch_desc_t(setup_sentinel::keep_u8); }
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
    cfg.pin_io0 = desc.io0;
    cfg.pin_io1 = desc.io1;
    cfg.pin_io2 = desc.io2;
    cfg.pin_io3 = desc.io3;
    bus->config(cfg);
    return bus;
  }

  lgfx::Bus_I2C* make_i2c_bus(const i2c_bus_desc_t& desc)
  {
    auto bus = new lgfx::Bus_I2C();
    auto cfg = bus->config();
    cfg.i2c_port = desc.port;
    cfg.freq_write = desc.freq_write;
    cfg.freq_read = desc.freq_read;
    cfg.pin_sda = desc.sda;
    cfg.pin_scl = desc.scl;
    cfg.i2c_addr = desc.addr;
    cfg.prefix_len = desc.prefix_len;
    bus->config(cfg);
    return bus;
  }

#if defined (CONFIG_IDF_TARGET_ESP32P4)
  lgfx::Bus_DSI* make_dsi_bus(const dsi_bus_desc_t& desc)
  {
    auto bus = new lgfx::Bus_DSI();
    auto cfg = bus->config();
    cfg.bus_id = desc.bus_id;
    cfg.lane_num = desc.lane_num;
    cfg.lane_mbps = desc.lane_mbps;
    cfg.ldo_chan_id = desc.ldo_chan_id;
    cfg.ldo_voltage_mv = desc.ldo_voltage_mv;
    bus->config(cfg);
    return bus;
  }
#endif

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
     || desc.bus_shared != setup_sentinel::keep_i8
     || desc.rgb_order != setup_sentinel::keep_i8)
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
      if (desc.rgb_order != setup_sentinel::keep_i8) { cfg.rgb_order = desc.rgb_order; }
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

#if defined (CONFIG_IDF_TARGET_ESP32P4)
  template <class PanelT>
  lgfx::Panel_Device* make_dsi_panel(const dsi_panel_desc_t& desc, lgfx::IBus* bus)
  {
    auto target = new PanelT();
    auto detail = target->config_detail();
    detail.dpi_freq_mhz = desc.dpi_freq_mhz;
    detail.hsync_back_porch = desc.hsync_back_porch;
    detail.hsync_pulse_width = desc.hsync_pulse_width;
    detail.hsync_front_porch = desc.hsync_front_porch;
    detail.vsync_back_porch = desc.vsync_back_porch;
    detail.vsync_pulse_width = desc.vsync_pulse_width;
    detail.vsync_front_porch = desc.vsync_front_porch;
    target->config_detail(detail);
    return apply_panel_desc(target, desc.panel, bus);
  }
#endif

  lgfx::ITouch* apply_i2c_touch_desc(lgfx::ITouch* target, const i2c_touch_desc_t& desc)
  {
    auto cfg = target->config();
    cfg.pin_int = desc.pin_int;
    cfg.pin_sda = desc.sda;
    cfg.pin_scl = desc.scl;
    if (desc.addr != setup_sentinel::keep_u8) { cfg.i2c_addr = desc.addr; }
    cfg.i2c_port = desc.port;
    cfg.freq = desc.freq;
    cfg.x_min = desc.x_min;
    cfg.x_max = desc.x_max;
    cfg.y_min = desc.y_min;
    cfg.y_max = desc.y_max;
    if (desc.offset_rotation != setup_sentinel::keep_u8) { cfg.offset_rotation = desc.offset_rotation; }
    cfg.bus_shared = desc.bus_shared;
    target->config(cfg);
    return target;
  }

  template <class TouchT>
  lgfx::ITouch* make_i2c_touch(const i2c_touch_desc_t& desc)
  { return apply_i2c_touch_desc(new TouchT(), desc); }

#if defined (CONFIG_IDF_TARGET_ESP32S3) && defined (CONFIG_ESP32S3_SPIRAM_SUPPORT) && defined (CONFIG_SPIRAM_MODE_OCT)
  // Same condition as the Panel_EPD include: the parallel EPD driver needs
  // OPI PSRAM for its frame buffer.
  lgfx::Bus_EPD* make_epd_bus(const epd_bus_desc_t& desc)
  {
    auto bus = new lgfx::Bus_EPD();
    auto cfg = bus->config();
    cfg.bus_speed = desc.bus_speed;
    cfg.bus_width = desc.bus_width;
    for (std::size_t i = 0; i < sizeof(desc.data); ++i) { cfg.pin_data[i] = desc.data[i]; }
    cfg.pin_pwr = desc.pwr;
    cfg.pin_spv = desc.spv;
    cfg.pin_ckv = desc.ckv;
    cfg.pin_sph = desc.sph;
    cfg.pin_oe = desc.oe;
    cfg.pin_le = desc.le;
    cfg.pin_cl = desc.cl;
    bus->config(cfg);
    return bus;
  }

  // Panel_EPD keeps line_padding in its config_detail, outside panel_desc_t.
  lgfx::Panel_Device* make_epd_panel(const panel_desc_t& desc, std::uint8_t line_padding,
                                     lgfx::IBus* bus)
  {
    auto panel = new lgfx::Panel_EPD();
    auto detail = panel->config_detail();
    detail.line_padding = line_padding;
    panel->config_detail(detail);
    return apply_panel_desc(panel, desc, bus);
  }
#endif

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

  template <class PartT, class... Args>
  PartT* make_default_part(Args... args) { return new PartT(args...); }

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

  void destroy_display_parts(display_parts_t* parts)
  {
    if (parts == nullptr) { return; }
    if (parts->bus != nullptr) { parts->bus->release(); }
    delete parts->touch;
    delete parts->light;
    delete parts->panel;
    delete parts->bus;
    *parts = {};
  }
}
}
