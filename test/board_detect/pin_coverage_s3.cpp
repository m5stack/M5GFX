#define CONFIG_IDF_TARGET_ESP32S3 1
#define CONFIG_IDF_TARGET "esp32s3"
#define COVERAGE_BOARD_HEADER "board_detect/m5/esp32s3.inl"
#define COVERAGE_CONDITIONAL_PINS pins(wiring::detection::opi_pins)
#define COVERAGE_FAMILIES cover(esp32s3_detectors_qfn56); cover(esp32s3_detectors_lga56);
static void cardputer_success();
#define COVERAGE_EXTRA cardputer_success();
#include "support/pin_coverage.hpp"

// Supply panel read bits at the GPIO boundary: production SPI clocking and
// probe_spi_id must pass before subdivision operations become observable.
static void cardputer_success() {
  for (bool recovery : {false, true})
    for (unsigned variant = 0; variant != 3; ++variant) {
      host_hardware::reset();
      unsigned spi_bits = 0;
      host_hardware::input = [&](int pin, int mode, bool) {
        ++host_hardware::now_us;
        if (pin == 35 && mode == lgfx::input) {
          assert(spi_bits < 32);
          // readData stores the first received byte in the low byte of its word.
          const std::uint8_t reply[] = {0x81, 0, 0, 0};
          const auto bit = spi_bits++;
          return bool((reply[bit / 8] >> (7 - bit % 8)) & 1);
        }
        if (pin == 5 || pin == 8) {
          // A held SDA releases only after its own SCL receives a recovery clock.
          if (recovery) {
            bool clocked = false;
            for (const auto& e : host_hardware::events)
              clocked |= e.kind == 4 && e.pin == (pin == 5 ? 6 : 9) && e.value == 0;
            if (!clocked) return false;
          }
        }
        return true;
      };
      host_hardware::wire = [=](int addr, int reg, std::uint8_t* data, std::size_t len, bool) {
        if (addr == 0x40 && reg == 0xFE && len == 2) { data[0] = 0x54; data[1] = 0x49; }
        return addr == 0x34 ? variant != 0 : variant != 1;
      };
      {
        detection_transaction_t tx(pins(wiring::detection::unconditional_pins),
                                   pins(wiring::detection::opi_pins), true);
        assert(tx.valid());
        std::set<int> captured;
        for (const auto& e : host_hardware::events) if (e.kind == 5) captured.insert(e.pin);
        host_hardware::events.clear();
        probe_ctx_t ctx;
        ctx.transaction = &tx;
        ctx.probe_port = -1;
        ctx.final_attempt = true;
        board_result_t result;
        const bool confirmed = cardputer_family_detector.confirm(ctx, &result);
        record_confirmation(cardputer_family_detector, confirmed);
        assert(confirmed == (variant != 2));
        if (confirmed) assert(result.desc == (variant == 0 ? &desc_vameter : &desc_cardputer_adv));
        assert(spi_bits == 32);
        for (int pin : {5, 6, 8, 9}) {
          bool read = false, i2c = false;
          for (const auto& e : host_hardware::events) if (e.pin == pin) {
            read |= e.kind == 2;
            i2c |= e.kind == 7;
          }
          assert(read && i2c);
        }
        if (recovery) {
          for (int scl : {6, 9}) {
            bool clocked = false;
            for (const auto& e : host_hardware::events)
              clocked |= e.kind == 4 && e.pin == scl && e.value == 0;
            assert(clocked);
          }
        }
        check_events(captured, cardputer_family_detector);
      }
      assert(host_hardware::ports.empty());
      ++cases;
    }
}
