#include "support/runtime.hpp"
#include "support/probes.hpp"
#include "board_detect/m5/esp32c61.inl"
#include "support/display_backend.hpp"
#include "support/snapshot.hpp"
int main() {
  using namespace m5gfx::board_detect;
  using namespace m5;
  for (const auto &entry : esp32c61_boards)
    assert(prepare_detail::description_valid(*entry.desc));
  host_snapshot::check("c61", esp32c61_boards);
}
