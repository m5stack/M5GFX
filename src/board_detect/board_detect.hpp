// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>

#include "../lgfx/v1/platforms/esp32/common.hpp"

namespace m5gfx
{
namespace board_detect
{
  using board_id_t = std::uint16_t;
  static constexpr board_id_t board_id_unknown = 0;

  enum board_def_flag_t : std::uint8_t
  {
    // A board with no positive signature. Such a board must be the final member
    // of the final detector; the framework preserves the caller's ordering.
    def_flag_fallback = 1 << 0,
  };

  struct board_def_t
  {
    board_id_t id;
    const char* name;
    std::uint8_t flags;
  };

#if __cplusplus >= 201703L
  inline const board_def_t board_def_unknown = { board_id_unknown, "unknown", 0 };
#else
  // Arduino-ESP32 2.x still compiles as C++11. Internal linkage provides the
  // same ODR safety there; C++17 and newer use the single inline definition.
  static const board_def_t board_def_unknown = { board_id_unknown, "unknown", 0 };
#endif

  enum prepared_state_t : std::uint32_t
  {
    // Bits 0..15 are manufacturer-independent completed operations or probe
    // side effects. Bits 16..31 are reserved for board-specific state.
    prepared_power  = 1u << 0,
    prepared_reset  = 1u << 1,
    prepared_sd_spi = 1u << 2,
    panel_dirty     = 1u << 3,
    // The GPIO reset line has been driven inactive, but a reset pulse may not
    // have been allowed. Keep this distinct from prepared_reset so a later
    // reset-enabled prepare can still pulse the panel reset.
    prepared_reset_line = 1u << 4,
  };

  enum class detect_status_t : std::uint8_t
  {
    no_match,
    matched,
    excluded,
  };

  struct board_desc_t;
  class detection_transaction_t;

  struct board_result_t
  {
    // A matched/direct result owns no description; this points at the static
    // canonical description and def aliases &desc->def. Unknown results have
    // desc == nullptr and retain board_def_unknown for compatibility.
    const board_desc_t* desc = nullptr;
    const board_def_t* def = &board_def_unknown;
    std::uint32_t option = 0;
    std::uint32_t prepared = 0;
    detect_status_t status = detect_status_t::no_match;

    void assign(const board_desc_t* value);
  };

  struct prepare_ctx_t
  {
    bool allow_reset = true;
    int i2c_port_probe = -1;
    // Non-null while board-detection components are running.
    detection_transaction_t* transaction = nullptr;
  };

  struct pmic_write_t
  {
    std::uint8_t addr;
    std::uint8_t reg;
    std::uint8_t value;
    std::uint8_t mask;
  };

  struct reg_bit_t
  {
    std::uint8_t reg;
    std::uint8_t mask;
  };

  // Build lists only with sequence(), registers(), pins() and options(), which
  // take the size from the array itself. A hand-written size cannot be verified.
  template <typename T>
  struct list_desc_t
  {
    const T* data;
    std::uint8_t size;
  };

  using pmic_sequence_t = list_desc_t<pmic_write_t>;
  using register_list_t = list_desc_t<std::uint8_t>;
  using pin_list_t = list_desc_t<std::int8_t>;
  using option_list_t = list_desc_t<const char*>;
  static constexpr std::uint8_t max_pmic_restore_registers = 6;

  template <std::size_t N>
  struct list_size_t
  {
    static_assert(N <= 255, "description list is too large");
    static constexpr std::uint8_t value = static_cast<std::uint8_t>(N);
  };

  template <std::size_t N>
  constexpr pmic_sequence_t sequence(const pmic_write_t (&data)[N])
  {
    return { data, list_size_t<N>::value };
  }

  constexpr pmic_sequence_t no_sequence() { return { nullptr, 0 }; }

  template <std::size_t N>
  constexpr register_list_t registers(const std::uint8_t (&data)[N])
  {
    return { data, list_size_t<N>::value };
  }

  template <std::size_t N>
  constexpr pin_list_t pins(const std::int8_t (&data)[N])
  {
    return { data, list_size_t<N>::value };
  }

  template <std::size_t N>
  constexpr option_list_t options(const char* const (&data)[N])
  {
    return { data, list_size_t<N>::value };
  }

  constexpr option_list_t no_options() { return { nullptr, 0 }; }
  constexpr pin_list_t no_pins() { return { nullptr, 0 }; }

  static constexpr std::uint8_t max_detection_pins = 40;

