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

  static constexpr spi_bus_desc_t bus_atoms3r =
    spi_bus(static_cast<spi_host_device_t>(specs::atoms3r::bus_host))
      .with_freq(specs::atoms3r::bus_freq_write, specs::atoms3r::bus_freq_read)
      .with_pins(wiring::atoms3r::display_sclk, wiring::atoms3r::display_mosi,
                 wiring::atoms3r::display_miso, wiring::atoms3r::display_dc)
      .with_three_wire(specs::atoms3r::bus_three_wire)
      .with_dma_channel(SPI_DMA_CH_AUTO);
  static constexpr panel_desc_t panel_atoms3r_st7735s =
    panel()
      .with_size(specs::atoms3r::panel_st7735s::width, specs::atoms3r::panel_st7735s::height)
      .with_memory(specs::atoms3r::panel_st7735s::memory_width,
                   specs::atoms3r::panel_st7735s::memory_height)
      .with_offset(specs::atoms3r::panel_st7735s::offset_x,
                   specs::atoms3r::panel_st7735s::offset_y)
      .with_pins(wiring::atoms3r::display_cs, wiring::atoms3r::display_rst)
      .with_rotation(specs::atoms3r::panel_st7735s::rotation_offset)
      .with_invert(specs::atoms3r::panel_st7735s::invert)
      .with_readable(specs::atoms3r::panel_st7735s::readable)
      .with_bus_shared(false);
  static constexpr panel_desc_t panel_atoms3r_gc9107 =
    panel()
      .with_size(specs::atoms3r::panel_gc9107::width, specs::atoms3r::panel_gc9107::height)
      .with_memory(specs::atoms3r::panel_gc9107::memory_width,
                   specs::atoms3r::panel_gc9107::memory_height)
      .with_offset(specs::atoms3r::panel_gc9107::offset_x,
                   specs::atoms3r::panel_gc9107::offset_y)
      .with_pins(wiring::atoms3r::display_cs, wiring::atoms3r::display_rst)
      .with_rotation(specs::atoms3r::panel_gc9107::rotation_offset)
      .with_invert(specs::atoms3r::panel_gc9107::invert)
      .with_readable(specs::atoms3r::panel_gc9107::readable)
      .with_bus_shared(false);

  bool construct_atoms3r(const board_result_t& result, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_atoms3r));
    if (result.option & generated_options::atoms3r::gc9107)
    {
      out.panel.reset(make_panel<Panel_GC9107>(panel_atoms3r_gc9107, out.bus.get()));
    }
    else
    {
      out.panel.reset(make_panel<lgfx::Panel_ST7735S>(panel_atoms3r_st7735s, out.bus.get()));
    }
    out.light.reset(make_default_part<Light_M5StackAtomS3R>(
      i2c_port, wiring::atoms3r::internal_i2c_sda, wiring::atoms3r::internal_i2c_scl,
      specs::atoms3r::backlight_i2c::i2c_addr, i2c_freq));
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

  static constexpr spi_bus_desc_t bus_airq =
    spi_bus(static_cast<spi_host_device_t>(specs::airq::bus_host))
      .with_freq(specs::airq::bus_freq_write, specs::airq::bus_freq_read)
      .with_pins(wiring::airq::display_sclk, wiring::airq::display_mosi,
                 wiring::airq::display_miso, wiring::airq::display_dc)
      .with_three_wire(specs::airq::bus_three_wire)
      .with_dma_channel(SPI_DMA_CH_AUTO);
  static constexpr panel_desc_t panel_airq_d67 =
    panel()
      .with_size(specs::airq::panel_gdew0154d67::width,
                 specs::airq::panel_gdew0154d67::height)
      .with_pins(wiring::airq::display_cs, wiring::airq::display_rst,
                 wiring::airq::display_busy);
  static constexpr panel_desc_t panel_airq_m09 =
    panel()
      .with_size(specs::airq::panel_gdew0154m09::width,
                 specs::airq::panel_gdew0154m09::height)
      .with_pins(wiring::airq::display_cs, wiring::airq::display_rst,
                 wiring::airq::display_busy);

  bool construct_airq(const board_result_t& result, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_airq));
    if (result.option & generated_options::airq::m09)
    {
      out.panel.reset(make_panel<lgfx::Panel_GDEW0154M09>(panel_airq_m09, out.bus.get()));
    }
    else
    {
      out.panel.reset(make_panel<lgfx::Panel_GDEW0154D67>(panel_airq_d67, out.bus.get()));
    }
    return out.release_to(parts);
  }

  static constexpr spi_bus_desc_t bus_stamplc =
    spi_bus(static_cast<spi_host_device_t>(specs::stamplc::bus_host))
      .with_freq(specs::stamplc::bus_freq_write, specs::stamplc::bus_freq_read)
      .with_pins(wiring::stamplc::display_sclk, wiring::stamplc::display_mosi,
                 wiring::stamplc::display_miso, wiring::stamplc::display_dc)
      .with_three_wire(specs::stamplc::bus_three_wire)
      .with_dma_channel(SPI_DMA_CH_AUTO);
  static constexpr panel_desc_t panel_stamplc =
    panel()
      .with_size(specs::stamplc::panel_st7789v2::width,
                 specs::stamplc::panel_st7789v2::height)
      .with_memory(specs::stamplc::panel_st7789v2::memory_width,
                   specs::stamplc::panel_st7789v2::memory_height)
      .with_offset(specs::stamplc::panel_st7789v2::offset_x,
                   specs::stamplc::panel_st7789v2::offset_y)
      .with_pins(wiring::stamplc::display_cs, wiring::stamplc::display_rst)
      .with_rotation(specs::stamplc::panel_st7789v2::rotation_offset)
      .with_invert(specs::stamplc::panel_st7789v2::invert)
      .with_readable(specs::stamplc::panel_st7789v2::readable)
      .with_bus_shared(true);

  bool construct_stamplc(const board_result_t&, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_stamplc));
    out.panel.reset(make_panel<Panel_ST7789>(panel_stamplc, out.bus.get()));
    out.light.reset(make_default_part<Light_M5StackStamPLC>(
      i2c_port, wiring::stamplc::internal_i2c_sda,
      wiring::stamplc::internal_i2c_scl,
      specs::stamplc::backlight_i2c::i2c_addr));
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

  static constexpr spi_bus_desc_t bus_cardputer =
    spi_bus(static_cast<spi_host_device_t>(specs::cardputer::bus_host))
      .with_freq(specs::cardputer::bus_freq_write, specs::cardputer::bus_freq_read)
      .with_pins(wiring::cardputer::display_sclk, wiring::cardputer::display_mosi,
                 wiring::cardputer::display_miso, wiring::cardputer::display_dc)
      .with_three_wire(specs::cardputer::bus_three_wire)
      .with_dma_channel(SPI_DMA_CH_AUTO);
  static constexpr panel_desc_t panel_cardputer =
    panel()
      .with_size(specs::cardputer::panel_st7789v2::width,
                 specs::cardputer::panel_st7789v2::height)
      .with_offset(specs::cardputer::panel_st7789v2::offset_x,
                   specs::cardputer::panel_st7789v2::offset_y)
      .with_pins(wiring::cardputer::display_cs, wiring::cardputer::display_rst)
      .with_rotation(specs::cardputer::panel_st7789v2::rotation_offset, 0)
      .with_invert(specs::cardputer::panel_st7789v2::invert)
      .with_readable(specs::cardputer::panel_st7789v2::readable);
  static constexpr pwm_light_desc_t light_cardputer =
    pwm_light(wiring::cardputer::backlight_gpio, specs::cardputer::backlight::freq,
              specs::cardputer::backlight::channel)
      .with_invert(specs::cardputer::backlight::invert)
      .with_offset(specs::cardputer::backlight::offset);

  static constexpr spi_bus_desc_t bus_cardputer_adv =
    spi_bus(static_cast<spi_host_device_t>(specs::cardputer_adv::bus_host))
      .with_freq(specs::cardputer_adv::bus_freq_write, specs::cardputer_adv::bus_freq_read)
      .with_pins(wiring::cardputer_adv::display_sclk, wiring::cardputer_adv::display_mosi,
                 wiring::cardputer_adv::display_miso, wiring::cardputer_adv::display_dc)
      .with_three_wire(specs::cardputer_adv::bus_three_wire)
      .with_dma_channel(SPI_DMA_CH_AUTO);
  static constexpr panel_desc_t panel_cardputer_adv =
    panel()
      .with_size(specs::cardputer_adv::panel_st7789v2::width,
                 specs::cardputer_adv::panel_st7789v2::height)
      .with_offset(specs::cardputer_adv::panel_st7789v2::offset_x,
                   specs::cardputer_adv::panel_st7789v2::offset_y)
      .with_pins(wiring::cardputer_adv::display_cs, wiring::cardputer_adv::display_rst)
      .with_rotation(specs::cardputer_adv::panel_st7789v2::rotation_offset, 0)
      .with_invert(specs::cardputer_adv::panel_st7789v2::invert)
      .with_readable(specs::cardputer_adv::panel_st7789v2::readable);
  static constexpr pwm_light_desc_t light_cardputer_adv =
    pwm_light(wiring::cardputer_adv::backlight_gpio, specs::cardputer_adv::backlight::freq,
              specs::cardputer_adv::backlight::channel)
      .with_invert(specs::cardputer_adv::backlight::invert)
      .with_offset(specs::cardputer_adv::backlight::offset);

  static constexpr spi_bus_desc_t bus_vameter =
    spi_bus(static_cast<spi_host_device_t>(specs::vameter::bus_host))
      .with_freq(specs::vameter::bus_freq_write, specs::vameter::bus_freq_read)
      .with_pins(wiring::vameter::display_sclk, wiring::vameter::display_mosi,
                 wiring::vameter::display_miso, wiring::vameter::display_dc)
      .with_three_wire(specs::vameter::bus_three_wire)
      .with_dma_channel(SPI_DMA_CH_AUTO);
  static constexpr panel_desc_t panel_vameter =
    panel()
      .with_size(specs::vameter::panel_st7789v2::width,
                 specs::vameter::panel_st7789v2::height)
      .with_offset(specs::vameter::panel_st7789v2::offset_x,
                   specs::vameter::panel_st7789v2::offset_y)
      .with_pins(wiring::vameter::display_cs, wiring::vameter::display_rst)
      .with_rotation(specs::vameter::panel_st7789v2::rotation_offset, 0)
      .with_invert(specs::vameter::panel_st7789v2::invert)
      .with_readable(specs::vameter::panel_st7789v2::readable);
  static constexpr pwm_light_desc_t light_vameter =
    pwm_light(wiring::vameter::backlight_gpio, specs::vameter::backlight::freq,
              specs::vameter::backlight::channel)
      .with_invert(specs::vameter::backlight::invert)
      .with_offset(specs::vameter::backlight::offset);

  bool construct_cardputer(const board_result_t&, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_cardputer));
    out.panel.reset(make_panel<Panel_ST7789>(panel_cardputer, out.bus.get()));
    out.light.reset(make_pwm_light(light_cardputer));
    return out.release_to(parts);
  }

  bool construct_cardputer_adv(const board_result_t&, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_cardputer_adv));
    out.panel.reset(make_panel<Panel_ST7789>(panel_cardputer_adv, out.bus.get()));
    out.light.reset(make_pwm_light(light_cardputer_adv));
    return out.release_to(parts);
  }

  bool construct_vameter(const board_result_t&, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_vameter));
    out.panel.reset(make_panel<Panel_ST7789>(panel_vameter, out.bus.get()));
    out.light.reset(make_pwm_light(light_vameter));
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
