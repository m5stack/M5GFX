// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "construct_common.inl"

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

  static_assert(bus_station.dma_channel == 1 && bus_core2.dma_channel == 1
             && bus_tough.dma_channel == 1 && bus_stack.dma_channel == 1
             && bus_paper.dma_channel == 1,
                "ESP32 D0WDQ6 DMA channel must retain its existing value");
  static_assert(panel_station.bus_shared == -1 && panel_core.bus_shared == -1
             && panel_stack.bus_shared == -1 && panel_paper.bus_shared == -1,
                "ESP32 D0WDQ6 panels must preserve their class bus_shared defaults");

  construct_status_t construct_station(const board_result_t&, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_station));
    out.panel.reset(make_panel<Panel_M5StickCPlus>(panel_station, out.bus.get()));
    out.light.reset(make_default_part<Light_M5Tough>());
    return construct_status(out.release_to(parts));
  }

  void construct_core_panel(const board_result_t& result, std::uint32_t lcd_e_option,
                            const spi_bus_desc_t& bus_desc, display_parts_owner_t* out)
  {
    out->bus.reset(make_spi_bus(bus_desc));
    // Same LCD reset bit as the detector's AXP2101 variant (reset2101 / release2101).
    const std::uint8_t rst_reg = (result.option & generated_options::core2::new_pmic) ? 0x90 : 0x96;
    if (result.option & lcd_e_option)
    {
      auto panel = static_cast<Panel_M5StackCore2E*>(make_panel<Panel_M5StackCore2E>(panel_core, out->bus.get()));
      panel->setResetReg(rst_reg);
      out->panel.reset(panel);
      _set_ili9342e_read(panel, bus_desc.freq_read);
    }
    else
    {
      auto panel = static_cast<Panel_M5StackCore2*>(make_panel<Panel_M5StackCore2>(panel_core, out->bus.get()));
      panel->setResetReg(rst_reg);
      out->panel.reset(panel);
    }
  }

  construct_status_t construct_core2(const board_result_t& result, display_parts_t* parts)
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
    return construct_status(out.release_to(parts));
  }

  construct_status_t construct_tough(const board_result_t& result, display_parts_t* parts)
  {
    display_parts_owner_t out;
    construct_core_panel(result, generated_options::tough::lcd_e, bus_tough, &out);
    // Only fixed Tough can carry AXP2101; avoid sending it AXP192 backlight writes.
    out.light.reset((result.option & generated_options::core2::new_pmic)
                  ? static_cast<lgfx::ILight*>(make_default_part<Light_M5StackCore2_AXP2101>())
                  : static_cast<lgfx::ILight*>(make_default_part<Light_M5Tough>()));
    out.touch.reset(make_i2c_touch<lgfx::Touch_CHSC6540>(touch_tough));
    out.panel->touch(out.touch.get());
    return construct_status(out.release_to(parts));
  }

  construct_status_t construct_stack(const board_result_t&, display_parts_t* parts)
  {
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_stack));
    out.panel.reset(make_panel<Panel_M5Stack>(panel_stack, out.bus.get()));
    out.light.reset(make_pwm_light(light_stack));
    return construct_status(out.release_to(parts));
  }

  construct_status_t construct_paper(const board_result_t&, display_parts_t* parts)
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
    return construct_status(out.release_to(parts));
  }

  construct_status_t construct_detected_board(const board_result_t& result, display_parts_t* parts)
  {
    return construct_board(esp32_d0wdq6_boards, result, parts);
  }
}
}
