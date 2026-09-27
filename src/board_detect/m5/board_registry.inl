// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

namespace m5gfx
{
namespace board_detect
{
namespace m5
{
  struct display_parts_t;
  enum class construct_status_t : std::uint8_t { ok, no_display, failed };
  constexpr construct_status_t construct_status(bool success)
  { return success ? construct_status_t::ok : construct_status_t::failed; }
  using construct_fn_t = construct_status_t (*)(const board_result_t&, display_parts_t*);
  using success_annotation_fn_t = const char* (*)(const board_result_t&);

  struct board_entry_t
  {
    const board_desc_t* desc;
    construct_fn_t construct;
    const char* success_log_name;
    success_annotation_fn_t success_annotation;
  };

  struct success_log_t
  {
    const char* name;
    const char* annotation;
  };

  template <std::size_t BoardCount>
  const board_desc_t* find_board_desc(const board_entry_t (&boards)[BoardCount],
                                      board_id_t board)
  {
    for (const auto& entry : boards)
    {
      if (entry.desc->def.id == board) { return entry.desc; }
    }
    ESP_LOGD("board_detect_m5", "board=%u is not available on the new detection path",
             static_cast<unsigned>(board));
    return nullptr;
  }

  template <std::size_t BoardCount>
  success_log_t success_log(const board_entry_t (&boards)[BoardCount],
                            const board_result_t& result)
  {
    for (const auto& entry : boards)
    {
      if (result.def != nullptr && entry.desc->def.id == result.def->id)
      {
        return {
          entry.success_log_name == nullptr ? entry.desc->def.name : entry.success_log_name,
          entry.success_annotation == nullptr ? "" : entry.success_annotation(result),
        };
      }
    }
    return { result.def == nullptr ? "unknown" : result.def->name, "" };
  }

  inline bool prepare(board_result_t& result, const prepare_ctx_t& ctx)
  {
    if (result.desc == nullptr || result.def != &result.desc->def
     || result.def->id == board_id_unknown) { return false; }
    return board_detect::prepare(*result.desc, result, ctx);
  }

  template <std::size_t DetectorCount>
  board_result_t detect_board_family(
    const board_detector_t* const (&detectors)[DetectorCount], board_id_t board,
    probe_ctx_t& ctx)
  {
    for (auto detector : detectors)
    {
      if (detector == nullptr) { break; }
      if (!detector->has_member(board)) { continue; }
      const board_detector_t* const family[] = { detector, nullptr };
      return detect_board(family, board, ctx);
    }
    return {};
  }

  template <std::size_t BoardCount>
  construct_status_t setup_board(const board_entry_t (&boards)[BoardCount],
                                 const board_result_t& result, display_parts_t* parts)
  {
    if (parts == nullptr || result.def == nullptr) { return construct_status_t::failed; }
    for (const auto& entry : boards)
    {
      if (entry.desc->def.id == result.def->id)
      {
        return entry.construct(result, parts);
      }
    }
    ESP_LOGE("M5GFX", "No display constructor for board id %u",
             static_cast<unsigned>(result.def->id));
    return construct_status_t::failed;
  }
}
}
}
