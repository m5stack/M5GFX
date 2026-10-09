#include "setup_common.inl"

namespace board_detect { namespace m5 {
construct_status_t setup_detected_board(const board_result_t& result, display_parts_t* parts)
{ return setup_board(esp32h2_boards, result, parts); }
} }
