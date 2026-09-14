#ifndef __M5GFX_M5UNITPOEP4HDMI__
#define __M5GFX_M5UNITPOEP4HDMI__

#include "M5GFX.h"

#include <cstddef>
#include <cstdint>
#include <functional>

#if __has_include(<sdkconfig.h>)
#include <sdkconfig.h>
#endif

#if defined(CONFIG_IDF_TARGET_ESP32P4)
#include <lgfx/v1/platforms/esp32p4/Bus_DSI.hpp>
#include <lgfx/v1/platforms/esp32p4/Panel_LT8912B.hpp>
#define M5UNITPOEP4HDMI_ENABLED
#endif

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

    // I2C bus of the LT8912B bridge (lgfx::i2c). Negative = the board's internal bus
    // (port 1, SDA 0, SCL 1); M5Unified fills these from In_I2C. An already open port
    // is shared as it is. Alternatively an ESP-IDF driver bus the application owns.
    int8_t i2c_port = -1;
    int8_t pin_sda = -1;
    int8_t pin_scl = -1;
    void* i2c_master_bus = nullptr; // i2c_master_bus_handle_t
  };

  M5UnitPoEP4HDMI(void) : M5UnitPoEP4HDMI(config_t{}) {}

  explicit M5UnitPoEP4HDMI(const config_t& cfg) : _config(cfg)
  {
    _board = lgfx::board_t::board_M5UnitPoEP4HDMI;
  }

  config_t config(void) const { return _config; }
  // A config set here is final: anything a previous setI2C() captured is dropped.
  void config(const config_t& cfg) { _config = cfg; _i2c_resolver = nullptr; }

  // Kept for sketches that hand over an M5Unified I2C_Class (or anything with
  // getPort / getSDA / getSCL returning small non-negative numbers). As before only
  // a pointer is kept and the object is read when the display is initialized, so it
  // has to stay alive until init(). While set it selects the bus, ahead of
  // i2c_master_bus and of the port / pins in config_t, which are left untouched;
  // config() or setI2C(nullptr) drops it again (pass nullptr, not 0 or NULL). A
  // template so that this header needs nothing from M5Unified; the lambda body
  // resolves at the call site, so the class has to be complete there (it is,
  // wherever the object was created).
  template <typename I2C>
  void setI2C(I2C* i2c)
  {
    if (i2c == nullptr) { _i2c_resolver = nullptr; return; }
    _i2c_resolver = [i2c](config_t& cfg) -> bool
    {
      const int port = i2c->getPort();
      const int sda = i2c->getSDA();
      const int scl = i2c->getSCL();
      // not begun, or out of the range the config can hold: fail as the old code did
      if (port < 0 || sda < 0 || scl < 0 || port > INT8_MAX || sda > INT8_MAX || scl > INT8_MAX) { return false; }
      cfg.i2c_port = static_cast<int8_t>(port);
      cfg.pin_sda = static_cast<int8_t>(sda);
      cfg.pin_scl = static_cast<int8_t>(scl);
      cfg.i2c_master_bus = nullptr;
      return true;
    };
  }
  void setI2C(std::nullptr_t) { _i2c_resolver = nullptr; }

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
    config_t used = _config; // what setI2C() resolves goes here, so _config stays as the sketch set it
    if (_i2c_resolver && !_i2c_resolver(used)) {
#if defined(ESP_LOGE)
      ESP_LOGE("M5UnitPoEP4HDMI", "the I2C object given to setI2C() is not started");
#endif
      return false;
    }
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
      if (used.i2c_port >= 0) { cfg.i2c_port = used.i2c_port; }
      if (used.pin_sda >= 0) { cfg.i2c_sda = used.pin_sda; }
      if (used.pin_scl >= 0) { cfg.i2c_scl = used.pin_scl; }
      cfg.i2c_master_bus = static_cast<i2c_master_bus_handle_t>(used.i2c_master_bus);
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
  std::function<bool(config_t&)> _i2c_resolver; // false = the object handed to setI2C() is unusable
};

#endif
