#include <cassert>
#include <initializer_list>
#include "support/runtime.hpp"
#include "support/probes.hpp"
using namespace m5gfx::board_detect;
static const std::int8_t held[] = {6};
static const board_desc_t first = {{60010, "SPI member one", 0},
                                   no_power(),
                                   no_reset(),
                                   no_shared_sd(),
                                   display_pins(2, 3, 4, 5, 6, -1, -1),
                                   pins(held),
                                   no_internal_i2c(),
                                   no_options(),
                                   pins(held)};
static const board_desc_t second = {{60011, "SPI member two", 0},
                                    first.power,
                                    first.reset,
                                    first.sd,
                                    first.display,
                                    first.hold_high_pins,
                                    first.internal_i2c,
                                    first.option_names,
                                    first.op_gpio_pins};
static const std::uint32_t standard[] = {0x11}, alternate[] = {0x22};
static const spi_id_probe_t variants[] = {spi_id_probe(4, 0xff, standard, 0),
                                          spi_id_probe(4, 0xff, alternate, 1)};
int main() {
  unsigned cases = 0;
  const std::int8_t watched[] = {2, 3, 4, 5, 6};
  for (int mode = 0; mode < 4; ++mode) {
    host_hardware::reset();
    host_probes::reset();
    unsigned reads = 0;
    host_probes::spi = [&](int sclk, int mosi, int miso, int dc, int cs, std::uint8_t cmd) {
      assert(sclk == 2 && mosi == 3 && miso == 3 && dc == 5 && cs == 6 && cmd == 4);
      assert(host_probes::half_us == 1 || host_probes::half_us == 5);
      ++reads;
      return mode == 0                 ? 0xffffffffu
             : mode == 1               ? 0x11u
             : mode == 3 && reads == 1 ? 0xffffffffu
                                       : 0x22u;
    };
    detection_transaction_t tx(pins(watched), no_pins(), false);
    prepare_ctx_t ctx;
    ctx.transaction = &tx;
    board_result_t r;
    r.assign(&first);
    const spi_id_member_t member = {&first, variants, 2, true, false, 5, false};
    assert(fixed_prepare_spi_variant(r, ctx, member));
    assert(r.def == &first.def && r.option == unsigned(mode >= 2) && !r.provisional &&
           r.refine == nullptr);
    assert(reads == unsigned(mode == 0 || mode == 3 ? 2 : 1) &&
           host_hardware::warnings == unsigned(mode == 0));
    assert(r.prepared & prepared_power);
    ++cases;
  }
  const board_def_t *defs[] = {&first.def, &second.def, nullptr};
  const spi_id_member_t members[] = {{&first, variants, 1, true, true, 0, false},
                                     {&second, variants + 1, 1, true, false, 0, false}};
  spi_id_detector_t detector(defs, members, 2);
  for (const auto *target : {&first, &second}) {
    host_hardware::reset();
    host_probes::reset();
    host_probes::spi = [&](int, int, int, int, int, std::uint8_t) {
      return target == &first ? 0x11u : 0x22u;
    };
    detection_transaction_t tx(pins(watched), no_pins(), false);
    detect_request_t req;
    req.max_attempts = 5;
    req.hint = target == &first ? second.def.id : first.def.id;
    auto out = run_detection_session(req, [&](const detect_request_t &request, bool final) {
      probe_ctx_t ctx;
      ctx.transaction = &tx;
      ctx.attempt = request.attempt;
      ctx.hint = request.hint;
      ctx.final_attempt = final;
      detect_outcome_t out;
      if (detector.confirm(ctx, &out.result)) {
        finalize_prepared_result(out, 0);
        out.adopted = true;
      }
      return out;
    });
    assert(out.adopted && out.attempts == 2 && out.result.desc == target);
    probe_ctx_t ctx;
    ctx.transaction = &tx;
    ctx.hint = req.hint;
    ctx.final_attempt = true;
    board_result_t r;
    assert(detector.confirm(ctx, &r) && r.desc == target);
    ++cases;
  }
  host_probes::spi = [](int, int, int, int, int, std::uint8_t) { return 0x22u; };
  detection_transaction_t tx(pins(watched), no_pins(), false);
  probe_ctx_t ctx;
  ctx.transaction = &tx;
  ctx.conditional_pins_unavailable = true;
  board_result_t r;
  assert(detector.confirm(ctx, &r) && r.desc == &second);
  ++cases;
  std::printf("SPI hint/fixed variants: %u cases passed\n", cases);
}
