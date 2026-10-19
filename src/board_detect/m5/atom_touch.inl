// Classic ESP32 touch/LED transport is supplied by the detection I/O layer.
namespace atom_touch
{
  static bool measure(std::uint16_t* t4, std::uint16_t* t7)
  {
    return io::classic_touch_measure<t4_pad.channel, t4_pad.gpio,
                                     t7_pad.channel, t7_pad.gpio, 200000>(t4, t7) && *t4 != 0;
  }

  static void clear_led() { io::classic_led_clear<led_gpio, 25>(); }
}
