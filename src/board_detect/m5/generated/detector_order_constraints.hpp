// Generated from detector_order.json and board catalog.
#pragma once

namespace m5gfx { namespace board_detect {
static constexpr unsigned max_detector_families = 12;
struct detector_order_edge_t { board_id_t before; board_id_t after; };
static constexpr detector_order_edge_t detector_order_edges[] = {
  { 9, 132 },
  { 1, 132 },
  { 7, 132 },
  { 4, 6 },
  { 4, 142 },
  { 6, 142 },
  { 32, 15 },
  { 10, 11 },
  { 12, 139 },
  { 11, 139 },
  { 10, 147 },
  { 12, 147 },
  { 30, 147 },
  { 32, 147 },
  { 34, 147 },
  { 11, 147 },
  { 14, 147 },
  { 15, 147 },
  { 21, 147 },
  { 145, 18 },
  { 145, 26 },
  { 18, 26 },
  { 153, 33 },
  { 154, 140 },
};
} } // namespace m5gfx::board_detect