  class detection_gpio_snapshot_t
  {
  public:
    bool capture(pin_list_t unconditional, pin_list_t conditional,
                 bool include_conditional);
    void restore_start(const std::int8_t* pins, std::size_t count);
    void rollback();

  private:
    lgfx::gpio::pin_backup_t saved_[max_detection_pins];
    std::uint8_t count_ = 0;
  };

  // External PMIC/IO-expander register restoration is added by the next stage.
  // Keeping it as a participant now fixes its place before bus and GPIO rollback.
  class detection_external_journal_t
  {
  public:
    void rollback() {}
    void commit() {}
  };

  class detection_bus_journal_t
  {
  public:
    void opened_i2c(int port);
    void rollback();
    void commit();

  private:
    std::int8_t i2c_ports_[2] = { -1, -1 };
    std::uint8_t i2c_count_ = 0;
  };

  class detection_irreversible_journal_t
  {
  public:
    void declare_prepared(std::uint32_t prepared)
    {
      operations_ |= prepared & (prepared_sd_spi | panel_dirty);
    }
    std::uint32_t operations() const { return operations_; }

  private:
    std::uint32_t operations_ = 0;
  };

  class detection_transaction_t
  {
  public:
    detection_transaction_t(pin_list_t unconditional, pin_list_t conditional,
                            bool include_conditional);
    ~detection_transaction_t();

    bool valid() const { return valid_; }
    void restore_start(const std::int8_t* pins, std::size_t count) { gpio_.restore_start(pins, count); }
    void restore_start(std::int8_t pin) { restore_start(&pin, 1); }
    void restore_start(std::initializer_list<int> pins);
    template <std::size_t N>
    void restore_start(const std::int8_t (&pins)[N]) { restore_start(pins, N); }
    void rollback();
    void commit();
    detection_bus_journal_t& buses() { return buses_; }
    detection_external_journal_t& external() { return external_; }
    detection_irreversible_journal_t& irreversible() { return irreversible_; }

  private:
    detection_gpio_snapshot_t gpio_;
    detection_external_journal_t external_;
    detection_bus_journal_t buses_;
    detection_irreversible_journal_t irreversible_;
    bool valid_ = false;
    bool committed_ = false;
  };

  struct pmic_variant_t
  {
    std::uint8_t i2c_addr;
    std::uint8_t id_reg;
    std::uint8_t id_value;
    // A zero mask deliberately reduces identification to an ACK-only match.
    std::uint8_t id_mask;
    pmic_sequence_t power_on;
    reg_bit_t power_state;
    reg_bit_t reset_state;
    pmic_sequence_t reset_assert;
    pmic_sequence_t reset_release;
    register_list_t restore_registers;
    std::uint32_t detected_option;
  };

  struct power_desc_t
  {
    // GPIO-held and I2C-controlled power are mutually exclusive.
    std::int8_t hold_pin;
    std::uint32_t i2c_freq;
    const pmic_variant_t* variants;
    std::uint8_t variant_count;
    std::uint8_t warm_wait_ms;
    std::uint8_t cold_wait_ms;
    // Some always-on controllers NACK the first access while waking from idle.
    // Zero preserves the single-attempt behavior for controllers that do not need polling.
    std::uint8_t wake_poll_ms;
  };

  using reset_custom_fn_t = bool (*)(const board_desc_t&, const prepare_ctx_t&,
                                     std::uint32_t* detected_option);

  enum class reset_kind_t : std::uint8_t
  {
    none,
    gpio,
    i2c_regs,
    custom,
  };

  enum reset_flag_t : std::uint8_t
  {
    reset_no_flags = 0,
    reset_always = 1 << 0,
    reset_hold_when_skipped = 1 << 1,
  };

  struct reset_desc_t
  {
    reset_kind_t kind;
    std::int8_t pin;
    std::uint8_t low_ms;
    std::uint8_t post_ms;
    std::uint8_t flags;
    reset_custom_fn_t custom;
  };

  struct shared_sd_desc_t
  {
    std::int8_t sclk;
    std::int8_t mosi;
    std::int8_t miso;
    std::int8_t sd_cs;
    std::int8_t other_cs;
  };

  struct display_pins_t
  {
    std::int8_t sclk;
    std::int8_t mosi;
    std::int8_t miso;
    std::int8_t dc;
    std::int8_t cs;
    std::int8_t rst;
    std::int8_t busy;
  };

