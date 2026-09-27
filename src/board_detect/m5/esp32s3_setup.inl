// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "setup_common.inl"

namespace board_detect
{
namespace m5
{
  static constexpr spi_bus_desc_t bus_atoms3 =
    spi_bus(static_cast<spi_host_device_t>(specs::atoms3::bus_host))
      .with_freq(specs::atoms3::bus_freq_write, specs::atoms3::bus_freq_read)
      .with_pins(wiring::atoms3::display_sclk, wiring::atoms3::display_mosi,
                 wiring::atoms3::display_miso, wiring::atoms3::display_dc)
      .with_three_wire(specs::atoms3::bus_three_wire)
      .with_dma_channel(SPI_DMA_CH_AUTO);
  static constexpr panel_desc_t panel_atoms3_st7735s =
    panel()
      .with_size(specs::atoms3::panel_st7735s::width, specs::atoms3::panel_st7735s::height)
      .with_memory(specs::atoms3::panel_st7735s::memory_width,
                   specs::atoms3::panel_st7735s::memory_height)
      .with_offset(specs::atoms3::panel_st7735s::offset_x, specs::atoms3::panel_st7735s::offset_y)
      .with_pins(wiring::atoms3::display_cs, wiring::atoms3::display_rst)
      .with_rotation(specs::atoms3::panel_st7735s::rotation_offset)
      .with_invert(specs::atoms3::panel_st7735s::invert)
      .with_readable(specs::atoms3::panel_st7735s::readable)
      .with_bus_shared(false);
  static constexpr panel_desc_t panel_atoms3_gc9107 =
    panel()
      .with_size(specs::atoms3::panel_gc9107::width, specs::atoms3::panel_gc9107::height)
      .with_memory(specs::atoms3::panel_gc9107::memory_width,
                   specs::atoms3::panel_gc9107::memory_height)
      .with_offset(specs::atoms3::panel_gc9107::offset_x, specs::atoms3::panel_gc9107::offset_y)
      .with_pins(wiring::atoms3::display_cs, wiring::atoms3::display_rst)
      .with_rotation(specs::atoms3::panel_gc9107::rotation_offset)
      .with_invert(specs::atoms3::panel_gc9107::invert)
      .with_readable(specs::atoms3::panel_gc9107::readable)
      .with_bus_shared(false);
  static constexpr pwm_light_desc_t light_atoms3 =
    pwm_light(wiring::atoms3::backlight_gpio, specs::atoms3::backlight::freq,
              specs::atoms3::backlight::channel)
      .with_invert(specs::atoms3::backlight::invert)
      .with_offset(specs::atoms3::backlight::offset);

