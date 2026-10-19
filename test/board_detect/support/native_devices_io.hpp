// Native device calls used only by construction fixtures and test setup.
#pragma once
#include "board_detect/detect_io.hpp"
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

#define ESP_LOGE(...) host_hardware::log(__VA_ARGS__)
#define ESP_LOGW(...) host_hardware::log(__VA_ARGS__)
#define ESP_LOGI(...) host_hardware::quiet_log(__VA_ARGS__)
#define ESP_LOGD(...) host_hardware::quiet_log(__VA_ARGS__)
#define ESP_IDF_VERSION_VAL(a, b, c) (((a) << 16) | ((b) << 8) | (c))
#define ESP_IDF_VERSION ESP_IDF_VERSION_VAL(5, 5, 0)
namespace lgfx {
  enum pin_mode_t { input, input_pullup, input_pulldown, output, output_open_drain };
  inline void pinMode(int pin, pin_mode_t mode) {
    using m5gfx::board_detect::io::pin_mode_t;
    const auto value = mode == input ? pin_mode_t::input : mode == input_pullup ? pin_mode_t::input_pullup
                     : mode == input_pulldown ? pin_mode_t::input_pulldown : pin_mode_t::output;
    m5gfx::board_detect::io::pin_mode(pin, value);
  }
  inline void gpio_lo(int pin) { m5gfx::board_detect::io::pin_low(pin); }
  inline void gpio_hi(int pin) { m5gfx::board_detect::io::pin_high(pin); }
  inline bool gpio_in(int pin) { return m5gfx::board_detect::io::pin_read(pin); }
  inline void delay(unsigned long ms) { m5gfx::board_detect::io::delay_ms(ms); }
  inline void delayMicroseconds(unsigned us) { m5gfx::board_detect::io::delay_us(us); }
  inline std::uint32_t millis() { return m5gfx::board_detect::io::millis(); }
  inline std::uint32_t micros() { return m5gfx::board_detect::io::micros(); }
namespace i2c {
  using host_hardware::response_t;
  inline bool isInitialized(int port) { return host_hardware::ports.count(port) != 0; }
  inline response_t init(int port, int sda, int scl)
  { return {m5gfx::board_detect::io::i2c_open(port, sda, scl), 0}; }
  inline response_t release(int port) { m5gfx::board_detect::io::i2c_close(port); return {true,0}; }
  inline response_t readRegister8(int port, int addr, int reg, int freq) {
    std::uint8_t value = 0;
    const bool ok = m5gfx::board_detect::io::i2c_read_reg8(port, addr, reg, &value, freq);
    return {ok,value};
  }
  inline response_t writeRegister8(int port, int addr, int reg, int value, int, int freq)
  { return {m5gfx::board_detect::io::i2c_write_reg8(port, addr, reg, value, freq),0}; }
  inline response_t beginTransaction(int port, int addr, int freq, bool)
  { return {m5gfx::board_detect::io::i2c_ack(port, addr, freq),0}; }
  inline response_t endTransaction(int) { return {true,0}; }
  inline response_t transactionWrite(int port, int addr, const std::uint8_t* data, std::size_t len, int freq)
  { return {m5gfx::board_detect::io::i2c_write(port,addr,data,len,freq),0}; }
  inline response_t transactionWriteRead(int port, int addr, const std::uint8_t* reg, std::size_t rlen,
                                        std::uint8_t* data, std::size_t len, int freq)
  { return {m5gfx::board_detect::io::i2c_write_read(port,addr,reg,rlen,data,len,freq),0}; }
}
}

namespace m5gfx { using lgfx::board_t; }