  struct i2c_desc_t
  {
    // PMIC power/reset descriptions require valid SDA/SCL even when hw_port
    // is -1; hw_port only controls whether the bus remains open after prepare.
    std::int8_t sda;
    std::int8_t scl;
    std::int8_t hw_port;
  };

  struct board_desc_t
  {
    board_def_t def;
    power_desc_t power;
    reset_desc_t reset;
    shared_sd_desc_t sd;
    display_pins_t display;
    pin_list_t hold_high_pins;
    i2c_desc_t internal_i2c;
    // Remaining settle time after a direct reset before panel settings are reread.
    std::uint16_t direct_reset_panel_reload_wait_ms;
    option_list_t option_names;
  };

  inline void board_result_t::assign(const board_desc_t* value)
  {
    desc = value;
    def = value ? &value->def : &board_def_unknown;
  }

  constexpr pmic_write_t pmic_write(std::uint8_t addr, std::uint8_t reg,
                                    std::uint8_t value, std::uint8_t mask)
  {
    return { addr, reg, value, mask };
  }

  constexpr reg_bit_t reg_bit(std::uint8_t reg, std::uint8_t mask)
  {
    return { reg, mask };
  }

  constexpr pmic_variant_t pmic_variant(
    std::uint8_t addr, std::uint8_t id_reg, std::uint8_t id_value,
    pmic_sequence_t power_on, reg_bit_t power_state, reg_bit_t reset_state,
    pmic_sequence_t reset_assert, pmic_sequence_t reset_release,
    register_list_t restore_registers, std::uint32_t detected_option)
  {
    return { addr, id_reg, id_value, 0xFF, power_on, power_state, reset_state,
             reset_assert, reset_release, restore_registers, detected_option };
  }

  constexpr pmic_variant_t pmic_variant_ack_only(
    std::uint8_t addr, std::uint8_t id_reg,
    pmic_sequence_t power_on, reg_bit_t power_state, reg_bit_t reset_state,
    pmic_sequence_t reset_assert, pmic_sequence_t reset_release,
    register_list_t restore_registers, std::uint32_t detected_option)
  {
    return { addr, id_reg, 0, 0, power_on, power_state, reset_state,
             reset_assert, reset_release, restore_registers, detected_option };
  }

  constexpr power_desc_t no_power()
  {
    return { -1, 0, nullptr, 0, 0, 0, 0 };
  }

  constexpr power_desc_t gpio_power(int hold_pin)
  {
    return { static_cast<std::int8_t>(hold_pin), 0, nullptr, 0, 0, 0, 0 };
  }

  template <std::size_t N>
  constexpr power_desc_t i2c_power(std::uint32_t freq,
                                   const pmic_variant_t (&variants)[N],
                                   std::uint8_t warm_wait_ms, std::uint8_t cold_wait_ms)
  {
    return { -1, freq, variants, list_size_t<N>::value, warm_wait_ms, cold_wait_ms, 0 };
  }

  template <std::size_t N>
  constexpr power_desc_t i2c_power_polled(std::uint32_t freq,
                                          const pmic_variant_t (&variants)[N],
                                          std::uint8_t warm_wait_ms,
                                          std::uint8_t cold_wait_ms,
                                          std::uint8_t wake_poll_ms)
  {
    return { -1, freq, variants, list_size_t<N>::value,
             warm_wait_ms, cold_wait_ms, wake_poll_ms };
  }

  constexpr std::uint16_t direct_reset_panel_reload_wait(std::uint16_t milliseconds)
  {
    return milliseconds;
  }

  constexpr std::uint16_t no_direct_reset_panel_reload_wait() { return 0; }

  constexpr reset_desc_t no_reset()
  {
    return { reset_kind_t::none, -1, 0, 0, reset_no_flags, nullptr };
  }

  constexpr reset_desc_t gpio_reset(int pin, std::uint8_t low_ms,
                                    std::uint8_t post_ms, std::uint8_t flags = reset_no_flags)
  {
    return { reset_kind_t::gpio, static_cast<std::int8_t>(pin),
             low_ms, post_ms, flags, nullptr };
  }

  constexpr reset_desc_t i2c_reset(std::uint8_t low_ms, std::uint8_t post_ms,
                                   std::uint8_t flags = reset_no_flags)
  {
    return { reset_kind_t::i2c_regs, -1, low_ms, post_ms, flags, nullptr };
  }

