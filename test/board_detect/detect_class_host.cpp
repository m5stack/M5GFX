#include "board_detect/detect_class.hpp"
#include <cassert>
#include <initializer_list>
using namespace m5gfx::board_detect;
int main()
{
  for (int pin : { 2, 48 }) {
    const auto bit = std::uint64_t(1) << pin;
    for (int expected = 0; expected < 4; ++expected) {
      detect_class_expected_t spec = { bit, 0, 0, 0, 0 };
      if (expected == 0) spec.up = bit;
      if (expected == 1) spec.down = bit;
      if (expected == 2) spec.floating = bit;
      if (expected == 3) spec.fixed = bit;
      for (int sample = 0; sample < 4; ++sample) {
        pin_pull_result_t measured;
        measured.pulldown_high = (sample & 1) ? bit : 0;
        measured.pullup_high = (sample & 2) ? bit : 0;
        const bool want = expected == 0 ? sample == 3 : expected == 1 ? sample == 0
                        : expected == 2 ? sample == 2 : sample == 0 || sample == 3;
        assert(match_detect_class(spec, measured) == want);
      }
    }
  }
  const detect_class_expected_t unconstrained = { 0, 0, 0, 0, 0 };
  assert(match_detect_class(unconstrained, {}));
  const detect_class_expected_t mixed = { 7, 1, 2, 4, 0 };
  pin_pull_result_t measured;
  measured.pulldown_high = 1;
  measured.pullup_high = 5;
  assert(match_detect_class(mixed, measured));
  measured.pulldown_high = 5;
  assert(!match_detect_class(mixed, measured));
}
