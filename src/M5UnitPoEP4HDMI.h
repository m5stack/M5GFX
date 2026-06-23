#ifndef __M5GFX_M5UNITPOEP4HDMI__
#define __M5GFX_M5UNITPOEP4HDMI__

#include "M5GFX.h"

#if __has_include(<sdkconfig.h>)
#include <sdkconfig.h>
#endif

#if defined(CONFIG_IDF_TARGET_ESP32P4)
#include <lgfx/v1/platforms/esp32p4/Bus_DSI.hpp>
#include <lgfx/v1/platforms/esp32p4/Panel_LT8912B.hpp>
#define M5UNITPOEP4HDMI_ENABLED
#endif

namespace m5
{
  class I2C_Class;
}

class M5UnitPoEP4HDMI : public M5GFX
{
public:
  struct config_t
  {
    // Supported timings: 1280x720@60 or 1920x1080@30 only.
    uint16_t width = 1280;
    uint16_t height = 720;
    uint8_t refresh_rate = 60;
    uint8_t fb_num = 1;

    uint8_t dsi_bus_id = 0;
    uint8_t dsi_lane_num = 2;
    uint16_t dsi_lane_mbps = 1000;
    uint8_t dsi_ldo_chan_id = 3;
    uint16_t dsi_ldo_voltage_mv = 2500;

    uint32_t i2c_freq = 100000;
    lgfx::color_depth_t output_depth = lgfx::rgb888_nonswapped;
  };

  M5UnitPoEP4HDMI(void) : M5UnitPoEP4HDMI(config_t{}) {}

  explicit M5UnitPoEP4HDMI(const config_t& cfg) : _config(cfg)
  {
    _board = lgfx::board_t::board_M5UnitPoEP4HDMI;
  }

  config_t config(void) const { return _config; }
  void config(const config_t& cfg) { _config = cfg; }

  void setI2C(m5::I2C_Class* i2c) { _i2c = i2c; }

  static bool isSupportedTiming(uint16_t width, uint16_t height, uint8_t refresh_rate)
  {
    return (width == 1280 && height == 720 && refresh_rate == 60)
        || (width == 1920 && height == 1080 && refresh_rate == 30);
  }

  bool init_impl(bool use_reset, bool use_clear) override
  {
    if (_panel_last.get() != nullptr) {
      return true;
    }

#if defined(M5UNITPOEP4HDMI_ENABLED)
    if (!isSupportedTiming(_config.width, _config.height, _config.refresh_rate)) {
#if defined(ESP_LOGE)
      ESP_LOGE("M5UnitPoEP4HDMI", "unsupported timing: %ux%u@%u. Supported timings: 1280x720@60, 1920x1080@30",
               static_cast<unsigned>(_config.width), static_cast<unsigned>(_config.height),
               static_cast<unsigned>(_config.refresh_rate));
#endif
      return false;
    }

    auto bus = new lgfx::Bus_DSI();
    auto panel = new lgfx::Panel_LT8912B();
    if (!bus || !panel) {
      delete panel;
      delete bus;
      return false;
    }

    {
      auto cfg = bus->config();
      cfg.bus_id = _config.dsi_bus_id;
      cfg.lane_num = _config.dsi_lane_num;
      cfg.lane_mbps = _config.dsi_lane_mbps;
      cfg.ldo_chan_id = _config.dsi_ldo_chan_id;
      cfg.ldo_voltage_mv = _config.dsi_ldo_voltage_mv;
      bus->config(cfg);
    }

    {
      auto cfg = panel->config_detail();
      cfg.h_res = _config.width;
      cfg.v_res = _config.height;
      cfg.refresh_rate = _config.refresh_rate;
      cfg.fb_num = _config.fb_num;
      cfg.lane_num = _config.dsi_lane_num;
      cfg.i2c = _i2c;
      cfg.i2c_freq = _config.i2c_freq;
      cfg.output_depth = _config.output_depth;
      panel->config_detail(cfg);
    }

    panel->setBus(bus);
    setPanel(panel);
    _bus_last.reset(bus);
    _panel_last.reset(panel);

    if (lgfx::LGFX_Device::init_impl(use_reset, use_clear)) {
      return true;
    }

    setPanel(nullptr);
    _panel_last.reset();
    _bus_last.reset();
#endif

    return false;
  }

protected:
  config_t _config;
  m5::I2C_Class* _i2c = nullptr;
};

#endif