  constexpr reset_desc_t custom_reset(int pin, std::uint8_t low_ms,
                                      std::uint8_t post_ms, reset_custom_fn_t custom,
                                      std::uint8_t flags = reset_no_flags)
  {
    // A null callback is an invalid description and prepare_reset rejects it.
    return { reset_kind_t::custom, static_cast<std::int8_t>(pin),
             low_ms, post_ms, flags, custom };
  }

  constexpr shared_sd_desc_t no_shared_sd()
  {
    return { -1, -1, -1, -1, -1 };
  }

  constexpr shared_sd_desc_t shared_sd(int sclk, int mosi, int miso,
                                       int sd_cs, int other_cs)
  {
    return { static_cast<std::int8_t>(sclk), static_cast<std::int8_t>(mosi),
             static_cast<std::int8_t>(miso), static_cast<std::int8_t>(sd_cs),
             static_cast<std::int8_t>(other_cs) };
  }

  constexpr display_pins_t display_pins(int sclk, int mosi, int miso, int dc,
                                        int cs, int rst, int busy)
  {
    return { static_cast<std::int8_t>(sclk), static_cast<std::int8_t>(mosi),
             static_cast<std::int8_t>(miso), static_cast<std::int8_t>(dc),
             static_cast<std::int8_t>(cs), static_cast<std::int8_t>(rst),
             static_cast<std::int8_t>(busy) };
  }

  constexpr i2c_desc_t no_internal_i2c()
  {
    return { -1, -1, -1 };
  }

  constexpr i2c_desc_t internal_i2c(int sda, int scl, int hw_port)
  {
    return { static_cast<std::int8_t>(sda), static_cast<std::int8_t>(scl),
             static_cast<std::int8_t>(hw_port) };
  }

  struct i2c_scan_cache_t
  {
    std::int16_t pin_sda = -1;
    std::int16_t pin_scl = -1;
    bool pullup_ok = false;
    // Only addresses requested by a detector are probed. `checked` separates
    // a cached NACK from an address that has not been touched yet.
    std::uint32_t checked[4] = {};
    std::uint32_t ack[4] = {};
  };

  struct pin_pull_result_t
  {
    std::uint64_t pulldown_high = 0;
    std::uint64_t pullup_high = 0;
  };

  struct detector_workspace_t
  {
    // Per-detection scalar scratch carried from signature() to its immediately
    // following confirm(). GPIO state is owned by detection_transaction_t.
    std::uint64_t values[4] = {};
  };

  struct probe_ctx_t
  {
    // This is the caller's reset policy. A retry must not turn it on: callers
    // that preserve a displayed image depend on every attempt honoring it.
    bool allow_reset = true;
    // The hint moves its detector family forward. Candidate selection within
    // that family is detector-specific: the S3 SPI-ID family probes only the
    // hinted member, while legacy families retain their established ordering.
    // A family containing only fallback definitions is never moved forward.
    board_id_t hint = board_id_unknown;
    // The caller's last retry may relax exclusions based only on negative
    // evidence, retaining the legacy broad probe as a final safety net.
    bool final_attempt = false;
    int i2c_port_probe = -1;
    i2c_scan_cache_t i2c_cache;
    detector_workspace_t detector_workspace;
    // Non-null while board-detection components are running.
    detection_transaction_t* transaction = nullptr;
    bool conditional_pins_unavailable = false;
    const board_id_t* enabled_ids = nullptr;
  };

  class board_detector_t
  {
  public:
    constexpr board_detector_t(const board_def_t* const* members_)
    : members { members_ } {}
    // Stage 1 is a non-destructive family signature and normally restores all
    // state. The sole general exception is releasing an unusable bus whose SDA
    // is held (up to 9 clocks plus STOP), intentionally changing slave state
    // to return it to idle. A family may document a stricter internal contract;
    // for example, Paper pulses and retains reset through its following confirm.
    virtual bool signature(probe_ctx_t& ctx) const = 0;
    virtual bool confirm(probe_ctx_t& ctx, board_result_t* result) const = 0;
    bool has_member(board_id_t id) const;

    // A failed confirmation restores every touched pin and bus. A successful
    // confirmation may retain power-enable, chip-select and reset pins at safe
    // levels until display construction takes ownership; restoring those pins
    // would power a confirmed device down, let another shared-bus device
    // select, or leave the confirmed display's reset input floating. An SD card
    // moved to SPI mode is never moved back to native mode. The surrounding
    // detection transaction restores GPIO state if a successful result is
    // excluded or its prepare/construction/setup subsequently fails.
    // PMIC-register restoration after a failed confirmation is best effort:
    // failures are warned and detection continues, since aborting would make
    // the transport failure appear to the caller as a different board.
    const board_def_t* const* members;
  };

