// Host hardware boundary. Production descriptions and detector types are included unchanged.
#pragma once
#include <cstdlib>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <vector>
#include <map>
#include "lgfx/boards.hpp"

enum {
  I2C_NUM_0 = 0,
  I2C_NUM_1 = 1,
  SPI2_HOST = 1,
  SPI3_HOST = 2,
  GPIO_NUM_MAX = 64,
  GPIO_NUM_NC = -1,
  GPIO_NUM_0 = 0,
  GPIO_NUM_1 = 1,
  GPIO_NUM_2 = 2,
  GPIO_NUM_3 = 3,
  GPIO_NUM_4 = 4,
  GPIO_NUM_5 = 5,
  GPIO_NUM_6 = 6,
  GPIO_NUM_7 = 7,
  GPIO_NUM_8 = 8,
  GPIO_NUM_9 = 9,
  GPIO_NUM_10 = 10,
  GPIO_NUM_11 = 11,
  GPIO_NUM_12 = 12,
  GPIO_NUM_13 = 13,
  GPIO_NUM_14 = 14,
  GPIO_NUM_15 = 15,
  GPIO_NUM_16 = 16,
  GPIO_NUM_17 = 17,
  GPIO_NUM_18 = 18,
  GPIO_NUM_19 = 19,
  GPIO_NUM_20 = 20,
  GPIO_NUM_21 = 21,
  GPIO_NUM_22 = 22,
  GPIO_NUM_23 = 23,
  GPIO_NUM_24 = 24,
  GPIO_NUM_25 = 25,
  GPIO_NUM_26 = 26,
  GPIO_NUM_27 = 27,
  GPIO_NUM_28 = 28,
  GPIO_NUM_29 = 29,
  GPIO_NUM_30 = 30,
  GPIO_NUM_31 = 31,
  GPIO_NUM_32 = 32,
  GPIO_NUM_33 = 33,
  GPIO_NUM_34 = 34,
  GPIO_NUM_35 = 35,
  GPIO_NUM_36 = 36,
  GPIO_NUM_37 = 37,
  GPIO_NUM_38 = 38,
  GPIO_NUM_39 = 39,
  GPIO_NUM_40 = 40,
  GPIO_NUM_41 = 41,
  GPIO_NUM_42 = 42,
  GPIO_NUM_43 = 43,
  GPIO_NUM_44 = 44,
  GPIO_NUM_45 = 45,
  GPIO_NUM_46 = 46,
  GPIO_NUM_47 = 47,
  GPIO_NUM_48 = 48,
  GPIO_NUM_49 = 49,
  GPIO_NUM_50 = 50,
  GPIO_NUM_51 = 51,
  GPIO_NUM_52 = 52,
  GPIO_NUM_53 = 53,
  GPIO_NUM_54 = 54,
  GPIO_NUM_55 = 55,
  GPIO_NUM_56 = 56,
  GPIO_NUM_57 = 57,
  GPIO_NUM_58 = 58,
  GPIO_NUM_59 = 59,
  GPIO_NUM_60 = 60,
  GPIO_NUM_61 = 61,
  GPIO_NUM_62 = 62,
  GPIO_NUM_63 = 63
};
#define GPIO_IS_VALID_GPIO(pin) ((pin) >= 0 && (pin) < GPIO_NUM_MAX)
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
#define ESP_LOGE(...) host_hardware::log(__VA_ARGS__)
#define ESP_LOGW(...) host_hardware::log(__VA_ARGS__)
#define ESP_LOGI(...) host_hardware::quiet_log(__VA_ARGS__)
#define ESP_LOGD(...) host_hardware::quiet_log(__VA_ARGS__)
#define REG_GET_FIELD(...) host_hardware::flash_capacity
#define REG_READ(register_index) host_hardware::reg(register_index)
#define GPIO_ENABLE_REG 0
#define GPIO_ENABLE1_REG 1
#define GPIO_OUT_REG 2
#define GPIO_OUT1_REG 3
namespace lgfx {
enum pin_mode_t {
  input = 0,
  input_pullup = 1,
  input_pulldown = 2,
  output = 3,
  output_open_drain = 4
};
inline void pinMode(int pin, pin_mode_t mode) {
  if (pin < 0)
    return;
  host_hardware::gpio[pin].mode = mode;
  host_hardware::events.push_back({0, pin, unsigned(mode)});
}
inline void gpio_lo(int pin) {
  if (pin >= 0) {
    host_hardware::gpio[pin].high = false;
    host_hardware::events.push_back({4, pin, 0});
  }
}
inline void gpio_hi(int pin) {
  if (pin >= 0) {
    host_hardware::gpio[pin].high = true;
    host_hardware::events.push_back({4, pin, 1});
  }
}
inline bool gpio_in(int pin) {
  assert(pin >= 0);
  auto &p = host_hardware::gpio[pin];
  bool high = host_hardware::input ? host_hardware::input(pin, p.mode, p.high) : p.high;
  host_hardware::events.push_back({2, pin, unsigned(high)});
  return high;
}
inline std::uint32_t millis() { return host_hardware::now_us / 1000; }
inline std::uint32_t micros() { return host_hardware::now_us; }
inline void delay(unsigned long ms) {
  host_hardware::now_us += ms * 1000;
  host_hardware::events.push_back({1, -1, unsigned(ms * 1000)});
}
inline void delayMicroseconds(unsigned us) {
  host_hardware::now_us += us;
  host_hardware::events.push_back({1, -1, us});
}
namespace gpio {
struct pin_backup_t {
  int pin = -1;
  host_hardware::pin_t saved;
  int getPin() const { return pin; }
  void setPin(int p) { pin = p; }
  void backup() { saved = host_hardware::gpio[pin]; }
  bool matches_current() const { return saved == host_hardware::gpio[pin]; }
  void restore() {
    host_hardware::gpio[pin] = saved;
    host_hardware::events.push_back({3, pin, 0});
  }
};
inline void release_lp_pad(int pin) { host_hardware::events.push_back({5, pin, 0}); }
} // namespace gpio
namespace i2c {
using host_hardware::response_t;
inline bool isInitialized(int port) { return host_hardware::ports.count(port) != 0; }
inline response_t init(int port, int sda, int scl) {
  if (!host_hardware::i2c_open_ok)
    return {false, 0};
  host_hardware::events.push_back({7, sda, 0});
  host_hardware::events.push_back({7, scl, 0});
  host_hardware::ports[port] = {sda, scl};
  return {true, 0};
}
inline response_t release(int port) {
  host_hardware::ports.erase(port);
  host_hardware::events.push_back({6, port, 0});
  return {true, 0};
}
inline response_t getPinSDA(int port) {
  return {isInitialized(port), host_hardware::ports[port].first};
}
inline response_t getPinSCL(int port) {
  return {isInitialized(port), host_hardware::ports[port].second};
}
inline response_t readRegister8(int, int addr, int reg, int) {
  std::uint8_t data = 0;
  bool ok = host_hardware::wire && host_hardware::wire(addr, reg, &data, 1, false);
  return {ok, data};
}
inline response_t writeRegister8(int, int addr, int reg, int value, int, int) {
  std::uint8_t data = value;
  bool ok = host_hardware::wire && host_hardware::wire(addr, reg, &data, 1, true);
  return {ok, 0};
}
inline response_t beginTransaction(int, int addr, int, bool) {
  std::uint8_t data = 0;
  return {host_hardware::wire && host_hardware::wire(addr, -1, &data, 0, false), 0};
}
inline response_t endTransaction(int) { return {true, 0}; }
inline response_t transactionWrite(int, int addr, const std::uint8_t *data, std::size_t len, int) {
  return {host_hardware::wire &&
              host_hardware::wire(addr, len ? data[0] : -1, const_cast<std::uint8_t *>(data + 1),
                                  len ? len - 1 : 0, true),
          0};
}
inline response_t transactionWriteRead(int, int addr, const std::uint8_t *reg, std::size_t reglen,
                                       std::uint8_t *data, std::size_t len, int) {
  return {host_hardware::wire && host_hardware::wire(addr, reglen ? reg[0] : -1, data, len, false),
          0};
}
} // namespace i2c
} // namespace lgfx

namespace lgfx {
inline volatile std::uint32_t *get_gpio_lo_reg(int) {
  static std::uint32_t reg;
  return &reg;
}
inline volatile std::uint32_t *get_gpio_hi_reg(int) {
  static std::uint32_t reg;
  return &reg;
}
} // namespace lgfx
namespace m5gfx {
using namespace lgfx;
}

#define EFUSE_RD_MAC_SYS2_REG 100
#define EFUSE_FLASH_CAP_S 0
#define EFUSE_FLASH_CAP_V 7
#define EFUSE_PSRAM_CAP_S 3
#define EFUSE_PSRAM_CAP_V 3
#define ESP_IDF_VERSION_VAL(a, b, c) (((a) << 16) | ((b) << 8) | (c))
#define ESP_IDF_VERSION ESP_IDF_VERSION_VAL(5, 5, 0)
