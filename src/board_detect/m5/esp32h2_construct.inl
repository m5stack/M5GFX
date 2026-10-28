#pragma once

#include "construct_common.inl"

namespace board_detect { namespace m5 {
construct_status_t construct_detected_board(const board_result_t& result, display_parts_t* parts)
{ return construct_board(esp32h2_boards, result, parts); }
} }
