#pragma once
#include <cstdlib>
namespace m5gfx {
namespace board_detect {
namespace m5 {
construct_status_t construct_airq(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_atompsram(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_atoms3(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_atoms3r(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_cardputer(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_cardputer_adv(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_chaincaptain(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_core2(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_coreink(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_corematrix(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_corep4x(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_cores3(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_dial(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_dinmeter(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_nesson1(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_paper(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_papercolor(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_paperdiy(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_papermono(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_papers3(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_stack(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_stamplc(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_station(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_stickc(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_stickcplus(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_stickcplus2(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_sticks3(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_stopwatch(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_tab5(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_tough(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_toughc5(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_unitc6l(const board_result_t &, display_parts_t *) {
  std::abort();
}
construct_status_t construct_vameter(const board_result_t &, display_parts_t *) {
  std::abort();
}
} // namespace m5
} // namespace board_detect
} // namespace m5gfx

namespace host_display {
inline const char *name(m5gfx::board_detect::m5::construct_fn_t function) {
  using namespace m5gfx::board_detect::m5;
  if (function == construct_displayless)
    return "construct_displayless";
  if (function == construct_airq)
    return "construct_airq";
  if (function == construct_atompsram)
    return "construct_atompsram";
  if (function == construct_atoms3)
    return "construct_atoms3";
  if (function == construct_atoms3r)
    return "construct_atoms3r";
  if (function == construct_cardputer)
    return "construct_cardputer";
  if (function == construct_cardputer_adv)
    return "construct_cardputer_adv";
  if (function == construct_chaincaptain)
    return "construct_chaincaptain";
  if (function == construct_core2)
    return "construct_core2";
  if (function == construct_coreink)
    return "construct_coreink";
  if (function == construct_corematrix)
    return "construct_corematrix";
  if (function == construct_corep4x)
    return "construct_corep4x";
  if (function == construct_cores3)
    return "construct_cores3";
  if (function == construct_dial)
    return "construct_dial";
  if (function == construct_dinmeter)
    return "construct_dinmeter";
  if (function == construct_nesson1)
    return "construct_nesson1";
  if (function == construct_paper)
    return "construct_paper";
  if (function == construct_papercolor)
    return "construct_papercolor";
  if (function == construct_paperdiy)
    return "construct_paperdiy";
  if (function == construct_papermono)
    return "construct_papermono";
  if (function == construct_papers3)
    return "construct_papers3";
  if (function == construct_stack)
    return "construct_stack";
  if (function == construct_stamplc)
    return "construct_stamplc";
  if (function == construct_station)
    return "construct_station";
  if (function == construct_stickc)
    return "construct_stickc";
  if (function == construct_stickcplus)
    return "construct_stickcplus";
  if (function == construct_stickcplus2)
    return "construct_stickcplus2";
  if (function == construct_sticks3)
    return "construct_sticks3";
  if (function == construct_stopwatch)
    return "construct_stopwatch";
  if (function == construct_tab5)
    return "construct_tab5";
  if (function == construct_tough)
    return "construct_tough";
  if (function == construct_toughc5)
    return "construct_toughc5";
  if (function == construct_unitc6l)
    return "construct_unitc6l";
  if (function == construct_vameter)
    return "construct_vameter";
  std::abort();
}
} // namespace host_display
