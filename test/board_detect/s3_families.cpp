#define CONFIG_IDF_TARGET_ESP32S3 1
#define CONFIG_IDF_TARGET "esp32s3"
#include <cassert>
#include <initializer_list>
#include "support/runtime.hpp"
#include "support/probes.hpp"
#include "board_detect/m5/esp32s3.inl"
#include "support/display_backend.hpp"
#include "support/snapshot.hpp"
using namespace m5gfx::board_detect;
using namespace m5gfx::board_detect::m5;
static unsigned cases;
static detection_transaction_t *tx;
static probe_ctx_t context() {
  probe_ctx_t c;
  c.transaction = tx;
  c.probe_port = -1;
  return c;
}

static void pm1_table() {
  for (int sw = 0; sw < 2; ++sw)
    for (int pm = 0; pm < 2; ++pm)
      for (int g12 = 0; g12 < 4; ++g12)
        for (int g13 = 0; g13 < 4; ++g13)
          for (bool final : {false, true})
            for (board_id_t fallback : {board_id_unknown, desc_stopwatch.def.id,
                                         desc_papermono.def.id, board_id_t(65535)})
              for (board_id_t hint :
                   {board_id_unknown, desc_stopwatch.def.id, desc_papermono.def.id}) {
                host_hardware::reset();
                host_probes::reset();
                host_probes::expected_sda = 47;
                host_probes::expected_scl = 48;
                unsigned reads = 0, writes = 0;
                host_probes::pulls = [&](std::uint64_t, std::uint32_t) {
                  pin_pull_result_t p;
                  p.pulldown_high = std::uint64_t(g12 & 1) << 12 | std::uint64_t(g13 & 1) << 13;
                  p.pullup_high =
                      std::uint64_t((g12 >> 1) & 1) << 12 | std::uint64_t((g13 >> 1) & 1) << 13;
                  return p;
                };
                host_hardware::wire = [&](int addr, int, std::uint8_t *data, std::size_t len,
                                          bool write) {
                  if (write) {
                    ++writes;
                    return true;
                  }
                  if (addr == 0x6e) {
                    assert(len == 2);
                    data[0] = 0x50;
                    data[1] = 0x20;
                    return true;
                  }
                  ++reads;
                  assert(addr == 0x15 || addr == 0x38);
                  return bool(addr == 0x15 ? sw : pm);
                };
                auto ctx = context();
                ctx.final_attempt = final;
                ctx.attempt = final ? 4 : 0;
                ctx.fallback = fallback;
                ctx.hint = hint;
                board_result_t r;
                bool ok = pm1_family_detector.confirm(ctx, &r);
                assert(reads == 2 && ctx.family_identified && writes == 0);
                board_id_t expected = 0;
                bool provisional = false, refine = false;
                if (sw != pm)
                  expected = sw ? desc_stopwatch.def.id : desc_papermono.def.id;
                else if (!sw && ((g12 == 0 && g13 == 0) || (g12 == 3 && g13 == 3))) {
                  expected = desc_papermono.def.id;
                  refine = true;
                } else if (final) {
                  provisional = true;
                  if (!sw && (g12 == 2 || (g12 == 0 && g13 == 3)))
                    expected = desc_stopwatch.def.id;
                  else
                    expected =
                        (fallback == desc_stopwatch.def.id || fallback == desc_papermono.def.id)
                            ? fallback
                        : (hint == desc_stopwatch.def.id || hint == desc_papermono.def.id) ? hint
                                                                                           : 0;
                }
                assert(ok == (expected != 0));
                if (ok) {
                  assert(r.def->id == expected && r.provisional == provisional &&
                         bool(r.refine) == refine);
                }
                ++cases;
              }
  host_hardware::wire = {};
  auto ctx = context();
  board_result_t r;
  assert(!pm1_family_detector.confirm(ctx, &r) && !ctx.family_identified);
}

