  __attribute__((unused))
  static bool select_provisional_member(const prepare_ctx_t& ctx, board_result_t* result,
                                const board_desc_t* preferred_if_possible,
                                const board_desc_t* hinted_if_possible,
                                const board_desc_t* family_default, const char* why)
  {
    if (!ctx.final_attempt || result == nullptr) { return false; }
    const auto* chosen = preferred_if_possible ? preferred_if_possible
                       : hinted_if_possible ? hinted_if_possible : family_default;
    if (chosen == nullptr) { return false; }
    result->assign(chosen);
    result->provisional = true;
    ESP_LOGW("board_detect_m5", "%s; using %s for this boot", why, chosen->def.name);
    return true;
  }
