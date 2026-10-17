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

test("only a GPIO-backed power-hold device advances its detector family", () => {
  const hold = { id: "hold", chip: "test", legacy_board_id: 1,
    devices: { latch: { kind: "power_hold" } }, pins: { 3: { roles: ["dev:latch.enable"] } } };
  const pmic = { id: "pmic", chip: "test", legacy_board_id: 2,
    devices: { pm1: { kind: "pmic" } }, pins: { 4: { roles: ["dev:pm1.enable"] } } };
  const noPin = { id: "no_pin", chip: "test", legacy_board_id: 3,
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
  });
});

test("constraint reasons are required and cycles fail generation", async () => {
  const schema = JSON.parse(await fs.readFile(path.join(root, "schema/detector_order.schema.json"), "utf8"));
  const board = { id: "board", chip: "test", legacy_board_id: 1 };
  const group = { chip: "test", source: "test.inl", output: "test.hpp", families: [
    { detector: "a", boards: ["board"] }, { detector: "b", boards: ["board"] },
  ], constraints: [{ before: "a", after: "b" }] };
  assert.ok(validateSchema({ test: group }, schema).some((issue) => issue.path.endsWith("/reason")));
  group.constraints[0].reason = "G1 overlaps reset";
  group.constraints.push({ before: "b", after: "a", reason: "G2 overlaps SCL" });
  assert.throws(() => detectorPriority([board], { test: group }), /cyclic/);
});

test("generated detector arrays follow catalog priority on every chip and package", async () => {
  const filenames = (await fs.readdir(path.join(root, "boards"))).filter((name) => name.endsWith(".json"));
  const boards = await Promise.all(filenames.map(async (name) =>
    JSON.parse(await fs.readFile(path.join(root, "boards", name), "utf8"))));
  const groups = JSON.parse(await fs.readFile(path.join(root, "detector_order.json"), "utf8"));
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
  const generatedDir = path.join(sourceRoot, "src/board_detect/m5/generated");
  for (const [name, expected] of outputs) {
    assert.equal(await fs.readFile(path.join(generatedDir, name), "utf8"), expected, name);
  }

});

test("generated detector count accepts the limit and rejects an overflowing list", (t) => {
  const compiler = process.env.CXX || "c++";
  if (spawnSync(compiler, ["--version"]).error?.code === "ENOENT") return t.skip("C++ compiler unavailable");
  const board = { id: "board", chip: "test", legacy_board_id: 1 };
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
