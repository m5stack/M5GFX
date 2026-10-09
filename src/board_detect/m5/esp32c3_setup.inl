// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

#include "setup_common.inl"

namespace board_detect { namespace m5 {
construct_status_t setup_detected_board(const board_result_t& result, display_parts_t* parts)
{ return setup_board(esp32c3_boards, result, parts); }
} }
