#include "support/runtime.hpp"
#include "support/probes.hpp"
#include "board_detect/m5/esp32h2.inl"
#include "support/display_backend.hpp"
#include "support/snapshot.hpp"
int main() {
  using namespace m5gfx::board_detect;
  using namespace m5;
  for (const auto &entry : esp32h2_boards)
    assert(startup_detail::description_valid(*entry.desc));
  host_snapshot::check("h2", esp32h2_boards);
}
