import assert from "node:assert/strict";
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
  for (const [name, group] of Object.entries(groups)) {
    const source = await fs.readFile(path.join(sourceRoot, group.source), "utf8");
    assert.ok(source.includes(`#include \"${path.relative(path.dirname(group.source), `src/board_detect/m5/generated/${group.output}`)}\"`), name);
    assert.ok(!source.includes(`const ${name}[] =`), `${name} must be generated`);
  }
});

test("GPIO power-hold catalog and M5GFX descriptions agree", async () => {
  const targets = JSON.parse(await fs.readFile(path.join(root, "targets.json"), "utf8"));
  for (const [id, target] of Object.entries(targets.m5gfx_board_desc.boards)) {
    if (!target.desc_name) continue;
    const board = JSON.parse(await fs.readFile(path.join(root, "boards", `${id}.json`), "utf8"));
    const expected = hasGpioPowerHold(board);
    const files = ["esp32_d0wdq6.inl", "esp32_pico.inl", "esp32s3/families.inl", "esp32s3/cores3.inl",
      "esp32c3.inl", "esp32c5/toughc5.inl", "esp32c6/c6_display.inl", "esp32c6/c6_displayless.inl", "esp32h2.inl",
      "esp32c61/corematrix.inl", "esp32p4/corep4x.inl", "esp32p4/tab5.inl", "esp32p4/unitpoep4.inl", "esp32p4/stampp4.inl"];
    const sources = await Promise.all(files.map((name) => fs.readFile(path.join(sourceRoot, "src/board_detect/m5", name), "utf8")));
    const source = sources.find((text) => text.includes(`${target.desc_name} = {`));
    assert.ok(source, id);
    const beginning = source.slice(source.indexOf(`${target.desc_name} = {`));
    const power = beginning.slice(0, beginning.indexOf("display_pins("));
    assert.equal(/gpio_power(?:_low)?\(/.test(power), expected, id);
  }
});
