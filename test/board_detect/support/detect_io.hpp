// In-memory detection I/O; independent of the firmware's device implementation.
#pragma once
#include <cstdlib>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <vector>
#include <map>
#include "lgfx/boards.hpp"
namespace host_hardware {
struct event_t {
  int kind, pin;
  unsigned value;
  bool operator==(const event_t &b) const {
    return kind == b.kind && pin == b.pin && value == b.value;
  }
};
struct pin_t {
  int mode = 0;
  bool high = false;
  bool operator==(const pin_t &b) const { return mode == b.mode && high == b.high; }
};
static pin_t gpio[64];
static std::vector<event_t> events;
static std::uint32_t now_us;
static std::function<bool(int, int, bool)> input;
static std::function<bool(int, int, std::uint8_t *, std::size_t, bool)> wire;
static std::map<int, std::pair<int, int>> ports;
static int flash_capacity = 0, psram_capacity = 0, package = 0;
static unsigned warnings;
static bool i2c_open_ok = true;
static bool psram_active;
template <class... Args> void log(Args...) { ++warnings; }
template <class... Args> void quiet_log(Args...) {}
inline void reset() {
  for (auto &pin : gpio)
    pin = {};
  events.clear();
  now_us = 0;
  input = {};
  wire = {};
  ports.clear();
  warnings = 0;
  i2c_open_ok = true;
}
inline unsigned reg(int which) {
  if (which == 100)
    return unsigned(flash_capacity) | (unsigned(psram_capacity) << 3);
  unsigned value = 0;
  for (int bit = 0; bit < 32; ++bit) {
    auto &p = gpio[(which & 1) * 32 + bit];
    if (which < 2 ? p.mode == 3 : p.high)
      value |= 1u << bit;
  }
  return value;
}
struct response_t {
  bool ok;
  int data;
  bool has_value() const { return ok; }
  int value() const { return data; }
  int value_or(int fallback) const { return ok ? data : fallback; }
};
} // namespace host_hardware

