// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include <cstddef>
#include <cstdint>

namespace m5gfx { namespace board_detect { namespace io {
  struct dedicated_gpio_handle_t { void* value = nullptr; };
  // Values are independent of the backend's public types.
  enum class pin_mode_t : std::uint8_t { output, input, input_pullup, input_pulldown };
}}}

/*
 * Replacement contract: M5GFX_BOARD_DETECT_IO_HEADER supplies the following in
 * m5gfx::board_detect::io. Use inline implementations and no public SDK types.
 * The replacement header must make ESP_LOGx macros visible to detection code.
 * The caller owns board choices, thresholds, retries, sampling and rollback order.
 * Chip-specific entries are required only where their detection path is compiled.
 *
 * Constants (constexpr):
 *   int i2c_port_0;                    // First hardware I2C controller ID.
 *   int i2c_port_1;                    // Second controller ID, when available.
 *   int low_power_i2c_port_0;          // LP controller ID, or first HP fallback.
 *   int pin_count;                    // Exclusive upper bound of GPIO numbers.
 *   unsigned package_classic_q6;       // Classic ESP32 Q6 package code.
 *   unsigned package_classic_pico_d4;  // Classic ESP32 PICO-D4 package code.
 *   bool dedicated_gpio_supported;    // Whether the S3 release probe can run.
 *
 * Types:
 *   pin_mode_t is defined above: output, input, input_pullup, input_pulldown.
 *   pin_state_t is default-constructible, with backend-private saved state:
 *     void save(int pin);             // Capture this pin's configuration.
 *     bool matches_current() const;   // Compare captured and current state.
 *     void restore();                 // Restore the captured configuration.
 *     int pin() const;                // Return the captured pin number.
 *   critical_section_t is default-constructible and initially unlocked:
 *     void enter();                   // Enter a critical interval.
 *     void exit();                    // Leave it; pairing belongs to the caller.
 *   dedicated_gpio_handle_t is defined above, with void* value = nullptr;
 *     value holds an opaque backend handle used by open/close/channel_masks.
 *
 * GPIO:
 *   void pin_mode(int pin, pin_mode_t mode); // Configure direction and pulls.
 *   void pin_high(int pin);                // Set the output latch.
 *   void pin_low(int pin);                 // Clear the output latch.
 *   bool pin_read(int pin);                // Read the raw input level.
 *   bool pin_valid(int pin);               // Test physical GPIO validity only.
 *   bool pin_output_level(int pin, bool* level);
 *     // Write the output latch to *level; return whether output is enabled.
 *   void release_lp_pad(int pin);          // Release stale LP pad routing.
 *   volatile std::uint32_t* pin_low_register(int pin);
 *     // Return the GPIO clear register; caller supplies the bit mask and pulses.
 *   volatile std::uint32_t* pin_high_register(int pin);
 *     // Return the GPIO set register; caller supplies the bit mask and pulses.
 *
 * Time:
 *   std::uint32_t cpu_cycles();            // Read the wrapping CPU cycle counter.
 *   std::uint32_t cpu_hz();                // Return the current CPU clock in Hz.
 *   std::uint32_t millis();                // Read the wrapping millisecond clock.
 *   std::uint32_t micros();                // Read the wrapping microsecond clock.
 *   void delay_ms(std::uint32_t ms);       // Delay in milliseconds.
 *   void delay_us(std::uint32_t us);       // Delay in microseconds.
 *   void delay_rom_us(std::uint32_t us);   // Use the ROM microsecond delay primitive.
 *
 * I2C (bool means success; one operation per call, with no policy retries):
 *   bool i2c_open(int port, int sda, int scl); // Initialize the selected controller.
 *   void i2c_close(int port);                // Release the selected controller.
 *   bool i2c_ack(int port, std::uint16_t addr, std::uint32_t freq,
 *                bool end_on_failure = false);
 *     // Begin a write and end it; true requires both to succeed. On failed begin,
 *     // end only when end_on_failure is true, preserving the caller's policy.
 *   bool i2c_read_reg8(int port, std::uint16_t addr, std::uint8_t reg,
 *                      std::uint8_t* value, std::uint32_t freq);
 *     // Read one register byte; leave *value unchanged on failure.
 *   bool i2c_write_reg8(int port, std::uint16_t addr, std::uint8_t reg,
 *                       std::uint8_t value, std::uint32_t freq);
 *     // Write one full register byte without a read/modify/write mask.
 *   bool i2c_write(int port, std::uint16_t addr, const std::uint8_t* data,
 *                  std::size_t len, std::uint32_t freq);
 *     // Perform one write transaction; address interpretation belongs to caller.
 *   bool i2c_write_read(int port, std::uint16_t addr, const std::uint8_t* wdata,
 *                       std::size_t wlen, std::uint8_t* rdata, std::size_t rlen,
 *                       std::uint32_t freq);
 *     // Perform one combined write/read transaction.
 *
 * Chip information:
 *   unsigned package_version();           // Read the raw package code.
 *   bool psram_ready();                   // Read S3 OPI PSRAM initialization state.
 *   unsigned chip_revision();             // Read P4 revision; caller sets cutoffs.
 *   unsigned flash_capacity_code();       // Read the C6/C5 flash capacity field.
 *   void module_memory_capacity(unsigned* flash, unsigned* psram);
 *     // Read C5 flash/PSRAM fields together; caller maps codes to module types.
 *   std::uint8_t st_touch_address();       // P4: ST7123 driver's default address.
 *   std::uint8_t gt911_touch_address();    // P4: GT911 driver's first default address.
 *   unsigned usb_serial_jtag_frame();     // C3: read raw USB SOF frame index.
 *   bool uart_rx_held_high();             // C3: pulldown/read UART RX, restoring mux.
 *
 * S3 dedicated GPIO (also provide compilable stubs when unsupported):
 *   std::uint32_t dedicated_release_read_in(); // Read raw dedicated input bits.
 *   void dedicated_release_write_mask(std::uint32_t mask, std::uint32_t value);
 *     // Write selected dedicated output bits.
 *   void dedicated_release_channel_masks(dedicated_gpio_handle_t handle,
 *       std::uint8_t channel_count, std::uint32_t* in_mask, std::uint32_t* out_mask,
 *       int* in_error, int* out_error);
 *     // Return bundle channel masks and native status codes; caller validates them.
 *   int dedicated_release_core_id();      // Read the current CPU core ID.
 *   std::uint32_t dedicated_release_cycle_count(); // Read its CPU cycle counter.
 *   int dedicated_release_task_core();    // Read the task's core affinity.
 *   int no_core_affinity();               // Return the unpinned-affinity sentinel.
 *   bool dedicated_gpio_in_use();         // Read the dedicated peripheral clock bit.
 *   void pin_weak_pullup_open_drain(int pin);
 *     // Configure weak pullup, minimum drive and open drain; no sample decisions.
 *   int dedicated_gpio_open(const int* pins, std::uint8_t count,
 *                           dedicated_gpio_handle_t* handle);
 *     // Allocate an input/output bundle; return zero on success, native error else.
 *   int dedicated_gpio_close(dedicated_gpio_handle_t handle);
 *     // Release the bundle; return zero on success, native error else.
 *
 * Classic ESP32 transfers (compile-time wiring and limits come from the caller):
 *   template<int channel_a, int pin_a, int channel_b, int pin_b,
 *            std::uint32_t timeout_us>
 *   bool classic_touch_measure(std::uint16_t* t4, std::uint16_t* t7);
 *     // channel_a/b select touch channels; pin_a/b are for host event recording
 *     // and are unused by the default implementation.
 *     // timeout_us bounds one scan. Restore peripheral state and return completion
 *     // plus raw T4/T7 counts (zero on timeout); caller applies ratios/thresholds.
 *   template<int led_gpio, std::uint16_t pixels>
 *   void classic_led_clear();
 *     // led_gpio selects DIN, pixels sets the GRB black-pixel count; perform one
 *     // WS2812 transfer with reset intervals and timing protected from flash stalls.
 */
#if defined (M5GFX_BOARD_DETECT_IO_HEADER)
#include M5GFX_BOARD_DETECT_IO_HEADER
#else
#include "detect_io_default.inl"
#endif