static void paper_refinement() {
  for (bool final : {false, true})
    for (bool response : {false, true})
      for (bool opened : {false, true}) {
        host_hardware::reset();
        host_probes::reset();
        host_probes::expected_sda = 47;
        host_probes::expected_scl = 48;
        host_hardware::i2c_open_ok = opened;
        unsigned reads = 0, writes = 0;
        host_hardware::wire = [&](int addr, int, std::uint8_t *, std::size_t, bool write) {
          if (write)
            ++writes;
          else {
            ++reads;
            assert(addr == 0x38);
          }
          return response;
        };
        auto ctx = context();
        ctx.final_attempt = final;
        board_result_t r;
        r.assign(&desc_papermono);
        bool ok = detail::refine_papermono_touch(r, ctx);
        assert(ok == ((opened && response) || final));
        assert(r.def == &desc_papermono.def && r.provisional == (final && !(opened && response)));
        assert(reads == (opened ? (response ? 1 : 200) : 0) && writes == 0);
        assert(host_hardware::ports.empty());
        ++cases;
      }
  for (board_id_t hint : {board_id_unknown, desc_stopwatch.def.id, desc_papermono.def.id})
    for (board_id_t fallback : {board_id_unknown, desc_stopwatch.def.id, desc_papermono.def.id}) {
      host_hardware::wire = [](int, int, std::uint8_t *, std::size_t, bool) { return false; };
      auto ctx = context();
      ctx.hint = hint;
      ctx.fallback = fallback;
      board_result_t r;
      r.assign(&desc_papermono);
      assert(!detail::refine_papermono_touch(r, ctx));
      ctx.final_attempt = true;
      assert(detail::refine_papermono_touch(r, ctx));
      assert(r.def == &desc_papermono.def && r.provisional);
      detect_outcome_t out;
      out.result = r;
      assert(finalize_prepared_result(out, fallback) == (fallback != desc_stopwatch.def.id));
      assert(host_hardware::ports.empty());
      ++cases;
    }
}

static void cardputer_table() {
  for (int a : {0, 15, 10, 14, 2, 1})
    for (int b : {0, 15, 10, 14, 2, 1})
      for (int c : {0, 15, 10, 14, 2, 1})
        for (int d : {0, 15, 10, 14, 2, 1})
          for (bool vam : {false, true})
            for (bool adv : {false, true})
              for (bool final : {false, true})
                for (board_id_t pref :
                     {board_id_unknown, desc_cardputer.def.id, desc_cardputer_adv.def.id,
                      desc_vameter.def.id, board_id_t(65535)})
                  for (board_id_t hint :
                       {board_id_unknown, desc_cardputer.def.id, desc_cardputer_adv.def.id,
                        desc_vameter.def.id, board_id_t(65535)}) {
                    host_hardware::reset();
                    host_probes::reset();
                    host_probes::expected_i2c = [](int sda, int scl, std::uint8_t addr) {
                      assert(sda == (addr == 0x34 ? 8 : 5));
                      assert(scl == (addr == 0x34 ? 9 : 6));
                    };
                    unsigned pulls = 0, vreads = 0, areads = 0, companion = 0;
                    int samples[] = {a, b, c, d};
                    host_probes::pulls = [&](std::uint64_t mask, std::uint32_t release) {
                      ++pulls;
                      pin_pull_result_t p;
                      p.release_sampled = release != 0;
                      assert(!release || release == pull_release_us);
                      for (unsigned n = 0; n < 4; ++n) {
                        auto bit = std::uint64_t(1)
                                   << wiring::cardputer::cardputer_subdivision::sense_pins[n];
                        assert(mask & bit);
                        auto sample = samples[n];
                        if (sample & 1)
                          p.pulldown_high |= bit;
                        if (sample & 2)
                          p.pullup_high |= bit;
                        if (sample & 4)
                          p.pulldown_release_high |= bit;
                        if (sample & 8)
                          p.pullup_release_high |= bit;
                      }
                      return p;
                    };
                    host_probes::spi = [](int, int, int, int, int, std::uint8_t) {
                      return cardputer_probes[0].values[0];
                    };
                    host_hardware::wire = [&](int addr, int, std::uint8_t *data, std::size_t len,
                                              bool write) {
                      assert(!write);
                      if (addr == 0x40) {
                        ++vreads;
                        assert(len == 2);
                        data[0] = 0x54;
                        data[1] = 0x49;
                        return vam;
                      }
                      if (addr == 0x34) {
                        ++areads;
                        return adv;
                      }
                      assert(addr == 0x41);
                      ++companion;
                      return vam;
                    };
                    auto ctx = context();
                    ctx.final_attempt = final;
                    ctx.fallback = pref;
                    ctx.hint = hint;
                    board_result_t r;
                    bool ok = cardputer_family_detector.confirm(ctx, &r);
                    bool vu = a == 15 && b == 15, au = c == 15 && d == 15,
                         floating = a == 10 && b == 10 && c == 10 && d == 10;
                    bool v = vu && vam, w = au && adv, provisional = false;
                    board_id_t expected = 0;
                    if (v && w) {
                    } else if (v)
                      expected = desc_vameter.def.id;
                    else if (w)
                      expected = desc_cardputer_adv.def.id;
                    else if (floating)
                      expected = desc_cardputer.def.id;
                    else if (final) {
                      provisional = true;
                      auto possible = [&](board_id_t id) {
                        return id == desc_cardputer.def.id ||
                               (id == desc_vameter.def.id && (a & 1) && (b & 1)) ||
                               (id == desc_cardputer_adv.def.id && (c & 1) && (d & 1));
                      };
                      expected = possible(pref)   ? pref
                                 : possible(hint) ? hint
                                                  : desc_cardputer.def.id;
                    }
                    assert(ok == (expected != 0));
                    assert(pulls == 2 && vreads == unsigned(vu) && areads == unsigned(au) &&
                           companion == unsigned(vu && vam));
                    if (ok)
                      assert(r.def->id == expected && r.provisional == provisional);
                    ++cases;
                  }
  host_hardware::reset();
  host_probes::reset();
  host_probes::expected_i2c = [](int sda, int scl, std::uint8_t addr) {
    assert(sda == (addr == 0x34 ? 8 : 5));
    assert(scl == (addr == 0x34 ? 9 : 6));
  };
  auto ctx = context();
  board_result_t result;
  // An unreadable LCD must return before any subdivision pull measurement.
  assert(!cardputer_family_detector.confirm(ctx, &result));
  ++cases;
  ctx.conditional_pins_unavailable = true;
  host_probes::spi = [](int, int, int, int, int, std::uint8_t) -> std::uint32_t {
    assert(false);
    return 0;
  };
  assert(!cardputer_family_detector.confirm(ctx, &result));
  ++cases;
}

