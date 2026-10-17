  bool board_detector_t::has_member(board_id_t id) const
  {
    if (members == nullptr) { return false; }
    for (auto p = members; *p != nullptr; ++p)
    {
      if ((*p)->id == id) { return true; }
    }
    return false;
  }

  namespace
  {
    static constexpr char tag[] = "board_detect";

    bool enabled(const probe_ctx_t& ctx, board_id_t id)
    {
      if (ctx.enabled_ids == nullptr) { return true; }
      for (auto p = ctx.enabled_ids; *p != board_id_unknown; ++p)
      {
        if (*p == id) { return true; }
      }
      return false;
    }

    bool fallback_only(const board_detector_t* detector)
    {
      if (detector->members == nullptr || detector->members[0] == nullptr) { return false; }
      for (auto p = detector->members; *p != nullptr; ++p)
      {
        if (!((*p)->flags & def_flag_fallback)) { return false; }
      }
      return true;
    }

    bool gpio_power_hold_family(const board_detector_t* detector)
    {
      for (const auto id : gpio_power_hold_board_ids)
      {
        if (detector->has_member(id)) { return true; }
      }
      return false;
    }

    bool run_detector(const board_detector_t* detector, probe_ctx_t& ctx, board_result_t* result)
    {
      if (ctx.transaction == nullptr)
      {
        ESP_LOGE(tag, "detector requires a detection transaction");
        return false;
      }
      const char* family = (detector->members && detector->members[0])
                         ? detector->members[0]->name : "empty";
      const bool signature = detector->signature(ctx);
      ESP_LOGD(tag, "stage=1 family=%s detector=%p match=%d", family,
               static_cast<const void*>(detector), signature);
      if (!signature)
      {
        ctx.transaction->rollback();
        return false;
      }

      ctx.confirm_attempted = true;

      board_result_t candidate;
      const bool confirmed = detector->confirm(ctx, &candidate);
      ESP_LOGD(tag, "stage=2 family=%s detector=%p match=%d board=%u name=%s", family,
               static_cast<const void*>(detector), confirmed,
               static_cast<unsigned>(candidate.def ? candidate.def->id : board_id_unknown),
               candidate.def ? candidate.def->name : "invalid");
      if (!confirmed)
      {
        ctx.transaction->rollback();
        return false;
      }

      if (candidate.desc == nullptr || candidate.def == nullptr
       || candidate.def != &candidate.desc->def
       || candidate.def->id == board_id_unknown)
      {
        ESP_LOGW(tag, "detector=%p returned a match without a consistent board description",
                 static_cast<const void*>(detector));
        ctx.transaction->rollback();
        return false;
      }
      candidate.status = enabled(ctx, candidate.def->id)
                       ? detect_status_t::matched
                       : detect_status_t::excluded;
      const auto& desc = *candidate.desc;
      char option_text[80] = {};
      std::uint32_t named_bits = 0;
      if (desc.option_names.data != nullptr)
      {
        for (std::uint_fast8_t bit = 0; bit < 32 && bit < desc.option_names.size; ++bit)
        {
          if (!(candidate.option & (std::uint32_t(1) << bit))) { continue; }
          const auto used = std::strlen(option_text);
          std::snprintf(option_text + used, sizeof(option_text) - used, "%s%s",
                        used ? "," : "", desc.option_names.data[bit]);
          named_bits |= std::uint32_t(1) << bit;
        }
      }
      const auto unnamed = candidate.option & ~named_bits;
      if (unnamed)
      {
        const auto used = std::strlen(option_text);
        std::snprintf(option_text + used, sizeof(option_text) - used, "%sunknown:%08x",
                      used ? "," : "",
                      static_cast<unsigned>(unnamed));
      }
      if (option_text[0] == '\0') { std::snprintf(option_text, sizeof(option_text), "none"); }
      ESP_LOGD(tag, "detected board=%u name=%s opt=0x%08x",
               static_cast<unsigned>(candidate.def->id), candidate.def->name,
               static_cast<unsigned>(candidate.option));
      ESP_LOGD(tag, "detected option names board=%u opt=0x%08x(%s)",
               static_cast<unsigned>(candidate.def->id),
               static_cast<unsigned>(candidate.option), option_text);
      *result = candidate;
      return true;
    }
  }

  board_result_t detect_board(const board_detector_t* const* list, board_id_t hint, probe_ctx_t& ctx)
  {
    board_result_t result;
    ctx.confirm_attempted = false;
    ctx.family_identified = false;
    if (list == nullptr) { return result; }
    ctx.hint = hint;

    const board_detector_t* hinted = nullptr;
    if (hint != board_id_unknown)
    {
      bool hint_found = false;
      for (auto p = list; *p != nullptr; ++p)
      {
        if ((*p)->has_member(hint))
        {
          hint_found = true;
          if (!fallback_only(*p)) { hinted = *p; }
          else
          {
            ESP_LOGD(tag, "hint=%u belongs to a fallback-only detector",
                     static_cast<unsigned>(hint));
          }
          break;
        }
      }
      if (!hint_found)
      {
        // A hint belonging to a detector outside this package-specific list is normal.
        ESP_LOGD(tag, "hint=%u is not present in detector list", static_cast<unsigned>(hint));
      }
    }

    unsigned count = 0;
    while (list[count] != nullptr && count <= max_detector_families) { ++count; }
    if (count > max_detector_families) { return result; }
    bool tried[max_detector_families] = {};
    for (unsigned pass = 0; pass < count; ++pass)
    {
      unsigned chosen = count;
      unsigned best = 4;
      for (unsigned i = 0; i < count; ++i)
      {
        if (tried[i]) { continue; }
        bool blocked = false;
        for (const auto& edge : detector_order_edges)
        {
          if (!list[i]->has_member(edge.after)) { continue; }
          for (unsigned j = 0; j < count; ++j)
          {
            if (!tried[j] && list[j]->has_member(edge.before)) { blocked = true; break; }
          }
          if (blocked) { break; }
        }
        if (blocked) { continue; }
        const bool hold = gpio_power_hold_family(list[i]);
        const unsigned rank = hold ? (list[i] == hinted ? 0 : 1)
                                   : (list[i] == hinted ? 2 : 3);
        if (rank < best) { chosen = i; best = rank; }
      }
      if (chosen == count) { break; } // Generated constraints must be acyclic.
      tried[chosen] = true;
      if (run_detector(list[chosen], ctx, &result)) { return result; }
      if (ctx.family_identified) { break; }
      if (list[chosen] == hinted)
      {
        ESP_LOGD(tag, "hint=%u did not match its detector family", static_cast<unsigned>(hint));
      }
    }
    result.candidate = ctx.candidate;
    if (ctx.candidate != nullptr)
    { ESP_LOGD(tag, "weak candidate board=%u name=%s",
               static_cast<unsigned>(ctx.candidate->id), ctx.candidate->name); }
    return result;
  }
