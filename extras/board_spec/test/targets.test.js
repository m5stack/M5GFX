import assert from "node:assert/strict";
import test from "node:test";
import { m5gfxBoardMapping } from "../lib/emit/m5gfx_board_wiring.js";
import { resolveTargetBoard, validateTarget, validateTargets } from "../lib/targets.js";

const board = { id: "m5example", name: "M5Example", legacy_board_id: 1000, chip: "esp32p4", pins: { 31: { roles: [] } }, devices: {} };
const errors = (board, target) => validateTarget(board, target).filter((issue) => issue.severity !== "warning");

test("a board without target metadata uses defaults and need not emit wiring", () => {
  assert.deepEqual(errors(board, {}), []);
  assert.deepEqual(resolveTargetBoard(board, {}), {});
  assert.equal(m5gfxBoardMapping({}, board), null);
});

test("catalog detection classes and target probes or fields infer chip wiring output", () => {
  const detection = { ...board, pins: { 31: { detect_class: "up" } } };
  for (const [input, metadata] of [
    [detection, {}],
    [board, { boards: { m5example: { probe_pins: [31] } } }],
    [board, { boards: { m5example: { wiring_fields: ["display"] } } }],
  ]) {
    assert.equal(m5gfxBoardMapping(metadata, input).wiringOutput, "esp32p4_wiring.hpp");
    assert.deepEqual(errors(input, metadata), []);
  }
  assert.equal(m5gfxBoardMapping({}, detection).specsOutput, undefined);
});

test("empty entries and explicit defaults are redundant, including inferred output", () => {
  assert.ok(errors(board, { boards: { m5example: {} } }).some((issue) => issue.id === "E_TGT_REDUNDANT_BOARD"));
  for (const [field, value] of Object.entries({ wiring_fields: [], probe_pins: [], options: [], identity_devices: [], desc_internal_i2c: false })) {
    assert.ok(errors(board, { boards: { m5example: { [field]: value } } }).some((issue) => issue.id === "E_TGT_REDUNDANT_FIELD" && issue.path.endsWith(`/${field}`)), field);
  }
  const detection = { ...board, pins: { 31: { detect_class: "up" } } };
  assert.ok(errors(detection, { boards: { m5example: { wiring_output: "esp32p4_wiring.hpp" } } }).some((issue) => issue.id === "E_TGT_REDUNDANT_FIELD"));
});

test("output overrides and specification opt-in remain explicit exceptions", () => {
  const target = { boards: { m5example: { wiring_output: "custom_wiring.hpp", specs_output: "custom_specs.hpp" } } };
  assert.deepEqual(errors(board, target), []);
  const mapping = m5gfxBoardMapping(target, board);
  assert.equal(mapping.wiringOutput, "custom_wiring.hpp");
  assert.equal(mapping.specsOutput, "custom_specs.hpp");
  assert.deepEqual(errors(board, { boards: { m5example: { wiring_output: "esp32p4_wiring.hpp" } } }), []);
});

test("catalog-wide collisions include boards absent from target metadata", () => {
  const detection = { ...board, pins: { 31: { detect_class: "up" } } };
  const alias = { ...detection, id: "m5_example", name: "M5Another", legacy_board_id: 1001 };
  assert.ok(validateTargets([detection, alias], {}).some((issue) => issue.id === "E_TGT_NAMESPACE_DUP"));
  const other = { ...board, id: "m5other", name: "M5Other", chip: "esp32s3", legacy_board_id: 1002 };
  assert.ok(validateTargets([detection, other], { boards: { m5other: { wiring_output: "esp32p4_wiring.hpp" } } }).some((issue) => issue.id === "E_TGT_OUTPUT_COLLISION"));
  assert.ok(validateTargets([board], { boards: { missing: {} } }).some((issue) => issue.id === "E_TGT_BOARD_UNKNOWN"));
});

test("specification output requires an explicit or inferred wiring mapping", () => {
  const target = { boards: { m5example: { specs_output: "esp32p4_specs.hpp" } } };
  assert.ok(errors(board, target).some((issue) => issue.id === "E_TGT_SPECS_WIRING"));
  const detection = { ...board, pins: { 31: { detect_class: "up" } } };
  assert.deepEqual(errors(detection, target), []);
});

test("target metadata rejects misspelled fields instead of applying absent defaults", () => {
  for (const field of ["probe_pin", "wiring_feilds"]) {
    const target = { boards: { m5example: { [field]: [31] } } };
    assert.ok(errors(board, target).some((issue) => issue.id === "E_TGT_UNKNOWN_FIELD" && issue.path.endsWith(`/${field}`)), field);
  }
});