static void paper_retry() {
  for (const auto *target : {&desc_papers3, &desc_paperdiy}) {
    host_hardware::reset();
    host_probes::reset();
    host_probes::expected_sda = 41;
    host_probes::expected_scl = 42;
    host_hardware::wire = [&](int addr, int, std::uint8_t *data, std::size_t, bool write) {
      assert(!write);
      if (target == &desc_paperdiy && addr == 0x6e) {
        data[0] = 0x50;
        data[1] = 0x20;
        return true;
      }
      if (target == &desc_papers3 && addr == 0x14) {
        data[0] = '9';
        data[1] = '1';
        data[2] = '1';
        data[3] = 0;
        return true;
      }
      return false;
    };
    detect_request_t request;
    request.max_attempts = 5;
    request.hint = target == &desc_papers3 ? desc_paperdiy.def.id : desc_papers3.def.id;
    auto out = run_detection_session(request, [&](const detect_request_t &req, bool last) {
      auto ctx = context();
      ctx.hint = req.hint;
      ctx.attempt = req.attempt;
      ctx.final_attempt = last;
      detect_outcome_t out;
      if (paper_family_detector.confirm(ctx, &out.result)) {
        finalize_prepared_result(out, 0);
        out.adopted = true;
      }
      return out;
    });
    assert(out.adopted && out.result.desc == target && out.attempts == 2 &&
           out.verdict == verdict_t::confirmed);
    auto ctx = context();
    ctx.hint = request.hint;
    ctx.final_attempt = true;
    board_result_t r;
    assert(paper_family_detector.confirm(ctx, &r) && r.desc == target);
    ++cases;
  }
}

