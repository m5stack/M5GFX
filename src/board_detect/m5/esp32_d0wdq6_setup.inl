// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "setup_common.inl"

namespace board_detect
{
namespace m5
{
  static constexpr spi_bus_desc_t bus_station = spi_bus(SPI2_HOST).with_freq(40000000, 15000000).with_pins(desc_station.display.sclk, desc_station.display.mosi, desc_station.display.miso, desc_station.display.dc);
  static constexpr panel_desc_t panel_station = panel().with_rst(desc_station.display.rst).with_rotation(1, 0);
  static constexpr spi_bus_desc_t bus_core2 = spi_bus(SPI3_HOST).with_freq(40000000, 16000000).with_pins(desc_core2.display.sclk, desc_core2.display.mosi, desc_core2.display.miso, desc_core2.display.dc);
  static constexpr spi_bus_desc_t bus_tough = spi_bus(SPI3_HOST).with_freq(40000000, 16000000).with_pins(desc_tough.display.sclk, desc_tough.display.mosi, desc_tough.display.miso, desc_tough.display.dc);
  static constexpr panel_desc_t panel_core = panel();
  static constexpr i2c_touch_desc_t touch_core2 = i2c_touch(0x38).with_port(desc_core2.internal_i2c.hw_port).with_pins(desc_core2.internal_i2c.sda, desc_core2.internal_i2c.scl, GPIO_NUM_39).with_freq(400000).with_range(0, 319, 0, 279);
  static constexpr i2c_touch_desc_t touch_tough = i2c_touch(0x2E).with_port(desc_tough.internal_i2c.hw_port).with_pins(desc_tough.internal_i2c.sda, desc_tough.internal_i2c.scl, GPIO_NUM_39).with_freq(detail::tough_touch_i2c_frequency).with_range(0, 319, 0, 239);
  static constexpr spi_bus_desc_t bus_stack = spi_bus(SPI3_HOST).with_freq(40000000, 16000000).with_pins(desc_stack.display.sclk, desc_stack.display.mosi, desc_stack.display.miso, desc_stack.display.dc);
  static constexpr panel_desc_t panel_stack = panel();
  static constexpr pwm_light_desc_t light_stack = pwm_light(GPIO_NUM_32, 44100, 7);
  static constexpr spi_bus_desc_t bus_paper = spi_bus(SPI3_HOST).with_freq(40000000, 20000000).with_pins(desc_paper.display.sclk, desc_paper.display.mosi, desc_paper.display.miso, desc_paper.display.dc).with_three_wire(false);
  static constexpr panel_desc_t panel_paper = panel().with_size(960, 540).with_pins(desc_paper.display.cs, desc_paper.display.rst, desc_paper.display.busy).with_rotation(3, -1);
  static constexpr i2c_touch_desc_t touch_paper = i2c_touch(0x5D).with_port(I2C_NUM_1).with_pins(GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_36).with_freq(400000).with_range(0, 539, 0, 959).with_rotation(1);

  static_assert(bus_station.dma_channel == 1 && bus_core2.dma_channel == 1
             && bus_tough.dma_channel == 1 && bus_stack.dma_channel == 1
             && bus_paper.dma_channel == 1,
                "ESP32 D0WDQ6 DMA channel must retain its existing value");
  static_assert(panel_station.bus_shared == -1 && panel_core.bus_shared == -1
             && panel_stack.bus_shared == -1 && panel_paper.bus_shared == -1,
                "ESP32 D0WDQ6 panels must preserve their class bus_shared defaults");

  bool construct_station(const board_result_t&, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_station));
    out.panel.reset(make_panel<Panel_M5StickCPlus>(panel_station, out.bus.get()));
    out.light.reset(make_default_part<Light_M5Tough>());
    return out.release_to(parts);
  }

  void construct_core_panel(const board_result_t& result, std::uint32_t lcd_e_option,
                            const spi_bus_desc_t& bus_desc, display_parts_owner_t* out)
  {
    out->bus.reset(make_spi_bus(bus_desc));
    if (result.option & lcd_e_option)
    {
      out->panel.reset(make_panel<Panel_M5StackCore2E>(panel_core, out->bus.get()));
      _set_ili9342e_read(static_cast<lgfx::Panel_ILI9342*>(out->panel.get()), bus_desc.freq_read);
    }
    else { out->panel.reset(make_panel<Panel_M5StackCore2>(panel_core, out->bus.get())); }
  }

  bool construct_core2(const board_result_t& result, display_parts_t* parts)
  {
    display_parts_owner_t out;
    construct_core_panel(result, generated_options::core2::lcd_e, bus_core2, &out);
    out.light.reset((result.option & generated_options::core2::new_pmic)
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
    construct_core_panel(result, generated_options::tough::lcd_e, bus_tough, &out);
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

  bool setup_esp32_d0wdq6(const board_result_t& result, display_parts_t* parts)
  {
    return setup_board(esp32_d0wdq6_boards, result, parts);
  }
}
}
