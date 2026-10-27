  bool probe_spi_id(probe_ctx_t& ctx, const board_desc_t& desc,
                    const spi_id_probe_t* probes, std::size_t probe_count,
                    board_result_t* result, bool three_wire,
                    std::uint8_t slow_retry_half_us,
                    bool deselected_zero_preamble)
  {
    if (probes == nullptr || probe_count == 0 || result == nullptr
     || !prepare_detail::description_valid(desc)
     || desc.display.cs < 0) { return false; }  // no_display_pins() has no SPI ID
    const std::int8_t pins[] = {
      desc.display.cs, desc.display.sclk, desc.display.mosi,
      desc.display.dc, desc.display.rst,
    };
    const std::int8_t shared_sd_pins[] = {
      desc.display.cs, desc.display.sclk, desc.display.mosi,
      desc.display.dc, desc.display.rst,
      desc.sd.sclk, desc.sd.mosi, desc.sd.miso, desc.sd.sd_cs,
    };
    const auto restore_probe_pins = [&]()
    {
      if (desc.sd.sd_cs >= 0) { ctx.transaction->restore_to_snapshot(shared_sd_pins); }
      else                    { ctx.transaction->restore_to_snapshot(pins); }
    };
    board_result_t candidate;
    candidate.assign(&desc);
    const prepare_ctx_t& prepare_ctx = ctx;
    if (desc.sd.sd_cs >= 0)
    {
      // Keep both devices deselected while the shared SD bus is switched to
      // SPI mode. Descriptions without shared SD take the original path.
      prepare_detail::pin_level(desc.sd.sd_cs, true);
      prepare_detail::pin_level(desc.display.cs, true);
      if (!prepare_detail::prepare_sd_spi(desc, candidate, prepare_ctx))
      {
        restore_probe_pins();
        return false;
      }
    }
    if (!prepare_reset(desc, candidate, prepare_ctx, ctx.probe_port))
    {
      restore_probe_pins();
      return false;
    }
    if (observe_spi_variant(ctx, desc, probes, probe_count, &candidate.option,
                            three_wire, slow_retry_half_us, deselected_zero_preamble))
    {
      *result = candidate;
      return true;
    }
    restore_probe_pins();
    return false;
  }

  struct spi_id_member_t
  {
    const board_desc_t* desc;
    const spi_id_probe_t* probes;
    std::uint8_t probe_count;
    bool three_wire;
    bool touches_conditional_pins;
    std::uint8_t slow_retry_half_us;
    bool deselected_zero_preamble;
  };

  bool fixed_prepare_spi_variant(board_result_t& result, const prepare_ctx_t& ctx,
                               const spi_id_member_t& member)
  {
    if (!prepare(*result.desc, result, ctx)) { return false; }
    probe_ctx_t probe;
    static_cast<prepare_ctx_t&>(probe) = ctx;
    if (!observe_spi_variant(probe, *result.desc, member.probes, member.probe_count,
                             &result.option, member.three_wire,
                             member.slow_retry_half_us, member.deselected_zero_preamble))
    {
      ESP_LOGW("M5GFX", "Fixed board:%u panel variant unreadable; using default",
               static_cast<unsigned>(result.def->id));
    }
    const auto& display = result.desc->display;
    const std::int8_t signals[] = { display.dc, display.sclk, display.mosi, display.miso };
    ctx.transaction->restore_to_snapshot(signals);
    return true;
  }

  class spi_id_detector_t final : public board_detector_t
  {
  public:
    spi_id_detector_t(const board_def_t* const* board_members, const spi_id_member_t* members_desc,
                      std::uint8_t member_count, bool shared_id_read = false,
                      bool (*signature_probe)(probe_ctx_t&) = nullptr)
    : board_detector_t(board_members),
      members_desc_(members_desc), member_count_(member_count),
      shared_id_read_(shared_id_read), signature_probe_(signature_probe) {}
    bool signature(probe_ctx_t& ctx) const override
    { return signature_probe_ == nullptr || signature_probe_(ctx); }
    bool confirm(probe_ctx_t& ctx, board_result_t* result) const override
    {
      if (shared_id_read_) { return probe_family(ctx, result); }
      if (ctx.attempt == 0 && !ctx.final_attempt && ctx.hint != board_id_unknown)
      {
        for (std::uint8_t index = 0; index < member_count_; ++index)
        {
          const auto& member = members_desc_[index];
          if (member.desc->def.id != ctx.hint) { continue; }
          if (probe_member(ctx, member, result)) { return true; }
          return false;
        }
      }
      for (std::uint8_t index = 0; index < member_count_; ++index)
      {
        const auto& member = members_desc_[index];
        if (probe_member(ctx, member, result)) { return true; }
      }
      return false;
    }

  private:
    bool probe_family(probe_ctx_t& ctx, board_result_t* result) const
    {
      if (member_count_ == 0 || result == nullptr) { return false; }
      const auto& first = members_desc_[0];
      if (first.desc == nullptr || first.probes == nullptr || first.probe_count == 0
       || !prepare_detail::description_valid(*first.desc)
       || first.desc->display.cs < 0) { return false; }

      const auto& desc = *first.desc;
      const std::int8_t pins[] = {
        desc.display.cs, desc.display.sclk, desc.display.mosi,
        desc.display.dc, desc.display.rst,
      };
      board_result_t candidate;
      candidate.assign(&desc);
      const prepare_ctx_t& prepare_ctx = ctx;
      if (!prepare_reset(desc, candidate, prepare_ctx, ctx.probe_port))
      {
        ctx.transaction->restore_to_snapshot(pins);
        return false;
      }

      const auto& read_probe = first.probes[0];
      const int read_pin = first.three_wire || desc.display.miso < 0
                         ? desc.display.mosi : desc.display.miso;
      const std::uint32_t panel_id = soft_spi_read32(
        ctx, desc.display.sclk, desc.display.mosi, read_pin,
        desc.display.dc, desc.display.cs, read_probe.cmd, read_probe.dummy_bits,
        1, first.deselected_zero_preamble);

      // Members of this family share one physical read. Compare the
      // captured value in member order instead of resetting the same
      // panel again for each possible member.
      for (std::uint8_t member_index = 0; member_index < member_count_; ++member_index)
      {
        const auto& member = members_desc_[member_index];
        for (std::uint8_t probe_index = 0; probe_index < member.probe_count; ++probe_index)
        {
          const auto& probe = member.probes[probe_index];
          if (probe.cmd != read_probe.cmd || probe.dummy_bits != read_probe.dummy_bits)
          {
            ctx.transaction->restore_to_snapshot(pins);
            return false;
          }
          for (std::size_t value = 0; value < probe.value_count; ++value)
          {
            if ((panel_id & probe.mask) != probe.values[value]) { continue; }
            candidate.assign(member.desc);
            candidate.option = probe.option_bit;
            *result = candidate;
            return true;
          }
        }
      }
      ctx.transaction->restore_to_snapshot(pins);
      return false;
    }

    static bool probe_member(probe_ctx_t& ctx, const spi_id_member_t& member,
                             board_result_t* result)
    {
      if (ctx.conditional_pins_unavailable && member.touches_conditional_pins)
      {
        // OPI PSRAM owns GPIO33..37. Skip only the candidate that touches
        // those pins; another member of the same family may remain safe.
        return false;
      }
      return probe_spi_id(ctx, *member.desc, member.probes, member.probe_count, result,
                          member.three_wire, member.slow_retry_half_us,
                          member.deselected_zero_preamble);
    }

    const spi_id_member_t* members_desc_;
    std::uint8_t member_count_;
    bool shared_id_read_;
    bool (*signature_probe_)(probe_ctx_t&);
  };
