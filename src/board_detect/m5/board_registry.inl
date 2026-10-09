// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

namespace m5gfx
{
namespace board_detect
{
namespace m5
{
  // PI4IOE5V6408 "Device ID and Control" register: B7:B5 are fixed at 101.
  constexpr std::uint8_t pi4io_id_register = 0x01;
  constexpr bool is_pi4io(std::uint8_t value) { return (value & 0xE0) == 0xA0; }

  struct display_parts_t;
  enum class construct_status_t : std::uint8_t { ok, no_display, failed };
  constexpr construct_status_t construct_status(bool success)
  { return success ? construct_status_t::ok : construct_status_t::failed; }
  inline construct_status_t construct_displayless(const board_result_t&, display_parts_t*)
  { return construct_status_t::no_display; }
  using construct_fn_t = construct_status_t (*)(const board_result_t&, display_parts_t*);
  using success_annotation_fn_t = const char* (*)(const board_result_t&);

  struct board_entry_t
  {
    const board_desc_t* desc;
    construct_fn_t construct;
    const char* success_log_name;
    success_annotation_fn_t success_annotation;
  };

  struct success_log_t
  {
    const char* name;
    const char* annotation;
  };

  template <std::size_t BoardCount>
  success_log_t success_log(const board_entry_t (&boards)[BoardCount],
                            const board_result_t& result)
  {
    for (const auto& entry : boards)
    {
      if (result.def != nullptr && entry.desc->def.id == result.def->id)
      {
        return {
          entry.success_log_name == nullptr ? entry.desc->def.name : entry.success_log_name,
          entry.success_annotation == nullptr ? "" : entry.success_annotation(result),
        };
      }
    }
    return { result.def == nullptr ? "unknown" : result.def->name, "" };
  }

  template <std::size_t BoardCount>
  construct_status_t setup_board(const board_entry_t (&boards)[BoardCount],
                                 const board_result_t& result, display_parts_t* parts)
  {
    if (parts == nullptr || result.def == nullptr) { return construct_status_t::failed; }
    for (const auto& entry : boards)
    {
      if (entry.desc->def.id == result.def->id)
      {
        return entry.construct(result, parts);
      }
    }
    ESP_LOGE("M5GFX", "No display constructor for board id %u",
             static_cast<unsigned>(result.def->id));
    return construct_status_t::failed;
  }
  namespace detail
  {
    enum class panel_variant_t : std::uint8_t { unknown, c, e };

    void write8(soft_spi_t& bus, int pin_cs, std::uint8_t cmd, std::uint8_t data)
    {
      bus.beginTransaction();
      lgfx::gpio_lo(pin_cs);
      bus.writeCommand(cmd, 8);
      bus.writeData(data, 8);
      // An uninitialised Panel_LCD still has _nop_closing=true. Its endWrite()
      // emits NOP before raising CS, so reproduce that probe transaction exactly.
      bus.writeCommand(0x00, 8);
      bus.wait();
      lgfx::gpio_hi(pin_cs);
      bus.endTransaction();
    }

    std::uint8_t read_parameter(soft_spi_t& bus, int pin_cs, std::uint8_t cmd, std::uint8_t index)
    {
      write8(bus, pin_cs, 0xD9, 0x10 | index);
      bus.beginTransaction();
      lgfx::gpio_lo(pin_cs);
      bus.writeCommand(cmd, 8);
      bus.beginRead(1);
      const auto value = static_cast<std::uint8_t>(bus.readData(8));
      lgfx::gpio_hi(pin_cs);
      bus.endRead();
      // Panel_LCD::readCommand() raises CS before endWrite(). The latter still
      // clocks its closing NOP (with CS high) because the panel is uninitialised.
      bus.writeCommand(0x00, 8);
      bus.wait();
      bus.endTransaction();
      return (value >> 1) & 0x7F;
    }

    panel_variant_t probe_panel_variant(soft_spi_t& bus, int pin_cs,
                                        std::uint32_t keys[4], bool try_c_key)
    {
      write8(bus, pin_cs, 0xD9, 0x00);
      write8(bus, pin_cs, 0xDD, 0x01);
      write8(bus, pin_cs, 0xCB, 0x1C);
      keys[0] = read_parameter(bus, pin_cs, 0xDD, 1);
      keys[1] = read_parameter(bus, pin_cs, 0xCB, 1);
      write8(bus, pin_cs, 0xD9, 0x00);
      if (keys[0] == 0x01 && keys[1] == 0x1C) { return panel_variant_t::e; }
      if (!try_c_key) { return panel_variant_t::unknown; }

      bus.beginTransaction();
      lgfx::gpio_lo(pin_cs);
      bus.writeCommand(0xC8, 8);
      bus.writeData(0xFF, 8);
      bus.writeData(0x93, 8);
      bus.writeData(0x42, 8);
      bus.writeCommand(0x00, 8);
      bus.wait();
      lgfx::gpio_hi(pin_cs);
      bus.endTransaction();
      keys[2] = read_parameter(bus, pin_cs, 0xD3, 2);
      keys[3] = read_parameter(bus, pin_cs, 0xD3, 3);
      write8(bus, pin_cs, 0xD9, 0x00);
      return (keys[2] == (0x93 & 0x7F) && keys[3] == 0x42)
           ? panel_variant_t::c : panel_variant_t::unknown;
    }

    panel_variant_t identify_panel_variant(soft_spi_t& bus, int pin_cs,
                                           std::uint32_t keys[4], std::uint32_t poll_ms,
                                           bool try_c_key = true)
    {
      auto variant = probe_panel_variant(bus, pin_cs, keys, try_c_key);
      const auto started = lgfx::millis();
      while (variant == panel_variant_t::unknown && poll_ms)
      {
        lgfx::delay(1);
        variant = probe_panel_variant(bus, pin_cs, keys, try_c_key);
        if (lgfx::millis() - started >= poll_ms) { break; }
      }
      return variant;
    }

    void log_panel_variant(panel_variant_t variant, const std::uint32_t keys[4])
    {
      if (variant == panel_variant_t::e)
      {
        ESP_LOGI("board_detect_m5", "ILI9342 read-back DDh:%02x CBh:%02x -> ILI9342E",
                 static_cast<unsigned>(keys[0]), static_cast<unsigned>(keys[1]));
      }
      else if (variant == panel_variant_t::c)
      {
        ESP_LOGI("board_detect_m5", "ILI9342 read-back DDh:%02x CBh:%02x ID4:%02x%02x -> ILI9342C",
                 static_cast<unsigned>(keys[0]), static_cast<unsigned>(keys[1]),
                 static_cast<unsigned>(keys[2]), static_cast<unsigned>(keys[3]));
      }
      else
      {
        ESP_LOGW("board_detect_m5", "ILI9342 read-back DDh:%02x CBh:%02x ID4:%02x%02x -> neither key answered, ILI9342C assumed",
                 static_cast<unsigned>(keys[0]), static_cast<unsigned>(keys[1]),
                 static_cast<unsigned>(keys[2]), static_cast<unsigned>(keys[3]));
      }
    }

  }

}
}
}
