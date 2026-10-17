import { detectionPinsForEntries } from "./m5gfx_board_wiring.js";

// Host-only expectations use the same per-board pin calculation as snapshots.
export function renderDetectorProbePins(entries, groups) {
  const byId = new Map(entries.map((entry) => [entry.board.id, entry]));
  const chips = new Map();
  const seen = new Set();
  for (const group of Object.values(groups)) {
    for (const family of group.families) {
      const members = family.boards.map((id) => {
        const entry = byId.get(id);
        if (!entry) throw new Error(`${family.detector}: missing board ${id}`);
        return entry;
      });
      const soc = members[0].chip.soc;
      const key = `${soc}:${family.detector}`;
      if (seen.has(key)) throw new Error(`${soc}: duplicate detector ${family.detector}`);
      seen.add(key);
      const rows = chips.get(soc) ?? [];
      const ids = members.map((entry) => entry.board.legacy_board_id);
      rows.push(`  { &${family.detector}, "${family.detector}", { ${detectionPinsForEntries(members).join(", ")} }, { ${ids.join(", ")} } },`);
      chips.set(soc, rows);
    }
  }
  const lines = [
    "// Generated from extras/board_spec; host tests only. Do not edit by hand.",
    "#pragma once", "#include <set>", "#include <vector>", "",
    "struct expected_detector_pins_t {",
    "  const board_detector_t* detector;",
    "  const char* name;",
    "  std::set<int> pins;",
    "  std::vector<board_id_t> members;",
    "};", "",
  ];
  for (const [soc, rows] of chips) {
    lines.push(`#if defined(CONFIG_IDF_TARGET_${soc.toUpperCase()})`,
      "static const expected_detector_pins_t expected_detector_pins[] = {",
      ...rows, "};", "#endif", "");
  }
  return lines.join("\n");
}
