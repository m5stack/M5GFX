// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "setup_common.inl"

namespace board_detect
{
namespace m5
{
  // [atoms3:construct]
  static constexpr spi_bus_desc_t bus_atoms3 = spi_bus(static_cast<spi_host_device_t>(specs::atoms3::bus_host)).with_freq(specs::atoms3::bus_freq_write, specs::atoms3::bus_freq_read).with_pins(wiring::atoms3::display_sclk, wiring::atoms3::display_mosi, wiring::atoms3::display_miso, wiring::atoms3::display_dc).with_three_wire(specs::atoms3::bus_three_wire).with_dma_channel(SPI_DMA_CH_AUTO);
  static constexpr panel_desc_t panel_atoms3_st7735s = panel().with_size(specs::atoms3::panel_st7735s::width, specs::atoms3::panel_st7735s::height).with_memory(specs::atoms3::panel_st7735s::memory_width, specs::atoms3::panel_st7735s::memory_height).with_offset(specs::atoms3::panel_st7735s::offset_x, specs::atoms3::panel_st7735s::offset_y).with_pins(wiring::atoms3::display_cs, wiring::atoms3::display_rst).with_rotation(specs::atoms3::panel_st7735s::rotation_offset).with_invert(specs::atoms3::panel_st7735s::invert).with_readable(specs::atoms3::panel_st7735s::readable).with_bus_shared(false);
  static constexpr panel_desc_t panel_atoms3_gc9107 = panel().with_size(specs::atoms3::panel_gc9107::width, specs::atoms3::panel_gc9107::height).with_memory(specs::atoms3::panel_gc9107::memory_width, specs::atoms3::panel_gc9107::memory_height).with_offset(specs::atoms3::panel_gc9107::offset_x, specs::atoms3::panel_gc9107::offset_y).with_pins(wiring::atoms3::display_cs, wiring::atoms3::display_rst).with_rotation(specs::atoms3::panel_gc9107::rotation_offset).with_invert(specs::atoms3::panel_gc9107::invert).with_readable(specs::atoms3::panel_gc9107::readable).with_bus_shared(false);
  static constexpr pwm_light_desc_t light_atoms3 = pwm_light(wiring::atoms3::backlight_gpio, specs::atoms3::backlight::freq, specs::atoms3::backlight::channel).with_invert(specs::atoms3::backlight::invert).with_offset(specs::atoms3::backlight::offset);
  bool construct_atoms3(const board_result_t& result, display_parts_t* parts) {
    display_parts_owner_t out; out.bus.reset(make_spi_bus(bus_atoms3));
    if (result.option & option_atoms3_gc9107) { out.panel.reset(make_panel<Panel_GC9107>(panel_atoms3_gc9107, out.bus.get())); }
    else { out.panel.reset(make_panel<lgfx::Panel_ST7735S>(panel_atoms3_st7735s, out.bus.get())); }
    out.light.reset(make_pwm_light(light_atoms3)); return out.release_to(parts); }
  // [/atoms3:construct]

  // [sticks3:construct]
  static constexpr spi_bus_desc_t bus_sticks3 = spi_bus(static_cast<spi_host_device_t>(specs::sticks3::bus_host)).with_freq(specs::sticks3::bus_freq_write, specs::sticks3::bus_freq_read).with_pins(wiring::sticks3::display_sclk, wiring::sticks3::display_mosi, wiring::sticks3::display_miso, wiring::sticks3::display_dc).with_three_wire(specs::sticks3::bus_three_wire).with_dma_channel(SPI_DMA_CH_AUTO);
  static constexpr panel_desc_t panel_sticks3 = panel().with_size(specs::sticks3::panel_st7789v2::width, specs::sticks3::panel_st7789v2::height).with_memory(specs::sticks3::panel_st7789v2::memory_width, specs::sticks3::panel_st7789v2::memory_height).with_offset(specs::sticks3::panel_st7789v2::offset_x, specs::sticks3::panel_st7789v2::offset_y).with_pins(wiring::sticks3::display_cs, wiring::sticks3::display_rst).with_rotation(specs::sticks3::panel_st7789v2::rotation_offset).with_invert(specs::sticks3::panel_st7789v2::invert).with_readable(specs::sticks3::panel_st7789v2::readable).with_bus_shared(false);
  static constexpr pwm_light_desc_t light_sticks3 = pwm_light(wiring::sticks3::backlight_gpio, specs::sticks3::backlight::freq, specs::sticks3::backlight::channel).with_invert(specs::sticks3::backlight::invert).with_offset(specs::sticks3::backlight::offset);
  bool construct_sticks3(const board_result_t&, display_parts_t* parts) {
    display_parts_owner_t out; out.bus.reset(make_spi_bus(bus_sticks3));
    out.panel.reset(make_panel<Panel_ST7789>(panel_sticks3, out.bus.get()));
    out.light.reset(make_pwm_light(light_sticks3)); return out.release_to(parts); }
  // [/sticks3:construct]

  using construct_fn_t = bool (*)(const board_result_t&, display_parts_t*);
  struct construct_entry_t { board_id_t id; construct_fn_t construct; };
  // [atoms3:register]
  static const construct_entry_t constructors[] = {
    { desc_atoms3.def.id, construct_atoms3 },
    // [sticks3:register]
    { desc_sticks3.def.id, construct_sticks3 },
    // [/sticks3:register]
  };
  // [/atoms3:register]

  bool setup_esp32s3(const board_result_t& result, display_parts_t* parts)
  {
    if (parts == nullptr || result.def == nullptr) { return false; }
    for (const auto& entry : constructors)
    {
      if (entry.id == result.def->id) { return entry.construct(result, parts); }
    }
    ESP_LOGE(LIBRARY_NAME, "No display constructor for board id %u", static_cast<unsigned>(result.def->id));
    return false;
  }
}
}
