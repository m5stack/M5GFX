#pragma once

#include "../board_detect.hpp"
#include "board_registry.inl"
#include "generated/esp32h2_wiring.hpp"

namespace m5gfx { namespace board_detect { namespace m5 {

static constexpr board_desc_t desc_nanoh2 = {
  { static_cast<board_id_t>(lgfx::board_M5NanoH2), "M5NanoH2", 0 },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), no_pins(),
};

class nanoh2_detector_t final : public board_detector_t
{
public:
  nanoh2_detector_t() : board_detector_t(members_) {}
  bool signature(probe_ctx_t& ctx) const override
  { return probe_pin_pullup_low(ctx, 3); }
  bool confirm(probe_ctx_t&, board_result_t* result) const override
  { if (!result) return false; result->assign(&desc_nanoh2); return true; }
private:
  static const board_def_t* const members_[2];
};
const board_def_t* const nanoh2_detector_t::members_[2] = { &desc_nanoh2.def, nullptr };
static const nanoh2_detector_t nanoh2_detector;
#include "generated/esp32h2_detector_order.hpp"

static const board_entry_t esp32h2_boards[] = {
  { &desc_nanoh2, construct_displayless, "board_M5NanoH2", nullptr },
};
success_log_t success_log(const board_result_t& result)
{ return success_log(esp32h2_boards, result); }

} } }
