// Physical display devices: retain the complete production description/owner types.
#pragma once
#include <memory>
using spi_host_device_t = int;
namespace lgfx {
struct device_config_t {
  int freq_write = 0, freq_read = 0, spi_mode = 0, spi_3wire = 0, use_lock = 0, spi_host = 0,
      dma_channel = 0;
  int pin_sclk = -1, pin_mosi = -1, pin_miso = -1, pin_dc = -1, pin_io0 = -1, pin_io1 = -1,
      pin_io2 = -1, pin_io3 = -1;
  int i2c_port = 0, pin_sda = -1, pin_scl = -1, i2c_addr = 0, prefix_len = 0;
  int panel_width = 0, panel_height = 0, memory_width = 0, memory_height = 0, offset_x = 0,
      offset_y = 0;
  int pin_cs = -1, pin_rst = -1, pin_busy = -1, offset_rotation = 0, invert = 0, readable = 0,
      bus_shared = 0, rgb_order = 0;
  int pin_int = -1, freq = 0, x_min = 0, x_max = 0, y_min = 0, y_max = 0, pin_bl = -1,
      pwm_channel = 0, offset = 0;
};
struct configured_t {
  device_config_t cfg;
  device_config_t config() const { return cfg; }
  void config(const device_config_t &value) { cfg = value; }
};
struct IBus : configured_t {
  virtual ~IBus() {}
  virtual void release() {}
};
struct Bus_SPI : IBus {};
struct Bus_I2C : IBus {};
struct ILight : configured_t {
  virtual ~ILight() {}
  virtual int kind() const { return 0; }
};
struct Light_PWM : ILight {};
struct ITouch : configured_t {
  virtual ~ITouch() {}
  virtual int kind() const { return 0; }
};
struct Touch_CHSC6540 : ITouch {
  int kind() const override { return 1; }
};
struct Touch_FT5x06 : ITouch {
  int kind() const override { return 2; }
};
struct Touch_GT911 : ITouch {
  static constexpr std::uint8_t default_addr_1 = 0x14;
  int kind() const override { return 3; }
};
struct Panel_Device : configured_t {
  IBus *attached = nullptr;
  ITouch *attached_touch = nullptr;
  int reset_reg = -1;
  virtual ~Panel_Device() {}
  virtual int kind() const { return 0; }
  void setRotation(int) {}
  void bus(IBus *b) { attached = b; }
  void touch(ITouch *t) { attached_touch = t; }
  void setTouch(ITouch *t) { touch(t); }
  void setCalibrateAffine(float *) {}
  void setResetReg(int reg) { reset_reg = reg; }
};
struct Panel_IT8951 : Panel_Device {};
struct Panel_GDEW0154M09 : Panel_Device {};
struct Panel_GDEW0154D67 : Panel_Device {};
} // namespace lgfx
namespace m5gfx {
struct Panel_M5StickC : lgfx::Panel_Device {};
struct Panel_M5StickCPlus : lgfx::Panel_Device {};
struct Panel_M5Stack : lgfx::Panel_Device {};
struct Panel_M5StackCore2 : lgfx::Panel_Device {
  int kind() const override { return 1; }
};
struct Panel_M5StackCore2E : lgfx::Panel_Device {
  int kind() const override { return 2; }
};
struct Light_M5Tough : lgfx::ILight {
  int kind() const override { return 2; }
};
struct Light_M5StackCore2 : lgfx::ILight {};
struct Light_M5StackCore2_AXP2101 : lgfx::ILight {
  int kind() const override { return 1; }
};
struct Light_M5StickC : lgfx::ILight {
  template <class... Args> Light_M5StickC(Args...) {}
};
inline void _set_ili9342e_read(lgfx::Panel_Device *, std::uint32_t) {}
} // namespace m5gfx
