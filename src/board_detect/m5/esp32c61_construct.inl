// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "construct_common.inl"

namespace board_detect
{
namespace m5
{
  static constexpr i2c_bus_desc_t bus_corematrix =
    i2c_bus(specs::corematrix::bus_port,
            specs::corematrix::bus_freq_write, specs::corematrix::bus_freq_read,
            wiring::corematrix::internal_i2c_sda, wiring::corematrix::internal_i2c_scl,
            specs::corematrix::bus_i2c_addr, specs::corematrix::bus_prefix_len);
  static constexpr panel_desc_t panel_corematrix =
    panel().with_rotation(specs::corematrix::panel_tm1680::rotation_offset)
      .with_bus_shared(false);

  construct_status_t construct_corematrix(const board_result_t&, display_parts_t* parts)
  {
    lgfx::i2c::init(corematrix_detail::i2c_port, wiring::corematrix::internal_i2c_sda,
                    wiring::corematrix::internal_i2c_scl);
    bool tm1680_ack = lgfx::i2c::beginTransaction(
      corematrix_detail::i2c_port, specs::corematrix::bus_i2c_addr,
      corematrix_detail::i2c_freq, false).has_value();
    if (tm1680_ack)
    {
      tm1680_ack = lgfx::i2c::endTransaction(corematrix_detail::i2c_port).has_value();
    }
    // Construction may fail; close the ACK probe and let Bus_I2C::init own the port.
    lgfx::i2c::release(corematrix_detail::i2c_port);
    if (!tm1680_ack)
    {
      ESP_LOGW(LIBRARY_NAME, "[Autodetect] CoreMatrix TM1680 did not acknowledge");
    }

    display_parts_owner_t out;
    out.bus.reset(make_i2c_bus(bus_corematrix));
    out.panel.reset(make_panel<lgfx::Panel_TM1680>(panel_corematrix, out.bus.get()));
    return construct_status(out.release_to(parts));
  }

  construct_status_t construct_detected_board(const board_result_t& result, display_parts_t* parts)
  {
    return construct_board(esp32c61_boards, result, parts);
  }
}
}
