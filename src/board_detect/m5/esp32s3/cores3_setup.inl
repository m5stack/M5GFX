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
  static constexpr std::uint32_t cores3_legacy_panel_id_freq = 8000000;
  static constexpr spi_bus_desc_t bus_cores3_panel_id =
    bus_cores3.with_freq(cores3_legacy_panel_id_freq, cores3_legacy_panel_id_freq);
  static constexpr panel_desc_t panel_cores3 = panel();

  construct_status_t construct_cores3(const board_result_t&, display_parts_t* parts)
  {
#if defined(M5GFX_AUTODETECT_TEST_FAIL_CORES3_SETUP)
    return construct_status_t::failed;
#else
    display_parts_owner_t out;
    out.bus.reset(make_spi_bus(bus_cores3_panel_id));
    auto spi = static_cast<lgfx::Bus_SPI*>(out.bus.get());
    if (!spi->init()) { return construct_status_t::failed; }
    const auto panel_id = _read_panel_id(spi, wiring::cores3::display_cs);
    if ((panel_id & specs::cores3::probe_ili9342c::mask)
        != specs::cores3::probe_ili9342c::values[0])
    {
      ESP_LOGW(LIBRARY_NAME, "[Autodetect] CoreS3 panel ID mismatch: 0x%08x",
               static_cast<unsigned>(panel_id));
    }
    // The legacy path read RDDID at the S3 default 8/8 MHz, then switched to
    // the panel's normal 40/16 MHz before the C/E register probe.
    auto bus_cfg = spi->config();
    bus_cfg.freq_write = bus_cores3.freq_write;
    bus_cfg.freq_read = bus_cores3.freq_read;
    spi->config(bus_cfg);

    std::uint32_t keys[4] = {};
    Panel_M5StackCoreS3 panel_probe;
    panel_probe.bus(spi);
    const auto variant = _identify_ili9342(&panel_probe, keys, 1);
    _log_ili9342_variant(variant, keys);
    if (variant == ili9342_variant_t::e)
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
