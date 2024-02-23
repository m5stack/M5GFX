// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#if defined ( SDL_h_ )

namespace m5gfx
{
  #include "frame_image_M5StackCore2.h"

  struct picture_frame_t
  {
  uint16_t w;
  uint16_t h;
  uint16_t x;
  uint16_t y;
  const uint32_t* img;
  };

  static constexpr const picture_frame_t picture_frame_M5StackCore2 =
  { 400, 400, 40, 80, frame_image_M5StackCore2 };

  const picture_frame_t* getPictureFrame(board_t b)
  {
    switch (b) {
    case board_M5StackCore2:
      return &picture_frame_M5StackCore2;
    default:
      return nullptr;
    }
  }
}

#endif
