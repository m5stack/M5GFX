#if !defined(CONFIG_IDF_TARGET)
#define CONFIG_IDF_TARGET_ESP32C6 1
#define CONFIG_IDF_TARGET "esp32c6"
#endif
#include <cassert>
#include <initializer_list>
#include "support/runtime.hpp"
#include "support/probes.hpp"
#include "board_detect/m5/board_registry.inl"
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#include "board_detect/m5/generated/esp32s3_wiring.hpp"
#else
#include "board_detect/m5/generated/esp32c6_wiring.hpp"
#endif
using namespace m5gfx::board_detect;
namespace test_driver {
static std::function<void(const probe_ctx_t &)> on_signature;
static bool conditional_unavailable;
static unsigned constructions, adoptions, destroys, refinements, observations, fixed_starts;
static bool construct_ok, adopt_ok, no_display, prepare_ok, provisional, observe_ok, power_failed;
static const board_desc_t *refinement;
static const std::int8_t hold[] = {2};
static const board_desc_t member = {{60000, "test member", 0},
                                    gpio_power(2),
                                    no_reset(),
                                    no_shared_sd(),
                                    no_display_pins(),
                                    pins(hold),
                                    internal_i2c(8, 9, 1),
                                    no_options(),
                                    pins(hold)};
static const board_desc_t other = {{60001, "other member", 0},
                                   gpio_power(2),
                                   no_reset(),
                                   no_shared_sd(),
                                   no_display_pins(),
                                   pins(hold),
                                   no_internal_i2c(),
                                   no_options(),
                                   pins(hold)};
static bool refine(board_result_t &r, const prepare_ctx_t &) {
  ++refinements;
  if (refinement)
    r.assign(refinement);
  return prepare_ok;
}
static bool observe(board_result_t &, const prepare_ctx_t &) {
  ++observations;
  return observe_ok;
}
static bool fixed(board_result_t &r, const prepare_ctx_t &ctx) {
  ++fixed_starts;
  return fixed_starts >= 3 && prepare(*r.desc, r, ctx);
}
static m5gfx::board_detect::m5::construct_status_t
construct(const board_result_t &, m5gfx::board_detect::m5::display_parts_t *) {
  ++constructions;
  using m5gfx::board_detect::m5::construct_status_t;
  return !construct_ok ? construct_status_t::failed
         : no_display  ? construct_status_t::no_display
                       : construct_status_t::ok;
}
static const m5gfx::board_detect::m5::board_entry_t entries[] = {
    {&member, construct, "member", nullptr}, {&other, construct, "other", nullptr}};
static const board_def_t *const members[] = {&member.def, &other.def, nullptr};
struct detector_t final : board_detector_t {
  mutable unsigned signatures = 0, confirms = 0;
  bool has_signature = true, identified = true, matched = true;
  detector_t() : board_detector_t(test_driver::members) {}
  bool signature(probe_ctx_t &ctx) const override {
    ++signatures;
    if (on_signature) on_signature(ctx);
    return has_signature;
  }
  bool confirm(probe_ctx_t &ctx, board_result_t *r) const override {
    ++confirms;
    ctx.family_identified = identified;
    lgfx::gpio_hi(2);
    lgfx::pinMode(2, lgfx::output);
    if (!matched)
      return false;
    r->assign(&member);
    r->provisional = provisional;
    r->refine = refine;
    r->prepared = prepared_power | (power_failed ? prepared_power_failed : 0);
    return true;
  }
};
static void reset() {
  host_hardware::reset();
  host_probes::reset();
  on_signature = {};
  conditional_unavailable = false;
  constructions = adoptions = destroys = refinements = observations = fixed_starts = 0;
  construct_ok = adopt_ok = prepare_ok = observe_ok = true;
  no_display = provisional = power_failed = false;
  refinement = nullptr;
}
} // namespace test_driver
namespace m5gfx {
namespace board_detect {
namespace m5 {
construct_status_t construct_detected_board(const board_result_t &r, display_parts_t *parts) {
  return construct_board(test_driver::entries, r, parts);
}
void destroy_display_parts(display_parts_t *) { ++test_driver::destroys; }
success_log_t success_log(const board_result_t &r) { return success_log(test_driver::entries, r); }
} // namespace m5
} // namespace board_detect
static constexpr char LIBRARY_NAME[] = "host";
static constexpr int detection_i2c_port = -1;
#if defined(CONFIG_IDF_TARGET_ESP32S3)
static bool conditional_detection_pins_unavailable() { return test_driver::conditional_unavailable; }
#endif
#include "board_detect/detection_integration.inl"
} // namespace m5gfx
using namespace test_driver;
static unsigned cases;
static void attempts() {
  for (bool prov : {false, true})
    for (board_id_t pref : {board_id_unknown, member.def.id, other.def.id})
      for (int failure = 0; failure < 4; ++failure)
        for (bool displayless : {false, true}) {
          reset();
          detector_t detector;
          const board_detector_t *list[] = {&detector, nullptr};
          provisional = prov;
          prepare_ok = failure != 1;
          construct_ok = failure != 2;
          adopt_ok = failure != 3;
          no_display = displayless;
          detect_request_t req;
          req.fallback = pref;
          req.max_attempts = 5;
          auto out = run_detection_session(req, [&](const detect_request_t &request, bool final) {
            return m5gfx::run_detection_attempt(
                list, request, final,
                [](m5gfx::board_detect::m5::display_parts_t &, lgfx::board_t) {
                  ++adoptions;
                  return adopt_ok;
                });
          });
          bool yields = prov && pref == other.def.id && prepare_ok,
               success = failure == 0 && !yields;
          assert(out.adopted == success &&
                 out.verdict == (prov ? verdict_t::candidate : verdict_t::confirmed));
          assert(out.candidate_kind ==
                 (prov ? candidate_kind_t::provisional : candidate_kind_t::none));
          assert(out.attempts == (success ? 1 : 5));
          assert(constructions == (failure == 1 || yields ? 0 : out.attempts));
          assert(adoptions == (failure == 1 || failure == 2 || yields ? 0 : out.attempts));
          assert(destroys == (failure == 3 && !yields ? out.attempts : 0));
          assert(should_persist_detection(out, 0) == (success && !prov));
          assert(!yields || out.result.candidate == &member.def);
          assert(host_hardware::gpio[2].high == (success && !displayless));
          assert(host_hardware::ports.empty());
          ++cases;
        }
  // Recovering power then refinement must apply the requested member before construction.
  reset();
  detector_t detector;
  const board_detector_t *list[] = {&detector, nullptr};
  detect_request_t req;
  req.max_attempts = 5;
  req.fallback = other.def.id;
  refinement = &other;
  auto recovered = run_detection_session(req, [&](const detect_request_t &request, bool final) {
    power_failed = request.attempt == 0;
    return m5gfx::run_detection_attempt(
        list, request, final, [](m5gfx::board_detect::m5::display_parts_t &, lgfx::board_t) {
          ++adoptions;
          return true;
        });
  });
  assert(recovered.adopted && recovered.attempts == 2 &&
         recovered.result.def == &other.def && constructions == 1 && adoptions == 1 &&
         refinements == 1);
  assert(host_hardware::ports.empty());
  ++cases;
  reset();
  refinement = &other;
  req.fallback = 65535;
  auto yielded = run_detection_session(req, [&](const detect_request_t &request, bool final) {
    power_failed = request.attempt == 0;
    prepare_ok = request.attempt == 0 || final;
    provisional = final;
    return m5gfx::run_detection_attempt(
        list, request, final, [](m5gfx::board_detect::m5::display_parts_t &, lgfx::board_t) {
          ++adoptions;
          return true;
        });
  });
  assert(yielded.attempts == 5 && !yielded.adopted &&
         yielded.result.candidate == &other.def &&
         yielded.candidate_kind == candidate_kind_t::provisional && constructions == 0 &&
         adoptions == 0);
  assert(host_hardware::ports.empty());
  ++cases;
}
static void fixed_startup() {
  for (bool allow_reset : {false, true})
    for (bool fail_construct : {false, true})
      for (bool fail_adopt : {false, true}) {
        reset();
        construct_ok = !fail_construct;
        adopt_ok = !fail_adopt;
        const m5gfx::board_detect::m5::board_entry_t entry(&member, construct, "member", nullptr,
                                                           fixed);
        auto out = m5gfx::run_fixed_startup_session(
            entry, allow_reset, [](m5gfx::board_detect::m5::display_parts_t &, lgfx::board_t) {
              ++adoptions;
              return adopt_ok;
            });
        bool success = !fail_construct && !fail_adopt;
        assert(out.adopted == success && out.attempts == (success ? 3 : 5));
        assert(out.verdict == verdict_t::confirmed && !out.result.provisional &&
               out.result.refine == nullptr && out.result.desc == &member);
        assert(fixed_starts == out.attempts && refinements == 0);
        assert(constructions == out.attempts - 2 &&
               adoptions == (fail_construct ? 0 : out.attempts - 2));
        assert(host_hardware::gpio[2].high == success);
        assert(host_hardware::ports.empty());
        ++cases;
      }
}
static void identified_stop() {
  for (bool signature : {false, true})
    for (bool identified : {false, true})
      for (bool match : {false, true}) {
        reset();
        detector_t first, later;
        first.has_signature = signature;
        first.identified = identified;
        first.matched = match;
        on_signature = [&](const probe_ctx_t &) {
          if (!later.signatures || !signature || match) return;
          bool restored = false;
          for (const auto &event : host_hardware::events)
            if (event.kind == 3 && event.pin == 2) restored = true;
          assert(restored && !host_hardware::gpio[2].high);
        };
        const board_detector_t *list[] = {&first, &later, nullptr};
        const std::int8_t pins_to_capture[] = {2};
        detection_transaction_t transaction(pins(pins_to_capture), no_pins(), false);
        probe_ctx_t ctx;
        ctx.transaction = &transaction;
        ctx.family_identified = true;
        auto result = detect_board(list, 0, ctx);
        bool stops = signature && (identified || match);
        assert(first.signatures == 1 && later.signatures == unsigned(!stops) &&
               first.confirms == unsigned(signature));
        if (signature && identified && !match) {
          assert(result.status == detect_status_t::no_match && result.desc == nullptr);
          assert(!host_hardware::gpio[2].high);
          unsigned restores = 0;
          for (const auto &event : host_hardware::events)
            if (event.kind == 3 && event.pin == 2)
              ++restores;
          assert(restores > 0);
        } else
          assert(result.def == &member.def);
        assert(host_hardware::ports.empty());
        ++cases;
      }
}
static void prepare_states() {
  const std::int8_t watched[] = {47, 48};
  static const ops::i2c_device_t devices[] = {{100000, 0, 0x6e, 0, 1, 0}};
  static const ops::op_t writes[] = {ops::i2c_write8(0, 0x10, 1)};
  static const pmic_variant_t variants[] = {
      pmic_variant_confirmed_option(0x6e, 0, ops::list(writes), reg_bit(0, 0), reg_bit(0, 0),
                                    ops::no_ops(), ops::no_ops(), 0, 0)};
  const board_desc_t desc = {{60002, "power fixture", 0},
                             i2c_power_confirmed(100000, variants, devices),
                             no_reset(),
                             no_shared_sd(),
                             no_display_pins(),
                             no_pins(),
                             internal_i2c(47, 48, -1),
                             no_options(),
                             no_pins()};
  for (int row : {0, 1, 2})
    for (bool final : {false, true})
      for (bool failed : {false, true})
        for (bool observation : {false, true})
          for (bool observation_failed : {false, true}) {
            reset();
            unsigned writes_count = 0;
            host_hardware::wire = [&](int, int, std::uint8_t *, std::size_t, bool write) {
              if (write)
                ++writes_count;
              return !failed;
            };
            detection_transaction_t transaction(pins(watched), no_pins(), false);
            prepare_ctx_t ctx;
            ctx.transaction = &transaction;
            ctx.final_attempt = final;
            ctx.probe_port = -1;
            board_result_t r;
            r.assign(&desc);
            r.refine = row < 2 ? refine : nullptr;
            r.observe_after_power = row == 2 && observation ? observe : nullptr;
            observe_ok = !observation_failed;
            bool ok = prepare(desc, r, ctx), retry = failed && row < 2 && !final;
            assert(ok == !retry && writes_count == 1);
            assert(refinements == unsigned(row < 2 && !failed) &&
                   observations == unsigned(row == 2 && observation && !failed));
            assert(bool(r.prepared & prepared_observation) == (row == 2 && observation));
            assert(r.provisional == (failed && row < 2 && final));
            assert(bool(r.prepared & prepared_refine) == (row < 2 && !retry));
            assert(bool(r.prepared & prepared_power_failed) == failed);
            assert(host_hardware::ports.empty());
            ++cases;
          }

  reset();
  detection_transaction_t transaction(pins(watched), no_pins(), false);
  prepare_ctx_t ctx;
  ctx.transaction = &transaction;
  board_result_t completed;
  completed.assign(&desc);
  completed.refine = refine;
  completed.prepared = prepared_power | prepared_power_failed | prepared_refine;
  assert(prepare(desc, completed, ctx));
  assert(!completed.provisional && refinements == 0);
  assert(host_hardware::ports.empty());
  ++cases;

  host_hardware::wire = [](int, int, std::uint8_t *, std::size_t, bool) { return false; };
  board_result_t pending;
  pending.assign(&desc);
  pending.refine = refine;
  pending.refine_state = 3;
  pending.option = 7;
  ctx.final_attempt = true;
  assert(prepare(desc, pending, ctx));
  assert(pending.provisional && pending.refine_state == 3 && pending.option == 7 && refinements == 0);
  assert(host_hardware::ports.empty());
  ++cases;
}
static void reset_faults() {
  static const ops::i2c_device_t devices[] = {{100000, 0, 0x6e, 0, 1, 0}};
  static const ops::op_t assert_ops[] = {ops::i2c_write8(0, 0x10, 0)};
  static const ops::op_t release_ops[] = {ops::i2c_write8(0, 0x11, 1)};
  static const pmic_variant_t variant[] = {
      pmic_variant_confirmed_option(0x6e, 0, ops::no_ops(), reg_bit(0, 0), reg_bit(0, 0),
                                    ops::list(assert_ops), ops::list(release_ops), 0, 0)};
  const board_desc_t base = {{60003, "reset fixture", 0},
                             i2c_power_confirmed(100000, variant, devices),
                             i2c_reset(1, 2, reset_hold_when_skipped),
                             no_shared_sd(), no_display_pins(), no_pins(),
                             internal_i2c(47, 48, -1), no_options(), no_pins()};
  // Confirmed preparation retains identity; confirmation and unidentified PMICs
  // use the strict default. Exercise assert, release, and skipped-reset release.
  for (int profile : {0, 1, 2})
    for (bool allow_reset : {false, true})
      for (int failed_reg : {-1, 0x10, 0x11}) {
        reset();
        auto desc = base;
        desc.power.variant_confirmed = profile != 2;
        const bool retained = profile == 0;
        unsigned asserts = 0, releases = 0, id_reads = 0;
        host_hardware::wire = [&](int, int reg, std::uint8_t *data, std::size_t, bool write) {
          if (!write) {
            ++id_reads;
            *data = 0;
            return true;
          }
          if (reg == 0x10) ++asserts;
          if (reg == 0x11) ++releases;
          return reg != failed_reg;
        };
        const std::int8_t watched[] = {47, 48};
        detection_transaction_t transaction(pins(watched), no_pins(), false);
        prepare_ctx_t ctx;
        ctx.transaction = &transaction;
        ctx.allow_reset = allow_reset;
        board_result_t result;
        result.assign(&desc);
        result.prepared = prepared_power;
        const bool actual = retained ? prepare(desc, result, ctx)
                                     : prepare_reset(desc, result, ctx, -1);
        const bool assert_failed = allow_reset && failed_reg == 0x10;
        const bool release_attempted = !assert_failed || retained;
        const bool release_failed = release_attempted && failed_reg == 0x11;
        const bool success = retained || (!assert_failed && !release_failed);
        assert(actual == success && result.desc == &desc && result.def == &desc.def);
        assert(bool(result.prepared & prepared_reset) == (success && allow_reset));
        assert(bool(result.prepared & prepared_reset_line) == (success && !allow_reset));
        assert(asserts == unsigned(allow_reset) && releases == unsigned(release_attempted));
        assert(id_reads == unsigned(profile == 2));
        assert(!result.provisional);
        assert(host_hardware::ports.empty());
        ++cases;
      }
  // A missing variant ID is a strict confirmation failure, before any reset op.
  auto desc = base;
  desc.power.variant_confirmed = false;
  reset();
  host_hardware::wire = [](int, int, std::uint8_t *, std::size_t, bool) { return false; };
  prepare_ctx_t ctx;
  ctx.allow_reset = true;
  board_result_t result;
  result.assign(&desc);
  assert(!prepare_reset(desc, result, ctx, -1) && result.prepared == 0);
  assert(host_hardware::ports.empty());
  ++cases;
}
// Detection must neither remap nor release a hardware port owned by the caller,
// including when construction or adoption rolls the GPIO transaction back.
static void caller_i2c_ownership() {
  for (bool fixed : {false, true})
    for (int failure : {0, 1, 2}) {
      reset();
      lgfx::i2c::init(1, 10, 11);
      const auto caller_ports = host_hardware::ports;
      host_hardware::events.clear();
      construct_ok = failure != 1;
      adopt_ok = failure != 2;
      detector_t detector;
      const board_detector_t *list[] = {&detector, nullptr};
      const auto adopt = [](m5gfx::board_detect::m5::display_parts_t &, lgfx::board_t) {
        return adopt_ok;
      };
      detect_request_t req;
      req.max_attempts = 1;
      const auto out = fixed
        ? m5gfx::run_fixed_startup(entries[0], false, adopt)
        : m5gfx::run_detection_attempt(list, req, true, adopt);
      assert(out.adopted == (failure == 0));
      assert(host_hardware::ports == caller_ports);
      for (const auto &event : host_hardware::events)
        assert(event.kind != 6 && event.kind != 7);
      ++cases;
    }
}
int main() {
  attempts();
  fixed_startup();
  identified_stop();
  prepare_states();
  reset_faults();
  caller_i2c_ownership();
  std::printf("Integration/fixed/prepare: %u cases passed\n", cases);
  return 0;
}
