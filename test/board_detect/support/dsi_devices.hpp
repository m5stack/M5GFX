// DSI transport/panel boundary; production setup descriptors and owners stay real.
#pragma once
#include "display_devices.hpp"
namespace host_dsi {
static bool ili9881;
static unsigned id_reads, page_writes;
inline void reset(bool answers) {
  ili9881 = answers;
  id_reads = page_writes = 0;
}
}
namespace lgfx {
struct Bus_DSI : IBus {
  struct config_t {
    int bus_id = 0, lane_num = 0, lane_mbps = 0, ldo_chan_id = 0, ldo_voltage_mv = 0;
  };
  config_t cfg;
  config_t config() const { return cfg; }
  void config(const config_t &value) { cfg = value; }
  bool init() { return true; }
  void writeParams(std::uint8_t reg, const std::uint8_t *, std::size_t) {
    assert(reg == 0xFF);
    ++host_dsi::page_writes;
  }
  void readParams(std::uint8_t reg, std::uint8_t *data, std::size_t len) {
    assert(reg <= 2 && len == 1);
    ++host_dsi::id_reads;
    *data = host_dsi::ili9881 ? (reg == 0 ? 0x98 : reg == 1 ? 0x81 : 0) : 0;
  }
};
struct DsiPanel : Panel_Device {
  struct detail_t {
    int dpi_freq_mhz = 0, hsync_back_porch = 0, hsync_pulse_width = 0,
        hsync_front_porch = 0, vsync_back_porch = 0, vsync_pulse_width = 0,
        vsync_front_porch = 0;
  };
  detail_t detail;
  detail_t config_detail() const { return detail; }
  void config_detail(const detail_t &value) { detail = value; }
};
struct Panel_ST7102 : DsiPanel {};
struct Touch_CST3530 : ITouch {};
struct Panel_ILI9881C : DsiPanel { int kind() const override { return 1; } };
struct Panel_ST7121 : DsiPanel { int kind() const override { return 2; } };
struct Panel_ST7123 : DsiPanel { int kind() const override { return 3; } };
struct Touch_ST7123 : ITouch {
  static constexpr std::uint8_t default_addr = 0x55;
  int kind() const override { return 4; }
};
}

namespace m5gfx {
struct Light_M5CoreP4X : lgfx::ILight {
  template <class... Args> Light_M5CoreP4X(Args...) {}
};
}
