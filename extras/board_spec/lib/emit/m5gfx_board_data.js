import { renderM5GFXWiringHeader } from "./m5gfx_board_wiring.js";
import { renderM5GFXSpecsHeader } from "./m5gfx_board_specs.js";

export function renderM5GFXBoardDataHeader(entries, specs) {
  if (!specs.length) return renderM5GFXWiringHeader(entries);
  // Keep the existing declaration order: classic ESP32 reads specifications
  // before wiring, while the other chips read wiring before specifications.
  if (entries[0].chip.soc === "esp32") {
    return renderM5GFXSpecsHeader(specs) + "\n"
      + renderM5GFXWiringHeader(entries, false);
  }
  return renderM5GFXWiringHeader(entries) + "\n"
    + renderM5GFXSpecsHeader(specs, false);
}
