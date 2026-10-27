#define CONFIG_IDF_TARGET_ESP32 1
#define CONFIG_IDF_TARGET "esp32"
#include <cassert>
#include <initializer_list>
#include "support/runtime.hpp"
#include "support/probes.hpp"
#include "board_detect/m5/esp32_d0wdq6.inl"
#include "support/display_backend.hpp"
#include "support/snapshot.hpp"
using namespace m5gfx::board_detect;
using namespace m5gfx::board_detect::m5;
static detection_transaction_t *tx;
static unsigned cases;
static probe_ctx_t context() {
  probe_ctx_t ctx;
  ctx.transaction = tx;
  ctx.probe_port = -1;
  ctx.allow_reset = false;
  return ctx;
}
static void axp_table() {
  for (int pmic : {3, 0x4a})
    for (bool core : {false, true})
      for (bool tough : {false, true})
        for (int g12 = 0; g12 < 4; ++g12)
          for (int g32 = 0; g32 < 4; ++g32)
            for (int g33 = 0; g33 < 4; ++g33)
              for (bool final : {false, true})
                for (bool panel : {false, true})
                  for (board_id_t preference :
                       {board_id_unknown, desc_core2.def.id, desc_tough.def.id, desc_station.def.id,
                        board_id_t(65535)})
                    for (board_id_t hint : {board_id_unknown, desc_core2.def.id, desc_tough.def.id,
                                            desc_station.def.id, board_id_t(65535)}) {
                      host_hardware::reset();
                      host_probes::reset();
                      host_probes::expected_sda = 21;
                      host_probes::expected_scl = 22;
                      unsigned ack = 0, station_reads = 0, writes = 0, sense = 0;
                      host_hardware::wire = [&](int addr, int reg, std::uint8_t *data, std::size_t,
                                                bool write) {
                        if (write) {
                          ++writes;
                          return true;
                        }
                        if (addr == 0x34) {
                          *data = pmic;
                          return true;
                        }
                        assert(addr == 0x38 || addr == 0x2e);
                        assert(reg == -1);
                        ++ack;
                        return addr == 0x38 ? core : tough;
                      };
                      host_probes::pulls = [&](std::uint64_t mask, std::uint32_t) {
                        pin_pull_result_t p;
                        std::uint64_t sense_mask = (1ull << 12) | (1ull << 32) | (1ull << 33);
                        if (mask != sense_mask) {
                          p.pulldown_high = p.pullup_high = mask;
                          return p;
                        }
                        ++sense;
                        p.pulldown_high = std::uint64_t(g12 & 1) << 12 |
                                          std::uint64_t(g32 & 1) << 32 |
                                          std::uint64_t(g33 & 1) << 33;
                        p.pullup_high = std::uint64_t((g12 >> 1) & 1) << 12 |
                                        std::uint64_t((g32 >> 1) & 1) << 32 |
                                        std::uint64_t((g33 >> 1) & 1) << 33;
                        return p;
                      };
                      host_probes::spi = [&](int, int, int, int, int, std::uint8_t) {
                        ++station_reads;
                        assert(host_probes::transfers > 0);
                        return panel ? 0x81u : 0u;
                      };
                      auto ctx = context();
                      ctx.final_attempt = final;
                      ctx.attempt = final ? 4 : 0;
                      ctx.fallback = preference;
                      ctx.hint = hint;
                      board_result_t r;
                      bool ok = axp_family_detector.confirm(ctx, &r);
                      board_id_t expected = 0;
                      bool provisional = false, refine = false, observe = false, station = false;
                      if (!(core && tough) && !(pmic == 0x4a && tough)) {
                        if (core || tough) {
                          expected = core ? desc_core2.def.id : desc_tough.def.id;
                          observe = true;
                        } else if (pmic == 0x4a) {
                          if (final) {
                            expected = desc_core2.def.id;
                            provisional = true;
                            observe = true;
                          }
                        } else if (g12 == 0 && g32 == 3 && g33 == 3) {
                          expected = desc_core2.def.id;
                          refine = true;
                        } else if (g12 == 2 || g12 == 0) {
                          station = true;
                          if (panel)
                            expected = desc_station.def.id;
                        }
                      }
                      assert(ok == (expected != 0) && ack == 2 && ctx.family_identified &&
                             writes == 0);
                      assert(sense == unsigned(!core && !tough && pmic == 3) &&
                             station_reads == unsigned(station));
                      assert(bool(host_probes::transfers) == station);
                      if (ok) {
                        assert(r.def->id == expected && r.provisional == provisional &&
                               bool(r.refine) == refine && bool(r.observe_after_power) == observe);
                        assert(r.option ==
                               (pmic == 0x4a ? generated_options::core2::new_pmic : 0u));
                      }
                      ++cases;
                    }
  host_hardware::wire = {};
  auto ctx = context();
  board_result_t r;
  assert(!axp_family_detector.confirm(ctx, &r) && !ctx.family_identified);
}
static void core_refinement() {
  for (bool new_pmic : {false, true})
    for (bool core : {false, true})
      for (bool tough : {false, true})
        for (bool final : {false, true})
          for (board_id_t preference : {board_id_unknown, desc_core2.def.id, desc_tough.def.id,
                                        desc_station.def.id, board_id_t(65535)})
            for (board_id_t hint : {board_id_unknown, desc_core2.def.id, desc_tough.def.id,
                                    desc_station.def.id, board_id_t(65535)}) {
              host_hardware::reset();
              host_probes::reset();
              host_probes::expected_sda = 21;
              host_probes::expected_scl = 22;
              unsigned core_reads = 0, tough_reads = 0;
              host_hardware::wire = [&](int addr, int, std::uint8_t *, std::size_t, bool write) {
                assert(!write);
                if (addr == 0x38) {
                  ++core_reads;
                  return core;
                }
                assert(addr == 0x2e);
                ++tough_reads;
                return tough;
              };
              host_probes::spi_data = {2, 56};
              host_probes::on_read = []() { assert(host_probes::transfers > 0); };
              auto ctx = context();
              ctx.final_attempt = final;
              ctx.fallback = preference;
              ctx.hint = hint;
              board_result_t r;
              r.assign(&desc_core2);
              r.option = new_pmic ? generated_options::core2::new_pmic : 0;
              bool ok = detail::refine_core_family(r, ctx),
                   contradiction = (core && tough) || (new_pmic && tough);
              assert(ok == (!contradiction && (core || tough || final)));
              assert(core_reads == tough_reads && core_reads > 0 && host_probes::reads == 2 &&
                     host_probes::transfers > 0);
              if (ok) {
                auto possible = [&](board_id_t id) {
                  return id == desc_core2.def.id || (id == desc_tough.def.id && !new_pmic);
                };
                board_id_t expected = core                   ? desc_core2.def.id
                                      : tough                ? desc_tough.def.id
                                      : possible(preference) ? preference
                                      : possible(hint)       ? hint
                                                             : desc_core2.def.id;
                assert(r.def->id == expected && r.provisional == (!core && !tough));
                assert(r.option == ((expected == desc_tough.def.id ? 0u
                                     : new_pmic ? generated_options::core2::new_pmic
                                                : 0u) |
                                    generated_options::core2::lcd_e));
              }
              assert(host_hardware::ports.empty());
              ++cases;
            }
  for (const auto *confirmed : {&desc_core2, &desc_tough}) {
    host_hardware::reset();
    host_probes::reset();
    host_probes::expected_sda = 21;
    host_probes::expected_scl = 22;
    host_hardware::wire = [](int, int, std::uint8_t *, std::size_t, bool) -> bool {
      assert(false);
      return false;
    };
    host_probes::spi_data = {2, 56};
    board_result_t r;
    r.assign(confirmed);
    auto ctx = context();
    assert(detail::observe_confirmed_core_panel(r, ctx));
    assert(r.def == &confirmed->def && r.option == generated_options::core2::lcd_e &&
           !r.provisional);
    auto transfers = host_probes::transfers;
    host_probes::cursor = 0;
    assert(detail::observe_confirmed_core_panel(r, ctx) && host_probes::transfers == transfers);
    assert(host_hardware::ports.empty());
    ++cases;
  }
}
static void atom_and_stack() {
  host_hardware::reset();
  host_probes::reset();
  host_probes::expected_sda = 21;
  host_probes::expected_scl = 22;
  host_touch::scans = host_touch::clears = 0;
  bool g2 = true, voice = false;
  int g18 = 2, g12 = 0, sd_class = 2;
  unsigned g32_reads = 0;
  host_hardware::input = [&](int pin, int, bool) {
    if (pin == 2)
      return g2;
    if (pin == 34)
      return voice;
    return false;
  };
  host_probes::pulls = [&](std::uint64_t mask, std::uint32_t release) {
    int kind = mask == (1ull << 18) ? g18 : mask == (1ull << 12) ? g12 : sd_class;
    if (mask == (1ull << 32))
      ++g32_reads;
    pin_pull_result_t p;
    if (kind & 1)
      p.pulldown_high = mask;
    if (kind & 2)
      p.pullup_high = mask;
    p.release_sampled = release != 0;
    return p;
  };
  atom_family_detector_t atom;
  auto ctx = context();
  board_result_t r;
  assert(atom.signature(ctx) && atom.confirm(ctx, &r));
  assert(r.def == &desc_atommatrix.def && r.provisional);
  detect_outcome_t out;
  out.result = r;
  finalize_prepared_result(out, 0);
  out.adopted = true;
  assert(out.candidate_kind == candidate_kind_t::provisional && !should_persist_detection(out, 0));
  assert(host_touch::scans == 1 && host_touch::clears == 1);
  ++cases;
  for (bool final : {false, true})
    for (bool hint : {false, true})
      for (sd_class = 0; sd_class < 4; ++sd_class) {
        ctx.final_attempt = final;
        ctx.hint = hint ? desc_stack.def.id : 0;
        stack_family_detector_t stack;
        assert(stack.signature(ctx) == (sd_class == 3) && g32_reads == 0);
        ++cases;
      }
  atom_family_detector_t excluded;
  g2 = false;
  voice = true;
  assert(!excluded.signature(ctx));
  g2 = true;
  for (g18 = 0; g18 < 4; ++g18)
    if (g18 != 2)
      assert(!excluded.signature(ctx));
  g18 = 2;
  g12 = 3;
  assert(excluded.signature(ctx) && excluded.confirm(ctx, &r) && r.def == &desc_atomvoice.def &&
         !r.provisional);
  assert(host_touch::scans == 1);
  voice = false;
  atom_family_detector_t blocked;
  g18 = 3;
  assert(!blocked.signature(ctx));
  g18 = 2;
  for (g12 = 1; g12 < 4; ++g12)
    assert(!blocked.signature(ctx));
  assert(host_touch::scans == 1);
  ++cases;
}
static void stack_confirmation() {
  for (bool final : {false, true})
    for (bool hint : {false, true}) {
      host_hardware::reset();
      host_probes::reset();
      host_probes::expected_sda = 21;
      host_probes::expected_scl = 22;
      auto ctx = context();
      ctx.final_attempt = final;
      ctx.hint = hint ? desc_stack.def.id : 0;
      stack_family_detector_t stack;
      board_result_t r;
      assert(!stack.confirm(ctx, &r));
      host_probes::spi = [](int, int, int, int, int, std::uint8_t) { return 0xe3u; };
      assert(stack.confirm(ctx, &r) && r.def == &desc_stack.def && !r.provisional);
      detect_outcome_t out;
      out.result = r;
      finalize_prepared_result(out, 0);
      out.adopted = true;
      assert(should_persist_detection(out, 0) && !should_persist_detection(out, desc_stack.def.id));
      ++cases;
    }
}
static void fixed_core() {
  for (bool pmic_known : {false, true})
    for (bool new_pmic : {false, true})
      for (bool panel_e : {false, true})
        for (const auto *selected : {&desc_core2, &desc_tough}) {
          host_hardware::reset();
          host_probes::reset();
          host_probes::expected_sda = 21;
          host_probes::expected_scl = 22;
          unsigned writes = 0, identity_reads = 0;
          host_hardware::wire = [&](int addr, int reg, std::uint8_t *data, std::size_t,
                                    bool write) {
            assert(addr == 0x34);
            if (write) {
              ++writes;
              return true;
            }
            ++identity_reads;
            *data = reg == 3 ? (new_pmic ? 0x4a : 3) : 0;
            return pmic_known;
          };
          host_probes::on_read = [&]() { assert(writes > 0); };
          if (panel_e)
            host_probes::spi_data = {2, 56};
          auto ctx = context();
          board_result_t r;
          r.assign(selected);
          assert(detail::fixed_prepare_core(r, ctx) == pmic_known);
          assert(identity_reads > 0);
          if (!pmic_known)
            assert(writes == 0 && host_probes::reads == 0);
          else {
            assert(writes > 0 && r.def == &selected->def && r.refine == nullptr && !r.provisional);
            assert(r.option == ((new_pmic ? generated_options::core2::new_pmic : 0u) |
                                (panel_e ? generated_options::core2::lcd_e : 0u)));
          }
          ++cases;
        }
  for (bool opened : {false, true})
    for (bool reset_ok : {false, true}) {
      host_hardware::reset();
      host_probes::reset();
      host_probes::expected_sda = 21;
      host_probes::expected_scl = 22;
      host_hardware::i2c_open_ok = opened;
      host_hardware::wire = [&](int addr, int, std::uint8_t *data, std::size_t, bool) {
        assert(addr == 0x34);
        *data = 3;
        return reset_ok;
      };
      host_probes::spi_data = {2, 56, 2, 56};
      auto ctx = context();
      ctx.allow_reset = true;
      board_result_t r;
      r.assign(&desc_core2);
      assert(detail::observe_confirmed_core_panel(r, ctx) == (opened && reset_ok));
      assert(r.def == &desc_core2.def && !r.provisional);
      if (!opened || !reset_ok)
        assert(!(r.option & generated_options::core2::lcd_e));
      ++cases;
    }
}