static void cores3_table() {
  for (bool before : {false, true})
    for (bool after : {false, true})
      for (unsigned band = 0; band < 4; ++band)
        for (bool gate : {false, true})
          for (int at : {-1, 0, 10})
            for (bool read_ok : {false, true})
              for (unsigned firmware : {3u, 4u})
                for (bool final : {false, true})
                  for (board_id_t preference :
                       {board_id_unknown, desc_cores3.def.id, desc_cores3se.def.id,
                        desc_stackchan.def.id, board_id_t(65535)})
                    for (board_id_t hint :
                         {board_id_unknown, desc_cores3.def.id, desc_cores3se.def.id,
                          desc_stackchan.def.id, board_id_t(65535)}) {
                      host_hardware::reset();
                      host_probes::reset();
                      host_probes::expected_sda = 12;
                      host_probes::expected_scl = 11;
                      unsigned cameras = 0, gates = 0, fwreads = 0, acks = 0;
                      std::uint32_t last_ack = 0;
                      host_probes::pulls = [&](std::uint64_t mask, std::uint32_t) {
                        ++gates;
                        assert(mask == ((1ull << 5) | (1ull << 6) | (1ull << 7)));
                        pin_pull_result_t p;
                        p.pulldown_high = gate ? 1ull << 6 : 0;
                        p.pullup_high = gate ? 1ull << 6 : mask;
                        return p;
                      };
                      host_hardware::wire = [&](int addr, int reg, std::uint8_t *data, std::size_t,
                                                bool write) {
                        assert(!write);
                        if (addr == specs::cores3::i2c_camera::i2c_addr) {
                          ++cameras;
                          *data = specs::cores3::i2c_camera::id_value;
                          return after;
                        }
                        assert(addr == 0x6f);
                        if (reg < 0) {
                          ++acks;
                          last_ack = lgfx::millis();
                          return at >= 0 && last_ack >= unsigned(at);
                        }
                        ++fwreads;
                        assert(reg == 2);
                        *data = firmware;
                        return read_ok;
                      };
                      auto ctx = context();
                      ctx.final_attempt = final;
                      ctx.fallback = preference;
                      ctx.hint = hint;
                      board_result_t r;
                      r.assign(&desc_cores3);
                      r.refine_state = band;
                      r.option = before ? cores3_detail::internal_camera_confirmed : 0;
                      bool camera = before || after,
                           acknowledged = at == 0 || (gate && at >= 0 && at <= 50),
                           provisional = false;
                      board_id_t expected = 0;
                      auto possible = [&](board_id_t id) {
                        return id == desc_cores3.def.id ||
                               (id == desc_cores3se.def.id && !camera && band != 2) ||
                               (id == desc_stackchan.def.id && acknowledged);
                      };
                      if (!camera && (band == 0 || band == 3))
                        expected = desc_cores3se.def.id;
                      else if (!camera && band == 1) {
                        if (final) {
                          provisional = true;
                          expected = possible(preference) ? preference
                                     : possible(hint)     ? hint
                                                          : desc_cores3.def.id;
                        }
                      } else if (acknowledged && !read_ok) {
                        if (final) {
                          provisional = true;
                          auto compatible = [&](board_id_t id) {
                            return id == desc_cores3.def.id || id == desc_stackchan.def.id;
                          };
                          expected = compatible(preference) ? preference
                                     : compatible(hint)     ? hint
                                                            : desc_stackchan.def.id;
                        }
                      } else
                        expected = acknowledged && read_ok && firmware >= 4 ? desc_stackchan.def.id
                                                                            : desc_cores3.def.id;
                      bool ok = cores3_detail::refine(r, ctx);
                      assert(ok == (expected != 0));
                      assert(r.refine_state == 0 &&
                             r.option == (before ? cores3_detail::internal_camera_confirmed : 0));
                      assert(cameras == unsigned(!before) && gates == 1 &&
                             fwreads == unsigned(acknowledged));
                      assert(acks > 0 && last_ack == (gate ? (at < 0 ? 50u : unsigned(at)) : 0u));
                      if (ok)
                        assert(r.def->id == expected && r.provisional == provisional);
                      assert(host_hardware::ports.empty());
                      ++cases;
                    }
}

