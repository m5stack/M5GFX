#pragma once
#include <cstdio>
#include <cstring>
#include "board_detect/board_detect.hpp"
#include "board_detect/detect_session.hpp"
#include "board_detect/i2c_bus_probe.hpp"
#include "board_detect/ops.inl"
#include "board_detect/m5/generated/gpio_power_hold_board_ids.hpp"
#include "board_detect/m5/generated/detector_order_constraints.hpp"
namespace m5gfx {
namespace board_detect {
#include "board_detect/member_selection.inl"
#include "board_detect/transaction.inl"
#include "board_detect/detector_runner.inl"
#include "board_detect/startup.inl"
#include "board_detect/spi_detector.inl"
#include "board_detect/prepare.inl"
} // namespace board_detect
} // namespace m5gfx