namespace host_touch {
static std::uint16_t t4 = 100, t7 = 25;
static bool available = true;
static unsigned scans, clears;
}
namespace host_chip { static int revision; }
namespace m5gfx { namespace board_detect { namespace io {
  static constexpr int pin_count = 64;
  static constexpr int i2c_port_0 = 0, i2c_port_1 = 1, low_power_i2c_port_0 = 0;
  static constexpr unsigned package_classic_q6 = 0, package_classic_pico_d4 = 5;
  inline bool pin_valid(int pin) { return pin >= 0 && pin < pin_count; }
  inline int host_mode(pin_mode_t mode)
  {
    switch (mode) {
      case pin_mode_t::input: return 0;
      case pin_mode_t::input_pullup: return 1;
      case pin_mode_t::input_pulldown: return 2;
      default: return 3;
    }
  }
  inline void pin_mode(int pin, pin_mode_t mode)
  {
    if (pin < 0) return;
    const auto value = host_mode(mode);
    host_hardware::gpio[pin].mode = value;
    host_hardware::events.push_back({0, pin, unsigned(value)});
  }
  inline void pin_low(int pin)
  {
    if (pin >= 0) {
      host_hardware::gpio[pin].high = false;
      host_hardware::events.push_back({4, pin, 0});
    }
  }
  inline void pin_high(int pin)
  {
    if (pin >= 0) {
      host_hardware::gpio[pin].high = true;
      host_hardware::events.push_back({4, pin, 1});
    }
  }
  inline bool pin_read(int pin)
  {
    assert(pin >= 0);
    auto &p = host_hardware::gpio[pin];
    const bool high = host_hardware::input ? host_hardware::input(pin, p.mode, p.high) : p.high;
    host_hardware::events.push_back({2, pin, unsigned(high)});
    return high;
  }
  inline bool pin_output_level(int pin, bool* level)
  {
    *level = host_hardware::gpio[pin].high;
    return host_hardware::gpio[pin].mode == 3;
  }
  class pin_state_t {
    int pin_ = -1;
    host_hardware::pin_t saved_;
  public:
    int pin() const { return pin_; }
    void save(int pin) { pin_ = pin; saved_ = host_hardware::gpio[pin]; }
    bool matches_current() const { return saved_ == host_hardware::gpio[pin_]; }
    void restore() {
      host_hardware::gpio[pin_] = saved_;
      host_hardware::events.push_back({3, pin_, 0});
    }
  };
  inline void release_lp_pad(int pin) { host_hardware::events.push_back({5, pin, 0}); }
  inline volatile std::uint32_t* pin_low_register(int) { static std::uint32_t r; return &r; }
  inline volatile std::uint32_t* pin_high_register(int) { static std::uint32_t r; return &r; }
  inline std::uint32_t cpu_cycles() { return host_hardware::now_us * 240; }
  inline std::uint32_t cpu_hz() { return 240000000; }
  inline std::uint32_t millis() { return host_hardware::now_us / 1000; }
  inline std::uint32_t micros() { return host_hardware::now_us; }
  inline void delay_us(std::uint32_t us) {
    host_hardware::now_us += us;
    host_hardware::events.push_back({1, -1, us});
  }
  inline void delay_ms(std::uint32_t ms) { delay_us(ms * 1000); }
  inline void delay_rom_us(std::uint32_t us) { delay_us(us); }
  inline bool i2c_open(int port, int sda, int scl) {
    if (!host_hardware::i2c_open_ok) return false;
    host_hardware::events.push_back({7, sda, 0});
    host_hardware::events.push_back({7, scl, 0});
    host_hardware::ports[port] = {sda, scl};
    return true;
  }
  inline void i2c_close(int port) {
    host_hardware::ports.erase(port);
    host_hardware::events.push_back({6, port, 0});
  }
  inline bool i2c_ack(int, std::uint16_t addr, std::uint32_t, bool = false) {
    std::uint8_t data = 0;
    return host_hardware::wire && host_hardware::wire(addr, -1, &data, 0, false);
  }
  inline bool i2c_read_reg8(int, std::uint16_t addr, std::uint8_t reg,
                           std::uint8_t* value, std::uint32_t) {
    std::uint8_t data = 0;
    const bool ok = host_hardware::wire && host_hardware::wire(addr, reg, &data, 1, false);
    if (ok) *value = data;
    return ok;
  }
  inline bool i2c_write_reg8(int, std::uint16_t addr, std::uint8_t reg,
                            std::uint8_t value, std::uint32_t) {
    return host_hardware::wire && host_hardware::wire(addr, reg, &value, 1, true);
  }
  inline bool i2c_write(int, std::uint16_t addr, const std::uint8_t* data,
                        std::size_t len, std::uint32_t) {
    return host_hardware::wire && host_hardware::wire(addr, len ? data[0] : -1,
      const_cast<std::uint8_t*>(data + 1), len ? len - 1 : 0, true);
  }
  inline bool i2c_write_read(int, std::uint16_t addr, const std::uint8_t* reg,
                             std::size_t reglen, std::uint8_t* data, std::size_t len,
                             std::uint32_t) {
    return host_hardware::wire && host_hardware::wire(addr, reglen ? reg[0] : -1, data, len, false);
  }
  inline unsigned package_version() { return host_hardware::package; }
  inline bool psram_ready() { return host_hardware::psram_active; }
  inline unsigned chip_revision() { return host_chip::revision; }
  inline unsigned flash_capacity_code() { return host_hardware::flash_capacity; }
  inline void module_memory_capacity(unsigned* flash, unsigned* psram)
  { *flash = host_hardware::flash_capacity; *psram = host_hardware::psram_capacity; }
  inline std::uint8_t st_touch_address() { return 0x55; }
  inline std::uint8_t gt911_touch_address() { return 0x14; }
  inline unsigned usb_serial_jtag_frame() { return host_hardware::flash_capacity; }
  inline bool uart_rx_held_high() { pin_mode(20, pin_mode_t::input_pulldown); return pin_read(20); }
  static constexpr bool dedicated_gpio_supported = false;
  inline std::uint32_t dedicated_release_read_in() { return 0; }
  inline void dedicated_release_write_mask(std::uint32_t, std::uint32_t) {}
  inline void dedicated_release_channel_masks(dedicated_gpio_handle_t, std::uint8_t,
    std::uint32_t*, std::uint32_t*, int*, int*) {}
  inline int dedicated_release_core_id() { return 0; }
  inline std::uint32_t dedicated_release_cycle_count() { return 0; }
  inline int dedicated_release_task_core() { return 0; }
  inline int no_core_affinity() { return -1; }
  inline bool dedicated_gpio_in_use() { return false; }
  inline void pin_weak_pullup_open_drain(int) {}
  inline int dedicated_gpio_open(const int*, std::uint8_t, dedicated_gpio_handle_t*) { return -1; }
  inline int dedicated_gpio_close(dedicated_gpio_handle_t) { return -1; }
  class critical_section_t { public: void enter() {} void exit() {} };
  template <int channel_a, int pin_a, int channel_b, int pin_b, std::uint32_t timeout_us>
  inline bool classic_touch_measure(std::uint16_t* a, std::uint16_t* b) {
    ++host_touch::scans;
    for (const auto pin : {pin_a, pin_b}) host_hardware::events.push_back({0, pin, 0});
    *a = host_touch::t4; *b = host_touch::t7;
    return host_touch::available;
  }
  template <int pin, std::uint16_t pixels>
  inline void classic_led_clear() {
    ++host_touch::clears;
    host_hardware::events.push_back({0, pin, 3});
    host_hardware::events.push_back({4, pin, 0});
  }
}}}
