#pragma once
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>
#include "board_detect/m5/generated/gpio_power_hold_board_ids.hpp"
namespace host_snapshot {
using namespace m5gfx::board_detect;
inline void operations(std::ostream &out, ops::op_list_t list) {
  out << '[';
  for (unsigned n = 0; n < list.size; ++n) {
    const auto &op = list.data[n];
    out << unsigned(op.kind()) << ':';
    ops::decoded_register_write_t r;
    ops::decoded_register_write16_t r16;
    if (ops::decode_register_write(op, &r))
      out << unsigned(r.dev) << ',' << r.reg << ',' << unsigned(r.value) << ',' << unsigned(r.mask);
    else if (op.kind() == ops::op_kind_t::i2c_write16le) {
      r16 = {ops::op_access_t::dev(op), ops::op_access_t::reg16(op).reg,
             ops::op_access_t::reg16(op).value};
      out << unsigned(r16.dev) << ',' << r16.reg << ',' << r16.value;
    } else if (op.kind() == ops::op_kind_t::gpio_set_mode)
      out << ops::op_access_t::pin(op) << ',' << unsigned(ops::op_access_t::mode(op));
    else if (op.kind() == ops::op_kind_t::gpio_write_high ||
             op.kind() == ops::op_kind_t::gpio_write_low)
      out << ops::op_access_t::pin(op);
    else if (op.kind() == ops::op_kind_t::delay_ms)
      out << ops::op_access_t::milliseconds(op);
    else if (op.kind() == ops::op_kind_t::i2c_wait_ready)
      out << unsigned(ops::op_access_t::dev(op)) << ',' << ops::op_access_t::timeout(op) << ','
          << ops::op_access_t::interval(op);
    out << ';';
  }
  out << ']';
}
inline void pins(std::ostream &out, pin_list_t list) {
  out << '[';
  for (unsigned i = 0; i < list.size; ++i)
    out << int(list.data[i]) << ',';
  out << ']';
}
inline void descriptor(std::ostream &out, const board_desc_t &d) {
  out << d.def.id << ' ' << d.def.name << ' ' << unsigned(d.def.flags) << '\n';
  const auto &p = d.power;
  out << "power " << int(p.hold_pin) << ' ' << p.hold_high << ' ' << p.i2c_freq << ' '
      << unsigned(p.warm_wait_ms) << ' ' << unsigned(p.cold_wait_ms) << ' '
      << unsigned(p.wake_poll_ms) << ' ' << p.variant_confirmed << '\n';
  for (unsigned i = 0; i < p.device_count; ++i) {
    const auto &v = p.devices[i];
    out << "device " << v.freq_hz << ' ' << v.wire_timeout_ms << ' ' << v.addr << ' '
        << unsigned(v.bus_id) << ' ' << unsigned(v.register_address_bytes) << ' '
        << unsigned(v.flags) << '\n';
  }
  for (unsigned i = 0; i < p.variant_count; ++i) {
    const auto &v = p.variants[i];
    out << "variant " << unsigned(v.i2c_addr) << ' ' << unsigned(v.id_reg) << ' '
        << unsigned(v.id_value) << ' ' << unsigned(v.id_mask) << ' ' << unsigned(v.power_state.reg)
        << ' ' << unsigned(v.power_state.mask) << ' ' << unsigned(v.reset_state.reg) << ' '
        << unsigned(v.reset_state.mask) << ' ' << v.detected_option << ' ' << v.option_select_mask
        << ' ' << v.option_select_value << ' ';
    operations(out, v.power_on);
    operations(out, v.reset_assert);
    operations(out, v.reset_release);
    out << "\n";
  }
  out << "reset " << unsigned(d.reset.kind) << ' ' << int(d.reset.pin) << ' '
      << unsigned(d.reset.low_ms) << ' ' << unsigned(d.reset.post_ms) << ' '
      << unsigned(d.reset.flags) << ' ' << bool(d.reset.custom) << '\n';
  const auto &sd = d.sd;
  out << "sd " << int(sd.sclk) << ' ' << int(sd.mosi) << ' ' << int(sd.miso) << ' ' << int(sd.sd_cs)
      << ' ' << int(sd.other_cs) << '\n';
  const auto &lcd = d.display;
  out << "display " << int(lcd.sclk) << ' ' << int(lcd.mosi) << ' ' << int(lcd.miso) << ' '
      << int(lcd.dc) << ' ' << int(lcd.cs) << ' ' << int(lcd.rst) << ' ' << int(lcd.busy) << '\n';
  out << "i2c " << int(d.internal_i2c.sda) << ' ' << int(d.internal_i2c.scl) << ' '
      << int(d.internal_i2c.hw_port) << " hold ";
  pins(out, d.hold_high_pins);
  out << " ops ";
  pins(out, d.op_gpio_pins);
  out << " options[";
  for (unsigned i = 0; i < d.option_names.size; ++i)
    out << d.option_names.data[i] << ',';
  out << "]\n";
}
template <std::size_t N>
void check(const char *name, const m5gfx::board_detect::m5::board_entry_t (&entries)[N]) {
  std::ostringstream out;
  for (const auto &entry : entries) {
    assert(entry.desc && entry.construct);
    bool holds = false;
    for (auto id : gpio_power_hold_board_ids)
      if (id == entry.desc->def.id)
        holds = true;
    assert(holds == (entry.desc->power.hold_pin >= 0));
    descriptor(out, *entry.desc);
    out << "entry " << host_display::name(entry.construct) << ' '
        << (entry.success_log_name ? entry.success_log_name : "<descriptor>") << ' '
        << bool(entry.success_annotation) << ' ' << bool(entry.fixed_start) << '\n';
  }
  const std::string path = std::string("test/board_detect/fixtures/") + name + ".txt";
  if (std::getenv("M5GFX_UPDATE_SNAPSHOTS")) {
    std::ofstream file(path);
    file << out.str();
    assert(file.good());
    return;
  }
  std::ifstream file(path);
  assert(file.good());
  std::string expected((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  if (expected != out.str()) {
    std::cerr << "Descriptor snapshot mismatch: " << name << '\n';
    std::abort();
  }
  std::cout << name << ": " << N << " full descriptions match\n";
}
} // namespace host_snapshot
