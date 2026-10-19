  bool prepare(const board_desc_t& desc, board_result_t& result, const prepare_ctx_t& ctx)
  {
    if (ctx.transaction == nullptr)
    {
      ESP_LOGE("board_detect", "prepare requires a detection transaction");
      return false;
    }
    if (result.desc != &desc || result.def != &desc.def || result.def->id == board_id_unknown)
    {
      return false;
    }
    if (!startup_detail::description_valid(desc)) { return false; }
    const board_desc_t* current = &desc;
    if (!(result.prepared & prepared_power))
    {
      if (desc.power.variants != nullptr)
      {
        startup_detail::i2c_scope_t i2c(*ctx.transaction, ctx.i2c_port_probe, desc.internal_i2c);
        if (!i2c.opened
         || !startup_detail::prepare_power(desc, result, i2c.port, true)) { return false; }
      }
      else if (!startup_detail::prepare_power(desc, result, ctx.i2c_port_probe, true)) { return false; }
    }
    if (!(result.prepared & prepared_refine) && result.refine != nullptr)
    {
      if (result.prepared & prepared_power_failed)
      {
        // Pending refinement means the member is not yet confirmed. Give a
        // failed power sequence another attempt before retaining it provisionally.
        if (!ctx.final_attempt) { return false; }
        result.provisional = true;
        ESP_LOGW("board_detect",
                 "member refinement skipped after retained power_on failure; board=%u",
                 static_cast<unsigned>(result.def->id));
      }
      else
      {
        if (!result.refine(result, ctx)) { return false; }
        if (result.desc == nullptr || result.def != &result.desc->def
         || result.def->id == board_id_unknown
         || !startup_detail::description_valid(*result.desc)) { return false; }
        current = result.desc;
      }
      result.prepared |= prepared_refine;
    }
    else if (result.desc != nullptr)
    {
      current = result.desc;
    }
    if (!(result.prepared & prepared_observation) && result.observe_after_power != nullptr)
    {
      // Optional metadata may skip retained power failure. Observers whose
      // controllers can still answer may opt in without changing board identity.
      if ((!(result.prepared & prepared_power_failed) || result.observe_on_power_failure)
       && !result.observe_after_power(result, ctx))
      {
        // Optional panel metadata must not discard a member confirmed by touch.
        ESP_LOGW("board_detect", "post-power observation failed; retaining board=%u",
                 static_cast<unsigned>(result.def->id));
      }
      result.prepared |= prepared_observation;
    }
    if (!startup_detail::prepare_sd_spi(*current, result, ctx)) { return false; }
    const bool reset_was_prepared = result.prepared & prepared_reset;
    if (!reset_was_prepared)
    {
      if (current->reset.kind == reset_kind_t::i2c_regs)
      {
        startup_detail::i2c_scope_t i2c(*ctx.transaction, ctx.i2c_port_probe, current->internal_i2c);
        if (!i2c.opened || !prepare_reset(*current, result, ctx, i2c.port, nullptr, true))
        {
          return false;
        }
      }
      else if (!prepare_reset(*current, result, ctx, ctx.i2c_port_probe, nullptr, true))
      {
        return false;
      }
    }
    startup_detail::hold_chip_selects(*current);
    // Detection uses software I2C only. Like SPI, hardware I2C belongs to
    // its consumers; preparation leaves only power and reset ready.
    return true;
  }