static void strict_reset() {
  for (const auto *desc : {&desc_core2, &desc_tough}) {
    host_hardware::reset();
    unsigned writes = 0;
    host_hardware::wire = [&](int, int, std::uint8_t *data, std::size_t, bool write) {
      if (write) { ++writes; return false; }
      *data = 3; // AXP192 identifies successfully before the reset operation fails.
      return true;
    };
    auto ctx = context();
    ctx.allow_reset = true;
    board_result_t result;
    result.assign(desc);
    assert(!prepare_reset(*desc, result, ctx, -1));
    assert(writes > 0 && !(result.prepared & prepared_reset));
    ++cases;
  }
}
static void stickc_shared_read() {
  for (std::uint32_t id : {0x7cu, 0x81u, 0x85u, 0u}) {
    host_hardware::reset();
    host_probes::reset();
    host_probes::expected_zero_preamble = true;
    host_probes::spi = [&](int sclk, int mosi, int miso, int dc, int cs, std::uint8_t cmd) {
      assert(sclk == 13 && mosi == 15 && miso == 15 && dc == 23 && cs == 5 && cmd == 4);
      return id;
    };
    auto ctx = context();
    board_result_t result;
    assert(stickc_family_detector.signature(ctx));
    const bool matched = stickc_family_detector.confirm(ctx, &result);
    assert(matched == (id != 0) && host_probes::spi_calls == 1);
    if (matched)
      assert(result.desc == (id == 0x7c ? &desc_stickc : &desc_stickcplus));
    assert(host_hardware::ports.empty());
    ++cases;
  }
}
int main() {
  assert(find_board(esp32_d0wdq6_boards, desc_core2.def.id)->fixed_prepare ==
         detail::fixed_prepare_core);
  assert(find_board(esp32_d0wdq6_boards, desc_tough.def.id)->fixed_prepare ==
         detail::fixed_prepare_core);
  assert(find_board(esp32_d0wdq6_boards, desc_coreink.def.id)->fixed_prepare == fixed_prepare_coreink);
  host_snapshot::check("esp32", esp32_d0wdq6_boards);
  detection_transaction_t transaction(pins(wiring::detection::unconditional_pins), no_pins(),
                                      false);
  assert(transaction.valid());
  tx = &transaction;
  axp_table();
  core_refinement();
  atom_and_stack();
  stack_confirmation();
  fixed_core();
  strict_reset();
  stickc_shared_read();
  std::printf("ESP32 families: %u cases passed\n", cases);
}
