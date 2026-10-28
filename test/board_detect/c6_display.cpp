#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <vector>
#include "board_detect/board_detect.hpp"
#include "board_detect/detect_types.hpp"
#include "board_detect/m5/generated/detector_order_constraints.hpp"
#include "board_detect/m5/esp32c6.inl"

using namespace m5gfx::board_detect;
namespace family = m5gfx::board_detect::m5;

namespace {
bool present, responds, high;
unsigned bus_calls, pull_calls;
std::vector<unsigned> reads;
std::vector<unsigned> events;
unsigned bad_address;
bool bad_id;
unsigned scenarios;

void reset(bool bus, bool response, bool pin_high) {
  present = bus; responds = response; high = pin_high;
  bus_calls = pull_calls = 0;
  reads.clear(); events.clear(); bad_address = 0; bad_id = false;
}

// Same session adapter as the original family test: hardware preparation and
// construction are outside this test. The detector and session are production code.
detect_outcome_t attempt(family::c6_display_family_detector_t& detector,
                         const detect_request_t& request, bool final) {
  probe_ctx_t ctx;
  ctx.hint = request.hint; ctx.attempt = request.attempt; ctx.final_attempt = final;
  detect_outcome_t out;
  if (!detector.signature(ctx)) {
    out.reason = fail_reason_t::no_signature;
  } else if (detector.confirm(ctx, &out.result)) {
    finalize_prepared_result(out, 0);
    out.adopted = true;
  } else if (ctx.candidate) {
    out.verdict = verdict_t::candidate;
    out.candidate_kind = candidate_kind_t::weak;
    out.result.candidate = ctx.candidate;
  }
  return out;
}
}

namespace m5gfx { namespace board_detect {
bool probe_i2c_bus_present(probe_ctx_t&, int sda, int scl) {
  assert(sda == family::wiring::nesson1::internal_i2c_sda);
  assert(scl == family::wiring::nesson1::internal_i2c_scl);
  ++bus_calls; events.push_back(1);
  return present;
}
pin_pull_result_t probe_pin_pulls(probe_ctx_t&, std::uint64_t mask, std::uint32_t release) {
  assert(mask == family::c6_display_detail::signature_bit && release == 0);
  ++pull_calls; events.push_back(2);
  pin_pull_result_t result;
  result.pullup_high = high ? mask : 0;
  return result;
}
bool probe_i2c_read(probe_ctx_t&, int sda, int scl, std::uint8_t address,
                    std::uint16_t reg, std::uint8_t* data, std::size_t length,
                    std::uint32_t freq, std::uint32_t poll, bool reg16) {
  assert(sda == family::wiring::nesson1::internal_i2c_sda);
  assert(scl == family::wiring::nesson1::internal_i2c_scl);
  assert(reg == 1 && length == 1 && freq == 100000 && poll == 0 && !reg16);
  reads.push_back(address); events.push_back(address);
  *data = bad_id && address == bad_address ? 0 : 0xa0;
  return responds && !(address == bad_address && !bad_id);
}

// Hardware used by other included families/registry helpers must never run here.
bool probe_pin_pullup_low(probe_ctx_t&, int) { std::abort(); }
void soft_spi_t::beginTransaction() { std::abort(); }
void soft_spi_t::endTransaction() { std::abort(); }
void soft_spi_t::writeCommand(std::uint32_t, std::uint_fast8_t) { std::abort(); }
void soft_spi_t::writeData(std::uint32_t, std::uint_fast8_t) { std::abort(); }
void soft_spi_t::beginRead(std::uint_fast8_t) { std::abort(); }
std::uint32_t soft_spi_t::readData(std::uint_fast8_t) { std::abort(); }
void soft_spi_t::endRead() { std::abort(); }
namespace m5 {
construct_status_t construct_unitc6l(const board_result_t&, display_parts_t*) { std::abort(); }
construct_status_t construct_nesson1(const board_result_t&, display_parts_t*) { std::abort(); }
}
} }

int main() {
  const auto unit = family::desc_unitc6l.def.id;
  // Preserve the original bus/response/hint matrix and its retry/persistence checks.
  for (bool bus : {false, true}) for (bool response : {false, true})
  for (board_id_t hint : {board_id_unknown, unit, board_id_t(65535)}) {
    reset(bus, response, true);
    family::c6_display_family_detector_t detector;
    detect_request_t request; request.hint = hint; request.max_attempts = 5;
    const auto out = run_detection_session(request, [&](const detect_request_t& r, bool last) {
      return attempt(detector, r, last);
    });
    assert(out.verdict == (!bus ? verdict_t::unknown : response ? verdict_t::confirmed : verdict_t::candidate));
    assert(out.adopted == (bus && response));
    assert(!bus || response || out.result.candidate == &family::desc_unitc6l.def);
    assert(should_persist_detection(out, 0) == (bus && response));
    const unsigned expected_attempts = !bus || response ? 1 : 5;
    assert(out.attempts == expected_attempts && bus_calls == expected_attempts);
    assert(pull_calls == (bus ? expected_attempts : 0));
    if (!bus) {
      assert(out.reason == fail_reason_t::no_signature && reads.empty());
    } else {
      assert(out.reason == fail_reason_t::none);
      assert(reads == std::vector<unsigned>(expected_attempts, 0x43));
      for (unsigned n = 0; n < expected_attempts; ++n) {
        assert(events[n * 3] == 1 && events[n * 3 + 1] == 2 && events[n * 3 + 2] == 0x43);
      }
      if (response) assert(out.result.desc == &family::desc_unitc6l);
      else assert(out.candidate_kind == candidate_kind_t::weak);
    }
    ++scenarios;
  }
  // The first weak candidate is not overwritten by this family.
  reset(true, false, true);
  family::c6_display_family_detector_t detector;
  probe_ctx_t ctx; board_result_t result;
  ctx.candidate = &family::desc_nanoc6.def;
  assert(detector.signature(ctx) && !detector.confirm(ctx, &result));
  assert(ctx.candidate == &family::desc_nanoc6.def);
  ctx.candidate = nullptr;
  assert(!detector.confirm(ctx, &result) && ctx.candidate == &family::desc_unitc6l.def);
  ++scenarios;

  // Exercise the other member, ID rejection, and short-circuit probe order too.
  for (unsigned bad : {0u, 0x43u, 0x44u}) for (bool invalid_id : {false, true}) {
    reset(true, true, false); bad_address = bad; bad_id = invalid_id;
    family::c6_display_family_detector_t nesson;
    probe_ctx_t probe; board_result_t found;
    assert(nesson.signature(probe));
    assert(nesson.confirm(probe, &found) == (bad == 0));
    assert(reads == (bad == 0x44 ? std::vector<unsigned>{0x44} : std::vector<unsigned>{0x44, 0x43}));
    assert(probe.candidate == nullptr);
    if (!bad) assert(found.desc == &family::desc_nesson1);
    ++scenarios;
  }
  reset(true, true, true); bad_address = 0x43; bad_id = true;
  assert(detector.signature(ctx) && !detector.confirm(ctx, &result));
  assert(ctx.candidate == &family::desc_unitc6l.def);
  assert(!detector.confirm(ctx, nullptr));
  // A missing bus resets a previously selected member before confirm().
  present = false;
  assert(!detector.signature(ctx) && !detector.confirm(ctx, &result));
  ++scenarios;
  std::printf("C6 display: %u scenarios passed\n", scenarios);
}