  board_result_t detect_board(const board_detector_t* const* list, board_id_t hint, probe_ctx_t& ctx);

  bool probe_i2c_ack(probe_ctx_t& ctx, int pin_sda, int pin_scl, std::uint8_t addr);

  // Recover a slave that retained SDA after the controller was reset during a
  // transaction. Leaves both pins as inputs so the caller can inspect them;
  // true means both lines released. The caller owns restoration.
  bool release_held_sda(int pin_sda, int pin_scl);

  // Do not include pins that another device may drive (for example MISO), or
  // pins without internal pulls. A PMIC-switched pull-up may indicate whether
  // its rail is powered, but must not be used as a board signature. U is high
  // in both masks, D in neither, F only in pullup_high, and X only in
  // pulldown_high. Every call measures the requested pins again.
  pin_pull_result_t probe_pin_pulls(probe_ctx_t& ctx, std::uint64_t pin_mask);

  // Mode-0 software SPI used only while detecting and preparing a board. Data
  // is laid out like Bus_SPI: low byte first, MSB first within each byte. On a
  // 3-wire ILI9342 bus the panel drives SDA only during the SCLK-low phase and
  // releases it at the rising edge; the board pull-up then restores high in
  // about 0.3 us. Reads therefore sample at the end of the low phase, just
  // before raising SCLK. endTransaction() always restores the shared data line
  // to write direction so a following command can reach the panel.
  class soft_spi_t
  {
  public:
    soft_spi_t(int pin_sclk, int pin_mosi, int pin_miso, int pin_dc,
               std::uint32_t half_us = 1)
    : pin_sclk_(pin_sclk), pin_mosi_(pin_mosi), pin_miso_(pin_miso), pin_dc_(pin_dc),
      half_us_(half_us) {}

    void init();
    void beginTransaction();
    void endTransaction();
    void wait() {}
    void writeCommand(std::uint32_t data, std::uint_fast8_t bits);
    void writeData(std::uint32_t data, std::uint_fast8_t bits);
    std::uint32_t transferData(std::uint32_t data, std::uint_fast8_t bits);
    void beginRead(std::uint_fast8_t dummy_bits = 0);
    std::uint32_t readData(std::uint_fast8_t bits);
    void readBytes(std::uint8_t* dst, std::size_t length);
    void endRead();

  private:
    void clock();
    void send(std::uint32_t data, std::uint_fast8_t bits);
    static std::uint_fast8_t bit_index(std::uint_fast8_t index)
    {
      return (index & ~std::uint_fast8_t(7)) + 7 - (index & 7);
    }

    int pin_sclk_;
    int pin_mosi_;
    int pin_miso_;
    int pin_dc_;
    std::uint32_t half_us_;
  };

  // The first received byte occupies bits 0..7. Bits within each byte arrive MSB first.
  std::uint32_t soft_spi_read32(probe_ctx_t& ctx, int pin_sclk, int pin_mosi, int pin_miso,
                                int pin_dc, int pin_cs, std::uint8_t cmd, std::uint8_t dummy_bits,
                                std::uint32_t half_us = 1);

  struct spi_id_probe_t
  {
    std::uint8_t cmd;
    std::uint8_t dummy_bits;
    std::uint32_t mask;
    const std::uint32_t* values;
    std::uint8_t value_count;
    std::uint32_t option_bit;
  };

  template <std::size_t N>
  constexpr spi_id_probe_t spi_id_probe(std::uint8_t cmd, std::uint32_t mask,
                                        const std::uint32_t (&values)[N],
                                        std::uint32_t option_bit,
                                        std::uint8_t dummy_bits = 1)
  {
    return { cmd, dummy_bits, mask, values, list_size_t<N>::value, option_bit };
  }

  bool probe_spi_id(probe_ctx_t& ctx, const board_desc_t& desc,
                    const spi_id_probe_t* probes, std::size_t probe_count,
                    board_result_t* result, bool three_wire,
                    std::uint8_t slow_retry_half_us = 0);

  // Kept callable by family detectors; validates desc before touching hardware.
  bool prepare_reset(const board_desc_t& desc, board_result_t& result,
                     const prepare_ctx_t& ctx, int i2c_port,
                     std::uint32_t* detected_option = nullptr);
  bool prepare(const board_desc_t& desc, board_result_t& result, const prepare_ctx_t& ctx);
}
}
