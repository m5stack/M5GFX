// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "../setup_common.inl"

namespace board_detect
{
namespace m5
{
  static constexpr spi_bus_desc_t bus_cores3 =
    spi_bus(static_cast<spi_host_device_t>(specs::cores3::bus_host))
      .with_freq(specs::cores3::bus_freq_write, specs::cores3::bus_freq_read)
      .with_pins(wiring::cores3::display_sclk, wiring::cores3::display_mosi,
                 wiring::cores3::display_miso, wiring::cores3::display_dc)
      .with_three_wire(specs::cores3::bus_three_wire)
      .with_dma_channel(SPI_DMA_CH_AUTO);
  static constexpr panel_desc_t panel_cores3 = panel();

  construct_status_t construct_cores3(const board_result_t& result, display_parts_t* parts)
  {
#if defined(M5GFX_AUTODETECT_TEST_FAIL_CORES3_SETUP)
    return construct_status_t::failed;
#else
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_cores3));
    if (result.option & generated_options::cores3::lcd_e)
    {
      auto target = make_panel<Panel_M5StackCoreS3E>(panel_cores3, out.bus.get());
      _set_ili9342e_read(static_cast<lgfx::Panel_ILI9342*>(target), bus_cores3.freq_read);
      auto cfg = target->config();
      cfg.readable = false;
      target->config(cfg);
      out.panel.reset(target);
    }
    else
    {
      out.panel.reset(make_panel<Panel_M5StackCoreS3>(panel_cores3, out.bus.get()));
    }
    out.light.reset(make_default_part<Light_M5StackCoreS3>());
    out.touch.reset(make_default_part<Touch_M5StackCoreS3>());
    out.panel->touch(out.touch.get());
    return construct_status(out.release_to(parts));
#endif
  }
}
}
