import assert from "node:assert/strict";
import test from "node:test";
import { renderDetectorProbePins } from "../lib/emit/m5gfx_detector_probe_pins.js";
import { validateTarget } from "../lib/targets.js";

test("declared detection pins require generated wiring to reach the firmware snapshot", () => {
  const board = { id: "board", name: "Board", pins: { 3: { roles: [] } } };
  const target = { boards: { board: {} } };
  const missingWiring = (value, mapping) => validateTarget(value, mapping)
    .filter((issue) => issue.id === "E_TGT_DETECTION_WIRING");
  assert.deepEqual(missingWiring(board, target), []);
  for (const source of ["probe_pins", "detect_class"]) {
    const value = structuredClone(board), mapping = structuredClone(target);
    if (source === "probe_pins") mapping.boards.board.probe_pins = [3];
    else value.pins[3].detect_class = "up";
    assert.deepEqual(missingWiring(value, mapping).map((issue) => issue.path),
      ["/targets/boards/board/wiring_output"], source);
    mapping.boards.board.wiring_fields = [];
    mapping.boards.board.wiring_output = "test_wiring.hpp";
    assert.deepEqual(missingWiring(value, mapping), [], source);
  }
});

test("host pin expectations combine only the detector's member boards", () => {
  const entries = [
    { board: { id: "first", legacy_board_id: 11, pins: { 5: { detect_class: "up" } } }, chip: { soc: "esp32c6" },
      emitted: { mapping: { boardId: "first", fields: ["display"], probePins: [3] }, display: { sclk: 7 } } },
    { board: { id: "sibling", legacy_board_id: 12 }, chip: { soc: "esp32c6" },
      emitted: { mapping: { boardId: "sibling", fields: [], probePins: [9, 3] } } },
    { board: { id: "other", legacy_board_id: 21 }, chip: { soc: "esp32c6" },
      emitted: { mapping: { boardId: "other", fields: [], probePins: [2] } } },
  ];
  const groups = { c6: { families: [
    { detector: "first_detector", boards: ["first", "sibling"] },
    { detector: "other_detector", boards: ["other"] },
  ] } };
  const output = renderDetectorProbePins(entries, groups);
  assert.ok(output.includes('#if defined(CONFIG_IDF_TARGET_ESP32C6)'));
  assert.ok(output.includes('{ &first_detector, "first_detector", { 3, 5, 7, 9 }, { 11, 12 } }'));
  assert.ok(output.includes('{ &other_detector, "other_detector", { 2 }, { 21 } }'));
  assert.throws(() => renderDetectorProbePins(entries.slice(0, 1), groups), /missing board sibling/);
});

test("host expectations reject duplicate detectors for the same SoC across groups", () => {
  const entries = [
    { board: { id: "first", legacy_board_id: 11 }, chip: { soc: "esp32c6" },
      emitted: { mapping: { boardId: "first", fields: [], probePins: [] } } },
    { board: { id: "other", legacy_board_id: 21 }, chip: { soc: "esp32c5" },
      emitted: { mapping: { boardId: "other", fields: [], probePins: [] } } },
  ];
  const family = { detector: "shared_detector", boards: ["first"] };
  const groups = { first: { families: [family] }, second: { families: [family] } };
  assert.throws(() => renderDetectorProbePins(entries, groups), /esp32c6: duplicate detector shared_detector/);
  groups.second.families = [{ detector: "shared_detector", boards: ["other"] }];
  assert.doesNotThrow(() => renderDetectorProbePins(entries, groups));
});
