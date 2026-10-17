import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import { promises as fs } from "node:fs";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

import { detectorPriority, hasGpioPowerHold, renderDetectorOrderHeaders } from "../lib/emit/m5gfx_detector_order.js";
import { validateSchema } from "../lib/schema.js";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const sourceRoot = path.resolve(root, "../..");

function stripCppComments(source) {
  return source.replace(/\?\?\//g, "\\").replace(/\\\r?\n/g, "")
    .replace(/\/\/[^\n]*/g, "").replace(/[ \t]+\n/g, "\n");
}

test("only a GPIO-backed power-hold device advances its detector family", () => {
  const hold = { id: "hold", name: "Hold", chip: "test", legacy_board_id: 1,
    devices: { latch: { kind: "power_hold" } }, pins: { 3: { roles: ["dev:latch.enable"] } } };
  const pmic = { id: "pmic", name: "Pmic", chip: "test", legacy_board_id: 2,
    devices: { pm1: { kind: "pmic" } }, pins: { 4: { roles: ["dev:pm1.enable"] } } };
  const noPin = { id: "no_pin", name: "NoPin", chip: "test", legacy_board_id: 3,
    devices: { latch: { kind: "power_hold" } }, pins: {} };
  assert.equal(hasGpioPowerHold(hold), true);
  assert.equal(hasGpioPowerHold(pmic), false);
  assert.equal(hasGpioPowerHold(noPin), false);
  const groups = { sample_detectors: { chip: "test", output: "sample.hpp", families: [
    { detector: "ordinary_a", boards: ["pmic"] },
    { detector: "hold_a", boards: ["hold"] },
    { detector: "ordinary_b", boards: ["no_pin"] },
  ] } };
  assert.deepEqual(detectorPriority([hold, pmic, noPin], groups).get("sample_detectors"), {
    output: "sample.hpp", ordered: ["hold_a", "ordinary_a", "ordinary_b"], holdCount: 1, edges: [],
    familyNames: new Map([["ordinary_a", "board_Pmic"], ["hold_a", "board_Hold"], ["ordinary_b", "board_NoPin"]]),
  });
});

test("constraint reasons are required and cycles fail generation", async () => {
  const schema = JSON.parse(await fs.readFile(path.join(root, "schema/detector_order.schema.json"), "utf8"));
  const board = { id: "board", name: "Board", chip: "test", legacy_board_id: 1 };
  const group = { chip: "test", source: "test.inl", output: "test.hpp", families: [
    { detector: "a", boards: ["board"] }, { detector: "b", boards: ["board"] },
  ], constraints: [{ before: "a", after: "b" }] };
  assert.ok(validateSchema({ test: group }, schema).some((issue) => issue.path.endsWith("/reason")));
  group.constraints[0].reason = "G1 overlaps reset";
  group.constraints.push({ before: "b", after: "a", reason: "G2 overlaps SCL" });
  assert.throws(() => detectorPriority([board], { test: group }), /cyclic/);
});

test("schema and generator reject reason tokens that can splice C++ comments", async () => {
  const schema = JSON.parse(await fs.readFile(path.join(root, "schema/detector_order.schema.json"), "utf8"));
  const board = { id: "board", name: "Board", chip: "test", legacy_board_id: 1 };
  const groups = { test: { chip: "test", source: "test.inl", output: "test.hpp", families: [
    { detector: "a", boards: ["board"] }, { detector: "b", boards: ["board"] },
  ], constraints: [{ before: "a", after: "b", reason: "safe" }] } };
  for (const reason of ["trailing \\", "interior \\ token", "line\n\\", "trailing ??/", "interior ??/ token", "line\n??/"]) {
    groups.test.constraints[0].reason = reason;
    assert.ok(validateSchema(groups, schema).some((issue) => issue.id === "E_SCHEMA_PATTERN"
      && issue.path === "/test/constraints/0/reason"), reason);
    assert.throws(() => renderDetectorOrderHeaders([board], groups),
      (error) => error.code === "E_DETECTOR_ORDER_REASON", reason);
  }
  assert.equal(stripCppComments("// splice ??/\n{ 1, 2 },\n{ 2, 3 },\n"), "\n{ 2, 3 },\n");
});

test("generated detector arrays follow catalog priority on every chip and package", async () => {
  const filenames = (await fs.readdir(path.join(root, "boards"))).filter((name) => name.endsWith(".json"));
  const boards = await Promise.all(filenames.map(async (name) =>
    JSON.parse(await fs.readFile(path.join(root, "boards", name), "utf8"))));
  const groups = JSON.parse(await fs.readFile(path.join(root, "detector_order.json"), "utf8"));
  const schema = JSON.parse(await fs.readFile(path.join(root, "schema/detector_order.schema.json"), "utf8"));
  assert.deepEqual(validateSchema(groups, schema), []);
  const priorities = detectorPriority(boards, groups);
  assert.deepEqual(priorities.get("esp32s3_detectors_qfn56").ordered, [
    "dial_detector", "paper_family_detector", "cores3_family_detector", "spi_id_detector", "capsule_detector",
    "pm1_family_detector", "pm1_ext_family_detector", "airq_detector",
    "cardputer_family_detector", "stamplc_detector", "powerhub_detector", "dualkey_detector",
  ]);
  assert.deepEqual(priorities.get("esp32s3_detectors_lga56").ordered, [
    "atomvoices3r_detector", "atoms3r_detector", "pmic_id_detector",
  ]);
  assert.deepEqual(priorities.get("esp32_d0wdq6_detectors").ordered,
                   ["paper_family_detector", "axp_family_detector", "stack_family_detector", "timercam_detector"]);
  assert.deepEqual(priorities.get("esp32_pico_d4_detectors").ordered,
                   ["stickc_family_detector", "coreink_detector", "atom_family_detector"]);
  assert.deepEqual(priorities.get("esp32c5_detectors").ordered,
                   ["stampc5_detector", "toughc5_detector"]);
  assert.deepEqual(priorities.get("esp32c6_detectors_qfn32").ordered,
                   ["stampc6_detector", "nanoc6_detector"]);
  assert.deepEqual(priorities.get("esp32h2_detectors").ordered,
                   ["nanoh2_detector"]);

  const outputs = renderDetectorOrderHeaders(boards, groups);
  const effectiveConstraints = stripCppComments(outputs.get("detector_order_constraints.hpp"));
  const effectiveEdges = [...effectiveConstraints.matchAll(/^\s*\{\s*\d+,\s*\d+\s*\},/gm)];
  assert.equal(effectiveEdges.length, Object.values(groups).reduce((count, group) => count + (group.constraints?.length ?? 0), 0));
  const generatedDir = path.join(sourceRoot, "src/board_detect/m5/generated");
  for (const [name, expected] of outputs) {
    assert.equal(await fs.readFile(path.join(generatedDir, name), "utf8"), expected, name);
  }

});

test("generated detector count accepts the limit and rejects an overflowing list", (t) => {
  const compiler = process.env.CXX || "c++";
  if (spawnSync(compiler, ["--version"]).error?.code === "ENOENT") return t.skip("C++ compiler unavailable");
  const board = { id: "board", name: "Board", chip: "test", legacy_board_id: 1 };
  const groups = { test_detectors: { chip: "test", output: "test_order.hpp", families: [
    { detector: "first", boards: ["board"] }, { detector: "second", boards: ["board"] },
  ] } };
  const array = renderDetectorOrderHeaders([board], groups).get("test_order.hpp");
  const compile = (limit) => spawnSync(compiler, ["-std=c++11", "-x", "c++", "-fsyntax-only", "-"], {
    encoding: "utf8", input: `struct board_detector_t {};
static const board_detector_t first{}, second{};
`
      + `static constexpr unsigned max_detector_families = ${limit};
` + array,
  });
  const atLimit = compile(2);
  assert.equal(atLimit.status, 0, atLimit.stderr);
  const overflow = compile(1);
  assert.notEqual(overflow.status, 0);
  assert.match(overflow.stderr, /test_detectors exceeds the detection session family limit/);
});

test("detector comments name every family member and preserve reasons on one line", () => {
  const boards = [
    { id: "first", name: "First", chip: "test", legacy_board_id: 11,
      devices: { latch: { kind: "power_hold" } }, pins: { 3: { roles: ["dev:latch.enable"] } } },
    { id: "sibling", name: "Sibling", chip: "test", legacy_board_id: 12 },
    { id: "last", name: "Last", chip: "test", legacy_board_id: 21 },
  ];
  const groups = { test_detectors: { chip: "test", output: "test_order.hpp", families: [
    { detector: "first_detector", boards: ["first", "sibling"] },
    { detector: "last_detector", boards: ["last"] },
  ], constraints: [{ before: "first_detector", after: "last_detector", reason: "G3 overlaps reset\r\nProbe first\n#error injected" }] } };
  const priority = detectorPriority(boards, groups).get("test_detectors");
  assert.deepEqual(priority.edges, [{ before: 11, after: 21,
    beforeDetector: "first_detector", afterDetector: "last_detector",
    beforeBoards: "board_First, board_Sibling", afterBoards: "board_Last",
    reason: groups.test_detectors.constraints[0].reason }]);
  const outputs = renderDetectorOrderHeaders(boards, groups);
  assert.ok(outputs.get("test_order.hpp").includes("&first_detector, // board_First, board_Sibling"));
  assert.ok(outputs.get("test_order.hpp").includes("&last_detector, // board_Last"));
  assert.ok(outputs.get("gpio_power_hold_board_ids.hpp").includes("11, // board_First"));
  const constraints = outputs.get("detector_order_constraints.hpp");
  assert.ok(constraints.includes("Each representative board ID orders the entire family, including all listed members."));
  assert.ok(constraints.includes("{ 11, 21 }, // first_detector (board_First, board_Sibling) before last_detector (board_Last): G3 overlaps reset Probe first #error injected\n"));
  const renamed = boards.map((board) => ({ ...board, name: `${board.name}Renamed` }));
  const changedReason = structuredClone(groups);
  changedReason.test_detectors.constraints[0].reason = "Different explanation";
  for (const [name, output] of renderDetectorOrderHeaders(renamed, changedReason)) {
    assert.equal(stripCppComments(output), stripCppComments(outputs.get(name)), name);
  }
});