static void cores3_transport() {
  for (unsigned band = 0; band < 3; ++band)
    for (bool available : {false, true})
      for (bool camera : {false, true}) {
        host_hardware::reset();
        host_probes::reset();
        host_probes::expected_sda = 12;
        host_probes::expected_scl = 11;
        auto &measured = host_probes::dedicated;
        measured.available = available;
        measured.cpu_hz = 1000000000;
        measured.reads = 64;
        measured.requested_samples = 8;
        measured.total_cycles = 64 * 8 * 100;
        measured.pin_count = 8;
        for (unsigned i = 0; i < 8; ++i) {
          measured.valid_samples[i] = 8;
          measured.median_first_high[i] = band + 2;
        }
        host_hardware::wire = [&](int addr, int, std::uint8_t *data, std::size_t, bool write) {
          assert(!write);
          if (addr == 0x34) {
            *data = 0x4a;
            return true;
          }
          if (addr == 0x58) {
            *data = 0x23;
            return true;
          }
          assert(addr == specs::cores3::i2c_camera::i2c_addr);
          *data = specs::cores3::i2c_camera::id_value;
          return camera;
        };
        host_probes::pulls = [](std::uint64_t, std::uint32_t) { return pin_pull_result_t{}; };
        auto ctx = context();
        board_result_t r;
        assert(cores3_family_detector.confirm(ctx, &r));
        unsigned transported = camera ? 2 : available ? band : 3;
        assert(r.refine_state == transported && host_probes::dedicated_calls == unsigned(!camera));
        assert(bool(r.option & cores3_detail::internal_camera_confirmed) == camera &&
               bool(r.option & cores3_detail::vbus_5v));
        assert(r.refine == cores3_detail::refine &&
               r.def == (transported == 0 ? &desc_cores3se.def : &desc_cores3.def));
        assert(host_hardware::ports.empty());
        ++cases;
      }
  auto ctx = context();
  ctx.conditional_pins_unavailable = true;
  host_hardware::wire = [](int, int, std::uint8_t *, std::size_t, bool) -> bool {
    assert(false);
    return false;
  };
  assert(!cores3_family_detector.signature(ctx));
  assert(host_hardware::ports.empty());
  ++cases;
}

static void fixed_core_s3() {
  host_hardware::reset();
  host_probes::reset();
  host_probes::expected_sda = 12;
  host_probes::expected_scl = 11;
  unsigned power_writes = 0, vbus_reads = 0;
  host_hardware::wire = [&](int addr, int reg, std::uint8_t *data, std::size_t, bool write) {
    assert(addr == 0x34 || addr == 0x58);
    if (write) {
      ++power_writes;
      return true;
    }
    *data = addr == 0x58 && reg == 2 ? 2 : 0;
    return true;
  };
  host_probes::pulls = [&](std::uint64_t, std::uint32_t) {
    ++vbus_reads;
    assert(power_writes == 0);
    return pin_pull_result_t{};
  };
  host_probes::spi = [&](int sclk, int mosi, int miso, int dc, int cs, std::uint8_t cmd) {
    assert(sclk == 36 && mosi == 37 && miso == 37 && dc == 35 && cs == 3 && cmd == 4);
    assert(miso == mosi && miso != dc);
    assert(power_writes > 0);
    return specs::cores3::probe_ili9342c::values[0];
  };
  host_probes::spi_data = {2, 56};
  auto ctx = context();
  ctx.allow_reset = false;
  board_result_t r;
  r.assign(&desc_cores3);
  assert(cores3_detail::fixed_prepare(r, ctx));
  assert(vbus_reads == 1 && power_writes > 0 && host_probes::reads == 2);
  assert(r.def == &desc_cores3.def && r.refine == nullptr && !r.provisional &&
         r.option == (cores3_detail::vbus_5v | generated_options::cores3::lcd_e));
  ++cases;
}

int main() {
  assert(find_board(esp32s3_boards, desc_cores3.def.id)->fixed_prepare == cores3_detail::fixed_prepare);
  assert(find_board(esp32s3_boards, desc_cores3se.def.id)->fixed_prepare ==
         cores3_detail::fixed_prepare);
  assert(find_board(esp32s3_boards, desc_stackchan.def.id)->fixed_prepare ==
         cores3_detail::fixed_prepare);
  assert(find_board(esp32s3_boards, desc_atoms3.def.id)->fixed_prepare == fixed_prepare_atoms3);
  assert(find_board(esp32s3_boards, desc_atoms3r.def.id)->fixed_prepare == fixed_prepare_atoms3r);
  assert(find_board(esp32s3_boards, desc_airq.def.id)->fixed_prepare == fixed_prepare_airq);
  host_snapshot::check("s3", esp32s3_boards);
  detection_transaction_t transaction(pins(wiring::detection::unconditional_pins),
                                      pins(wiring::detection::opi_pins), true);
  assert(transaction.valid());
  tx = &transaction;
  pm1_table();
  paper_refinement();
  cardputer_table();
  paper_retry();
  cores3_table();
  cores3_transport();
  fixed_core_s3();
  std::printf("S3 families: %u cases passed\n", cases);
}
