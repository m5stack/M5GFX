// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

namespace m5gfx
{
namespace board_detect
{
namespace i2c_bus_probe_detail
{
  struct line_state_t
  {
    line_state_t(bool sda, bool scl) : sda_high(sda), scl_high(scl) {}
    bool sda_high;
    bool scl_high;
  };

  template <typename Sample, typename Recover>
  bool probe_i2c_bus_present(Sample sample, Recover recover)
  {
    line_state_t state = sample();
    if (state.scl_high && !state.sda_high)
    {
      recover();
      state = sample();
    }
    return state.sda_high && state.scl_high;
  }
}
}
}

