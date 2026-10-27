// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#pragma once

namespace m5gfx
{
namespace board_detect
{
namespace m5
{
  #include "../generated/esp32s3_detector_order.hpp"

  construct_status_t construct_atoms3(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_atoms3r(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_dinmeter(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_airq(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_stamplc(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_dial(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_cardputer(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_cardputer_adv(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_vameter(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_sticks3(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_stopwatch(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_papermono(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_chaincaptain(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_papercolor(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_papers3(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_paperdiy(const board_result_t& result, display_parts_t* parts);
  construct_status_t construct_cores3(const board_result_t& result, display_parts_t* parts);

  const char* atoms3_success_annotation(const board_result_t& result)
  {
    return result.option & generated_options::atoms3::gc9107 ? " (GC9107)" : " (ST7735)";
  }

  const char* atoms3r_success_annotation(const board_result_t& result)
  {
    return result.option & generated_options::atoms3r::gc9107 ? " (GC9107)" : " (ST7735)";
  }

  static const board_entry_t esp32s3_boards[] = {
    { &desc_dualkey, construct_displayless, "board_M5DualKey", nullptr },
    { &desc_cores3, construct_cores3, "board_M5StackCoreS3", nullptr, cores3_detail::fixed_prepare },
    { &desc_cores3se, construct_cores3, "board_M5StackCoreS3SE", nullptr, cores3_detail::fixed_prepare },
    { &desc_stackchan, construct_cores3, "board_M5StackChan", nullptr, cores3_detail::fixed_prepare },
    { &desc_atoms3, construct_atoms3, "board_M5AtomS3", atoms3_success_annotation, fixed_prepare_atoms3 },
    { &desc_atoms3lite, construct_displayless, "board_M5AtomS3Lite", nullptr },
    { &desc_atoms3u, construct_displayless, "board_M5AtomS3U", nullptr },
    { &desc_stamps3, construct_displayless, "board_M5StampS3", nullptr },
    { &desc_capsule, construct_displayless, "board_M5Capsule", nullptr },
    { &desc_powerhub, construct_displayless, "board_M5PowerHub", nullptr },
    { &desc_atoms3r, construct_atoms3r, "board_M5AtomS3R", atoms3r_success_annotation, fixed_prepare_atoms3r },
    { &desc_atoms3rcam, construct_displayless, "board_M5AtomS3RCam", nullptr },
    { &desc_atoms3rext, construct_displayless, "board_M5AtomS3RExt", nullptr },
    { &desc_atomvoices3r, construct_displayless, "board_M5AtomVoiceS3R", nullptr },
    { &desc_stamps3bat, construct_displayless, "board_M5StampS3Bat", nullptr },
    { &desc_stamps3mini, construct_displayless, "board_M5StampS3Mini", nullptr },
    { &desc_dinmeter, construct_dinmeter, "board_M5DinMeter", nullptr },
    { &desc_airq, construct_airq, "M5AirQ", nullptr, fixed_prepare_airq },
    { &desc_stamplc, construct_stamplc, "board_M5StamPLC", nullptr },
    { &desc_dial, construct_dial, "board_M5Dial", nullptr },
    { &desc_cardputer, construct_cardputer, "board_M5Cardputer", nullptr },
    { &desc_cardputer_adv, construct_cardputer_adv, "board_M5CardputerADV", nullptr },
    { &desc_vameter, construct_vameter, "board_M5VAMeter", nullptr },
    { &desc_sticks3, construct_sticks3, "board_M5StickS3", nullptr },
    { &desc_stopwatch, construct_stopwatch, "board_M5StopWatch", nullptr },
    { &desc_papermono, construct_papermono, "board_M5PaperMono", nullptr },
    { &desc_chaincaptain, construct_chaincaptain, "board_M5ChainCaptain", nullptr },
    { &desc_papercolor, construct_papercolor, "board_M5PaperColor", nullptr },
    { &desc_papers3, construct_papers3, "board_M5PaperS3", nullptr },
    { &desc_paperdiy, construct_paperdiy, "board_M5PaperDIY", nullptr },
  };

  success_log_t success_log(const board_result_t& result)
  {
    return success_log(esp32s3_boards, result);
  }
  construct_status_t construct_detected_board(const board_result_t& result, display_parts_t* parts)
  {
    return construct_board(esp32s3_boards, result, parts);
  }
}
}
}
