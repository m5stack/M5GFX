// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "../board_detect.hpp"
#include "board_registry.inl"
#include "generated/esp32c3_wiring.hpp"
#include <soc/io_mux_reg.h>
#include <soc/usb_serial_jtag_reg.h>

namespace m5gfx { namespace board_detect { namespace m5 {

static constexpr board_desc_t desc_stampc3 = {
  { static_cast<board_id_t>(lgfx::board_M5StampC3), "M5StampC3", def_flag_fallback },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), no_pins(),
};
static constexpr board_desc_t desc_stampc3u = {
  { static_cast<board_id_t>(lgfx::board_M5StampC3U), "M5StampC3U", 0 },
  no_power(), no_reset(), no_shared_sd(), no_display_pins(), no_pins(),
  no_internal_i2c(), no_options(), no_pins(),
};

namespace c3_detail
{
  inline bool sof_advances()
  {
    const auto first = REG_GET_FIELD(USB_SERIAL_JTAG_FRAM_NUM_REG,
                                     USB_SERIAL_JTAG_SOF_FRAME_INDEX);
    // USB SOF is 1 ms apart. Four further samples span at most 4 ms; do not
    // wait for USB enumeration, which starts much later on a cold boot.
    for (int i = 0; i < 4; ++i)
    {
      lgfx::delayMicroseconds(1000);
      if (REG_GET_FIELD(USB_SERIAL_JTAG_FRAM_NUM_REG,
                        USB_SERIAL_JTAG_SOF_FRAME_INDEX) != first) { return true; }
    }
    return false;
  }

  inline bool gpio20_held_high(probe_ctx_t& ctx)
  {
    // GPIO20 is UART0 RX. Restore its IO_MUX immediately after the read so
    // the console input keeps its pre-detection configuration.
    const auto mux = REG_READ(IO_MUX_GPIO20_REG);
    lgfx::pinMode(20, lgfx::pin_mode_t::input_pulldown);
    const bool high = lgfx::gpio_in(20);
    REG_WRITE(IO_MUX_GPIO20_REG, mux);
    ctx.transaction->restore_start({ 20 });
    return high;
  }
}

class c3_family_detector_t final : public board_detector_t
{
public:
  c3_family_detector_t() : board_detector_t(members_) {}
  bool signature(probe_ctx_t& ctx) const override
  {
    if (c3_detail::sof_advances()) { return true; }
    // GPIO20 can be driven high on either board, so it only suggests StampC3.
    // A candidate alone must not enter confirm or trigger another attempt.
    if (c3_detail::gpio20_held_high(ctx))
    {
      if (ctx.candidate == nullptr) { ctx.candidate = &desc_stampc3.def; }
    }
    return false;
  }
  bool confirm(probe_ctx_t&, board_result_t* result) const override
  {
    if (!result) { return false; }
    result->assign(&desc_stampc3u);
    return true;
  }
private:
  static const board_def_t* const members_[3];
};
const board_def_t* const c3_family_detector_t::members_[3] = {
  &desc_stampc3.def, &desc_stampc3u.def, nullptr,
};
static const c3_family_detector_t c3_family_detector;

#include "generated/esp32c3_detector_order.hpp"

static const board_entry_t esp32c3_boards[] = {
  { &desc_stampc3, construct_displayless, "board_M5StampC3", nullptr },
  { &desc_stampc3u, construct_displayless, "board_M5StampC3U", nullptr },
};
success_log_t success_log(const board_result_t& result)
{ return success_log(esp32c3_boards, result); }

} } }
