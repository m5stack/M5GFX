// Shared pad mapping for the touch implementation and its host event model.
#pragma once
namespace atom_touch
{
  struct touch_pad_t { int channel; int gpio; };
  static constexpr touch_pad_t t4_pad = { 4, 13 };
  static constexpr touch_pad_t t7_pad = { 7, 27 };
  // T7 measures the LED DIN line, so LED clearing uses the same pad.
  static constexpr int led_gpio = t7_pad.gpio;
}
