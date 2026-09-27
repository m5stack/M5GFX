#include "setup_common.inl"

namespace board_detect
{
namespace m5
{
#include "esp32c6/c6_display_setup.inl"

construct_status_t setup_detected_board(const board_result_t& result, display_parts_t* parts)
{
  return setup_board(esp32c6_boards, result, parts);
}
}
}