  bool construct_atoms3(const board_result_t& result, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_atoms3));
    if (result.option & generated_options::atoms3::gc9107)
    {
      out.panel.reset(make_panel<Panel_GC9107>(panel_atoms3_gc9107, out.bus.get()));
    }
    else
    {
      out.panel.reset(make_panel<lgfx::Panel_ST7735S>(panel_atoms3_st7735s, out.bus.get()));
    }
    out.light.reset(make_pwm_light(light_atoms3));
    return out.release_to(parts);
  }

  static constexpr spi_bus_desc_t bus_dinmeter =
    spi_bus(static_cast<spi_host_device_t>(specs::dinmeter::bus_host))
      .with_freq(specs::dinmeter::bus_freq_write, specs::dinmeter::bus_freq_read)
      .with_pins(wiring::dinmeter::display_sclk, wiring::dinmeter::display_mosi,
                 wiring::dinmeter::display_miso, wiring::dinmeter::display_dc)
      .with_three_wire(specs::dinmeter::bus_three_wire)
      .with_dma_channel(SPI_DMA_CH_AUTO);
  static constexpr panel_desc_t panel_dinmeter =
    panel()
      .with_size(specs::dinmeter::panel_st7789v2::width, specs::dinmeter::panel_st7789v2::height)
      .with_memory(specs::dinmeter::panel_st7789v2::memory_width,
                   specs::dinmeter::panel_st7789v2::memory_height)
      .with_offset(specs::dinmeter::panel_st7789v2::offset_x,
                   specs::dinmeter::panel_st7789v2::offset_y)
      .with_pins(wiring::dinmeter::display_cs, wiring::dinmeter::display_rst)
      .with_rotation(specs::dinmeter::panel_st7789v2::rotation_offset)
      .with_invert(specs::dinmeter::panel_st7789v2::invert)
      .with_readable(specs::dinmeter::panel_st7789v2::readable);
  static constexpr pwm_light_desc_t light_dinmeter =
    pwm_light(wiring::dinmeter::backlight_gpio, specs::dinmeter::backlight::freq,
              specs::dinmeter::backlight::channel)
      .with_invert(specs::dinmeter::backlight::invert)
      .with_offset(specs::dinmeter::backlight::offset);

  bool construct_dinmeter(const board_result_t&, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_dinmeter));
    out.panel.reset(make_panel<Panel_ST7789>(panel_dinmeter, out.bus.get()));
    out.light.reset(make_pwm_light(light_dinmeter));
    return out.release_to(parts);
  }

  static constexpr spi_bus_desc_t bus_dial =
    spi_bus(static_cast<spi_host_device_t>(specs::dial::bus_host))
      .with_freq(specs::dial::bus_freq_write, specs::dial::bus_freq_read)
      .with_pins(wiring::dial::display_sclk, wiring::dial::display_mosi,
                 wiring::dial::display_miso, wiring::dial::display_dc)
      .with_three_wire(specs::dial::bus_three_wire)
      .with_dma_channel(SPI_DMA_CH_AUTO);
  static constexpr panel_desc_t panel_dial =
    panel()
      .with_size(specs::dial::panel_gc9a01::width, specs::dial::panel_gc9a01::height)
      .with_memory(specs::dial::panel_gc9a01::memory_width,
                   specs::dial::panel_gc9a01::memory_height)
      .with_offset(specs::dial::panel_gc9a01::offset_x, specs::dial::panel_gc9a01::offset_y)
      .with_pins(wiring::dial::display_cs, wiring::dial::display_rst)
      .with_rotation(specs::dial::panel_gc9a01::rotation_offset)
      .with_invert(specs::dial::panel_gc9a01::invert)
      .with_readable(specs::dial::panel_gc9a01::readable);
  static constexpr pwm_light_desc_t light_dial =
    pwm_light(wiring::dial::backlight_gpio, specs::dial::backlight::freq,
              specs::dial::backlight::channel)
      .with_invert(specs::dial::backlight::invert)
      .with_offset(specs::dial::backlight::offset);
  static constexpr i2c_touch_desc_t touch_dial =
    i2c_touch(specs::dial::touch::i2c_addr)
      .with_port(wiring::dial::internal_i2c_port)
      .with_pins(wiring::dial::internal_i2c_sda, wiring::dial::internal_i2c_scl,
                 wiring::dial::touch_int)
      .with_freq(specs::dial::touch::i2c_freq)
      .with_range(specs::dial::touch::x_min, specs::dial::touch::x_max,
                  specs::dial::touch::y_min, specs::dial::touch::y_max)
      .with_rotation(specs::dial::touch::rotation_offset);

  bool construct_dial(const board_result_t&, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_dial));
    out.panel.reset(make_panel<Panel_GC9A01>(panel_dial, out.bus.get()));
    out.light.reset(make_pwm_light(light_dial));
    out.touch.reset(make_i2c_touch<lgfx::Touch_FT5x06>(touch_dial));
    out.panel->touch(out.touch.get());
    return out.release_to(parts);
  }

  static constexpr spi_bus_desc_t bus_sticks3 =
    spi_bus(static_cast<spi_host_device_t>(specs::sticks3::bus_host))
      .with_freq(specs::sticks3::bus_freq_write, specs::sticks3::bus_freq_read)
      .with_pins(wiring::sticks3::display_sclk, wiring::sticks3::display_mosi,
                 wiring::sticks3::display_miso, wiring::sticks3::display_dc)
      .with_three_wire(specs::sticks3::bus_three_wire)
      .with_dma_channel(SPI_DMA_CH_AUTO);
  static constexpr panel_desc_t panel_sticks3 =
    panel()
      .with_size(specs::sticks3::panel_st7789v2::width, specs::sticks3::panel_st7789v2::height)
      .with_memory(specs::sticks3::panel_st7789v2::memory_width,
                   specs::sticks3::panel_st7789v2::memory_height)
      .with_offset(specs::sticks3::panel_st7789v2::offset_x,
                   specs::sticks3::panel_st7789v2::offset_y)
      .with_pins(wiring::sticks3::display_cs, wiring::sticks3::display_rst)
      .with_rotation(specs::sticks3::panel_st7789v2::rotation_offset)
      .with_invert(specs::sticks3::panel_st7789v2::invert)
      .with_readable(specs::sticks3::panel_st7789v2::readable)
      .with_bus_shared(false);
  static constexpr pwm_light_desc_t light_sticks3 =
    pwm_light(wiring::sticks3::backlight_gpio, specs::sticks3::backlight::freq,
              specs::sticks3::backlight::channel)
      .with_invert(specs::sticks3::backlight::invert)
      .with_offset(specs::sticks3::backlight::offset);

  bool construct_sticks3(const board_result_t&, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_sticks3));
    out.panel.reset(make_panel<Panel_ST7789>(panel_sticks3, out.bus.get()));
    out.light.reset(make_pwm_light(light_sticks3));
    return out.release_to(parts);
  }

  bool setup_esp32s3(const board_result_t& result, display_parts_t* parts)
  {
    return setup_board(esp32s3_boards, result, parts);
  }
}
}
