import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import vm from "node:vm";

import { assertUniqueFlattenedNames, renderBundle } from "../build/bundle.js";
import { pinChangeLayer } from "../editor/provenance.js";
import { filterBoardOptions, gpioMatchesFilter, matchesSearchText } from "../editor/filters.js";
import { assignConnectorLanes, connectorRolesByLane, defaultRoleColor, objectRoleOwner, roleColor, roleContrastColor, roleOwner } from "../editor/role_colors.js";
import { escapeHtml, gpioTargetButton, ownerSignalsHtml } from "../editor/html.js";
import { addRole, setBoardField } from "../editor/board_ops.js";
import { validateChoices } from "../lib/choices.js";
import { compose, validateAccessory, validateComposition } from "../lib/compose.js";
import { isCompatible, validateConnectorTypes } from "../lib/ctypes.js";
import { deriveSd } from "../lib/derive/sd.js";
import { consumesPinTableRole, emitPinTable, PIN_NAMES, pinNameForRole, pinTableAssignments } from "../lib/emit/m5unified_pin_table.js";
import { detectionPinsForEntries, emitM5GFXWiring, emitM5GFXWiringForMapping, m5gfxBoardMapping, partitionDetectionPins, renderM5GFXWiringHeader, selectM5GFXWiringBoards, validateDetectionPins, wiringAssignments, wiringFieldsForRole } from "../lib/emit/m5gfx_board_wiring.js";
import { emitM5GFXSpecs, renderM5GFXSpecsHeader } from "../lib/emit/m5gfx_board_specs.js";
import { formatBoard } from "../lib/format.js";
import { clone, effectiveChip } from "../lib/model.js";
import { isGeneratedField, pintableAssignments } from "../lib/pintable_roles.js";
import { validateOwners } from "../lib/owners.js";
import { validatePartCatalog, validateParts } from "../lib/parts.js";
import { resolveAll, resolveBoard, resolvedFilename } from "../lib/resolve.js";
import { validateTarget, validateTargets } from "../lib/targets.js";
import { validateBoard, validateCatalog, validateFormat, validateResolvedVariants } from "../lib/validate.js";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const board = JSON.parse(await fs.readFile(path.join(root, "boards/m5stack_core2.json"), "utf8"));
const schema = JSON.parse(await fs.readFile(path.join(root, "schema/board.schema.json"), "utf8"));
const chip = JSON.parse(await fs.readFile(path.join(root, "chips/esp32_d0wdq6.json"), "utf8"));
const chipS3 = JSON.parse(await fs.readFile(path.join(root, "chips/esp32s3.json"), "utf8"));
const chipC5 = JSON.parse(await fs.readFile(path.join(root, "chips/esp32c5.json"), "utf8"));
const chipC6 = JSON.parse(await fs.readFile(path.join(root, "chips/esp32c6.json"), "utf8"));
const chipC61 = JSON.parse(await fs.readFile(path.join(root, "chips/esp32c61.json"), "utf8"));
const chipP4 = JSON.parse(await fs.readFile(path.join(root, "chips/esp32p4.json"), "utf8"));
const chips = { esp32_d0wdq6: chip, esp32s3: chipS3, esp32c5: chipC5, esp32c6: chipC6, esp32c61: chipC61, esp32p4: chipP4 };
const connectorTypeFiles = (await fs.readdir(path.join(root, "connector_types"))).filter((name) => name.endsWith(".json")).sort();
const connectorTypes = Object.fromEntries(await Promise.all(connectorTypeFiles.map(async (name) => {
  const value = JSON.parse(await fs.readFile(path.join(root, "connector_types", name), "utf8"));
  return [value.id, value];
})));
const partFiles = (await fs.readdir(path.join(root, "parts"))).filter((name) => name.endsWith(".json")).sort();
const parts = Object.fromEntries(await Promise.all(partFiles.map(async (name) => {
  const value = JSON.parse(await fs.readFile(path.join(root, "parts", name), "utf8"));
  return [value.id, value];
})));
const accessoryFiles = (await fs.readdir(path.join(root, "accessories"))).filter((name) => name.endsWith(".json")).sort();
const accessories = Object.fromEntries(await Promise.all(accessoryFiles.map(async (name) => {
  const value = JSON.parse(await fs.readFile(path.join(root, "accessories", name), "utf8"));
  return [value.id, value];
})));
const targets = JSON.parse(await fs.readFile(path.join(root, "targets.json"), "utf8"));
const target = targets.m5gfx_board_desc;
const context = { schema, chip, connectorTypes, parts, target };
const catalogFiles = (await fs.readdir(path.join(root, "boards"))).filter((name) => name.endsWith(".json")).sort();
const catalogBoards = await Promise.all(catalogFiles.map(async (name) => JSON.parse(await fs.readFile(path.join(root, "boards", name), "utf8"))));
const editorHtml = await fs.readFile(path.join(root, "editor/index.html"), "utf8");
const editorSource = await fs.readFile(path.join(root, "editor/editor.js"), "utf8");
const editorCSS = await fs.readFile(path.join(root, "editor/editor.css"), "utf8");
const cliSource = await fs.readFile(path.join(root, "cli/spec.js"), "utf8");
const picoSource = await fs.readFile(path.join(root, "../../src/board_detect/m5/esp32_pico.inl"), "utf8");
const picoSetupSource = await fs.readFile(path.join(root, "../../src/board_detect/m5/esp32_pico_setup.inl"), "utf8");
const d0wdq6Source = await fs.readFile(path.join(root, "../../src/board_detect/m5/esp32_d0wdq6.inl"), "utf8")
  + "\n" + picoSource;
const esp32s3DetectorPath = path.join(root, "../../src/board_detect/m5/esp32s3/families.inl");
const esp32s3SetupPath = path.join(root, "../../src/board_detect/m5/esp32s3/families_setup.inl");
const esp32s3Source = (await fs.readFile(esp32s3DetectorPath, "utf8"))
  + "\n" + (await fs.readFile(path.join(root, "../../src/board_detect/m5/esp32s3/cores3.inl"), "utf8"));
const esp32c5Source = await fs.readFile(path.join(root, "../../src/board_detect/m5/esp32c5/toughc5.inl"), "utf8");
const esp32c6Source = await fs.readFile(path.join(root, "../../src/board_detect/m5/esp32c6/c6_display.inl"), "utf8");
const esp32c6SetupSource = await fs.readFile(path.join(root, "../../src/board_detect/m5/esp32c6/c6_display_setup.inl"), "utf8");
const esp32c61Source = await fs.readFile(path.join(root, "../../src/board_detect/m5/esp32c61/corematrix.inl"), "utf8");
const esp32p4Source = (await fs.readFile(path.join(root, "../../src/board_detect/m5/esp32p4/corep4x.inl"), "utf8"))
  + "\n" + (await fs.readFile(path.join(root, "../../src/board_detect/m5/esp32p4/tab5.inl"), "utf8"));
const tab5SetupSource = await fs.readFile(path.join(root, "../../src/board_detect/m5/esp32p4/tab5_setup.inl"), "utf8");
const pmicOpsSource = await fs.readFile(path.join(root, "../../src/board_detect/m5/pmic_ops.hpp"), "utf8");
const i18nMatch = /<script type="application\/json" id="board-spec-i18n">([\s\S]*?)<\/script>/.exec(editorHtml);
const messages = JSON.parse(i18nMatch[1]);
const compositionTarget = targets.m5unified_pin_table;
const resolveCatalog = (item) => resolveAll(item, connectorTypes, {
  chip: chips[item.chip], parts, accessories,
  composition: compositionTarget.compositions?.[item.id]?.default,
  allow_origins: compositionTarget.allow_origins,
});

test("board search matches name, board ID, chip, and legacy ID case-insensitively", () => {
  const options = [
    { value: "m5stack_core2", label: "M5StackCore2 — M5Stack Core2", officialName: "M5Stack Core2", chip: "esp32_d0wdq6", legacyId: 3, aliases: ["Core II", "second core"] },
    { value: "m5stack_tab5", label: "M5Stack Tab5", chip: "esp32p4", legacyId: 55 },
  ];
  assert.deepEqual(filterBoardOptions(options, "CORE2").map((item) => item.value), ["m5stack_core2"]);
  assert.deepEqual(filterBoardOptions(options, "tab5").map((item) => item.value), ["m5stack_tab5"]);
  assert.deepEqual(filterBoardOptions(options, "ESP32P4").map((item) => item.value), ["m5stack_tab5"]);
  assert.deepEqual(filterBoardOptions(options, "55").map((item) => item.value), ["m5stack_tab5"]);
  assert.deepEqual(filterBoardOptions(options, "SECOND").map((item) => item.value), ["m5stack_core2"]);
  assert.deepEqual(filterBoardOptions(options, "M5Stack Core2").map((item) => item.value), ["m5stack_core2"]);
  assert.equal(matchesSearchText("M5Stack Core2 esp32", "m5 core"), true);
});

test("connector lanes use deterministic greedy coloring by shared GPIO", () => {
  const pins = (entries) => Object.fromEntries(entries.map(([gpio, connectors]) => [gpio, { roles: connectors.map((id) => `conn:${id}.1`) }]));
  assert.deepEqual(assignConnectorLanes({}), { lanes: [], laneByConnector: {} });
  assert.deepEqual(assignConnectorLanes(pins([["1", ["a"]], ["2", ["a"]], ["3", ["b"]]])), {
    lanes: [["a", "b"]], laneByConnector: { a: 0, b: 0 },
  });
  assert.deepEqual(assignConnectorLanes(pins([["1", ["a"]], ["2", ["a", "b"]]])), {
    lanes: [["a"], ["b"]], laneByConnector: { a: 0, b: 1 },
  });
  const chain = pins([["1", ["a", "b"]], ["2", ["b", "c"]]]);
  assert.deepEqual(assignConnectorLanes(chain), {
    lanes: [["b"], ["a", "c"]], laneByConnector: { b: 0, a: 1, c: 1 },
  });
  const reordered = Object.fromEntries(Object.entries(chain).reverse().map(([gpio, pin]) => [gpio, { roles: [...pin.roles].reverse() }]));
  assert.deepEqual(assignConnectorLanes(reordered), assignConnectorLanes(chain));
  assert.deepEqual(assignConnectorLanes({ 7: { roles: ["m5go_bottom2/conn:port_c.1"] } }), {
    lanes: [["port_c"]], laneByConnector: { port_c: 0 },
  });
});

test("resolved catalog never places different connectors in one lane on one GPIO", () => {
  for (const source of catalogBoards) {
    for (const output of resolveCatalog(source)) {
      const assignment = assignConnectorLanes(output.board.pins);
      for (const [gpio, pin] of Object.entries(output.board.pins ?? {})) {
        for (const lane of connectorRolesByLane(pin.roles, assignment)) {
          const owners = new Set(lane.map((role) => roleOwner(role)?.id));
          assert.ok(owners.size <= 1, `${output.filename} GPIO ${gpio}: ${lane.join(", ")}`);
        }
      }
    }
  }
});

test("board aliases validate, format after name, and never affect generated outputs", () => {
  const withAliases = clone(board);
  setBoardField(withAliases, "aliases", [" Core II ", "core ii", "second core"]);
  assert.deepEqual(withAliases.aliases, ["Core II", "second core"]);
  assert.deepEqual(validateBoard(withAliases, context).filter((item) => item.path.startsWith("/aliases")), []);
  const formatted = formatBoard(withAliases);
  assert.ok(formatted.indexOf('"name"') < formatted.indexOf('"aliases"'));
  assert.ok(formatted.indexOf('"aliases"') < formatted.indexOf('"legacy_board_id"'));

  const invalid = clone(board);
  invalid.aliases = [" Core II", "core ii"];
  assert.deepEqual(validateBoard(invalid, context).filter((item) => item.id === "E_ALIAS_FORMAT").map((item) => item.path), ["/aliases/0", "/aliases/1"]);

  const originalResolved = resolveCatalog(board)[0].board;
  const aliasResolved = resolveCatalog(withAliases)[0].board;
  assert.deepEqual(emitPinTable(aliasResolved, compositionTarget), emitPinTable(originalResolved, compositionTarget));
  assert.equal(
    renderM5GFXWiringHeader([{ board: withAliases, chip, emitted: emitM5GFXWiring(aliasResolved, parts, target) }]),
    renderM5GFXWiringHeader([{ board, chip, emitted: emitM5GFXWiring(originalResolved, parts, target) }]),
  );
  const specBoard = catalogBoards.find((item) => item.id === "m5stack_tab5");
  const specWithAliases = clone(specBoard);
  specWithAliases.aliases = ["Large P4 display"];
  const mapping = m5gfxBoardMapping(target, specBoard.id);
  assert.equal(
    renderM5GFXSpecsHeader(emitM5GFXSpecs(specWithAliases, resolveCatalog(specWithAliases).map((item) => item.board), parts, mapping)),
    renderM5GFXSpecsHeader(emitM5GFXSpecs(specBoard, resolveCatalog(specBoard).map((item) => item.board), parts, mapping)),
  );
});

test("only catalog names confirmed against official sources carry the verified flag", () => {
  const documentedProductNames = {
    arduino_nesso_n1: "Arduino Nesso N1",
    m5airq: "Air Quality",
    m5atoms3: "AtomS3",
    m5atoms3r: "AtomS3R",
    m5cardputer: "Cardputer",
    m5cardputer_adv: "Cardputer-Adv",
    m5chaincaptain: "Chain Captain",
    m5dial: "Dial",
    m5dinmeter: "DinMeter",
    m5paper: "Paper",
    m5papercolor: "PaperColor",
    m5papermono: "PaperMono",
    m5papers3: "PaperS3",
    m5stack_core2: "Core2",
    m5stack_coreink: "CoreInk",
    m5stack_corep4x: "CoreP4X",
    m5stack_cores3: "CoreS3",
    m5stack_cores3se: "CoreS3-SE",
    m5stack_stackchan: "StackChan",
    m5stack_tab5: "Tab5",
    m5stamplc: "StamPLC",
    m5stickc: "StickC",
    m5stickcplus: "StickC-Plus",
    m5stickcplus2: "StickC-Plus2",
    m5sticks3: "StickS3",
    m5stopwatch: "StopWatch",
    m5timercam: "TimerCamera",
    m5tough: "Tough",
    m5unit_c6l: "Unit C6L",
    m5vameter: "VAMeter",
  };
  const unverified = catalogBoards.filter((item) => item.official_name_verified !== true).map((item) => item.id).sort();
  assert.deepEqual(unverified, [
    "m5atom_psram", "m5paperdiy", "m5stack", "m5stack_corematrix", "m5stack_tab5x", "m5station", "m5toughc5",
  ]);
  const verified = catalogBoards.filter((item) => item.official_name_verified === true).map((item) => item.id).sort();
  assert.deepEqual(verified, Object.keys(documentedProductNames).sort());
  for (const source of catalogBoards.filter((item) => item.official_name_verified === true)) {
    const productName = source.official_name.replace(/^M5Stack /, "");
    assert.equal(productName, documentedProductNames[source.id], source.id);
  }
  assert.equal(catalogBoards.some((item) => item.official_name_verified === false), false);
  assert.match(schema.properties.official_name_verified.description, /product-name portion.*official documentation page title/);
  assert.match(editorHtml, /help\.officialNameVerified/);
  const sample = clone(board);
  setBoardField(sample, "official_name_verified", false);
  assert.equal(Object.hasOwn(sample, "official_name_verified"), false);
  setBoardField(sample, "official_name_verified", true);
  assert.equal(sample.official_name_verified, true);
});

test("schema-invalid hostile board strings are escaped in every editor HTML sink", () => {
  const payload = '\"><img src=x onerror="globalThis.__boardSpecPwned=1">';
  const hostile = clone(board);
  hostile.legacy_board_id = payload;
  hostile.pins[payload] = { roles: [] };
  hostile.devices.hostile = { kind: "unknown", signals: { [payload]: `pin:soc.${payload}` } };
  const errors = validateBoard(hostile, context);
  assert.ok(errors.some((item) => item.id === "E_SCHEMA_TYPE" && item.path === "/legacy_board_id"));
  assert.ok(errors.some((item) => item.path.includes("/pins/")));

  const rendered = [escapeHtml(hostile.legacy_board_id), gpioTargetButton(payload), ownerSignalsHtml(hostile.devices.hostile)].join("\n");
  assert.doesNotMatch(rendered, /<img|onerror="globalThis/);
  assert.match(rendered, /&lt;img/);
  assert.match(rendered, /&quot;/);
  assert.match(editorSource, /value="\$\{escapeHtml\(board\.legacy_board_id\)\}"/);
  assert.match(editorSource, /pins\.map\(gpioTargetButton\)/);
  assert.match(editorSource, /ownerSignalsHtml\(board\.devices\?\.\[current\.id\]\)/);
  assert.match(editorSource, /const safeKey = escapeHtml\(key\)/);
  assert.doesNotMatch(editorSource, /value="\$\{board\.legacy_board_id\}"/);
});

test("opening the same board asks before replacing dirty memory and draft state", () => {
  const openFileSource = /async function openFile\(file\) \{([\s\S]*?)\n\}\n\nfunction downloadCurrent/.exec(editorSource)?.[1];
  assert.ok(openFileSource);
  const confirmAt = openFileSource.indexOf("existing?.dirty && !window.confirm");
  assert.ok(confirmAt >= 0);
  assert.ok(confirmAt < openFileSource.indexOf("state.documents.set"));
  assert.ok(confirmAt < openFileSource.indexOf("removeDraft(value.id)"));
  assert.match(editorHtml, /file\.replaceDirtyConfirm/);
});

test("GPIO filtering matches pin numbers and roles and can hide unassigned pins", () => {
  assert.equal(gpioMatchesFilter("21", { roles: ["bus:internal_i2c.sda"] }, "21"), true);
  assert.equal(gpioMatchesFilter("21", { roles: ["bus:internal_i2c.sda"] }, "I2C"), true);
  assert.equal(gpioMatchesFilter("22", { roles: [] }, "i2c"), false);
  assert.equal(gpioMatchesFilter("22", { roles: [] }, "", true), false);
  assert.equal(gpioMatchesFilter("21", { roles: ["bus:internal_i2c.sda"] }, "", true), true);
});

test("role colors are stable per object and keep readable contrast", () => {
  assert.deepEqual(roleOwner("bus:main_spi.sclk"), { kind: "bus", id: "main_spi", key: "bus:main_spi" });
  assert.deepEqual(roleOwner("m5go_bottom2/conn:mbus.24"), { kind: "conn", id: "mbus", key: "conn:mbus" });
  assert.equal(roleOwner("invalid"), null);
  assert.equal(defaultRoleColor("mbus"), "#f6d6a8");
  assert.equal(defaultRoleColor("custom_object"), defaultRoleColor("custom_object"));
  assert.match(defaultRoleColor("custom_object"), /^#[0-9a-f]{6}$/);
  assert.equal(roleContrastColor("#111111"), "#ffffff");
  assert.equal(roleContrastColor("#eeeeee"), "#17212b");
  assert.deepEqual(roleColor(objectRoleOwner("dev", "lcd"), { "dev:lcd": "#112233" }), { background: "#112233", foreground: "#ffffff" });
});

const legacyEsp32Wiring = {
  station: {
    display_sclk: 18, display_mosi: 23, display_miso: -1, display_dc: 19,
    display_cs: 5, display_rst: 15, display_busy: -1,
    internal_i2c_sda: 21, internal_i2c_scl: 22, internal_i2c_port: 1,
    reset_gpio: 15, hold: [5],
  },
  core2: {
    display_sclk: 18, display_mosi: 23, display_miso: 38, display_dc: 15,
    display_cs: 5, display_rst: -1, display_busy: -1,
    shared_sd_sclk: 18, shared_sd_mosi: 23, shared_sd_miso: 38,
    shared_sd_sd_cs: 4, shared_sd_other_cs: 5,
    internal_i2c_sda: 21, internal_i2c_scl: 22, internal_i2c_port: 1,
    hold: [4, 5],
  },
  tough: {
    display_sclk: 18, display_mosi: 23, display_miso: 38, display_dc: 15,
    display_cs: 5, display_rst: -1, display_busy: -1,
    shared_sd_sclk: 18, shared_sd_mosi: 23, shared_sd_miso: 38,
    shared_sd_sd_cs: 4, shared_sd_other_cs: 5,
    internal_i2c_sda: 21, internal_i2c_scl: 22, internal_i2c_port: 1,
    hold: [4, 5],
  },
  stack: {
    display_sclk: 18, display_mosi: 23, display_miso: 19, display_dc: 27,
    display_cs: 14, display_rst: 33, display_busy: -1,
    shared_sd_sclk: 18, shared_sd_mosi: 23, shared_sd_miso: 19,
    shared_sd_sd_cs: 4, shared_sd_other_cs: 14,
    reset_gpio: 33, hold: [4, 14],
  },
  paper: {
    display_sclk: 14, display_mosi: 12, display_miso: 13, display_dc: -1,
    display_cs: 15, display_rst: 23, display_busy: 27,
    shared_sd_sclk: 14, shared_sd_mosi: 12, shared_sd_miso: 13,
    shared_sd_sd_cs: 4, shared_sd_other_cs: 15,
    reset_gpio: 23, power_gpio: 2, hold: [4, 15],
  },
  stickc: {
    display_sclk: 13, display_mosi: 15, display_miso: 14, display_dc: 23,
    display_cs: 5, display_rst: 18, display_busy: -1,
    internal_i2c_sda: 21, internal_i2c_scl: 22, internal_i2c_port: 1,
    reset_gpio: 18, hold: [5],
  },
  stickcplus: {
    display_sclk: 13, display_mosi: 15, display_miso: 14, display_dc: 23,
    display_cs: 5, display_rst: 18, display_busy: -1,
    internal_i2c_sda: 21, internal_i2c_scl: 22, internal_i2c_port: 1,
    reset_gpio: 18, hold: [5],
  },
  coreink: {
    display_sclk: 18, display_mosi: 23, display_miso: 34, display_dc: 15,
    display_cs: 9, display_rst: 0, display_busy: 4,
    reset_gpio: 0, power_gpio: 12, hold: [9],
  },
  stickcplus2: {
    display_sclk: 13, display_mosi: 15, display_miso: -1, display_dc: 14,
    display_cs: 5, display_rst: 12, display_busy: -1,
    reset_gpio: 12, power_gpio: 4, backlight_gpio: 27, hold: [5],
  },
};

function parseGeneratedWiring(source) {
  const result = {};
  const wiring = source.split("namespace m5gfx { namespace board_detect { namespace m5 { namespace wiring {")[1]
    .split("} // namespace wiring")[0];
  for (const match of wiring.matchAll(/namespace (\w+) \{([\s\S]*?)\n\} \/\/ namespace \1/g)) {
    if (match[1] === "detection") continue;
    const values = {};
    for (const scalar of match[2].matchAll(/constexpr std::int8_t (\w+) = (-?\d+);/g)) {
      values[scalar[1]] = Number(scalar[2]);
    }
    for (const array of match[2].matchAll(/constexpr std::int8_t (\w+)\[\] = \{ ([^}]*) \};/g)) {
      values[array[1]] = array[2].split(",").map((value) => Number(value.trim()));
    }
    result[match[1]] = values;
  }
  return result;
}

function stripGenerated(value) {
  if (Array.isArray(value)) return value.forEach(stripGenerated);
  if (!value || typeof value !== "object") return;
  if (value.verified) {
    for (const [key, status] of Object.entries(value.verified)) if (status === "generated") delete value.verified[key];
    if (Object.keys(value.verified).length === 0) delete value.verified;
  }
  for (const child of Object.values(value)) stripGenerated(child);
}

function fixture(mutator) {
  const value = clone(board);
  stripGenerated(value);
  mutator(value);
  return value;
}

function assertSingle(id, value) {
  const errors = validateBoard(value, context).filter((item) => item.severity !== "warning");
  assert.deepEqual(errors.map((item) => item.id), [id], JSON.stringify(errors, null, 2));
}

test("valid Core2 has only the expected target warning", () => {
  assert.deepEqual(validateBoard(board, context).map((item) => item.id), ["W_TGT_REV_INDISTINGUISHABLE", "W_TGT_REV_INDISTINGUISHABLE"]);
});

test("all catalog boards validate against their chip tables", () => {
  assert.deepEqual(catalogFiles, ["arduino_nesso_n1.json", "m5airq.json", "m5atom_psram.json", "m5atoms3.json", "m5atoms3r.json", "m5cardputer.json", "m5cardputer_adv.json", "m5chaincaptain.json", "m5dial.json", "m5dinmeter.json", "m5paper.json", "m5papercolor.json", "m5paperdiy.json", "m5papermono.json", "m5papers3.json", "m5stack.json", "m5stack_core2.json", "m5stack_coreink.json", "m5stack_corematrix.json", "m5stack_corep4x.json", "m5stack_cores3.json", "m5stack_cores3se.json", "m5stack_stackchan.json", "m5stack_tab5.json", "m5stack_tab5x.json", "m5stamplc.json", "m5station.json", "m5stickc.json", "m5stickcplus.json", "m5stickcplus2.json", "m5sticks3.json", "m5stopwatch.json", "m5timercam.json", "m5tough.json", "m5toughc5.json", "m5unit_c6l.json", "m5vameter.json"]);
  assert.deepEqual(validateCatalog(catalogBoards, (item) => ({ ...context, chip: chips[item.chip] })).filter((item) => item.severity !== "warning"), []);
});

test("catalog button devices match M5Unified GPIO, expander, and PMIC inputs", () => {
  const gpioButtons = {
    m5stack: { btn_a: 39, btn_b: 38, btn_c: 37 }, m5atom_psram: { btn_a: 39 },
    m5paper: { btn_a: 37, btn_b: 38, btn_c: 39 }, m5station: { btn_a: 37, btn_b: 38, btn_c: 39 },
    m5stack_coreink: { btn_a: 37, btn_b: 38, btn_c: 39, btn_ext: 5, btn_pwr: 27 },
    m5stickc: { btn_a: 37, btn_b: 39 }, m5stickcplus: { btn_a: 37, btn_b: 39 },
    m5stickcplus2: { btn_a: 37, btn_b: 39, btn_pwr: 35 }, m5airq: { btn_a: 0, btn_b: 8 },
    m5vameter: { btn_a: 2, btn_b: 0 }, m5cardputer: { btn_a: 0 }, m5cardputer_adv: { btn_a: 0 },
    m5atoms3: { btn_a: 41 }, m5atoms3r: { btn_a: 41 }, m5dial: { btn_a: 42, btn_b: 0 },
    m5dinmeter: { btn_a: 42, btn_b: 0 }, m5sticks3: { btn_a: 11, btn_b: 12 },
    m5paperdiy: { btn_a: 4, btn_b: 3 }, m5papercolor: { btn_a: 10, btn_b: 9, btn_c: 1 },
    m5chaincaptain: { btn_a: 1, btn_b: 4, btn_c: 5 }, m5papermono: { btn_a: 2, btn_b: 3 },
    m5stopwatch: { btn_a: 2, btn_b: 1 },
  };
  const ownedButtons = {
    m5stamplc: { btn_a: "pin:ioe.p2", btn_b: "pin:ioe.p1", btn_c: "pin:ioe.p0" },
    m5unit_c6l: { btn_a: "pin:ioe.p0" },
    arduino_nesso_n1: { btn_a: "pin:pi4io1.p0", btn_b: "pin:pi4io1.p1" },
    m5stack_corematrix: { btn_a: "pin:pmic.gpio0", btn_b: "pin:pmic.gpio1", btn_c: "pin:pmic.gpio2" },
  };
  const pmicPowerBoards = new Set([
    "m5chaincaptain", "m5papercolor", "m5paperdiy", "m5papermono", "m5stack_core2",
    "m5stack_corematrix", "m5stack_cores3", "m5stack_cores3se", "m5stack_stackchan",
    "m5stickc", "m5stickcplus", "m5sticks3", "m5stopwatch", "m5tough", "m5toughc5",
  ]);
  for (const source of catalogBoards) for (const { board: resolved } of resolveCatalog(source)) {
    const expected = Object.fromEntries(Object.entries(gpioButtons[source.id] ?? {}).map(([id, gpio]) => [id, `gpio:${gpio}`]));
    Object.assign(expected, ownedButtons[source.id] ?? {});
    if (pmicPowerBoards.has(source.id)) expected.btn_pwr = "pin:pmic.pwrkey";
    const actualIds = Object.entries(resolved.devices).filter(([, device]) => device.kind === "button").map(([id]) => id).sort();
    assert.deepEqual(actualIds, Object.keys(expected).sort(), source.id);
    for (const [id, endpoint] of Object.entries(expected)) {
      assert.equal(resolved.devices[id].part, "button", `${source.id}.${id}.part`);
      assert.equal(resolved.devices[id].signals.in, endpoint, `${source.id}.${id}.in`);
      assert.equal(resolved.devices[id].spec.active_low, true, `${source.id}.${id}.active_low`);
    }
  }
});

test("ESP32 USB-UART bridges use fixed UART0 wiring and automatic boot control", () => {
  const expectedParts = {
    m5atom_psram: "ch9102", m5paper: "cp2104", m5stack_coreink: "cp2104",
    m5station: "ch9102f", m5stickc: "ch552", m5stickcplus: "ch552",
    m5stickcplus2: "ch9102", m5timercam: "ch552", m5tough: "ch9102",
  };
  const expectedBoards = new Set([...Object.keys(expectedParts), "m5stack", "m5stack_core2"]);
  const actualBoards = new Set(catalogBoards.filter((item) => item.devices.usb_serial).map((item) => item.id));
  assert.deepEqual(actualBoards, expectedBoards);
  for (const source of catalogBoards.filter((item) => expectedBoards.has(item.id))) {
    assert.deepEqual(source.buses.console_uart, {
      kind: "uart", signals: ["tx", "rx"], preferred_host: 0, fixed: true,
    });
    assert.ok(source.pins["0"].roles.includes("dev:usb_serial.boot"), source.id);
    assert.ok(source.pins["1"].roles.includes("bus:console_uart.tx"), source.id);
    assert.ok(source.pins["3"].roles.includes("bus:console_uart.rx"), source.id);
    for (const { board: resolved } of resolveCatalog(source)) {
      assert.equal(resolved.devices.usb_serial.kind, "usb_uart", source.id);
      assert.equal(resolved.devices.usb_serial.bus, "console_uart", source.id);
      assert.equal(resolved.devices.usb_serial.spec.auto_reset, true, source.id);
      assert.equal(resolved.devices.usb_serial.signals.boot, "gpio:0", source.id);
      if (expectedParts[source.id]) assert.equal(resolved.devices.usb_serial.part, expectedParts[source.id], source.id);
    }
  }
  assert.equal(catalogBoards.find((item) => item.id === "m5stack").devices.usb_serial.choices.ch9102f.part, "ch9102f");
  assert.equal(catalogBoards.find((item) => item.id === "m5stack_core2").devices.usb_serial.choices.ch9102f.part, "ch9102f");
});

test("catalog PSRAM devices match storage metadata and chip pin definitions", () => {
  const fixed = {
    m5atom_psram: [2, "quad"], m5paper: [8, "quad"], m5stack_core2: [8, "quad"],
    m5stickcplus2: [2, "quad"], m5timercam: [8, "quad"], m5tough: [8, "quad"],
    m5atoms3r: [8, "opi"], m5chaincaptain: [8, "opi"], m5papercolor: [8, "opi"],
    m5paperdiy: [8, "opi"], m5papermono: [8, "opi"], m5papers3: [8, "opi"],
    m5stack_cores3: [8, "quad"], m5stack_cores3se: [8, "quad"], m5stack_stackchan: [8, "quad"],
    m5sticks3: [8, "opi"], m5stack_corep4x: [32, "opi"], m5stack_tab5: [32, "opi"],
    m5stack_tab5x: [32, "opi"], m5toughc5: [8, "quad"],
  };
  for (const [id, [size, mode]] of Object.entries(fixed)) {
    const source = catalogBoards.find((item) => item.id === id);
    assert.equal(source.spec.storage.psram_mb, size, id);
    assert.equal(source.spec.storage.psram_mode, mode, id);
    assert.equal(source.devices.psram.part, "psram", id);
    for (const output of resolveCatalog(source)) {
      assert.equal(validateBoard(output.board, { ...context, chip: chips[source.chip], resolved: true }).filter((item) => item.id.startsWith("E_PSRAM")).length, 0, id);
    }
  }
  const stack = catalogBoards.find((item) => item.id === "m5stack");
  const variants = Object.fromEntries(resolveCatalog(stack).map((item) => [item.revision, item.board]));
  assert.equal(variants.pre_v2_6_basic.devices.psram, undefined);
  assert.equal(variants.pre_v2_6_gray.devices.psram, undefined);
  assert.equal(variants.pre_2020_04_fire.spec.storage.psram_mb, 4);
  assert.equal(variants.from_2020_04_to_v2_5_fire.spec.storage.psram_mb, 8);
  assert.equal(variants.v2_6_plus_fire.spec.storage.psram_mb, 8);
  for (const id of ["pre_2020_04_fire", "from_2020_04_to_v2_5_fire", "v2_6_plus_fire"]) {
    assert.equal(variants[id].devices.psram.part, "psram");
    assert.ok(variants[id].pins[16].roles.includes("conn:mbus.15"));
    assert.ok(variants[id].pins[16].roles.includes("dev:psram.cs"));
    assert.ok(variants[id].pins[17].roles.includes("conn:mbus.16"));
    assert.ok(variants[id].pins[17].roles.includes("dev:psram.clk"));
  }
});

test("PICO-V3-02 exposes GPIO20 without changing the generated chip target", () => {
  for (const id of ["m5atom_psram", "m5stickcplus2"]) {
    const source = clone(catalogBoards.find((item) => item.id === id));
    source.pins[20] = { roles: [] };
    assert.equal(validateBoard(source, { ...context, chip: chips[source.chip] }).some((item) => item.id === "E_CHIP_ABSENT"), false, id);
    assert.equal(source.chip, "esp32_d0wdq6");
    assert.equal(source.chip_package, "pico_v3_02");
  }
  const invalid = clone(catalogBoards.find((item) => item.id === "m5atom_psram"));
  invalid.chip_package = "unknown_package";
  assert.equal(validateBoard(invalid, { ...context, chip: chips[invalid.chip] }).some((item) => item.id === "E_CHIP_PACKAGE"), true);
});

test("PICO-V3-02 package overrides reserve and validate the integrated PSRAM pins", () => {
  for (const id of ["m5atom_psram", "m5stickcplus2"]) {
    const source = catalogBoards.find((item) => item.id === id);
    const packageChip = effectiveChip(chips[source.chip], source);
    assert.deepEqual(packageChip.psram.quad, { cs: 9, clk: 10 }, id);
    assert.ok(packageChip.reserved.includes(9), id);
    assert.ok(packageChip.reserved.includes(10), id);
    assert.ok(source.pins[9].roles.includes("dev:psram.cs"), id);
    assert.ok(source.pins[10].roles.includes("dev:psram.clk"), id);
    const errors = validateBoard(source, { ...context, chip: chips[source.chip] });
    assert.equal(errors.some((item) => ["E_PSRAM_PINS", "E_CHIP_RESERVED"].includes(item.id)), false, id);

    const externalPins = clone(source);
    externalPins.pins[9].roles = externalPins.pins[9].roles.filter((role) => role !== "dev:psram.cs");
    externalPins.pins[10].roles = externalPins.pins[10].roles.filter((role) => role !== "dev:psram.clk");
    externalPins.pins[16] ??= { roles: [] };
    externalPins.pins[17] ??= { roles: [] };
    externalPins.pins[16].roles.push("dev:psram.cs");
    externalPins.pins[17].roles.push("dev:psram.clk");
    assert.ok(validateBoard(externalPins, { ...context, chip: chips[source.chip] }).some((item) => item.id === "E_PSRAM_PINS"), id);

    const conflict = clone(source);
    conflict.pins[9].roles.push("dev:rgb_led.data");
    assert.ok(validateBoard(conflict, { ...context, chip: chips[source.chip] }).some((item) => item.id === "E_CHIP_RESERVED"), id);
  }
});

test("ESP32-S3 Quad and OPI PSRAM map IO2 to SPIWP and IO3 to SPIHD", () => {
  assert.equal(chipS3.psram.quad.d2, 28);
  assert.equal(chipS3.psram.quad.d3, 27);
  assert.equal(chipS3.psram.opi.d2, 28);
  assert.equal(chipS3.psram.opi.d3, 27);
  const source = catalogBoards.find((item) => item.id === "m5sticks3");
  assert.ok(source.pins[28].roles.includes("dev:psram.d2"));
  assert.ok(source.pins[27].roles.includes("dev:psram.d3"));
  const swapped = clone(source);
  swapped.pins[28].roles = swapped.pins[28].roles.map((role) => role === "dev:psram.d2" ? "dev:psram.d3" : role);
  swapped.pins[27].roles = swapped.pins[27].roles.map((role) => role === "dev:psram.d3" ? "dev:psram.d2" : role);
  assert.ok(validateBoard(swapped, { ...context, chip: chipS3 }).some((item) => item.id === "E_PSRAM_PINS"));
});

test("PSRAM may share connector roles but rejects another device signal", () => {
  const stack = catalogBoards.find((item) => item.id === "m5stack");
  const fire = clone(resolveCatalog(stack).find((item) => item.revision === "pre_2020_04_fire").board);
  assert.equal(validateBoard(fire, { ...context, chip: chips[stack.chip], resolved: true }).some((item) => item.id === "E_PSRAM_CONFLICT"), false);
  fire.pins[16].roles.push("dev:lcd.cs");
  assert.equal(validateBoard(fire, { ...context, chip: chips[stack.chip], resolved: true }).some((item) => item.id === "E_PSRAM_CONFLICT"), true);

  const core2Source = catalogBoards.find((item) => item.id === "m5stack_core2");
  const missingDevice = clone(core2Source);
  delete missingDevice.devices.psram;
  assert.equal(validateBoard(missingDevice, { ...context, chip: chips[core2Source.chip] }).some((item) => item.id === "E_PSRAM_DEVICE"), true);

  const wrongPins = clone(core2Source);
  wrongPins.pins[16].roles = wrongPins.pins[16].roles.filter((role) => role !== "dev:psram.cs");
  wrongPins.pins[25].roles.push("dev:psram.cs");
  assert.equal(validateBoard(wrongPins, { ...context, chip: chips[core2Source.chip] }).some((item) => item.id === "E_PSRAM_PINS"), true);
});

test("generated ESP32 wiring preserves the replaced board values", async () => {
  const source = await fs.readFile(
    path.join(root, "../../src/board_detect/m5/generated/esp32_d0wdq6_wiring.hpp"), "utf8");
  assert.deepEqual(parseGeneratedWiring(source), legacyEsp32Wiring);
});

test("ESP32 detector reset permission is promoted before the final retry", async () => {
  const main = await fs.readFile(path.join(root, "../../src/M5GFX.cpp"), "utf8");
  const loop = /int retry = 4;([\s\S]*?)board = autodetect\(use_reset, board\);/.exec(main)?.[1];
  assert.ok(loop);
  const promotion = loop.indexOf("if (retry == 1) { use_reset = true; }");
  const detected = loop.indexOf("try_setup_detected(esp32_detectors");
  assert.ok(promotion >= 0 && promotion < detected);
  assert.match(loop, /static_cast<board_t>\(nvs_board\), use_reset,/);
  assert.doesNotMatch(loop, /detector_allow_reset/);
});

test("all 62 revision and runtime combinations match snapshots", async () => {
  const outputs = catalogBoards.flatMap(resolveCatalog);
  assert.equal(outputs.length, 62);
  for (const output of outputs) {
    const snapshot = await fs.readFile(path.join(root, "generated/resolved", output.filename), "utf8");
    assert.equal(snapshot, formatBoard(output.board), output.filename);
  }
});

test("standalone editor bundle embeds the catalog and valid scripts", async () => {
  const html = await renderBundle();
  assert.equal(html.includes("<!-- @inline"), false);
  for (const id of ["board-spec-schema", "board-spec-chips", "board-spec-connector-types", "board-spec-parts", "board-spec-accessories", "board-spec-targets", "board-spec-boards"]) {
    const match = new RegExp(`<script type="application/json" id="${id}">(.*?)</script>`, "s").exec(html);
    assert.ok(match, `${id} is embedded`);
    JSON.parse(match[1]);
  }
  const scripts = [...html.matchAll(/<script>([\s\S]*?)<\/script>/g)].map((match) => match[1]);
  assert.equal(scripts.length, 2);
  for (const source of scripts) assert.doesNotThrow(() => new Function(source));
  const embedded = JSON.parse(/<script type="application\/json" id="board-spec-boards">(.*?)<\/script>/s.exec(html)[1]);
  assert.deepEqual(Object.keys(embedded).sort(), catalogBoards.map((item) => item.id).sort());
  assert.deepEqual(schema["x-editor"].pinColumns.map((column) => column.id), [
    "gpio", "roles", "pull", "note", "verified", "validation",
  ]);

  const sandbox = vm.createContext({ console });
  vm.runInContext(scripts[0], sandbox);
  const input = JSON.parse(JSON.stringify(board));
  const validationContext = JSON.parse(JSON.stringify(context));
  const bundled = vm.runInContext("validateBoard(__board, __context)", Object.assign(sandbox, { __board: input, __context: validationContext }));
  assert.deepEqual(JSON.parse(JSON.stringify(bundled)), validateBoard(input, validationContext));
});

test("bundle rejects duplicate flattened top-level declarations", () => {
  assert.throws(() => assertUniqueFlattenedNames([
    ["first.js", "export function collision() {}"],
    ["second.js", "const collision = 1;"],
  ]), /collision.*first\.js.*second\.js/);
});

test("editor translations are complete and every static reference exists", async () => {
  assert.ok(i18nMatch, "embedded i18n dictionary exists");
  for (const [key, translations] of Object.entries(messages)) {
    assert.deepEqual(Object.keys(translations).sort(), ["en", "ja", "zh"], key);
    for (const value of Object.values(translations)) assert.ok(value.length > 0, key);
  }
  assert.deepEqual(messages["gpio.rowHint"], { en: "Click to edit", zh: "点击编辑", ja: "クリックして編集" });
  const referenced = new Set([
    ...[...editorSource.matchAll(/\bt\("([^"]+)"/g)].map((match) => match[1]),
    ...[...editorHtml.matchAll(/data-i18n(?:-aria-label|-placeholder|-title)?="([^"]+)"/g)].map((match) => match[1]),
  ]);
  assert.deepEqual([...referenced].filter((key) => !messages[key]), []);
  assert.equal(/[ぁ-んァ-ン一-龯]/.test(editorSource), false, "editor UI source has no hard-coded Japanese");
  assert.match(editorHtml, /id="onboarding"[^>]*hidden/);
  assert.match(editorSource, /m5-board-spec-onboarding:v1/);
  assert.match(editorHtml, /id="repository-next"[^>]*hidden/);
  assert.match(editorSource, /repository\.replace/);
  assert.match(editorSource, /tabindex="0" role="button" aria-label="GPIO/);
  assert.match(editorSource, /detail\.noRoleMatches/);
  assert.match(editorCSS, /grid-template-rows:\s*0fr/);
  assert.match(editorCSS, /grid-template-rows:\s*1fr/);
  assert.match(editorSource, /function setDisclosureOpen\(details, open\)/);
  assert.match(editorSource, /details\.classList\.add\("motion-opening"\)/);
  assert.match(editorSource, /details\.classList\.add\("motion-closing"\)/);
  assert.match(editorSource, /collapse\.addEventListener\("transitionend", motion\.finish\)/);
  assert.match(editorSource, /setTimeout\(\(\) => motion\.finish\(\), motionDuration \+ 50\)/);
  assert.match(editorCSS, /\.motion-details\[open\]:not\(\.motion-opening, \.motion-closing\)/);
  assert.match(editorCSS, /prefers-reduced-motion:\s*reduce/);
  assert.match(editorSource, /closingGPIOs/);
  assert.match(editorSource, /renderWithFade/);
  assert.match(editorCSS, /\.search-combobox ul \{[^}]*position:\s*fixed/s);
  assert.match(editorSource, /const opensUpwards = spaceBelow < desiredHeight/);
  assert.match(editorSource, /--combobox-max-height/);
  assert.match(editorHtml, /<colgroup>[\s\S]*gpio-col-validation[\s\S]*<\/colgroup>/);
  assert.match(editorCSS, /#gpio-table \{ table-layout: fixed; \}/);
  assert.match(editorHtml, /id="board-info"[^>]*data-layout-section="boardInfo"/);
  assert.match(editorHtml, /id="side-resizer"[^>]*role="separator"[^>]*aria-valuemin="360"[^>]*aria-valuemax="620"/);
  assert.match(editorSource, /m5-board-spec-layout:v1/);
  assert.match(editorSource, /sidebarWidthLimits = \{ min: 360, max: 620, default: 460 \}/);
  assert.match(editorSource, /event\.key === "ArrowLeft" \? step : -step/);
  assert.match(editorCSS, /\.side-column \{[^}]*position:\s*sticky[^}]*max-height:\s*100%[^}]*overflow-y:\s*auto/s);
  assert.match(editorCSS, /@media \(max-width: 980px\)[\s\S]*?\.side-resizer \{ display: none; \}/);
  assert.match(editorCSS, /\.workspace \{[^}]*--side-width, 460px/s);
  assert.match(editorCSS, /\.row-edit-hint \{[^}]*opacity:\s*0[^}]*visibility:\s*hidden/s);
  assert.doesNotMatch(editorCSS, /\.gpio-row:hover \.row-edit-hint \{\s*display:/);
  assert.match(editorSource, /internalRoles: `<td class="gpio-roles-cell gpio-internal-roles-cell"><div class="role-row-main">[\s\S]*\$\{rowHint\}/);
  assert.match(editorSource, /const compactInternalRoles = internalRoles\.map/);
  assert.doesNotMatch(editorSource, /allRoles\.slice\(0, 3\)/);
  assert.match(editorCSS, /\.compact-roles \{[^}]*flex-wrap:\s*wrap[^}]*overflow:\s*visible/s);
  assert.match(editorCSS, /\.gpio-roles-cell:has\(\.row-readonly-origin, \.psram-shared-note\) \.row-edit-hint/);
  assert.match(editorSource, /assignConnectorLanes\(board\.pins\)/);
  assert.match(editorSource, /sizeConnectorLanes\(connectorLanes\.lanes\.length\)/);
  assert.match(editorCSS, /\.connector-role-lanes \{[^}]*grid-template-columns:\s*var\(--connector-lane-columns\)/s);
  assert.match(editorSource, /connector\.pinoutHeading/);
  assert.match(editorSource, /validation: `<td>\$\{errorSummary\}<\/td>`/);
  assert.doesNotMatch(editorSource, /row-disclosure/);
  assert.match(editorSource, /\$\{expanded \? "▾" : "▸"\}/);
  assert.match(editorSource, /function roleOwnerAttributes\(owner, connectorPosition = false\)/);
  assert.match(editorSource, /class="related-object-link" \$\{roleOwnerAttributes\(owner\)\}/);
  assert.match(editorSource, /roleOwnerLabel\(objectRoleOwner\("bus", id\)/);
  assert.match(editorSource, /roleOwnerLabel\(objectRoleOwner\("conn", id\)/);
  assert.match(editorSource, /roleOwnerLabel\(objectRoleOwner\("dev", id\)/);
  assert.match(editorCSS, /\.related-object-link\[data-role-kind="conn"\]/);
  assert.match(editorCSS, /\.config-object summary \.configuration-owner-label \{[^}]*justify-self:\s*start[^}]*inline-size:\s*max-content/s);
  assert.match(editorSource, /class="reference-mark"[^>]*>🔗<\/span>/);
  assert.doesNotMatch(editorSource, /class="reference-mark"[^>]*>●<\/span>/);

  const validatorSources = await Promise.all(["validate.js", "ctypes.js", "compose.js"].map((name) => fs.readFile(path.join(root, "lib", name), "utf8")));
  const validationIds = new Set(validatorSources.flatMap((source) => [...source.matchAll(/(?:error|issue)\("([EW]_[A-Z0-9_]+)"/g)].map((match) => match[1])));
  assert.deepEqual([...validationIds].filter((id) => !messages[`validation.${id}`]), []);
});

test("editor read-only controls use the shared reason path", () => {
  assert.match(editorHtml, /id="edit-base-button"/);
  assert.match(editorSource, /function readonlyAttributes\(reason\)/);
  assert.match(editorSource, /data-readonly-reason/);
  assert.doesNotMatch(editorHtml, /\sdisabled(?:\s|=|>)/);
  assert.doesNotMatch(editorSource, /\sdisabled(?:\s|=|>)/);
});

test("editor validates every resolved variant and confirms invalid downloads", () => {
  assert.match(editorSource, /\.\.\.validateResolvedVariants\(board, context\)/);
  assert.match(editorSource, /validationFor\(entry\.base\).*filter\(\(item\) => item\.severity !== "warning"\)/s);
  assert.match(editorSource, /window\.confirm\(t\("file\.downloadErrorsConfirm"/);
});

test("editor classifies revision pin changes separately from later layers", () => {
  const base = { roles: ["dev:lcd.cs"], pull: "up" };
  const revision = { roles: ["dev:lcd.cs"], pull: "down" };
  assert.equal(pinChangeLayer({ base, defaults: base, revision, runtime: revision, shown: revision }), "revision");
  assert.equal(pinChangeLayer({ base, defaults: base, revision: base, runtime: base, shown: base }), null);
});

test("device signals are derived only in resolved output", () => {
  assert.equal(board.devices.lcd.signals, undefined);
  const resolved = resolveCatalog(board)[0].board;
  assert.equal(resolved.devices.lcd.signals.rst, "pin:pmic.gpio3");
  assert.equal(resolved.devices.speaker.enable, "pin:pmic.gpio2");
});

test("shared SD SPI modes and pin-table values stay stable", () => {
  const expected = {
    m5cardputer: { sd_mmc_clk: 40, sd_mmc_cmd: 14, sd_mmc_d0: 39, sd_mmc_d3: 12 },
    m5cardputer_adv: { sd_mmc_clk: 40, sd_mmc_cmd: 14, sd_mmc_d0: 39, sd_mmc_d3: 12 },
    m5paper: { sd_mmc_clk: 14, sd_mmc_cmd: 12, sd_mmc_d0: 13, sd_mmc_d3: 4 },
    m5papercolor: { sd_mmc_clk: 15, sd_mmc_cmd: 13, sd_mmc_d0: 14, sd_mmc_d3: 47 },
    m5paperdiy: { sd_mmc_clk: 39, sd_mmc_cmd: 38, sd_mmc_d0: 40, sd_mmc_d3: 47 },
    m5papers3: { sd_mmc_clk: 39, sd_mmc_cmd: 38, sd_mmc_d0: 40, sd_mmc_d3: 47 },
    m5papermono: { sd_mmc_clk: 13, sd_mmc_cmd: 12, sd_mmc_d0: 11, sd_mmc_d1: 10, sd_mmc_d2: 9, sd_mmc_d3: 8 },
    m5stack: { sd_mmc_clk: 18, sd_mmc_cmd: 23, sd_mmc_d0: 19, sd_mmc_d3: 4 },
    m5stack_core2: { sd_mmc_clk: 18, sd_mmc_cmd: 23, sd_mmc_d0: 38, sd_mmc_d3: 4 },
    m5stack_corematrix: { sd_mmc_clk: 25, sd_mmc_cmd: 27, sd_mmc_d0: 26, sd_mmc_d3: 28 },
    m5stack_corep4x: { sd_mmc_clk: 10, sd_mmc_cmd: 7, sd_mmc_d0: 8, sd_mmc_d3: 50 },
    m5stack_cores3: { sd_mmc_clk: 36, sd_mmc_cmd: 37, sd_mmc_d0: 35, sd_mmc_d3: 4 },
    m5stack_cores3se: { sd_mmc_clk: 36, sd_mmc_cmd: 37, sd_mmc_d0: 35, sd_mmc_d3: 4 },
    m5stack_stackchan: { sd_mmc_clk: 36, sd_mmc_cmd: 37, sd_mmc_d0: 35, sd_mmc_d3: 4 },
    m5stack_tab5: { sd_mmc_clk: 43, sd_mmc_cmd: 44, sd_mmc_d0: 39, sd_mmc_d1: 40, sd_mmc_d2: 41, sd_mmc_d3: 42 },
    m5stack_tab5x: { sd_mmc_clk: 43, sd_mmc_cmd: 44, sd_mmc_d0: 39, sd_mmc_d1: 40, sd_mmc_d2: 41, sd_mmc_d3: 42 },
    m5stamplc: { sd_mmc_clk: 7, sd_mmc_cmd: 8, sd_mmc_d0: 9, sd_mmc_d3: 10 },
    m5tough: { sd_mmc_clk: 18, sd_mmc_cmd: 23, sd_mmc_d0: 38, sd_mmc_d3: 4 },
    m5toughc5: { sd_mmc_clk: 9, sd_mmc_cmd: 7, sd_mmc_d0: 8, sd_mmc_d3: 10 },
  };
  for (const source of catalogBoards.filter((item) => item.devices.sd)) {
    const actual = Object.fromEntries(Object.entries(pintableAssignments(source)).filter(([name]) => name.startsWith("sd_mmc_")));
    assert.deepEqual(actual, expected[source.id], source.id);
    for (const output of resolveCatalog(source)) {
      const derived = source.id === "m5stamplc"
        ? { modes: ["spi", "sdio1"], bus: "main_spi", cs: "gpio:10" }
        : source.buses[source.devices.sd.bus].kind === "sdmmc"
        ? { modes: ["m5papermono", "m5stack_tab5", "m5stack_tab5x"].includes(source.id) ? ["sdio1", "sdio4"] : ["sdio1"] }
        : { modes: ["m5papercolor", "m5papers3", "m5paperdiy", "m5stack_corep4x", "m5stack_cores3", "m5stack_cores3se", "m5stack_stackchan"].includes(source.id) ? ["spi", "sdio1"] : ["spi"],
            bus: source.devices.sd.bus,
            cs: ["m5papercolor", "m5papers3", "m5paperdiy"].includes(source.id) ? "gpio:47"
              : source.id === "m5toughc5" ? "gpio:10"
              : source.id === "m5stack_corep4x" ? "gpio:50" : "gpio:4" };
      assert.deepEqual(output.board.devices.sd.derived, derived, output.filename);
    }
  }
});

test("M5Unified emitter owns the complete pin_name_t role mapping", () => {
  const resolved = resolveCatalog(board)[0].board;
  assert.equal(PIN_NAMES.length, 50);
  assert.deepEqual(PIN_NAMES.slice(0, 4), ["in_i2c_scl", "in_i2c_sda", "port_a_pin1", "port_a_pin2"]);
  assert.equal(PIN_NAMES.at(-1), "mbus_pin30");
  assert.equal(pinNameForRole(resolved, "bus:internal_i2c.scl"), "in_i2c_scl");
  assert.equal(pinNameForRole(resolved, "m5go_bottom2/conn:port_b.1"), "port_b_pin1");
  assert.equal(pinNameForRole(resolved, "conn:mbus.24"), "mbus_pin24");
  assert.equal(pinNameForRole(resolved, "dev:sd.d3"), "sd_mmc_d3");
  assert.equal(consumesPinTableRole(resolved, "dev:lcd.cs"), false);

  const emitted = emitPinTable(resolved, {});
  assert.equal(emitted.values.in_i2c_scl, 22);
  assert.equal(emitted.values.port_e_pin2, 19);
  assert.equal(emitted.values.sd_mmc_d1, 255);
  assert.equal(emitted.values.mbus_pin30, 255);
});

test("M5Unified emitter applies documented overrides last", () => {
  const resolved = resolveCatalog(board)[0].board;
  const emitted = emitPinTable(resolved, { overrides: {
    m5stack_core2: { power_hold: { value: 7, reason: "program contract" } },
  } });
  assert.equal(emitted.values.power_hold, 7);
  assert.equal(emitted.overrides.power_hold.reason, "program contract");
  assert.throws(() => emitPinTable(resolved, { overrides: {
    m5stack_core2: { missing: { value: 7, reason: "invalid name" } },
  } }), /unknown pin-table override/);
});

test("M5Unified emitter rejects duplicate logical pin assignments", () => {
  const resolved = resolveCatalog(board)[0].board;
  resolved.pins[1].roles.push("bus:internal_i2c.scl");
  assert.throws(() => pinTableAssignments(resolved), /in_i2c_scl is assigned to both GPIO/);
});

test("AtomS3 pin table matches the ESP32-S3 unknown fallback", () => {
  const source = catalogBoards.find((item) => item.id === "m5atoms3");
  const emitted = emitPinTable(resolveCatalog(source)[0].board, compositionTarget).values;
  assert.deepEqual(Object.fromEntries(Object.entries(emitted).filter(([, value]) => value !== 255)), {
    in_i2c_scl: 39,
    in_i2c_sda: 38,
    port_a_pin1: 1,
    port_a_pin2: 2,
  });
});

test("AtomS3 runtime panels resolve to both probed display parts", () => {
  const source = catalogBoards.find((item) => item.id === "m5atoms3");
  const outputs = resolveCatalog(source);
  assert.deepEqual(outputs.map((output) => output.filename), [
    "m5atoms3+lcd=st7735s.json",
    "m5atoms3+lcd=gc9107.json",
  ]);
  assert.deepEqual(outputs.map((output) => output.board.devices.lcd.part), ["st7735s", "gc9107"]);
  assert.deepEqual(outputs.map((output) => output.board.devices.lcd.spec), [
    { width: 128, height: 128, memory_height: 132, offset_x: 2, offset_y: 1, rotation_offset: 2, invert: true, readable: true, panel_type: "st7735s" },
    { width: 128, height: 128, offset_x: 0, offset_y: 32, rotation_offset: 0, invert: false, readable: false, panel_type: "gc9107" },
  ]);
  assert.deepEqual(source.devices.backlight.spec, { freq: 256, channel: 7, invert: false, offset: 48 });
  assert.deepEqual(source.spec.storage, { psram_mb: 0 });
  assert.equal(source.spec.storage.psram_mode, undefined);
  assert.deepEqual(parts.st7735s.id_probe.values, ["0x7683", "0x897C"]);
  assert.equal(parts.gc9107.id_probe.value, "0x079100");
  assert.equal(parts.st7735s.spec_keys.memory_width.default, 132);
  assert.equal(parts.st7735s.spec_keys.memory_height.default, 162);
  assert.equal(parts.gc9107.spec_keys.memory_width.default, 128);
  assert.equal(parts.gc9107.spec_keys.memory_height.default, 160);
});

test("M5Unified live pin tables match generated values", async (t) => {
  const compiler = process.env.CXX || "c++";
  const probe = spawnSync(compiler, ["--version"], { encoding: "utf8" });
  if (probe.error?.code === "ENOENT") return t.skip(`C++ compiler not found (${compiler})`);
  const m5unified = process.env.M5UNIFIED_PATH || path.resolve(root, "../../../M5Unified");
  try { await fs.access(path.join(m5unified, "src/M5Unified.inl")); }
  catch { return t.skip(`M5Unified checkout not found (${m5unified})`); }
  const compared = spawnSync(process.execPath, ["cli/spec.js", "compare-pintable", "--m5unified", m5unified], { cwd: root, encoding: "utf8" });
  assert.equal(compared.status, 0, compared.stderr || compared.stdout);
  assert.match(compared.stdout, /comparison passed: 34 board\(s\), 1700 value\(s\), 4 target\(s\)/);
});

test("M5GFX wiring emitter maps every board-description GPIO", () => {
  const expected = {
    m5station: { display: [18, 23, -1, 19, 5, 15, -1], sd: null, i2c: [21, 22, 1], reset: 15, power: -1, hold: [5] },
    m5stack_core2: { display: [18, 23, 38, 15, 5, -1, -1], sd: [18, 23, 38, 4, 5], i2c: [21, 22, 1], reset: -1, power: -1, hold: [4, 5] },
    m5tough: { display: [18, 23, 38, 15, 5, -1, -1], sd: [18, 23, 38, 4, 5], i2c: [21, 22, 1], reset: -1, power: -1, hold: [4, 5] },
    m5stack: { display: [18, 23, 19, 27, 14, 33, -1], sd: [18, 23, 19, 4, 14], i2c: [21, 22, 0], reset: 33, power: -1, hold: [4, 14] },
    m5paper: { display: [14, 12, 13, -1, 15, 23, 27], sd: [14, 12, 13, 4, 15], i2c: [21, 22, 1], reset: 23, power: 2, hold: [4, 15] },
    m5stickc: { display: [13, 15, 14, 23, 5, 18, -1], sd: null, i2c: [21, 22, 1], reset: 18, power: -1, hold: [5] },
    m5stickcplus: { display: [13, 15, 14, 23, 5, 18, -1], sd: null, i2c: [21, 22, 1], reset: 18, power: -1, hold: [5] },
    m5stack_coreink: { display: [18, 23, 34, 15, 9, 0, 4], sd: null, i2c: [21, 22, 1], reset: 0, power: 12, hold: [9] },
    m5stickcplus2: { display: [13, 15, -1, 14, 5, 12, -1], sd: null, i2c: [21, 22, 1], reset: 12, power: 4, hold: [5] },
    m5atoms3: { display: [17, 21, -1, 33, 15, 34, -1], sd: null, i2c: [38, 39, 1], reset: 34, power: -1, hold: [15] },
    m5atoms3r: { display: [15, 21, -1, 42, 14, 48, -1], sd: null, i2c: [45, 0, 1], reset: 48, power: -1, hold: [14] },
    m5dial: { display: [6, 5, -1, 4, 7, 8, -1], sd: null, i2c: [11, 12, 1], reset: 8, power: 46, hold: [7] },
    m5dinmeter: { display: [6, 5, -1, 4, 7, 8, -1], sd: null, i2c: [11, 12, 1], reset: 8, power: 46, hold: [7] },
    m5stamplc: { display: [7, 8, 9, 6, 12, 3, -1], sd: [7, 8, 9, 10, 12], i2c: [13, 15, 1], reset: 3, power: -1, hold: [10, 12] },
    m5sticks3: { display: [40, 39, -1, 45, 41, 21, -1], sd: null, i2c: [47, 48, 1], reset: 21, power: -1, hold: [41] },
    m5stopwatch: { display: [40, -1, -1, 41, 42, 46, 45, -1, 39, -1, -1], sd: null, i2c: [47, 48, 1], reset: -1, power: -1, hold: [39] },
    m5papermono: { display: [15, 14, -1, 17, 16, -1, 18], sd: null, i2c: [47, 48, 1], reset: -1, power: -1, hold: [16] },
    m5toughc5: { display: [9, 7, 8, 26, 25, -1, -1], sd: [9, 7, 8, 10, 25], i2c: [2, 3, 1], reset: -1, power: -1, hold: [10, 25] },
    m5stack_corematrix: { display: [-1, -1, -1, -1, -1, -1, -1], sd: null, i2c: [0, 1, 0], reset: -1, power: -1, hold: [] },
  };
  const entries = [];
  for (const source of catalogBoards.filter((item) => expected[item.id])) {
    const resolved = resolveAll(source, connectorTypes, { chip: chips[source.chip], parts })[0].board;
    const emitted = emitM5GFXWiring(resolved, parts, target);
    const flattened = {
      display: Object.values(emitted.display),
      sd: emitted.sharedSd && Object.values(emitted.sharedSd),
      i2c: emitted.i2c && Object.values(emitted.i2c),
      reset: emitted.resetGpio,
      power: emitted.powerGpio,
      hold: emitted.hold,
    };
    assert.deepEqual(flattened, expected[source.id], source.id);
    entries.push({ board: source, chip: chips[source.chip], emitted });
  }
  const header = renderM5GFXWiringHeader(entries.filter(({ board }) => board.chip === "esp32_d0wdq6"));
  assert.match(header, /constexpr std::int8_t display_sclk = 18;/);
  assert.match(header, /namespace detection \{\s+constexpr std::int8_t unconditional_pins\[] = \{ 0, 2, 4, 5, 9, 12, 13, 14, 15, 18, 19, 21, 22, 23, 27, 33, 34, 38 \};/);
  assert.deepEqual(wiringFieldsForRole(board, "bus:main_spi.sclk", parts), ["display_sclk", "shared_sd_sclk"]);
  assert.deepEqual(wiringFieldsForRole(board, "dev:lcd.rst", parts), ["display_rst"]);
  assert.deepEqual(wiringFieldsForRole(catalogBoards.find((item) => item.id === "m5dial"), "dev:touch.int", parts), ["touch_int"]);
});

test("M5GFX target metadata matches descriptor internal-I2C declarations", () => {
  const sources = { esp32_d0wdq6: d0wdq6Source, esp32s3: esp32s3Source, esp32c5: esp32c5Source, esp32c6: esp32c6Source, esp32c61: esp32c61Source, esp32p4: esp32p4Source };
  for (const [boardId, mapping] of Object.entries(target.boards)) {
    if (!mapping.wiring_output) continue;
    const match = new RegExp(`static constexpr board_desc_t ${mapping.desc_name} = \\{([\\s\\S]*?)\\n\\s*\\};`).exec(sources[mapping.chip]);
    assert.ok(match, `${boardId} descriptor exists`);
    const usesInternalI2c = /\n\s+internal_i2c\(|\.internal_i2c/.test(match[1]);
    assert.equal(mapping.desc_internal_i2c ?? false, usesInternalI2c, `${boardId} internal I2C metadata`);
  }
});

test("AtomS3 exposes the display wiring expected by the future reset declaration", () => {
  const source = catalogBoards.find((item) => item.id === "m5atoms3");
  const resolved = resolveCatalog(source)[0].board;
  const emitted = emitM5GFXWiringForMapping(resolved, parts, {
    boardId: "m5atoms3", cppNamespace: "atoms3", boardEnum: "board_M5AtomS3", descName: "desc_atoms3",
    fields: ["display", "hold", "backlight"], reset: "display_rst",
  });
  assert.deepEqual(emitted.display, { sclk: 17, mosi: 21, miso: -1, dc: 33, cs: 15, rst: 34, busy: -1 });
  assert.equal(emitted.resetGpio, 34);
  assert.equal(emitted.backlightGpio, 16);
  assert.deepEqual(emitted.hold, [15]);
});

test("AtomS3 generated panel construction matches every legacy field", async () => {
  const source = catalogBoards.find((item) => item.id === "m5atoms3");
  const variants = resolveCatalog(source).map((item) => item.board);
  const emitted = emitM5GFXSpecs(source, variants, parts, m5gfxBoardMapping(target, source.id));
  assert.deepEqual(emitted.bus, { host: 2, hostSymbol: "SPI3_HOST", freqWrite: 40000000, freqRead: 16000000, threeWire: true });
  assert.deepEqual(emitted.panels.st7735s, { width: 128, height: 128, memoryWidth: 132, memoryHeight: 132, offsetX: 2, offsetY: 1, rotationOffset: 2, invert: true, readable: true });
  assert.deepEqual(emitted.panels.gc9107, { width: 128, height: 128, memoryWidth: 128, memoryHeight: 160, offsetX: 0, offsetY: 32, rotationOffset: 0, invert: false, readable: false });
  assert.deepEqual(emitted.probes.st7735s, { cmd: 4, mask: 0xFFFF, values: [0x7683, 0x897C] });
  assert.deepEqual(emitted.probes.gc9107, { cmd: 4, mask: 0xFFFFFF, values: [0x079100] });
  assert.match(renderM5GFXSpecsHeader(emitted), /constexpr int bus_host = SPI3_HOST;/);
  const setup = await fs.readFile(esp32s3SetupPath, "utf8");
  assert.match(setup, /panel_atoms3_st7735s[^;]*\.with_bus_shared\(false\);/);
  assert.match(setup, /panel_atoms3_gc9107[^;]*\.with_bus_shared\(false\);/);
});

test("AtomS3R generated specs preserve the legacy setup", async () => {
  const source = catalogBoards.find((item) => item.id === "m5atoms3r");
  const variants = resolveCatalog(source).map((item) => item.board);
  const emitted = emitM5GFXSpecs(source, variants, parts, m5gfxBoardMapping(target, source.id));
  const wiring = emitM5GFXWiring(variants[0], parts, target);
  const pinTable = emitPinTable(variants[0], {});
  assert.deepEqual(emitted.bus, { host: 2, hostSymbol: "SPI3_HOST", freqWrite: 40000000, freqRead: 16000000, threeWire: true });
  assert.deepEqual(emitted.panels.st7735s, { width: 128, height: 128, memoryWidth: 132, memoryHeight: 132, offsetX: 2, offsetY: 1, rotationOffset: 2, invert: true, readable: true });
  assert.deepEqual(emitted.panels.gc9107, { width: 128, height: 128, memoryWidth: 128, memoryHeight: 160, offsetX: 0, offsetY: 32, rotationOffset: 0, invert: false, readable: false });
  assert.deepEqual(emitted.probes.st7735s, { cmd: 4, mask: 0xFFFF, values: [0x7683, 0x897C] });
  assert.deepEqual(emitted.probes.gc9107, { cmd: 4, mask: 0xFFFFFF, values: [0x079100] });
  assert.equal(emitted.backlight, null);
  assert.deepEqual(emitted.backlightI2c, { i2cAddr: 0x30 });
  assert.deepEqual(wiring.display, { sclk: 15, mosi: 21, miso: -1, dc: 42, cs: 14, rst: 48, busy: -1 });
  assert.deepEqual(wiring.i2c, { sda: 45, scl: 0, port: 1 });
  assert.deepEqual(wiring.hold, [14]);
  assert.equal(wiring.backlightGpio, -1);
  assert.equal(source.spec.storage.psram_mode, "opi");
  assert.equal(pinTable.values.in_i2c_sda, 45);
  assert.equal(pinTable.values.in_i2c_scl, 0);
  assert.equal(pinTable.values.port_a_pin1, 1);
  assert.equal(pinTable.values.port_a_pin2, 2);

  const setup = await fs.readFile(esp32s3SetupPath, "utf8");
  assert.match(setup, /bus_atoms3r[\s\S]*?\.with_dma_channel\(SPI_DMA_CH_AUTO\);/);
  assert.match(setup, /panel_atoms3r_st7735s[^;]*\.with_bus_shared\(false\);/);
  assert.match(setup, /panel_atoms3r_gc9107[^;]*\.with_bus_shared\(false\);/);
  assert.match(setup, /construct_atoms3r[\s\S]*?make_default_part<Light_M5StackAtomS3R>/);

  const detector = await fs.readFile(esp32s3DetectorPath, "utf8");
  assert.match(detector, /atoms3r_member_descs[\s\S]*?wiring::atoms3r::touches_opi_pins, 5/);
  assert.match(detector, /spi_id_member_descs[\s\S]*?wiring::atoms3::touches_opi_pins, 0[\s\S]*?wiring::dinmeter::touches_opi_pins, 0/);
  assert.match(detector, /dial_member_descs[\s\S]*?wiring::dial::touches_opi_pins, 0/);

  const wiringHeader = await fs.readFile(path.join(root, "../../src/board_detect/m5/generated/esp32s3_wiring.hpp"), "utf8");
  assert.match(wiringHeader, /namespace atoms3r \{[\s\S]*?touches_opi_pins = false/);
  assert.match(wiringHeader, /unconditional_pins\[\] = \{[^}]*14, 15,[^}]*21,[^}]*42, 43, 44, 45,[^}]*48/);
  assert.doesNotMatch(wiringHeader, /unconditional_pins\[\] = \{[^}]*\b0\b[^}]*\}/);
});

test("StickS3 generated display, backlight, and PMIC specs match the legacy setup", () => {
  const source = catalogBoards.find((item) => item.id === "m5sticks3");
  const variants = resolveCatalog(source).map((item) => item.board);
  const emitted = emitM5GFXSpecs(source, variants, parts, m5gfxBoardMapping(target, source.id));
  assert.deepEqual(emitted.bus, { host: 2, hostSymbol: "SPI3_HOST", freqWrite: 40000000, freqRead: 16000000, threeWire: true });
  assert.deepEqual(emitted.panels.st7789v2, { width: 135, height: 240, memoryWidth: null, memoryHeight: null, offsetX: 52, offsetY: 40, rotationOffset: 0, invert: true, readable: true });
  assert.deepEqual(emitted.backlight, { freq: 256, channel: 7, invert: false, offset: 16 });
  assert.deepEqual(emitted.pmic, { i2cAddr: 0x6E, i2cFreq: 100000, idReg: 0x00 });
  const header = renderM5GFXSpecsHeader(emitted);
  assert.match(header, /namespace pmic \{[\s\S]*i2c_addr = 0x6E;[\s\S]*i2c_freq = 100000;[\s\S]*id_reg = 0x0;/);
  assert.doesNotMatch(header, /namespace pmic \{[\s\S]*id_value/);
});

test("ChainCaptain and PaperColor generated setup preserves legacy fields", async () => {
  const chain = catalogBoards.find((item) => item.id === "m5chaincaptain");
  const paper = catalogBoards.find((item) => item.id === "m5papercolor");
  const chainResolved = resolveCatalog(chain)[0].board;
  const paperResolved = resolveCatalog(paper)[0].board;
  const chainSpecs = emitM5GFXSpecs(chain, [chainResolved], parts,
    m5gfxBoardMapping(target, chain.id));
  const paperSpecs = emitM5GFXSpecs(paper, [paperResolved], parts,
    m5gfxBoardMapping(target, paper.id));
  const chainWiring = emitM5GFXWiring(chainResolved, parts, target);
  const paperWiring = emitM5GFXWiring(paperResolved, parts, target);
  assert.deepEqual(chainSpecs.bus, { host: 1, hostSymbol: "SPI2_HOST",
    freqWrite: 40000000, freqRead: 16000000, threeWire: true });
  assert.deepEqual(chainSpecs.panels.jd9853, { width: 240, height: 240,
    memoryWidth: 240, memoryHeight: 320, offsetX: 0, offsetY: 0,
    rotationOffset: 2, invert: false, readable: false, rgbOrder: true });
  assert.deepEqual(chainWiring.display,
    { sclk: 15, mosi: 16, miso: -1, dc: 46, cs: 45, rst: -1, busy: -1 });
  assert.deepEqual(paperSpecs.bus, { host: 1, hostSymbol: "SPI2_HOST",
    freqWrite: 4000000, freqRead: 1000000, threeWire: true });
  assert.deepEqual(paperSpecs.panels.ed2208, { width: 400, height: 600,
    memoryWidth: null, memoryHeight: null, offsetX: 0, offsetY: 0,
    rotationOffset: 0, invert: false, readable: false });
  assert.deepEqual(paperWiring.display,
    { sclk: 15, mosi: 13, miso: 14, dc: 43, cs: 44, rst: 12, busy: 11 });
  assert.deepEqual(paperWiring.sharedSd,
    { sclk: 15, mosi: 13, miso: 14, sd_cs: 47, other_cs: 44 });

  const detector = await fs.readFile(esp32s3DetectorPath, "utf8");
  assert.match(detector, /chain_addresses\[\] = \{ 0x32, 0x4F, 0x68, 0x6E \}/);
  assert.match(detector, /paper_addresses\[\] = \{ 0x32, 0x44 \}/);
  assert.match(detector, /desc_chaincaptain[\s\S]*?i2c_reset\(10, 20, reset_hold_when_skipped\)/);
  assert.match(detector, /desc_papercolor[\s\S]*?shared_sd\(wiring::papercolor::shared_sd_sclk/);
  const setup = await fs.readFile(esp32s3SetupPath, "utf8");
  assert.match(setup, /panel_chaincaptain[\s\S]*?with_rgb_order[\s\S]*?with_bus_shared\(false\)/);
  assert.match(setup, /panel_papercolor[\s\S]*?display_rst[\s\S]*?with_bus_shared\(true\)/);
});

test("Dial and DinMeter generated specs preserve the legacy setup", async () => {
  const dial = catalogBoards.find((item) => item.id === "m5dial");
  const dialSpecs = emitM5GFXSpecs(dial, resolveCatalog(dial).map((item) => item.board), parts, m5gfxBoardMapping(target, dial.id));
  assert.deepEqual(dialSpecs.bus, { host: 1, hostSymbol: "SPI2_HOST", freqWrite: 80000000, freqRead: 16000000, threeWire: true });
  assert.deepEqual(dialSpecs.panels.gc9a01, { width: 240, height: 240, memoryWidth: null, memoryHeight: null, offsetX: null, offsetY: null, rotationOffset: null, invert: true, readable: false });
  assert.deepEqual(dialSpecs.probes.gc9a01, { cmd: 4, mask: 0xFFFFFF, values: [0x019A00] });
  assert.deepEqual(dialSpecs.touch, { i2cAddr: 0x38, i2cFreq: 400000, xMin: 0, xMax: 239, yMin: 0, yMax: 239, rotationOffset: 0 });

  const dinmeter = catalogBoards.find((item) => item.id === "m5dinmeter");
  const dinmeterSpecs = emitM5GFXSpecs(dinmeter, resolveCatalog(dinmeter).map((item) => item.board), parts, m5gfxBoardMapping(target, dinmeter.id));
  assert.deepEqual(dinmeterSpecs.bus, { host: 1, hostSymbol: "SPI2_HOST", freqWrite: 40000000, freqRead: 16000000, threeWire: true });
  assert.deepEqual(dinmeterSpecs.panels.st7789v2, { width: 135, height: 240, memoryWidth: null, memoryHeight: null, offsetX: 52, offsetY: 40, rotationOffset: 3, invert: true, readable: true });
  assert.deepEqual(dinmeterSpecs.probes.st7789v2, { cmd: 4, mask: 0xFB, values: [0x81] });
  assert.equal(dinmeterSpecs.touch, null);

  const setup = await fs.readFile(esp32s3SetupPath, "utf8");
  assert.match(setup, /bus_dinmeter[\s\S]*?\.with_dma_channel\(SPI_DMA_CH_AUTO\);/);
  assert.match(setup, /bus_dial[\s\S]*?\.with_dma_channel\(SPI_DMA_CH_AUTO\);/);
  assert.doesNotMatch(setup, /panel_dinmeter[^;]*\.with_bus_shared\(/);
  assert.doesNotMatch(setup, /panel_dial[^;]*\.with_bus_shared\(/);
  assert.match(setup, /construct_dinmeter[\s\S]*?make_panel<Panel_ST7789>\(panel_dinmeter/);
  assert.match(setup, /construct_dial[\s\S]*?make_panel<Panel_GC9A01>\(panel_dial/);
  assert.match(setup, /construct_dial[\s\S]*?make_i2c_touch<lgfx::Touch_FT5x06>\(touch_dial/);
});

test("StamPLC generated specs preserve the legacy setup", async () => {
  const source = catalogBoards.find((item) => item.id === "m5stamplc");
  const resolved = resolveCatalog(source)[0].board;
  const emitted = emitM5GFXSpecs(source, [resolved], parts, m5gfxBoardMapping(target, source.id));
  const wiring = emitM5GFXWiring(resolved, parts, target);
  const pinTable = emitPinTable(resolved, {});
  assert.deepEqual(emitted.bus, { host: 1, hostSymbol: "SPI2_HOST", freqWrite: 40000000, freqRead: 16000000, threeWire: true });
  assert.deepEqual(emitted.panels.st7789v2, { width: 135, height: 240, memoryWidth: null, memoryHeight: null, offsetX: 52, offsetY: 40, rotationOffset: 1, invert: true, readable: true });
  assert.deepEqual(emitted.probes.st7789v2, { cmd: 4, mask: 0xFB, values: [0x81] });
  assert.equal(emitted.backlight, null);
  assert.deepEqual(emitted.backlightI2c, { i2cAddr: 0x43 });
  assert.deepEqual(wiring.display, { sclk: 7, mosi: 8, miso: 9, dc: 6, cs: 12, rst: 3, busy: -1 });
  assert.deepEqual(wiring.sharedSd, { sclk: 7, mosi: 8, miso: 9, sd_cs: 10, other_cs: 12 });
  assert.deepEqual(wiring.i2c, { sda: 13, scl: 15, port: 1 });
  assert.deepEqual(wiring.hold, [10, 12]);
  assert.equal(wiring.mapping.descInternalI2c, false);
  assert.deepEqual(resolved.devices.sd.derived.modes, ["spi", "sdio1"]);
  const expectedPins = {
    in_i2c_scl: 15, in_i2c_sda: 13, port_a_pin1: 1, port_a_pin2: 2,
    sd_mmc_clk: 7, sd_mmc_cmd: 8, sd_mmc_d0: 9, sd_mmc_d3: 10, rgb_led: 21,
  };
  for (const [name, value] of Object.entries(expectedPins)) assert.equal(pinTable.values[name], value, name);

  const detector = await fs.readFile(esp32s3DetectorPath, "utf8");
  assert.match(detector, /stamplc_member_descs[\s\S]*?specs::stamplc::bus_three_wire,[\s\S]*?wiring::stamplc::touches_opi_pins, 0/);
  const setup = await fs.readFile(esp32s3SetupPath, "utf8");
  assert.match(setup, /bus_stamplc[\s\S]*?\.with_dma_channel\(SPI_DMA_CH_AUTO\);/);
  assert.match(setup, /panel_stamplc[\s\S]*?\.with_bus_shared\(true\);/);
  assert.match(setup, /construct_stamplc[\s\S]*?make_default_part<Light_M5StackStamPLC>/);

  const wiringHeader = await fs.readFile(path.join(root, "../../src/board_detect/m5/generated/esp32s3_wiring.hpp"), "utf8");
  assert.match(wiringHeader, /namespace stamplc \{[\s\S]*?shared_sd_miso = 9;[\s\S]*?shared_sd_sd_cs = 10;/);
  assert.match(wiringHeader, /unconditional_pins\[\] = \{[^}]*\b9, 10,[^}]*\}/);
});

test("AirQ generated specs preserve the legacy setup", async () => {
  const source = catalogBoards.find((item) => item.id === "m5airq");
  const variants = resolveCatalog(source).map((item) => item.board);
  const emitted = emitM5GFXSpecs(source, variants, parts, m5gfxBoardMapping(target, source.id));
  const wiring = emitM5GFXWiring(variants[0], parts, target);
  const pinTable = emitPinTable(variants[0], {});

  assert.deepEqual(emitted.bus, {
    host: 1, hostSymbol: "SPI2_HOST", freqWrite: 40000000,
    freqRead: 16000000, threeWire: true,
  });
  assert.deepEqual(Object.keys(emitted.probes), ["gdew0154d67", "gdew0154m09"]);
  assert.deepEqual(emitted.probes.gdew0154d67, {
    cmd: 0x2F, dummyBits: 0, mask: 0xFFFFFFFF, values: [0x00010001],
  });
  assert.deepEqual(emitted.probes.gdew0154m09, {
    cmd: 0x70, dummyBits: 0, mask: 0xFFFF00FF, values: [0x00F00000],
  });
  for (const panel of Object.values(emitted.panels)) {
    assert.deepEqual(panel, {
      width: 200, height: 200, memoryWidth: null, memoryHeight: null,
      offsetX: null, offsetY: null, rotationOffset: null, invert: null, readable: null,
    });
  }
  assert.equal(emitted.backlight, null);
  assert.equal(emitted.backlightI2c, null);
  assert.deepEqual(wiring.display, {
    sclk: 5, mosi: 6, miso: -1, dc: 3, cs: 4, rst: 2, busy: 1,
  });
  assert.equal(wiring.powerGpio, 46);
  assert.deepEqual(wiring.hold, [4]);
  assert.equal(wiring.mapping.descInternalI2c, false);
  assert.equal(pinTable.values.in_i2c_scl, 12);
  assert.equal(pinTable.values.in_i2c_sda, 11);
  assert.equal(pinTable.values.port_a_pin1, 15);
  assert.equal(pinTable.values.port_a_pin2, 13);
  assert.equal(pinTable.values.rgb_led, 21);
  assert.equal(pinTable.values.power_hold, 46);

  const header = renderM5GFXSpecsHeader(emitted);
  assert.match(header, /probe_gdew0154d67[\s\S]*?cmd = 0x2F;[\s\S]*?dummy_bits = 0;[\s\S]*?mask = 0xFFFFFFFF;/);
  assert.match(header, /probe_gdew0154m09[\s\S]*?cmd = 0x70;[\s\S]*?dummy_bits = 0;[\s\S]*?mask = 0xFFFF00FF;/);

  const setup = await fs.readFile(esp32s3SetupPath, "utf8");
  assert.match(setup, /bus_airq[\s\S]*?\.with_dma_channel\(SPI_DMA_CH_AUTO\);/);
  assert.doesNotMatch(setup, /panel_airq_(?:d67|m09)[^;]*\.with_memory\(/);
  assert.match(setup, /construct_airq[\s\S]*?make_panel<lgfx::Panel_GDEW0154D67>/);
  assert.match(setup, /construct_airq[\s\S]*?make_panel<lgfx::Panel_GDEW0154M09>/);

  const detector = await fs.readFile(esp32s3DetectorPath, "utf8");
  assert.match(detector, /airq_probes[\s\S]*?probe_gdew0154d67[\s\S]*?probe_gdew0154m09/);
  assert.match(detector, /gpio_reset\(wiring::airq::reset_gpio, 2, 10, reset_always\)/);

  const wiringHeader = await fs.readFile(path.join(root, "../../src/board_detect/m5/generated/esp32s3_wiring.hpp"), "utf8");
  assert.match(wiringHeader, /namespace airq \{[\s\S]*?display_busy = 1;[\s\S]*?power_gpio = 46;/);
  assert.match(wiringHeader, /unconditional_pins\[\] = \{ 1, 2,[^}]*45, 46, 47, 48 \};/);
});

test("ESP32 PICO catalogs preserve probe order, legacy SPI reads, and fallback", async () => {
  const byId = Object.fromEntries(catalogBoards.map((item) => [item.id, item]));
  const specs = Object.fromEntries([
    "m5stickc", "m5stickcplus", "m5stack_coreink", "m5stickcplus2",
  ].map((boardId) => [boardId, emitM5GFXSpecs(
    byId[boardId], resolveCatalog(byId[boardId]).map((item) => item.board),
    parts, m5gfxBoardMapping(target, boardId))]));

  assert.deepEqual(specs.m5stickc.bus, {
    host: 1, hostSymbol: "SPI2_HOST", freqWrite: 27000000,
    freqRead: 14000000, threeWire: true,
  });
  assert.deepEqual(specs.m5stickcplus.probes.st7789v2,
                   { cmd: 4, mask: 0xFB, values: [0x81] });
  assert.deepEqual(specs.m5stack_coreink.probes.gdew0154d67,
                   { cmd: 0x2F, mask: 0xFFFFFFFF, values: [0x00010001], dummyBits: 0 });
  assert.deepEqual(specs.m5stack_coreink.probes.gdew0154m09,
                   { cmd: 0x70, mask: 0xFFFF00FF, values: [0x00F00000], dummyBits: 0 });
  assert.equal(specs.m5stack_coreink.bus.threeWire, true);
  assert.deepEqual(specs.m5stickcplus2.backlight,
                   { freq: 256, channel: 7, invert: false, offset: 40 });
  assert.ok(!chip.reserved.includes(9), "PICO-D4 CoreInk exposes GPIO9");

  assert.match(picoSource,
    /stickc_family_members\[] = \{\s*&desc_stickcplus\.def, &desc_stickc\.def/);
  assert.match(picoSource,
    /stickc_family_detector\([\s\S]*?stickc_family_member_descs, 2, true\)/);
  assert.match(picoSource, /stickcplus_id_values\[] = \{ 0x81, 0x85 \}/);
  assert.match(picoSource,
    /coreink_probes\[] = \{[\s\S]*?probe_gdew0154d67[\s\S]*?probe_gdew0154m09/);
  assert.match(picoSource,
    /esp32_picov3_detectors\[] = \{\s*&stickcplus2_detector, &atompsram_detector/);
  assert.match(picoSource, /M5AtomPsram", def_flag_fallback/);
  assert.match(picoSetupSource, /construct_atompsram[\s\S]*?construct_status_t::no_display/);

  const detector = await fs.readFile(path.join(root, "../../src/board_detect/board_detect.inl"), "utf8");
  assert.match(detector,
    /if \(legacy_zero_preamble\)[\s\S]*?writeCommand\(0, 8\);[\s\S]*?wait\(\);[\s\S]*?gpio_lo\(pin_cs\)[\s\S]*?writeCommand\(cmd, 8\)[\s\S]*?beginRead\(dummy_bits\)[\s\S]*?readData\(32\)/);
  assert.match(detector,
    /shared_id_read_[\s\S]*?probe_family\(ctx, result\)/);
  assert.match(detector,
    /panel_id = soft_spi_read32[\s\S]*?for \(std::uint8_t member_index/);
  assert.doesNotMatch(picoSource,
    /desc_(?:coreink|stickcplus2),[^\n]*true, true/);
  assert.equal(specs.m5stickc.probes.st7735s, undefined);
});

test("ESP32-C6 catalogs and detector preserve both display boards", async () => {
  const byId = Object.fromEntries(catalogBoards.map((item) => [item.id, item]));
  const unitResolved = resolveCatalog(byId.m5unit_c6l)[0].board;
  const nessoResolved = resolveCatalog(byId.arduino_nesso_n1)[0].board;
  const unitSpecs = emitM5GFXSpecs(
    byId.m5unit_c6l, [unitResolved], parts,
    m5gfxBoardMapping(target, "m5unit_c6l"));
  const nessoSpecs = emitM5GFXSpecs(
    byId.arduino_nesso_n1, [nessoResolved], parts,
    m5gfxBoardMapping(target, "arduino_nesso_n1"));
  const entries = [
    { board: byId.m5unit_c6l, chip: chipC6, emitted: emitM5GFXWiring(unitResolved, parts, target) },
    { board: byId.arduino_nesso_n1, chip: chipC6, emitted: emitM5GFXWiring(nessoResolved, parts, target) },
  ];

  assert.deepEqual(unitSpecs.bus, {
    host: 1, hostSymbol: "SPI2_HOST", freqWrite: 40000000,
    freqRead: 10000000, threeWire: true,
  });
  assert.deepEqual(unitSpecs.panels.ssd1306, {
    width: 64, height: 48, memoryWidth: null, memoryHeight: null,
    offsetX: 32, offsetY: 0, rotationOffset: 0, invert: null, readable: true,
  });
  assert.deepEqual(nessoSpecs.bus, {
    host: 1, hostSymbol: "SPI2_HOST", freqWrite: 40000000,
    freqRead: 16000000, threeWire: true,
  });
  assert.deepEqual(nessoSpecs.probes.st7789v2,
                   { cmd: 4, mask: 0xFB, values: [0x81] });
  assert.deepEqual(nessoSpecs.backlightI2c,
                   { i2cAddr: 0x44, i2cFreq: 400000 });
  assert.deepEqual(nessoSpecs.touch, {
    i2cAddr: 0x38, i2cFreq: 400000, xMin: 0, xMax: 134,
    yMin: 0, yMax: 239, rotationOffset: 0,
  });
  assert.deepEqual(detectionPinsForEntries(entries),
                   [3, 6, 8, 10, 15, 16, 17, 18, 20, 21, 22]);

  assert.match(esp32c6Source,
    /members_\[3\][\s\S]*?&desc_unitc6l.def, &desc_nesson1.def, nullptr/);
  assert.match(esp32c6Source,
    /probe_i2c_bus_present\(ctx, c6_display_detail::sda,\s*c6_display_detail::scl\)[\s\S]*?probe_pin_pulls\(ctx, c6_display_detail::signature_bit\)/);
  assert.match(esp32c6Source,
    /pi4io_id_mask = 0xE0[\s\S]*?pi4io_id_value = 0xA0[\s\S]*?i2c_pi4io2::id_reg[\s\S]*?is_pi4io\(value\)[\s\S]*?i2c_pi4io1::id_reg[\s\S]*?is_pi4io\(value\)/);
  assert.match(esp32c6Source,
    /gpio_reset\(wiring::unitc6l::reset_gpio, 2, 10, reset_hold_when_skipped\)/);
  assert.match(esp32c6SetupSource,
    /make_spi_bus\(bus_nesson1\)[\s\S]*?make_panel<lgfx::Panel_ST7789>/);
  assert.doesNotMatch(esp32c6SetupSource, /_read_panel_id/);
  assert.match(esp32c6SetupSource, /with_bus_shared\(true\)/);
  assert.match(esp32c6SetupSource,
    /backlight_i2c::i2c_addr[\s\S]*?backlight_i2c::i2c_freq/);
  assert.match(pmicOpsSource,
    /nesson1_devices\[] = \{[\s\S]*?0x44[\s\S]*?0x43[\s\S]*?nesson1_power_on\[] = \{[\s\S]*?i2c_write8\(0, 0x03, 0xC7\)[\s\S]*?i2c_write8\(1, 0x11, 0xFC\)/);

  // Fake pull-backend results corresponding to the legacy command values:
  // family bus bits high under pulldown, with G18 high only on UnitC6L.
  const family = (1n << 8n) | (1n << 10n);
  const signature = 1n << 18n;
  const classify = ({ pulldownHigh, pullupHigh }) =>
    (pulldownHigh & family) !== family ? "none"
      : (pullupHigh & signature) !== 0n ? "unitc6l" : "nesson1";
  assert.equal(classify({ pulldownHigh: family, pullupHigh: signature }), "unitc6l");
  assert.equal(classify({ pulldownHigh: family, pullupHigh: 0n }), "nesson1");
  assert.equal(classify({ pulldownHigh: 0n, pullupHigh: 0n }), "none");

  const main = await fs.readFile(path.join(root, "../../src/M5GFX.cpp"), "utf8");
  assert.match(main, /pkg_ver == 0[\s\S]*?try_setup_detected\(board_detect::m5::esp32c6_detectors/);
});

test("M5GFX probe dummy bits reject invalid catalog values", () => {
  const malformed = clone(parts);
  malformed.gdew0154d67.id_probe.dummy_bits = -1;
  assert.deepEqual(validatePartCatalog(malformed).filter((item) => item.id === "E_PART_ID_PROBE"), [{
    id: "E_PART_ID_PROBE",
    path: "/gdew0154d67/id_probe/dummy_bits",
    message: "dummy_bits must be an integer from 0 to 255",
  }]);
});

test("Cardputer family generated specs and wiring preserve the legacy setup", async () => {
  const fixtures = {
    m5cardputer: {
      bus: { host: 2, hostSymbol: "SPI3_HOST", freqWrite: 40000000, freqRead: 16000000, threeWire: true },
      panel: { width: 135, height: 240, memoryWidth: null, memoryHeight: null, offsetX: 52, offsetY: 40, rotationOffset: 1, invert: true, readable: true },
      backlight: { freq: 256, channel: 7, invert: false, offset: 16 },
      display: { sclk: 36, mosi: 35, miso: -1, dc: 34, cs: 37, rst: 33, busy: -1 },
      i2c: null,
      pins: { in_i2c_scl: 255, in_i2c_sda: 255, port_a_pin1: 1, port_a_pin2: 2, sd_mmc_clk: 40, sd_mmc_cmd: 14, sd_mmc_d0: 39, sd_mmc_d3: 12, rgb_led: 21 },
    },
    m5cardputer_adv: {
      bus: { host: 2, hostSymbol: "SPI3_HOST", freqWrite: 40000000, freqRead: 16000000, threeWire: true },
      panel: { width: 135, height: 240, memoryWidth: null, memoryHeight: null, offsetX: 52, offsetY: 40, rotationOffset: 1, invert: true, readable: true },
      backlight: { freq: 256, channel: 7, invert: false, offset: 16 },
      display: { sclk: 36, mosi: 35, miso: -1, dc: 34, cs: 37, rst: 33, busy: -1 },
      i2c: { sda: 8, scl: 9, port: 1 },
      pins: { in_i2c_scl: 9, in_i2c_sda: 8, port_a_pin1: 1, port_a_pin2: 2, sd_mmc_clk: 40, sd_mmc_cmd: 14, sd_mmc_d0: 39, sd_mmc_d3: 12, rgb_led: 21 },
    },
    m5vameter: {
      bus: { host: 2, hostSymbol: "SPI3_HOST", freqWrite: 40000000, freqRead: 16000000, threeWire: true },
      panel: { width: 240, height: 240, memoryWidth: null, memoryHeight: null, offsetX: 0, offsetY: 0, rotationOffset: 0, invert: true, readable: true },
      backlight: { freq: 512, channel: 7, invert: false, offset: 64 },
      display: { sclk: 36, mosi: 35, miso: -1, dc: 34, cs: 37, rst: 33, busy: -1 },
      i2c: { sda: 5, scl: 6, port: 1 },
      pins: { in_i2c_scl: 6, in_i2c_sda: 5, port_a_pin1: 9, port_a_pin2: 8, sd_mmc_clk: 255, sd_mmc_cmd: 255, sd_mmc_d0: 255, sd_mmc_d3: 255, rgb_led: 255 },
    },
  };
  for (const [id, expected] of Object.entries(fixtures)) {
    const source = catalogBoards.find((item) => item.id === id);
    const resolved = resolveCatalog(source)[0].board;
    const mapping = m5gfxBoardMapping(target, id);
    const specs = emitM5GFXSpecs(source, [resolved], parts, mapping);
    const wiring = emitM5GFXWiring(resolved, parts, target);
    const pinTable = emitPinTable(resolved, {});
    assert.deepEqual(specs.bus, expected.bus, `${id} bus`);
    assert.deepEqual(specs.panels.st7789v2, expected.panel, `${id} panel`);
    assert.deepEqual(specs.probes.st7789v2, { cmd: 4, mask: 0xFB, values: [0x81] }, `${id} probe`);
    assert.deepEqual(specs.backlight, expected.backlight, `${id} backlight`);
    assert.deepEqual(wiring.display, expected.display, `${id} display wiring`);
    assert.deepEqual(wiring.i2c, expected.i2c, `${id} internal I2C wiring`);
    assert.equal(wiring.resetGpio, 33, `${id} reset`);
    assert.equal(wiring.backlightGpio, 38, `${id} backlight pin`);
    assert.deepEqual(wiring.hold, [37], `${id} hold`);
    assert.equal(wiring.mapping.fields.includes("shared_sd"), false, `${id} separate SD bus`);
    for (const [name, value] of Object.entries(expected.pins)) assert.equal(pinTable.values[name], value, `${id} ${name}`);
  }

  const wiringHeader = await fs.readFile(path.join(root, "../../src/board_detect/m5/generated/esp32s3_wiring.hpp"), "utf8");
  assert.match(wiringHeader, /namespace cardputer \{[\s\S]*?namespace cardputer_subdivision \{[\s\S]*?sense_pins\[\] = \{ 5, 6, 8, 9 \};/);
  assert.match(wiringHeader, /vameter_i2c_addrs\[\] = \{ 0x40, 0x41 \};[\s\S]*?vameter_i2c_sda = 5;[\s\S]*?vameter_i2c_scl = 6;/);
  assert.match(wiringHeader, /unconditional_pins\[\] = \{[^}]*5, 6, 7, 8, 9[^}]*\}/);
  assert.match(wiringHeader, /opi_pins\[\] = \{ 33, 34, 35, 36, 37 \};/);

  const withUnrelatedDevice = new Map(["m5cardputer", "m5cardputer_adv", "m5vameter"].map((id) => {
    const source = clone(catalogBoards.find((item) => item.id === id));
    if (id === "m5vameter") source.devices.unrelated_sensor = {
      kind: "sensor", bus: "internal_i2c", i2c_addr: "0x42",
    };
    return [id, source];
  }));
  const unrelatedHeader = renderM5GFXWiringHeader([...withUnrelatedDevice.values()].map((source) => ({
    board: source,
    chip: chipS3,
    emitted: emitM5GFXWiring(resolveCatalog(source)[0].board, parts, target),
  })));
  assert.match(unrelatedHeader, /vameter_i2c_addrs\[\] = \{ 0x40, 0x41 \};/);
  assert.doesNotMatch(unrelatedHeader, /vameter_i2c_addrs\[\] = \{[^}]*0x42/);

  const setup = await fs.readFile(esp32s3SetupPath, "utf8");
  for (const name of ["cardputer", "cardputer_adv", "vameter"]) {
    assert.match(setup, new RegExp(`bus_${name}[\\s\\S]*?\\.with_dma_channel\\(SPI_DMA_CH_AUTO\\);`));
    assert.doesNotMatch(setup, new RegExp(`panel_${name}[^;]*\\.with_memory\\(`));
    assert.doesNotMatch(setup, new RegExp(`panel_${name}[^;]*\\.with_bus_shared\\(`));
  }
});

test("StickS3 PMIC specs reject unspecified generated values", () => {
  const source = clone(catalogBoards.find((item) => item.id === "m5sticks3"));
  const variants = resolveCatalog(source).map((item) => item.board);
  delete source.buses.internal_i2c.freq;
  assert.throws(() => emitM5GFXSpecs(source, variants, parts, m5gfxBoardMapping(target, source.id)), /internal_i2c\.freq must be an integer/);
});

test("M5GFX specs preserve an unspecified controller memory dimension", () => {
  const source = catalogBoards.find((item) => item.id === "m5atoms3");
  const variants = resolveCatalog(source).map((item) => item.board);
  const withoutMemoryDefaults = clone(parts);
  delete withoutMemoryDefaults.gc9107.spec_keys.memory_width.default;
  delete withoutMemoryDefaults.gc9107.spec_keys.memory_height.default;
  const withoutPanelDefaults = clone(variants);
  const gc9107 = withoutPanelDefaults.find((variant) => variant.devices.lcd.part === "gc9107");
  delete gc9107.devices.lcd.spec.offset_x;
  delete gc9107.devices.lcd.spec.offset_y;
  delete gc9107.devices.lcd.spec.rotation_offset;
  delete gc9107.devices.lcd.spec.invert;
  delete gc9107.devices.lcd.spec.readable;
  const emitted = emitM5GFXSpecs(source, withoutPanelDefaults, withoutMemoryDefaults, m5gfxBoardMapping(target, source.id));
  assert.equal(emitted.panels.gc9107.memoryWidth, null);
  assert.equal(emitted.panels.gc9107.memoryHeight, null);
  assert.deepEqual(emitted.panels.gc9107, { width: 128, height: 128, memoryWidth: null, memoryHeight: null, offsetX: null, offsetY: null, rotationOffset: null, invert: null, readable: null });
  assert.match(renderM5GFXSpecsHeader(emitted), /namespace panel_gc9107 \{[\s\S]*memory_width = setup_sentinel::keep_dimension;[\s\S]*memory_height = setup_sentinel::keep_dimension;[\s\S]*offset_x = setup_sentinel::keep_offset;[\s\S]*offset_y = setup_sentinel::keep_offset;[\s\S]*rotation_offset = setup_sentinel::keep_u8;[\s\S]*invert = setup_sentinel::keep_i8;[\s\S]*readable = setup_sentinel::keep_i8;/);
});

test("M5GFX wiring APIs require the parts catalog", () => {
  const resolved = resolveAll(board, connectorTypes, { chip, parts })[0].board;
  assert.throws(() => wiringFieldsForRole(resolved, "dev:sd.d3"), /requires the parts catalog/);
  assert.throws(() => wiringAssignments(resolved), /requires the parts catalog/);
  assert.throws(() => emitM5GFXWiring(resolved), /requires the parts catalog/);
});

test("M5GFX reset GPIO requires an explicit display-reset declaration", () => {
  const station = catalogBoards.find((item) => item.id === "m5station");
  const resolved = resolveBoard(station, {}, connectorTypes, { chip, parts });
  const mapping = emitM5GFXWiring(resolved, parts, target).mapping;
  assert.equal(mapping.reset, "display_rst");
  for (const pin of Object.values(resolved.pins)) pin.roles = pin.roles.filter((role) => role !== "dev:lcd.rst");
  assert.throws(() => emitM5GFXWiring(resolved, parts, target), /reset uses display_rst.*unavailable/);
});

test("M5GFX shared SD requires a derived SPI mode", () => {
  const resolved = resolveAll(board, connectorTypes, { chip, parts })[0].board;
  resolved.devices.sd.derived.modes = resolved.devices.sd.derived.modes.filter((mode) => mode !== "spi");
  assert.throws(() => emitM5GFXWiring(resolved, parts, target), /SD shares the display bus but does not support SPI/);
});

test("M5GFX SD terminal mapping follows the part SPI aliases", () => {
  const resolved = resolveAll(board, connectorTypes, { chip, parts })[0].board;
  const changedParts = clone(parts);
  changedParts.sd_slot.spi_alias.cmd = "cs";
  assert.deepEqual(wiringFieldsForRole(resolved, "dev:sd.cmd", changedParts), ["shared_sd_sd_cs"]);
});

test("variant validation reports wiring assignment failures as diagnostics", () => {
  const value = clone(board);
  value.pins[38].roles = value.pins[38].roles.filter((role) => role !== "dev:sd.d0");
  value.pins[19].roles.push("dev:sd.d0");
  const errors = validateResolvedVariants(value, {
    ...context,
    accessories,
    composition: compositionTarget.compositions[value.id]?.default,
    allow_origins: compositionTarget.allow_origins,
    pinTableTarget: compositionTarget,
  });
  assert.ok(errors.some((item) => item.id === "E_WIRING_GENERATION" && item.message.includes("shared_sd_miso")), JSON.stringify(errors, null, 2));
});

test("M5GFX wiring catalog requires every mapped board exactly once", () => {
  const mappings = Object.keys(target.boards).map((id) => m5gfxBoardMapping(target, id)).filter(Boolean);
  assert.throws(() => selectM5GFXWiringBoards(catalogBoards.filter((item) => item.id !== "m5paper"), mappings), /m5paper.*found 0/);
  const paper = catalogBoards.find((item) => item.id === "m5paper");
  assert.throws(() => selectM5GFXWiringBoards([...catalogBoards, clone(paper)], mappings), /m5paper.*found 2/);
});

test("M5GFX wiring rejects conflicting assignments to one emitted field", () => {
  const resolved = resolveAll(board, connectorTypes, { chip, parts })[0].board;
  resolved.pins[38].roles = resolved.pins[38].roles.filter((role) => role !== "dev:sd.d0");
  resolved.pins[19].roles.push("dev:sd.d0");
  assert.throws(() => emitM5GFXWiring(resolved, parts, target), /shared_sd_miso is assigned to both GPIO (38 and GPIO 19|19 and GPIO 38)/);
});

test("M5GFX wiring selects one display device without mixing another display", () => {
  const resolved = resolveAll(board, connectorTypes, { chip, parts })[0].board;
  resolved.devices.aux_panel = { kind: "display", bus: "main_spi" };
  resolved.pins[12].roles.push("dev:aux_panel.cs");
  resolved.pins[13].roles.push("dev:aux_panel.dc");
  const emitted = emitM5GFXWiring(resolved, parts, target);
  assert.equal(emitted.display.cs, 5);
  assert.equal(emitted.display.dc, 15);
  assert.deepEqual(emitted.hold, [4, 5]);
});

test("M5GFX wiring does not share an SD device on another bus", () => {
  const resolved = resolveAll(board, connectorTypes, { chip, parts })[0].board;
  resolved.devices.sd.bus = "sd_spi";
  resolved.buses.sd_spi = { kind: "spi", signals: ["sclk", "mosi", "miso"] };
  assert.equal(emitM5GFXWiring(resolved, parts, target).sharedSd, null);
});

test("M5GFX generated I2C host is an integer in the supported range", () => {
  const resolved = resolveAll(board, connectorTypes, { chip, parts })[0].board;
  resolved.buses.internal_i2c.preferred_host = "I2C_NUM_1";
  assert.throws(() => emitM5GFXWiring(resolved, parts, target), /integer from 0 to 127/);
  resolved.buses.internal_i2c.preferred_host = 128;
  assert.throws(() => emitM5GFXWiring(resolved, parts, target), /integer from 0 to 127/);
});

test("M5GFX generated namespaces contain only fields consumed by their descriptor", () => {
  const mappings = Object.keys(target.boards).map((id) => m5gfxBoardMapping(target, id)).filter(Boolean);
  const entries = selectM5GFXWiringBoards(catalogBoards, mappings).map((source) => ({
    board: source,
    chip: chips[source.chip],
    emitted: emitM5GFXWiring(resolveBoard(source, {}, connectorTypes, { chip, parts }), parts, target),
  }));
  const header = ["esp32_d0wdq6", "esp32s3"].map((chipId) =>
    renderM5GFXWiringHeader(entries.filter(({ board }) => board.chip === chipId))).join("\n");
  const scope = (name) => new RegExp(`namespace ${name} \\{([\\s\\S]*?)\\n\\} // namespace ${name}`).exec(header)[1];
  assert.doesNotMatch(scope("station"), /shared_sd_|power_gpio/);
  assert.doesNotMatch(scope("core2"), /reset_gpio|power_gpio/);
  assert.doesNotMatch(scope("tough"), /reset_gpio|power_gpio/);
  assert.doesNotMatch(scope("stack"), /internal_i2c_|power_gpio/);
  assert.doesNotMatch(scope("paper"), /internal_i2c_/);
});

test("M5GFX detection pin sets cover descriptors and reject reserved pins", () => {
  const source = catalogBoards.find((item) => item.id === "m5atoms3");
  const emitted = emitM5GFXWiring(resolveCatalog(source)[0].board, parts, target);
  const entry = { board: source, chip: chipS3, emitted };
  const pins = detectionPinsForEntries([entry]);
  assert.ok(pins.includes(33));
  assert.deepEqual(partitionDetectionPins([entry], chipS3), {
    unconditional: [15, 16, 17, 21],
    conditional: { opi: [33, 34] },
  });
  assert.throws(() => validateDetectionPins([entry], chipS3, pins.filter((pin) => pin !== 33)), /descriptor GPIO 33 is absent/);

  const opi = clone(source);
  opi.spec.storage = { psram_mb: 8, psram_mode: "opi" };
  assert.throws(() => validateDetectionPins([{ board: opi, chip: chipS3, emitted }], chipS3), /GPIO 33 is reserved when PSRAM mode is opi/);

  const usb = clone(emitted);
  usb.display.sclk = chipS3.usb.dn;
  assert.throws(() => validateDetectionPins([{ board: source, chip: chipS3, emitted: usb }], chipS3), /reserved for native USB/);
});

test("ESP32-S3 detector filters hinted and OPI-conflicting candidates per model", async () => {
  const detector = await fs.readFile(path.join(root, "../../src/board_detect/board_detect.inl"), "utf8");
  const s3 = await fs.readFile(esp32s3DetectorPath, "utf8");
  const main = await fs.readFile(path.join(root, "../../src/M5GFX.cpp"), "utf8");
  assert.doesNotMatch(detector, /detector->touches_conditional_pins/);
  assert.match(detector, /member\.desc->def\.id != ctx\.hint[\s\S]*?probe_member\(ctx, member, result\)/);
  assert.match(detector, /conditional_pins_unavailable && member\.touches_conditional_pins/);
  assert.match(s3, /spi_id_member_descs[\s\S]*?wiring::atoms3::touches_opi_pins[\s\S]*?wiring::dinmeter::touches_opi_pins/);
  const wiring = await fs.readFile(path.join(root, "../../src/board_detect/m5/generated/esp32s3_wiring.hpp"), "utf8");
  assert.match(wiring, /namespace atoms3 \{[\s\S]*?touches_opi_pins = true/);
  assert.match(wiring, /namespace dinmeter \{[\s\S]*?touches_opi_pins = false/);
  assert.match(main, /ESP_IDF_VERSION_VAL\(5, 0, 0\)[\s\S]*?esp_psram_is_initialized\(\)[\s\S]*?esp_spiram_is_initialized\(\)/);
  assert.match(main, /opi_pins\),\s*!conditional_pins_unavailable/);
  assert.match(s3, /cardputer_family_detector_t[\s\S]*?ctx\.conditional_pins_unavailable[\s\S]*?&desc_cardputer.def, &desc_cardputer_adv.def, &desc_vameter.def/);
});

test("detection transaction exposes non-consuming start-state restoration", async () => {
  const header = await fs.readFile(path.join(root, "../../src/board_detect/board_detect.hpp"), "utf8");
  const implementation = await fs.readFile(path.join(root, "../../src/board_detect/board_detect.inl"), "utf8");
  const gpioHeader = await fs.readFile(path.join(root, "../../src/lgfx/v1/platforms/esp32/common.hpp"), "utf8");
  const gpio = await fs.readFile(path.join(root, "../../src/lgfx/v1/platforms/esp32/common.inl"), "utf8");
  const main = await fs.readFile(path.join(root, "../../src/M5GFX.cpp"), "utf8");
  assert.match(header, /restore_start\(const std::int8_t\* pins, std::size_t count\)/);
  assert.match(header, /restore_start\(std::initializer_list<int> pins\)/);
  assert.match(header, /prepare_ctx_t[\s\S]*?detection_transaction_t\* transaction = nullptr/);
  assert.match(implementation, /restore_if_changed[\s\S]*?!saved\.matches_current\(\)[\s\S]*?saved\.restore\(\)/);
  assert.match(implementation, /index = count_; index != 0; --index[\s\S]*?restore_if_changed\(saved_\[index - 1\]\)/);
  assert.match(implementation, /saved_\[count_\]\.backup\(\)[\s\S]*?lgfx::gpio::release_lp_pad\(saved_\[index\]\.getPin\(\)\)/);
  assert.match(gpioHeader, /void release_lp_pad\(int pin_num\)/);
  assert.match(gpio, /release_lp_pad\(int pin_num\)[\s\S]*?rtc_gpio_is_valid_gpio[\s\S]*?rtc_gpio_deinit/);
  assert.ok(gpio.indexOf("#define LGFX_LP_I2C_NUM") < gpio.indexOf("void release_lp_pad(int pin_num)"));
  assert.match(gpio, /release_lp_pad\(int pin_num\)[\s\S]*?#if !defined \(LGFX_LP_I2C_NUM\)[\s\S]*?#error/);
  assert.doesNotMatch(gpio, /SOC_RTCIO_PIN_COUNT must be defined before release_lp_pad/);
  assert.match(gpio, /LGFX_LP_I2C_NUM > 0 \|\| defined \(CONFIG_IDF_TARGET_ESP32C61\)[\s\S]*?defined \(SOC_RTCIO_PIN_COUNT\)[\s\S]*?SOC_RTCIO_PIN_COUNT > 0/);
  assert.match(gpio, /matches_current[\s\S]*?_io_mux_gpio_reg\s*==[\s\S]*?_gpio_pin_reg\s*==[\s\S]*?_gpio_func_out_reg\s*==[\s\S]*?_gpio_enable\s*==[\s\S]*?_gpio_out\s*==[\s\S]*?_in_func_num\s*==[\s\S]*?_gpio_func_in_reg\s*==/);
  assert.match(main, /prepare_ctx = probe/);
});

test("confirmed boards survive post-detection power setup failures", async () => {
  const implementation = await fs.readFile(path.join(root, "../../src/board_detect/board_detect.inl"), "utf8");
  const setup = await fs.readFile(path.join(root, "../../src/board_detect/m5/setup_common.inl"), "utf8");
  const main = await fs.readFile(path.join(root, "../../src/M5GFX.cpp"), "utf8");
  assert.match(implementation, /prepare_power\(desc, result, i2c\.port, true\)/);
  assert.match(implementation, /if \(!retain_confirmed_board\) \{ return false; \}[\s\S]*?power_on stopped after board confirmation:[\s\S]*?failed_index[\s\S]*?status[\s\S]*?0/);
  assert.match(main, /construct_result == board_detect::m5::construct_status_t::no_display[\s\S]*?transaction\.restore_start\(result\.desc->hold_high_pins\.data,[\s\S]*?result\.desc->hold_high_pins\.size\)/);
  assert.match(main, /if \(!setup\(parts\)\)[\s\S]*?destroy_display_parts\(&parts\)[\s\S]*?transaction\.rollback\(\)/);
  assert.match(setup, /destroy_display_parts\(display_parts_t\* parts\)[\s\S]*?parts->bus->release\(\)[\s\S]*?delete parts->touch;[\s\S]*?delete parts->light;[\s\S]*?delete parts->panel;[\s\S]*?delete parts->bus;/);
});

test("embedded autodetect routes detected boards through descriptor setup", async () => {
  const main = await fs.readFile(path.join(root, "../../src/M5GFX.cpp"), "utf8");
  const autodetect = /board_t M5GFX::autodetect\(bool use_reset, board_t board\)\n  \{([\s\S]*?)\n  \}\n\n#else/.exec(main)?.[1];
  assert.ok(autodetect, "embedded autodetect implementation is present");
  assert.doesNotMatch(autodetect, /\bboard\s*=\s*board_t::board_(?!unknown\b)/);
});

test("confirmed boards tolerate reset-list faults while confirm remains strict", async () => {
  const implementation = await fs.readFile(path.join(root, "../../src/board_detect/board_detect.inl"), "utf8");
  const esp32 = await fs.readFile(path.join(root, "../../src/board_detect/m5/esp32_d0wdq6.inl"), "utf8");
  assert.match(implementation, /prepare_reset\(\*current, result, ctx, i2c\.port, nullptr, true\)/);
  assert.match(implementation, /variant_confirmed && desc\.power\.variant_count == 1[\s\S]*?&desc\.power\.variants\[0\]/);
  assert.match(implementation, /if \(!retain_confirmed_board\) \{ return false; \}[\s\S]*?reset_release stopped after board confirmation: op=%u status=%u native=%d/);
  assert.match(implementation, /reset_assert stopped after board confirmation: op=%u status=%u native=%d/);
  assert.match(implementation, /desc\.power\.variant_confirmed && desc\.power\.variant_count > 1[\s\S]*?desc\.reset\.kind == reset_kind_t::i2c_regs/);
  assert.match(esp32, /prepare_reset\(desc_station, \*result, prepare_ctx, i2c\.port\)/);
  assert.doesNotMatch(esp32, /prepare_reset\(desc_(?:station|core2|tough)[^\n]*true\)/);
});

test("post-power member refinement and confirmed option variants are opt-in", async () => {
  const header = await fs.readFile(path.join(root, "../../src/board_detect/board_detect.hpp"), "utf8");
  const implementation = await fs.readFile(path.join(root, "../../src/board_detect/board_detect.inl"), "utf8");
  assert.match(header, /refine_fn_t refine = nullptr/);
  assert.match(header, /option_select_mask;[\s\S]*?option_select_value;/);
  assert.match(implementation, /if \(!\(result\.prepared & prepared_refine\) && result\.refine != nullptr\)/);
  assert.match(implementation, /result\.prepared \|= prepared_power_failed;[\s\S]*?member refinement skipped after retained power_on failure; board=%u[\s\S]*?else[\s\S]*?!result\.refine\(result, ctx\)/);
  assert.match(header, /prepared_power_failed = 1u << 6/);
  assert.match(implementation, /\(result\.option & candidate\.option_select_mask\) == candidate\.option_select_value/);
});

test("PM1 extension failure injection rejects only its selected board", async () => {
  const main = await fs.readFile(path.join(root, "../../src/M5GFX.cpp"), "utf8");
  assert.match(main, /board_t setup_board = board_t::board_unknown/);
  assert.match(main, /FAIL_CHAINCAPTAIN_SETUP[\s\S]*?setup_board == board_t::board_M5ChainCaptain/);
  assert.match(main, /FAIL_PAPERCOLOR_SETUP[\s\S]*?setup_board == board_t::board_M5PaperColor/);
  assert.doesNotMatch(main, /FAIL_PAPERCOLOR_SETUP\)[\s\S]{0,100}\(void\)parts;\s*return false;\s*#endif\s*return _adopt/);
});

test("generated wiring header links from two translation units", async (t) => {
  const compiler = process.env.CXX || "c++";
  const probe = spawnSync(compiler, ["--version"], { encoding: "utf8" });
  if (probe.error?.code === "ENOENT") return t.skip(`C++ compiler not found (${compiler})`);
  const directory = await fs.mkdtemp(path.join(os.tmpdir(), "m5gfx-wiring-odr-"));
  const include = '#include "src/board_detect/m5/generated/esp32_d0wdq6_wiring.hpp"\n';
  try {
    await fs.writeFile(path.join(directory, "a.cpp"), `${include}int wiring_a() { return m5gfx::board_detect::m5::wiring::tough::hold[0]; }\n`);
    await fs.writeFile(path.join(directory, "b.cpp"), `${include}int wiring_a();\nint main() { return wiring_a() == m5gfx::board_detect::m5::wiring::tough::hold[0] ? 0 : 1; }\n`);
    const compiled = spawnSync(compiler, ["-std=c++11", "-I", path.resolve(root, "../.."), path.join(directory, "a.cpp"), path.join(directory, "b.cpp"), "-o", path.join(directory, "test")], { encoding: "utf8" });
    assert.equal(compiled.status, 0, compiled.stderr || compiled.stdout);
  } finally {
    await fs.rm(directory, { recursive: true, force: true });
  }
});

test("E_SD_ALIAS_MISMATCH", () => {
  const value = fixture((item) => {
    item.pins[23].roles = item.pins[23].roles.filter((role) => role !== "dev:sd.cmd");
    item.pins[18].roles = item.pins[18].roles.map((role) => role === "dev:sd.clk" ? "dev:sd.cmd" : role);
  });
  assert.ok(validateBoard(value, context).some((item) => item.id === "E_SD_ALIAS_MISMATCH"));
});

test("ESP32 fixed IOMUX derives SDIO widths", () => {
  const value = {
    pins: {}, buses: {}, devices: {
      sd: { kind: "sd", part: "sd_slot", signals: { clk: "gpio:14", cmd: "gpio:15", d0: "gpio:2", d1: "gpio:4", d2: "gpio:12", d3: "gpio:13" } },
    },
  };
  deriveSd(value, chip, parts.sd_slot);
  assert.deepEqual(value.devices.sd.derived, { modes: ["sdio1", "sdio4"] });
});

test("W_SD_NO_MODE", () => {
  const resolved = resolveCatalog(board)[0].board;
  resolved.devices.sd.derived.modes = [];
  assert.ok(validateBoard(resolved, { ...context, resolved: true }).some((item) => item.id === "W_SD_NO_MODE"));
});

test("generated device fields follow kind rather than device ID", () => {
  const renamed = clone(board);
  renamed.devices.panel_with_any_id = renamed.devices.lcd;
  delete renamed.devices.lcd;
  assert.equal(isGeneratedField(renamed, "/devices/panel_with_any_id", "signals"), true);
  assert.equal(isGeneratedField(renamed, "/devices/panel_with_any_id", "part"), false);
  renamed.devices.status_light = { kind: "led_strip" };
  assert.equal(isGeneratedField(renamed, "/devices/status_light", "signals"), true);
});

test("Core2 revisions match all four snapshots and confirmed PMIC endpoints", async () => {
  const outputs = resolveCatalog(board);
  assert.deepEqual(outputs.map((output) => output.filename), [
    "m5stack_core2@v1_0.json",
    "m5stack_core2@v1_1.json",
    "m5stack_core2@v1_2.json",
    "m5stack_core2@v1_3.json",
  ]);
  for (const output of outputs) {
    const snapshot = await fs.readFile(path.join(root, "generated/resolved", output.filename), "utf8");
    assert.equal(snapshot, formatBoard(output.board));
  }
  assert.equal(outputs[0].board.devices.lcd.signals.rst, "pin:pmic.gpio3");
  assert.equal(outputs[0].board.devices.speaker.enable, "pin:pmic.gpio2");
  assert.equal(outputs[1].board.devices.lcd.signals.rst, "pin:pmic.aldo2");
  assert.equal(outputs[1].board.devices.speaker.enable, "pin:pmic.aldo3");
  assert.equal(outputs[1].board.devices.imu.part, "mpu6886");
});

test("E_ROLE_DUP", () => assertSingle("E_ROLE_DUP", fixture((value) => value.pins[25].roles.push("bus:port_a_i2c.sda"))));

test("E_ROLE_DUP ignores accessory source prefixes", () => {
  const modifiedAccessory = clone(accessories.m5go_bottom2);
  modifiedAccessory.host_pins["mbus.4"].roles.push("dev:lcd.cs");
  const resolved = resolveAll(board, connectorTypes, {
    chip, parts,
    accessories: { ...accessories, m5go_bottom2: modifiedAccessory },
    composition: compositionTarget.compositions[board.id].default,
    allow_origins: compositionTarget.allow_origins,
  })[0].board;
  assert.ok(validateBoard(resolved, { ...context, resolved: true }).some((item) => item.id === "E_ROLE_DUP"));
});

test("E_ROLE_SOURCE rejects source-prefixed roles on a base SoC pin", () => {
  const value = fixture((item) => {
    item.pins[5].roles = item.pins[5].roles.map((role) => role === "dev:lcd.cs" ? "accessory/dev:lcd.cs" : role);
  });
  assert.ok(validateBoard(value, context).some((item) => item.id === "E_ROLE_SOURCE"));
});

test("variant validation compares generator defaults with every revision", () => {
  const value = clone(board);
  value.buses.alternate_spi = { kind: "spi", signals: ["sclk", "mosi", "miso"] };
  value.pins[26].roles.push("bus:alternate_spi.sclk");
  value.pins[27].roles.push("bus:alternate_spi.mosi");
  value.pins[35].roles.push("bus:alternate_spi.miso");
  delete value.devices.lcd.bus;
  value.devices.lcd.default = "ili9342e";
  value.devices.lcd.choices.ili9342c.bus = "main_spi";
  value.devices.lcd.choices.ili9342e.bus = "alternate_spi";
  for (const revision of value.revisions) revision.select.lcd = "ili9342c";
  const errors = validateResolvedVariants(value, {
    ...context,
    accessories,
    composition: compositionTarget.compositions[value.id]?.default,
    allow_origins: compositionTarget.allow_origins,
    pinTableTarget: compositionTarget,
  });
  assert.ok(errors.some((item) => item.id === "E_CHOICE_PINTABLE"), JSON.stringify(errors, null, 2));
});

test("E_BUS_CONFLICT", () => assertSingle("E_BUS_CONFLICT", fixture((value) => {
  value.pins[18].roles = value.pins[18].roles.filter((role) => role !== "bus:main_spi.sclk");
  value.pins[0].roles.push("bus:main_spi.sclk");
})));

test("E_CS_CONFLICT", () => assertSingle("E_CS_CONFLICT", fixture((value) => {
  value.pins[4].roles = [];
  value.pins[5].roles.push("dev:sd.d3");
})));

test("E_REF_MISSING", () => assertSingle("E_REF_MISSING", fixture((value) => value.pins[1].roles.push("conn:no_such_connector.1"))));

test("E_SIGNAL_UNBOUND", () => assertSingle("E_SIGNAL_UNBOUND", fixture((value) => {
  value.pins[32].roles = value.pins[32].roles.filter((role) => role !== "bus:port_a_i2c.sda");
})));

test("E_CHIP_INPUT_ONLY", () => assertSingle("E_CHIP_INPUT_ONLY", fixture((value) => {
  value.pins[12].roles = [];
  value.pins[35].roles.push("bus:i2s_spk.bck");
})));

test("E_CHIP_RESERVED", () => assertSingle("E_CHIP_RESERVED", fixture((value) => {
  value.pins[39].roles = value.pins[39].roles.filter((role) => role !== "dev:touch.int");
  value.pins[6] = { roles: ["dev:touch.int"], pull: "none" };
})));

test("ESP32-S3 OPI PSRAM conditionally reserves GPIO33 through GPIO37", () => {
  assert.deepEqual({
    gpio_count: chipS3.gpio_count,
    input_only: chipS3.input_only,
    strapping: chipS3.strapping,
    reserved: chipS3.reserved,
    opi: chipS3.reserved_conditional.opi,
    usb: chipS3.usb,
    spi_hosts: chipS3.spi_hosts,
    i2c_hosts: chipS3.i2c_hosts,
    i2s_ports: chipS3.i2s_ports,
  }, {
    gpio_count: 49,
    input_only: [],
    strapping: [0, 3, 45, 46],
    reserved: [26, 27, 28, 29, 30, 31, 32],
    opi: [33, 34, 35, 36, 37],
    usb: { dn: 19, dp: 20 },
    spi_hosts: 2,
    i2c_hosts: 2,
    i2s_ports: 2,
  });
  const source = clone(catalogBoards.find((item) => item.id === "m5atoms3"));
  source.spec.storage = { psram_mode: "opi" };
  source.pins[35] = { roles: ["dev:lcd.busy"] };
  const errors = validateBoard(source, { ...context, chip: chipS3 });
  assert.ok(errors.some((item) => item.id === "E_CHIP_RESERVED_COND" && item.path === "/pins/35/roles"), JSON.stringify(errors, null, 2));
});

test("ESP32-S3 native USB pins produce warnings when assigned roles", () => {
  const source = clone(catalogBoards.find((item) => item.id === "m5atoms3"));
  source.pins[19] = { roles: ["dev:lcd.busy"] };
  const errors = validateBoard(source, { ...context, chip: chipS3 });
  assert.ok(errors.some((item) => item.id === "W_CHIP_USB_PIN" && item.severity === "warning" && item.path === "/pins/19/roles"), JSON.stringify(errors, null, 2));
});

test("E_HEX_FORMAT", () => {
  const errors = validateBoard(fixture((value) => { value.devices.touch.i2c_addr = 56; }), context);
  assert.ok(errors.some((item) => item.id === "E_HEX_FORMAT"));
  assert.ok(errors.some((item) => item.id === "E_SCHEMA_TYPE"));
});

test("E_CHOICE_DEFAULT", () => {
  const value = fixture((item) => { item.devices.lcd.default = "missing"; });
  assert.ok(validateChoices(value).some((item) => item.id === "E_CHOICE_DEFAULT"));
});

test("W_CHOICE_SINGLE", () => {
  const value = fixture((item) => { item.devices.lcd.choices = { ili9342c: item.devices.lcd.choices.ili9342c }; });
  assert.ok(validateChoices(value).some((item) => item.id === "W_CHOICE_SINGLE" && item.severity === "warning"));
});

test("E_CHOICE_KEY_OVERLAP", () => {
  const value = fixture((item) => { item.devices.lcd.choices.ili9342c.bus = "main_spi"; });
  assert.ok(validateChoices(value).some((item) => item.id === "E_CHOICE_KEY_OVERLAP"));
});

test("E_REV_SELECT_RUNTIME and E_REV_SELECT_UNKNOWN", () => {
  const value = fixture((item) => {
    item.devices.imu.selected_by = "runtime";
    item.revisions[0].select.imu = "mpu6886";
    item.revisions[1].select.ghost = "missing";
  });
  const ids = validateChoices(value).map((item) => item.id);
  assert.ok(ids.includes("E_REV_SELECT_RUNTIME"));
  assert.ok(ids.includes("E_REV_SELECT_UNKNOWN"));
});

test("owner pin validation IDs", () => {
  const resolved = resolveCatalog(board)[0].board;
  const unknown = clone(resolved);
  unknown.devices.pmic.pins.mystery = { roles: ["dev:lcd.rst"] };
  assert.ok(validateOwners(unknown, parts).some((item) => item.id === "E_OWNER_PIN_UNKNOWN"));
  const noPart = clone(resolved);
  delete noPart.devices.pmic.part;
  assert.ok(validateOwners(noPart, parts).some((item) => item.id === "E_OWNER_NO_PART"));
  const weakCapParts = clone(parts);
  weakCapParts.axp192.pins.gpio3.cap = ["input"];
  assert.ok(validateOwners(resolved, weakCapParts).some((item) => item.id === "E_OWNER_CAP"));
});

test("E_ENDPOINT_OWNER", () => {
  const resolved = resolveCatalog(board)[0].board;
  resolved.devices.lcd.signals.rst = "pin:ghost.gpio3";
  assert.ok(validateBoard(resolved, { ...context, resolved: true }).some((item) => item.id === "E_ENDPOINT_OWNER"));
});

test("part catalog and board validation IDs", () => {
  const malformed = clone(parts);
  malformed.axp192.id = "AXP192";
  assert.ok(validatePartCatalog(malformed).some((item) => item.id === "E_PART_FORMAT"));
  const resolved = resolveCatalog(board)[0].board;
  const wrongKind = clone(resolved);
  wrongKind.devices.lcd.part = "ns4168";
  assert.ok(validateParts(wrongKind, parts).some((item) => item.id === "E_PART_KIND"));
  const wrongBus = clone(resolved);
  wrongBus.devices.lcd.bus = "internal_i2c";
  assert.ok(validateParts(wrongBus, parts).some((item) => item.id === "E_PART_BUS_KIND"));
  const missingSignal = clone(resolved);
  delete missingSignal.devices.lcd.signals.cs;
  assert.ok(validateParts(missingSignal, parts).some((item) => item.id === "E_PART_SIGNAL_MISSING"));
  const unknownSignal = clone(resolved);
  unknownSignal.devices.lcd.signals.ghost = "gpio:1";
  assert.ok(validateParts(unknownSignal, parts).some((item) => item.id === "E_PART_SIGNAL_UNKNOWN"));
  const unusualAddress = clone(resolved);
  unusualAddress.devices.touch.i2c_addr = "0x40";
  assert.ok(validateParts(unusualAddress, parts).some((item) => item.id === "W_PART_ADDR_UNUSUAL"));
});

test("target option validation IDs", () => {
  const wrongName = clone(board);
  wrongName.name = "M5Stack Core2";
  assert.ok(validateTarget(wrongName, target).some((item) => item.id === "E_TGT_BOARD_NAME" && item.path === "/name"));
  const duplicate = clone(target);
  duplicate.boards.m5stack_core2.options.push({ name: "duplicate", bit: 2, select: { pmic: "axp2101" } });
  assert.ok(validateTarget(board, duplicate).some((item) => item.id === "E_TGT_SLOT_DUP"));
  duplicate.boards.m5stack_core2.options.push({ name: "unknown", bit: 3, select: { lcd: "missing" } });
  assert.ok(validateTarget(board, duplicate).some((item) => item.id === "E_TGT_SELECT_UNKNOWN"));
  const duplicateBit = clone(target);
  duplicateBit.boards.m5stack_core2.options[1].bit = 0;
  assert.ok(validateTarget(board, duplicateBit).some((item) => item.id === "E_TGT_BIT_DUP"));
  const gap = clone(target);
  gap.boards.m5stack_core2.options[1].bit = 2;
  assert.ok(validateTarget(board, gap).some((item) => item.id === "E_TGT_BIT_GAP"));
  const range = clone(target);
  range.boards.m5stack_core2.options[1].bit = 32;
  assert.ok(validateTarget(board, range).some((item) => item.id === "E_TGT_BIT_RANGE"));
  const badName = clone(target);
  badName.boards.m5stack_core2.options[1].name = "Bad Name";
  assert.ok(validateTarget(board, badName).some((item) => item.id === "E_TGT_OPTION_NAME"));
  assert.ok(validateTarget(board, target).some((item) => item.id === "W_TGT_REV_INDISTINGUISHABLE"));
  const runtime = clone(board);
  runtime.devices.imu.selected_by = "runtime";
  assert.ok(validateTarget(runtime, target).some((item) => item.id === "E_TGT_CHOICE_UNREPRESENTED"));
});

test("three runtime choices may share one slot when they select different choices", () => {
  const atoms3 = clone(catalogBoards.find((item) => item.id === "m5atoms3"));
  atoms3.devices.lcd.choices.third_panel = clone(atoms3.devices.lcd.choices.gc9107);
  const threeChoiceTarget = clone(target);
  threeChoiceTarget.boards.m5atoms3.options.push({ name: "third_panel", bit: 1, select: { lcd: "third_panel" } });
  assert.equal(validateTarget(atoms3, threeChoiceTarget).some((item) => item.id === "E_TGT_SLOT_DUP"), false);
  assert.equal(validateTarget(atoms3, threeChoiceTarget).some((item) => item.id === "E_TGT_CHOICE_UNREPRESENTED"), false);
});

test("target generation controls reject mismatches and unsafe values", () => {
  const malformed = clone(target);
  malformed.boards.m5sticks3.board_enum = malformed.boards.m5atoms3.board_enum;
  malformed.boards.m5sticks3.chip = "esp32_d0wdq6";
  malformed.boards.m5sticks3.cpp_namespace = "bad::namespace";
  malformed.boards.m5sticks3.wiring_fields.push("unknown");
  malformed.boards.m5sticks3.wiring_output = "../escape.hpp";
  malformed.boards.m5sticks3.reset = "display_rts";
  const ids = new Set(validateTargets(catalogBoards, malformed).map((item) => item.id));
  for (const id of ["E_TGT_BOARD_ENUM_DUP", "E_TGT_CHIP", "E_TGT_CPP_IDENTIFIER", "E_TGT_WIRING_FIELD", "E_TGT_OUTPUT", "E_TGT_RESET"]) assert.ok(ids.has(id), id);
  assert.match(cliSource, /validateGenerationInputs\([\s\S]*?await fs\.mkdir/);
  assert.match(cliSource, /fs\.writeFile\(safeOutputPath/);
});

test("release probe target requires exactly eight valid ESP32-S3 GPIO pins", () => {
  const core = catalogBoards.find((item) => item.id === "m5stack_cores3");
  const releaseRoles = target.boards.m5stack_cores3.release_probe.pins;
  const onePin = clone(target);
  onePin.boards.m5stack_cores3.release_probe.pins = [releaseRoles[0]];
  assert.ok(validateTarget(core, onePin).some((item) => item.id === "E_TGT_RELEASE_PROBE"));

  const lowerPin = clone(core);
  lowerPin.pins["21"] = clone(lowerPin.pins["38"]);
  delete lowerPin.pins["38"];
  assert.equal(validateTarget(lowerPin, target).some((item) => item.id === "E_TGT_RELEASE_PROBE"), false);

  for (const invalidPin of [22, 25, 49]) {
    const moved = clone(core);
    moved.pins[String(invalidPin)] = clone(moved.pins["38"]);
    delete moved.pins["38"];
    assert.ok(validateTarget(moved, target).some((item) => item.id === "E_TGT_RELEASE_PROBE"));
  }
});

test("target output filenames cannot collide across output kinds", () => {
  const withinBoard = clone(target);
  withinBoard.boards.m5atoms3.specs_output = withinBoard.boards.m5atoms3.wiring_output;
  assert.ok(validateTarget(catalogBoards.find((item) => item.id === "m5atoms3"), withinBoard)
    .some((item) => item.id === "E_TGT_OUTPUT_COLLISION"));

  const acrossBoards = clone(target);
  acrossBoards.boards.m5dial.specs_output = acrossBoards.boards.m5station.wiring_output;
  assert.ok(validateTargets(catalogBoards, acrossBoards).some((item) => item.id === "E_TGT_OUTPUT_COLLISION"));

  const acrossChips = clone(target);
  acrossChips.boards.m5atoms3.wiring_output = acrossChips.boards.m5station.wiring_output;
  assert.ok(validateTargets(catalogBoards, acrossChips).some((item) => item.id === "E_TGT_OUTPUT_COLLISION"));
});

test("part defaults and C++ types validate their own definitions", () => {
  const malformed = clone(parts);
  malformed.st7735s.spec_keys.memory_width.default = 0;
  malformed.st7735s.spec_keys.memory_width["x-cpp-type"] = "bool";
  const ids = new Set(validatePartCatalog(malformed).map((item) => item.id));
  assert.ok(ids.has("E_SCHEMA_MINIMUM"));
  assert.ok(ids.has("E_PART_CPP_TYPE"));
});

test("part schemas and values must fit their declared C++ type", () => {
  const narrow = clone(parts);
  narrow.st7735s.spec_keys.memory_width["x-cpp-type"] = "std::uint8_t";
  narrow.st7735s.spec_keys.memory_width.default = 300;
  const catalogErrors = validatePartCatalog(narrow);
  assert.ok(catalogErrors.some((item) => item.id === "E_PART_CPP_RANGE" && item.path.endsWith("/memory_width/maximum")));
  assert.ok(catalogErrors.some((item) => item.id === "E_PART_CPP_RANGE" && item.path.endsWith("/memory_width/default")));

  const atoms3 = clone(catalogBoards.find((item) => item.id === "m5atoms3"));
  atoms3.devices.lcd.choices.st7735s.spec.memory_width = 300;
  const resolved = resolveCatalog(atoms3).find((item) => item.board.devices.lcd.part === "st7735s").board;
  assert.ok(validateParts(resolved, narrow).some((item) => item.id === "E_PART_CPP_RANGE"
    && item.path.endsWith("/lcd/spec/memory_width")));
});

test("generated C++ identifiers reject language keywords", () => {
  const malformed = clone(target);
  malformed.boards.m5sticks3.cpp_namespace = "class";
  malformed.boards.m5atoms3.options[0].name = "delete";
  const errors = validateTargets(catalogBoards, malformed);
  assert.ok(errors.some((item) => item.id === "E_TGT_CPP_KEYWORD" && item.path.endsWith("/cpp_namespace")));
  assert.ok(errors.some((item) => item.id === "E_TGT_CPP_KEYWORD" && item.path.endsWith("/options/0/name")));
});

test("M5GFX sentinel fields require a C++ type that preserves the sentinel", () => {
  const malformed = clone(parts);
  malformed.gc9a01.spec_keys.invert["x-cpp-type"] = "bool";
  malformed.gc9a01.spec_keys.offset_x["x-cpp-type"] = "std::uint16_t";
  const errors = validatePartCatalog(malformed);
  assert.ok(errors.some((item) => item.id === "E_PART_CPP_SENTINEL" && item.path.endsWith("/invert/x-cpp-type")));
  assert.ok(errors.some((item) => item.id === "E_PART_CPP_SENTINEL" && item.path.endsWith("/offset_x/x-cpp-type")));

  const dial = catalogBoards.find((item) => item.id === "m5dial");
  assert.throws(() => emitM5GFXSpecs(dial, resolveCatalog(dial).map((item) => item.board), malformed,
    m5gfxBoardMapping(target, dial.id)), /cannot represent the -32768 sentinel/);
});

test("partless backlight specs use kind-wide types, ranges, and defaults", () => {
  const malformed = clone(catalogBoards.find((item) => item.id === "m5dial"));
  malformed.devices.backlight.spec.channel = 256;
  malformed.devices.backlight.spec.offset = -1;
  const errors = validateBoard(malformed, { ...context, chip: chipS3 });
  assert.ok(errors.some((item) => item.id === "E_SCHEMA_MAXIMUM" && item.path.endsWith("/backlight/spec/channel")));
  assert.ok(errors.some((item) => item.id === "E_SCHEMA_MINIMUM" && item.path.endsWith("/backlight/spec/offset")));

  const defaults = clone(catalogBoards.find((item) => item.id === "m5dial"));
  delete defaults.devices.backlight.spec.invert;
  delete defaults.devices.backlight.spec.offset;
  assert.equal(validateBoard(defaults, { ...context, chip: chipS3 }).some((item) => item.severity !== "warning"), false);
  const emitted = emitM5GFXSpecs(defaults, resolveCatalog(defaults).map((item) => item.board), parts,
    m5gfxBoardMapping(target, defaults.id));
  assert.equal(emitted.backlight.invert, false);
  assert.equal(emitted.backlight.offset, 0);
});

test("required generator specs fail validation before emission", () => {
  const malformed = clone(catalogBoards.find((item) => item.id === "m5atoms3"));
  delete malformed.devices.lcd.choices.gc9107.spec.width;
  assert.ok(validateBoard(malformed, { ...context, chip: chipS3 })
    .some((item) => item.id === "E_PART_SPEC_REQUIRED" && item.path.endsWith("/choices/gc9107/spec/width")));

  const missingBacklight = clone(catalogBoards.find((item) => item.id === "m5dial"));
  delete missingBacklight.devices.backlight.spec.freq;
  assert.ok(validateBoard(missingBacklight, { ...context, chip: chipS3 })
    .some((item) => item.id === "E_PART_SPEC_REQUIRED" && item.path.endsWith("/backlight/spec/freq")));
});

test("unsafe choice IDs cannot escape resolved output directories", () => {
  const atoms3 = clone(catalogBoards.find((item) => item.id === "m5atoms3"));
  atoms3.devices.lcd.choices["../../../../escape"] = atoms3.devices.lcd.choices.gc9107;
  atoms3.devices.lcd.default = "../../../../escape";
  const errors = validateBoard(atoms3, { ...context, chip: chipS3 });
  assert.ok(errors.some((item) => item.id === "E_SCHEMA_PATTERN" && item.path.includes("../../../../escape")));
  assert.throws(() => resolveAll(atoms3, connectorTypes, { chip: chipS3, parts }), /safe ID/);
  assert.throws(() => resolvedFilename({ id: "../escape" }, null), /safe ID/);
});

test("SoC pin keys must be numeric, in range, and present on the chip", () => {
  const malformed = fixture((value) => { value.pins.foo = { roles: [] }; });
  assert.ok(validateBoard(malformed, context).some((item) => item.id === "E_PIN_KEY"));
  const outside = fixture((value) => { value.pins[String(chip.gpio_count)] = { roles: [] }; });
  assert.ok(validateBoard(outside, context).some((item) => item.id === "E_PIN_KEY"));
  const absent = fixture((value) => { value.pins[String(chip.absent[0])] = { roles: [] }; });
  assert.ok(validateBoard(absent, context).some((item) => item.id === "E_CHIP_ABSENT"));
  assert.throws(() => addRole(absent, chip.absent[0], "dev:lcd.cs", { chip }), /does not exist/);
});

test("chip catalogs distinguish absent GPIO numbers from reserved pads", () => {
  assert.deepEqual(chip.absent, [20, 24, 28, 29, 30, 31]);
  assert.deepEqual(chipS3.absent, [22, 23, 24, 25]);
  assert.deepEqual(chipC5.absent, []);
  assert.deepEqual(chipC6.absent, []);
  assert.deepEqual(chipC61.absent, []);
  assert.deepEqual(chipP4.absent, []);
  assert.ok(!chip.absent.includes(9), "PICO-D4 GPIO9 is reserved separately from absent pads");
  assert.ok(!chipS3.absent.some((gpio) => chipS3.reserved.includes(gpio)));
});

test("Core2 and Tough share the PMIC option bit at the C++ assumption", async () => {
  assert.match(d0wdq6Source, /static_assert\(generated_options::core2::new_pmic == generated_options::tough::reserved,/);
  const swapped = clone(target);
  swapped.boards.m5stack_core2.options[0].bit = 1;
  swapped.boards.m5stack_core2.options[1].bit = 0;
  const coreBit = swapped.boards.m5stack_core2.options.find((option) => option.name === "new_pmic").bit;
  const toughBit = swapped.boards.m5tough.options.find((option) => option.name === "reserved").bit;
  const directory = await fs.mkdtemp(path.join(os.tmpdir(), "m5gfx-option-invariant-"));
  try {
    const probe = path.join(directory, "probe.cpp");
    await fs.writeFile(probe, `static_assert((1u << ${coreBit}) == (1u << ${toughBit}), "shared option bit");\n`);
    const compiled = spawnSync("c++", ["-std=c++11", "-c", probe, "-o", path.join(directory, "probe.o")], { encoding: "utf8" });
    assert.notEqual(compiled.status, 0, "swapping the Core2 bit must trip the C++ invariant");
  } finally {
    await fs.rm(directory, { recursive: true, force: true });
  }
});

test("M5Stack and Tough LCD choices are runtime-selected", () => {
  const stack = catalogBoards.find((item) => item.id === "m5stack");
  const tough = catalogBoards.find((item) => item.id === "m5tough");
  assert.equal(stack.devices.lcd.selected_by, "runtime");
  assert.equal(tough.devices.lcd.selected_by, "runtime");
  assert.deepEqual(stack.revisions.map((revision) => revision.id), [
    "pre_v2_6_basic", "pre_v2_6_gray", "pre_2020_04_fire", "from_2020_04_to_v2_5_fire",
    "v2_6_plus_basic", "v2_6_plus_gray", "v2_6_plus_fire",
  ]);
  assert.equal(tough.revisions, undefined);
  assert.deepEqual(resolveCatalog(stack).map((item) => item.filename), stack.revisions.flatMap((revision) => [
    `m5stack@${revision.id}+lcd=tn.json`, `m5stack@${revision.id}+lcd=ips.json`,
  ]));
  assert.deepEqual(resolveCatalog(tough).map((item) => item.filename), ["m5tough+lcd=ili9342c.json", "m5tough+lcd=ili9342e.json"]);
  assert.equal(validateTarget(stack, target).some((item) => item.id === "E_TGT_CHOICE_UNREPRESENTED"), false);
  assert.equal(validateTarget(tough, target).some((item) => item.id === "E_TGT_CHOICE_UNREPRESENTED"), false);
});

test("runtime choices expand beside revisions", () => {
  const value = clone(board);
  value.devices.imu.selected_by = "runtime";
  delete value.revisions[3].select.imu;
  const outputs = resolveCatalog(value);
  assert.equal(outputs.length, 8);
  assert.ok(outputs.some((output) => output.filename === "m5stack_core2@v1_0+imu=bmi270.json"));
});

test("source device signals are rejected as derived fields", () => {
  const value = fixture((item) => { item.devices.lcd.signals = {}; });
  assert.deepEqual(validateBoard(value, context).filter((item) => item.severity !== "warning").map((item) => item.id), ["E_UNKNOWN_KEY"]);
});

test("schema validator rejects missing required board fields", () => {
  for (const key of ["schema_version", "name"]) {
    const value = clone(board);
    delete value[key];
    assert.ok(validateBoard(value, context).some((item) => item.id === "E_SCHEMA_REQUIRED" && item.path === `/${key}`));
  }
});

test("schema validator rejects board and bus scalar type mismatches", () => {
  const legacy = clone(board);
  legacy.legacy_board_id = "bad";
  assert.ok(validateBoard(legacy, context).some((item) => item.id === "E_SCHEMA_TYPE" && item.path === "/legacy_board_id"));
  const frequency = clone(board);
  frequency.buses.main_spi.freq = "fast";
  assert.ok(validateBoard(frequency, context).some((item) => item.id === "E_SCHEMA_TYPE" && item.path === "/buses/main_spi/freq"));
});

test("part spec definitions reject numeric strings in devices and choices", () => {
  const device = clone(board);
  device.devices.touch.spec.x_max = "319";
  assert.ok(validateBoard(device, context).some((item) => item.id === "E_SCHEMA_TYPE" && item.path === "/devices/touch/spec/x_max"));
  const choice = clone(board);
  choice.devices.lcd.choices.ili9342e.spec = { width: "320" };
  assert.ok(validateBoard(choice, context).some((item) => item.id === "E_SCHEMA_TYPE" && item.path === "/devices/lcd/choices/ili9342e/spec/width"));
});

test("E_VERIFIED_FAKE", () => assertSingle("E_VERIFIED_FAKE", fixture((value) => { value.pins[1].verified = { note: "generated" }; })));

test("E_VERIFIED_KEY", () => assertSingle("E_VERIFIED_KEY", fixture((value) => { value.pins[1].verified = { ghost: "datasheet" }; })));

test("E_MEASURED_BY", () => assertSingle("E_MEASURED_BY", fixture((value) => { value.pins[1].verified = { note: "measured" }; })));

test("E_CONN_TYPE_MISSING", () => assertSingle("E_CONN_TYPE_MISSING", fixture((value) => { value.connectors.port_a.type = "missing_type"; })));

test("E_CONN_POS_UNKNOWN", () => assertSingle("E_CONN_POS_UNKNOWN", fixture((value) => { value.connectors.port_a.positions = { 9: "nc" }; })));

test("E_CONN_POS_MISSING", () => assertSingle("E_CONN_POS_MISSING", fixture((value) => {
  value.pins[32].roles = value.pins[32].roles.filter((role) => role !== "conn:port_a.2");
})));

test("E_CONN_POS_DUP", () => assertSingle("E_CONN_POS_DUP", fixture((value) => { value.pins[1].roles.push("conn:port_a.1"); })));

test("E_CONN_POS_GPIO", () => assertSingle("E_CONN_POS_GPIO", fixture((value) => {
  value.pins[33].roles = value.pins[33].roles.filter((role) => role !== "conn:port_a.1");
  value.connectors.port_a.positions = { 1: "gpio:33" };
})));

test("E_CONN_POS_FIXED", () => assertSingle("E_CONN_POS_FIXED", fixture((value) => { value.connectors.port_a.positions = { 4: "pwr:gnd" }; })));

test("E_RAIL_UNKNOWN", () => assertSingle("E_RAIL_UNKNOWN", fixture((value) => { value.connectors.port_a.positions = { 3: "pwr:mystery" }; })));

test("E_CONN_POS_ENDPOINT", () => assertSingle("E_CONN_POS_ENDPOINT", fixture((value) => { value.connectors.port_a.positions = { 3: "none" }; })));

test("multi_gpio permits a declared physical fan-out", () => {
  const value = fixture((item) => {
    item.connectors.port_a.multi_gpio = ["1"];
    item.pins[1].roles.push("conn:port_a.1");
  });
  assert.deepEqual(validateBoard(value, context).filter((item) => item.severity !== "warning"), []);
});

test("shared_gpio permits one GPIO on multiple physical connector positions", () => {
  const source = catalogBoards.find((item) => item.id === "m5stack_corematrix");
  const resolved = resolveAll(source, connectorTypes, { chip: chipC61, parts })[0].board;
  assert.deepEqual(validateBoard(resolved, { ...context, chip: chipC61, resolved: true }).filter((item) => item.severity !== "warning"), []);
  const withoutException = clone(resolved);
  delete withoutException.connectors.mbus.shared_gpio;
  assert.equal(validateBoard(withoutException, { ...context, chip: chipC61, resolved: true })
    .filter((item) => item.id === "E_CONN_SAME_GPIO").length, 2);
});

test("W_CONN_MULTI_UNUSED is a warning only", () => {
  const value = fixture((item) => { item.connectors.port_a.multi_gpio = ["1"]; });
  assert.deepEqual(validateBoard(value, context).filter((item) => item.id === "W_CONN_MULTI_UNUSED").map((item) => [item.id, item.severity]), [["W_CONN_MULTI_UNUSED", "warning"]]);
});

test("W_CONN_SHARED_UNUSED is a warning only", () => {
  const source = catalogBoards.find((item) => item.id === "m5stack_corematrix");
  const resolved = resolveAll(source, connectorTypes, { chip: chipC61, parts })[0].board;
  resolved.connectors.mbus.shared_gpio.push("2");
  assert.deepEqual(validateBoard(resolved, { ...context, chip: chipC61, resolved: true })
    .filter((item) => item.id === "W_CONN_SHARED_UNUSED").map((item) => [item.id, item.severity]),
    [["W_CONN_SHARED_UNUSED", "warning"]]);
});

test("connector type inheritance validates independently", () => {
  const parent = { id: "hat", name: "Hat", layout: { rows: 1, cols: 1 }, positions: [{ id: "1", row: 0, col: 0 }], fixed_positions: { 1: "pwr:gnd" } };
  const child = { id: "hat_child", name: "Hat child", extends: "hat", layout: { rows: 1, cols: 2 }, positions: [{ id: "1", row: 0, col: 0 }, { id: "2", row: 0, col: 1 }], fixed_positions: { 1: "pwr:gnd" } };
  assert.deepEqual(validateConnectorTypes({ hat: parent, hat_child: child }), []);
  assert.equal(isCompatible({ hat: parent, hat_child: child }, "hat_child", "hat"), true);
  const broken = clone(child);
  broken.positions.shift();
  broken.fixed_positions[1] = "pwr:5v";
  assert.ok(validateConnectorTypes({ hat: parent, hat_child: broken }).some((item) => item.id === "E_CTYPE_EXTENDS"));
});

test("connector type catalog validation does not require a board", () => {
  const broken = clone(connectorTypes);
  broken.mbus.positions[1].row = broken.mbus.positions[0].row;
  broken.mbus.positions[1].col = broken.mbus.positions[0].col;
  assert.ok(validateConnectorTypes(broken).some((item) => item.id === "E_CTYPE_FORMAT"));
});

test("all resolved connectors have every catalog position", () => {
  for (const output of catalogBoards.flatMap(resolveCatalog)) {
    for (const connector of Object.values(output.board.connectors ?? {})) {
      assert.deepEqual(Object.keys(connector.positions).sort(), connectorTypes[connector.type].positions.map((position) => position.id).sort(), output.filename);
    }
  }
});

test("E_UNKNOWN_KEY", () => assertSingle("E_UNKNOWN_KEY", fixture((value) => { value.unexpected = true; })));

test("E_FORMAT", () => {
  let first = true;
  const unstable = () => {
    const output = first ? "{}\n" : "{ }\n";
    first = false;
    return output;
  };
  assert.deepEqual(validateFormat({}, unstable).map((item) => item.id), ["E_FORMAT"]);
});

test("E_ID_FORMAT rejects invalid IDs", () => {
  const errors = validateBoard(fixture((value) => { value.id = "M5Stack-Core2"; }), context);
  assert.ok(errors.some((item) => item.id === "E_SCHEMA_PATTERN"));
  assert.ok(errors.some((item) => item.id === "E_ID_FORMAT"));
});

test("E_ID_FORMAT rejects duplicate legacy IDs across boards", () => {
  const second = fixture((value) => { value.id = "another_board"; });
  const errors = validateCatalog([board, second], () => context).filter((item) => item.id === "E_ID_FORMAT");
  assert.equal(errors.length, 1);
  assert.match(errors[0].message, /duplicates/);
});

test("C3 accessory catalog validates and base boards contain no Bottom wiring", () => {
  for (const accessory of Object.values(accessories)) assert.deepEqual(validateAccessory(accessory, { connectorTypes, parts }), [], accessory.id);
  const removed = {
    m5stack: ["port_b", "port_c", "port_d", "port_e", "rgb_led"],
    m5stack_core2: ["port_b", "port_c", "port_d", "port_e", "bottom_led"],
    m5tough: ["port_a", "port_b", "port_c"],
  };
  for (const [id, names] of Object.entries(removed)) {
    const source = catalogBoards.find((item) => item.id === id);
    const roles = Object.values(source.pins).flatMap((pin) => pin.roles);
    for (const name of names) {
      assert.equal(source.connectors?.[name], undefined, `${id} connector ${name}`);
      assert.equal(source.devices?.[name], undefined, `${id} device ${name}`);
      assert.equal(roles.some((role) => role.includes(`:${name}.`)), false, `${id} role ${name}`);
    }
  }
});

test("default compositions preserve M5Unified pin-table values and provenance", () => {
  const expected = {
    m5stack_core2: { port_a_pin1: 33, port_a_pin2: 32, port_b_pin1: 36, port_b_pin2: 26, port_c_pin1: 13, port_c_pin2: 14, port_d_pin1: 34, port_d_pin2: 35, port_e_pin1: 27, port_e_pin2: 19, rgb_led: 25 },
    m5stack: { port_a_pin1: 22, port_a_pin2: 21, port_b_pin1: 36, port_b_pin2: 26, port_c_pin1: 16, port_c_pin2: 17, port_d_pin1: 34, port_d_pin2: 35, port_e_pin1: 5, port_e_pin2: 13, rgb_led: 15 },
    m5tough: { port_a_pin1: 33, port_a_pin2: 32, port_b_pin1: 36, port_b_pin2: 26, port_c_pin1: 13, port_c_pin2: 14 },
  };
  for (const [id, pins] of Object.entries(expected)) {
    const source = catalogBoards.find((item) => item.id === id);
    const resolved = resolveCatalog(source)[0].board;
    const actual = Object.fromEntries(Object.entries(pintableAssignments(resolved)).filter(([name]) => name === "rgb_led" || name.startsWith("port_")));
    assert.deepEqual(actual, pins, id);
    assert.ok(Object.values(resolved.pins).flatMap((pin) => pin.roles).some((role) => role.startsWith(`${compositionTarget.compositions[id].default.accessories[0].id}/`)));
  }
});

test("default composition leaves base GPIO rows distinguishable from accessory rows", () => {
  const source = catalogBoards.find((item) => item.id === "m5stack");
  const resolved = resolveCatalog(source)[0].board;
  assert.deepEqual(resolved.pins[14], source.pins[14], "LCD GPIO remains the base row");
  assert.notDeepEqual(resolved.pins[36], source.pins[36], "PORT.B GPIO is extended by the accessory");
  assert.ok(resolved.pins[36].roles.some((role) => role.startsWith("m5go_bottom/")));
});

test("Tough RS485 positions are owned by the transceiver, not the SoC", () => {
  const source = catalogBoards.find((item) => item.id === "m5tough");
  const resolved = resolveCatalog(source)[0].board;
  assert.equal(resolved.connectors.rs485.positions[1], "pin:rs485_xcvr.b");
  assert.equal(resolved.connectors.rs485.positions[2], "pin:rs485_xcvr.a");
  assert.equal(resolved.connectors.rs485.positions[3], "pwr:vin");
  assert.equal(resolved.devices.rs485_xcvr.part, "sp485e");
  assert.equal(resolved.devices.dcdc_in.part, "me3116");
  assert.equal(resolved.buses.uart_rs485.signals.join(","), "tx,rx");
  assert.equal(resolved.devices.rs485_xcvr.pins.a.roles[0], "tough_subboard/conn:rs485.2");
  assert.equal(resolved.devices.rs485_xcvr.pins.b.roles[0], "tough_subboard/conn:rs485.1");
  assert.equal(resolved.connectors.reset_port.positions[1], "chip:en");
  assert.equal(resolved.connectors.reset_port.positions[2], "gpio:25");
  assert.equal(resolved.connectors.reset_port.positions[3], "pwr:5v");
  assert.equal(resolved.connectors.reset_port.positions[4], "pwr:gnd");
  assert.ok(resolved.pins[19].roles.includes("tough_subboard/bus:uart_rs485.tx"));
  assert.ok(resolved.pins[27].roles.includes("tough_subboard/bus:uart_rs485.rx"));
});

test("ExtPort user setting reroutes a connector and catches same-GPIO choices", () => {
  const source = catalogBoards.find((item) => item.id === "m5stack_core2");
  const host = resolveAll(source, connectorTypes, { chip, parts })[0].board;
  const changed = compose(host, [{ id: "extport_core2", use: ["port_d"], settings: { port_d_1: "g22" } }], { accessories, connectorTypes });
  assert.deepEqual(changed.issues, []);
  assert.equal(changed.board.pins[22].roles.includes("extport_core2/conn:port_d.1"), true);

  const collision = resolveAll(source, connectorTypes, {
    chip, parts, accessories,
    composition: { accessories: [{ id: "extport_core2", use: ["port_e"], settings: { port_e_1: "g2", port_e_2: "g2" } }] },
  })[0].board;
  assert.ok(validateBoard(collision, { ...context, resolved: true }).some((item) => item.id === "E_CONN_SAME_GPIO"));
});

test("composition validation reports C3 contract errors", () => {
  const source = catalogBoards.find((item) => item.id === "m5stack_core2");
  const host = resolveAll(source, connectorTypes, { chip, parts })[0].board;
  const ids = (entries, catalogs = {}) => validateComposition(host, entries, { accessories, connectorTypes, ...catalogs }).map((item) => item.id);
  assert.ok(ids([{ id: "m5go_bottom2" }, { id: "m5go_bottom2", as: { port_b: "port_b_2", port_c: "port_c_2", rgb_led: "rgb_led_2" } }]).includes("E_COMP_BOTTOM_DUP"));
  assert.ok(ids([{ id: "m5go_bottom" }]).includes("E_COMP_INCOMPATIBLE"));
  assert.ok(ids([{ id: "m5go_bottom2" }, { id: "extport_core2" }]).includes("E_COMP_CONN_DUP"));
  assert.ok(ids([{ id: "extport_core2", settings: { missing: "g1" } }]).includes("E_COMP_SETTING_UNKNOWN"));
  assert.ok(ids([{ id: "m5basic_base_lite" }], { allow_origins: ["official"] }).includes("E_COMP_ORIGIN"));

  const required = clone(accessories.extport_core2);
  required.settings.port_d_1.default = null;
  assert.ok(validateComposition(host, [{ id: required.id }], { accessories: { ...accessories, [required.id]: required }, connectorTypes }).some((item) => item.id === "E_COMP_SETTING_REQUIRED"));
  const bad = clone(accessories.extport_core2);
  bad.host_pins["mbus.1"] = { roles: ["conn:port_b.1"] };
  assert.ok(validateAccessory(bad, { connectorTypes }).some((item) => item.id === "E_ACC_HOST_POS") === false);
  const composedBad = compose(host, [{ id: bad.id }], { accessories: { ...accessories, [bad.id]: bad }, connectorTypes });
  assert.ok(composedBad.issues.some((item) => item.id === "E_ACC_HOST_POS"));
});

test("gpio_power keeps the active-high hold level for existing members", () => {
  for (const source of [d0wdq6Source, esp32s3Source]) {
    const users = [...source.matchAll(/gpio_power(_low)?\(wiring::([a-z0-9_]+)::power_gpio\)/g)];
    for (const [, low, board] of users) {
      if (["papers3"].includes(board)) assert.equal(low, "_low", board);
      else assert.equal(low, undefined, board);
    }
  }
  assert.match(d0wdq6Source, /static_assert\(desc_paper\.power\.hold_high/);
  assert.match(esp32s3Source, /static_assert\(desc_airq\.power\.hold_high/);
});

test("no_display_pins is limited to non-GPIO display buses", () => {
  const allowed = new Set(["papers3", "paperdiy", "corematrix", "corep4x", "tab5", "tab5x"]);
  for (const source of [d0wdq6Source, esp32s3Source, esp32c61Source, esp32p4Source]) {
    const descs = [...source.matchAll(/static constexpr board_desc_t desc_([a-z0-9_]+) = \{([\s\S]*?)\n  \};/g)];
    assert.ok(descs.length > 0);
    for (const [, name, body] of descs) {
      assert.equal(body.includes("no_display_pins()"), allowed.has(name), name);
      assert.equal(/(?<!no_)display_pins\(|\.display/.test(body), !allowed.has(name), name);
    }
  }
});

test("CoreP4X generated DSI setup preserves the legacy fields", async () => {
  const source = catalogBoards.find((item) => item.id === "m5stack_corep4x");
  const resolved = resolveCatalog(source)[0].board;
  const specs = emitM5GFXSpecs(source, [resolved], parts, m5gfxBoardMapping(target, source.id));
  const wiring = emitM5GFXWiring(resolved, parts, target);
  assert.deepEqual(specs.bus, { kind: "dsi", busId: 0, laneNum: 2, laneMbps: 600, ldoChanId: 3, ldoVoltageMv: 2500 });
  assert.equal(specs.panels.st7102.dpi_freq_mhz, 24);
  assert.equal(specs.panels.st7102.vsync_front_porch, 200);
  assert.deepEqual(wiring.i2c, { sda: 11, scl: 9, port: 1 });
  assert.equal(specs.touch.intPin, 1);
  assert.deepEqual(detectionPinsForEntries([{ board: source, chip: chipP4, emitted: wiring }]), [9, 11]);
  assert.match(esp32p4Source, /probe_i2c_read[\s\S]*?pm1_i2c_addr[\s\S]*?200[\s\S]*?probe_i2c_read[\s\S]*?ioe1_i2c_addr[\s\S]*?200/);
  const main = await fs.readFile(path.join(root, "../../src/M5GFX.cpp"), "utf8");
  assert.match(main, /pkg_ver == 0[\s\S]*?try_setup_detected\(board_detect::m5::esp32p4_detectors/);
});

test("Tab5 touch identities select the three DSI panels", () => {
  const source = catalogBoards.find((item) => item.id === "m5stack_tab5");
  const resolved = resolveCatalog(source).map((item) => item.board);
  const specs = emitM5GFXSpecs(source, resolved, parts, m5gfxBoardMapping(target, source.id));
  const wiring = emitM5GFXWiring(resolved[0], parts, target);
  assert.deepEqual(specs.bus, { kind: "dsi", busId: 0, laneNum: 2, laneMbps: 1040, ldoChanId: 3, ldoVoltageMv: 2500 });
  // The catalog has one board-level DSI bus, but legacy ST7121 hardware alone
  // requires 900 Mbps after runtime touch-FW identification.
  assert.match(tab5SetupSource, /tab5_st7121_lane_mbps = 900/);
  assert.match(tab5SetupSource, /hit_st7121 \? tab5_st7121_lane_mbps : specs::tab5::bus_lane_mbps/);
  assert.match(tab5SetupSource, /fw_version == 1[\s\S]*?hit_st7121 = true[\s\S]*?fw_version == 3[\s\S]*?hit_st7123 = true/);
  assert.match(tab5SetupSource, /if \(!read_st_touch_fw && !found_gt911\)[\s\S]*?delay\(80\)[\s\S]*?i < 3 && !hit_ili9881[\s\S]*?id\[0\] == 0x98 && id\[1\] == 0x81/);
  assert.match(tab5SetupSource, /if \(found_gt911 \|\| hit_ili9881\)[\s\S]*?Panel_ILI9881C/);
  assert.equal(specs.panels.ili9881c.dpi_freq_mhz, 80);
  assert.equal(specs.panels.st7121.dpi_freq_mhz, 70);
  assert.equal(specs.panels.st7123.vsync_back_porch + specs.panels.st7123.vsync_pulse_width, 10);
  assert.equal(specs.panels.st7123.vsync_front_porch, 220);
  assert.deepEqual(specs.backlight, { pin: 22, freq: 44100, channel: 7, invert: false, offset: 0 });
  assert.deepEqual(wiring.i2c, { sda: 31, scl: 32, port: 1 });
  assert.deepEqual(detectionPinsForEntries([{ board: source, chip: chipP4, emitted: wiring }]), [23, 31, 32]);
  assert.match(esp32p4Source, /probe_i2c_bus_present\(ctx, tab5_detail::sda, tab5_detail::scl\)/);
  assert.match(esp32p4Source, /pi4io1_addr[\s\S]*?0x01[\s\S]*?pi4io2_addr[\s\S]*?0x01/);
});

test("every descriptor operation GPIO is inside its execution and rollback scope", async () => {
  const gpioSequences = [...pmicOpsSource.matchAll(
    /static constexpr ops::op_t (\w+)\[] = \{([\s\S]*?)\n  \};/g,
  )].filter(([, , body]) => /ops::gpio_(?:set_mode|write_high|write_low)\(/.test(body));
  assert.deepEqual(gpioSequences.map(([, name]) => name).sort(), [
    "papercolor_power_on", "papermono_power_on", "stopwatch_power_on", "tab5_power_on",
  ]);

  const gpioPins = Object.fromEntries(gpioSequences.map(([, name, body]) => [
    name,
    [...body.matchAll(/ops::gpio_(?:set_mode|write_high|write_low)\((\d+)/g)]
      .map((match) => Number(match[1])),
  ]));
  assert.deepEqual([...new Set(gpioPins.stopwatch_power_on)], [39]);
  assert.deepEqual([...new Set(gpioPins.papermono_power_on)], [16]);
  assert.deepEqual([...new Set(gpioPins.papercolor_power_on)], [44]);
  assert.deepEqual([...new Set(gpioPins.tab5_power_on)], [23]);

  const s3Wiring = parseGeneratedWiring(await fs.readFile(
    path.join(root, "../../src/board_detect/m5/generated/esp32s3_wiring.hpp"), "utf8",
  ));
  for (const [boardName, sequence] of [
    ["stopwatch", "stopwatch_power_on"],
    ["papermono", "papermono_power_on"],
    ["papercolor", "papercolor_power_on"],
  ]) {
    assert.match(esp32s3Source, new RegExp(
      `ops::list\\(pmic_ops::${sequence}\\)[\\s\\S]*?desc_${boardName} = \\{[\\s\\S]*?pins\\(wiring::${boardName}::hold\\)[\\s\\S]*?pins\\(wiring::${boardName}::hold\\)`,
    ));
    for (const pin of gpioPins[sequence]) assert.ok(s3Wiring[boardName].hold.includes(pin), `${boardName} GPIO${pin}`);
  }

  assert.match(esp32p4Source, /prepare_gpio_pins\[] = \{ touch_int \}/);
  assert.equal(parseGeneratedWiring(await fs.readFile(
    path.join(root, "../../src/board_detect/m5/generated/esp32p4_wiring.hpp"), "utf8",
  )).tab5.touch_int, 23);
  for (const desc of ["tab5", "tab5x"]) {
    assert.match(esp32p4Source, new RegExp(
      `desc_${desc} = \\{[\\s\\S]*?no_display_pins\\(\\), no_pins\\(\\),[\\s\\S]*?no_options\\(\\), pins\\(tab5_detail::prepare_gpio_pins\\)`,
    ));
  }
  assert.match(esp32p4Source, /tab5_power_on[\s\S]*?pins\(tab5_detail::prepare_gpio_pins\)/);
  assert.match(pmicOpsSource, /tab5_power_on[\s\S]*?gpio_set_mode\(23, ops::gpio_mode_t::input\)[\s\S]*?delay_ms\(100\)/);
  assert.doesNotMatch(esp32p4Source, /no_display_pins\(\), pins\(tab5_detail::prepare_gpio_pins\)/);
});

test("CoreMatrix generated I2C setup preserves the legacy fields", async () => {
  const source = catalogBoards.find((item) => item.id === "m5stack_corematrix");
  const resolved = resolveCatalog(source)[0].board;
  const specs = emitM5GFXSpecs(source, [resolved], parts, m5gfxBoardMapping(target, source.id));
  const wiring = emitM5GFXWiring(resolved, parts, target);
  // Legacy source: M5GFX.cpp's ESP32-C61 branch declares i2c_freq = 400000
  // and assigns it to both Bus_I2C freq_write and freq_read. Its one-shot
  // TM1680 ACK probe is separate and remains at 100 kHz.
  assert.deepEqual(specs.bus, { kind: "i2c", port: 0, freqWrite: 400000, freqRead: 400000, addr: 0x72, prefixLen: 0 });
  assert.deepEqual(specs.panels.tm1680, {
    width: 16, height: 16, memoryWidth: null, memoryHeight: null,
    offsetX: null, offsetY: null, rotationOffset: 0, invert: null, readable: null,
  });
  assert.deepEqual(wiring.i2c, { sda: 0, scl: 1, port: 0 });
  assert.deepEqual(detectionPinsForEntries([{ board: source, chip: chipC61, emitted: wiring }]), [0, 1]);
  const header = renderM5GFXSpecsHeader(specs);
  assert.match(header, /bus_port = 0;[\s\S]*?bus_freq_write = 400000;[\s\S]*?bus_freq_read = 400000;[\s\S]*?bus_i2c_addr = 0x72;[\s\S]*?bus_prefix_len = 0;/);
  assert.match(esp32c61Source, /probe_i2c_bus_present\(ctx, corematrix_detail::sda, corematrix_detail::scl\)/);
  assert.match(esp32c61Source, /probe_i2c_read[\s\S]*?pm1_i2c_addr[\s\S]*?200[\s\S]*?probe_i2c_read[\s\S]*?ioe1_i2c_addr[\s\S]*?200/);
});

test("PaperS3 and PaperDIY generated setup preserves the legacy fields", async () => {
  const papers3 = catalogBoards.find((item) => item.id === "m5papers3");
  const paperdiy = catalogBoards.find((item) => item.id === "m5paperdiy");
  const s3Resolved = resolveCatalog(papers3)[0].board;
  const diyResolved = resolveCatalog(paperdiy)[0].board;
  const s3Specs = emitM5GFXSpecs(papers3, [s3Resolved], parts, m5gfxBoardMapping(target, papers3.id));
  const diySpecs = emitM5GFXSpecs(paperdiy, [diyResolved], parts, m5gfxBoardMapping(target, paperdiy.id));
  const s3Wiring = emitM5GFXWiring(s3Resolved, parts, target);
  const diyWiring = emitM5GFXWiring(diyResolved, parts, target);
  const epdPins = { data0: 6, data1: 14, data2: 7, data3: 12, data4: 9, data5: 11, data6: 8, data7: 10,
    pwr: 46, spv: 17, ckv: 18, sph: 13, oe: 45, le: 15, cl: 16 };
  const epdPanel = { width: 960, height: 540, memoryWidth: 960, memoryHeight: 540, offsetX: 0, offsetY: 0,
    rotationOffset: 3, invert: null, readable: null, linePadding: 8 };
  for (const [specs, wiring] of [[s3Specs, s3Wiring], [diySpecs, diyWiring]]) {
    assert.deepEqual(specs.bus, { kind: "parallel_epd", speed: 16000000, width: 8 });
    assert.deepEqual(specs.panels.ed047tc1, epdPanel);
    assert.deepEqual(wiring.display, epdPins);
    assert.deepEqual(wiring.i2c, { sda: 41, scl: 42, port: 1 });
    assert.deepEqual(wiring.hold, []);
    assert.equal(wiring.sharedSd, null);
  }
  assert.deepEqual(s3Specs.touch, { i2cAddr: null, i2cFreq: 400000, xMin: 0, xMax: 539, yMin: 0, yMax: 959, rotationOffset: 1 });
  assert.deepEqual(s3Specs.powerHold, { activeLow: true });
  assert.equal(s3Specs.pmic, null);
  assert.equal(s3Wiring.powerGpio, 44);
  assert.equal(s3Wiring.touchInt, 48);
  assert.deepEqual(diySpecs.pmic, { i2cAddr: 0x6E, i2cFreq: 100000, idReg: 0x00 });
  assert.equal(diySpecs.touch, null);
  assert.equal(diySpecs.powerHold, null);
  assert.equal(diyWiring.powerGpio, -1);
  const s3Header = renderM5GFXSpecsHeader(s3Specs);
  assert.match(s3Header, /bus_speed = 16000000;\nconstexpr std::uint8_t bus_width = 8;/);
  assert.match(s3Header, /line_padding = 8;/);
  assert.match(s3Header, /namespace power_hold \{\n  constexpr bool active_low = true;/);
  assert.doesNotMatch(s3Header, /touch \{[\s\S]*?i2c_addr/);
  assert.doesNotMatch(s3Header, /bus_host|bus_three_wire/);

  const wiringHeader = await fs.readFile(path.join(root, "../../src/board_detect/m5/generated/esp32s3_wiring.hpp"), "utf8");
  const unconditional = /unconditional_pins\[\] = \{ ([^}]*) \}/.exec(wiringHeader)[1].split(", ").map(Number);
  assert.deepEqual(unconditional, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48]);
  for (const pin of [...Object.values(epdPins), 41, 42, 44, 48]) {
    assert.ok(unconditional.includes(pin), `GPIO ${pin}`);
    assert.ok(!chipS3.reserved_conditional.opi.includes(pin), `GPIO ${pin} is an OPI pin`);
  }
  assert.match(wiringHeader, /namespace papers3 \{[\s\S]*?touches_opi_pins = false;/);
  assert.match(wiringHeader, /namespace paperdiy \{[\s\S]*?touches_opi_pins = false;/);

  const detector = esp32s3Source;
  assert.match(detector, /desc_papers3 = \{[\s\S]*?gpio_power_low\(wiring::papers3::power_gpio\), no_reset\(\), no_shared_sd\(\),\n\s*no_display_pins\(\), no_pins\(\)/);
  assert.match(detector, /desc_paperdiy = \{[\s\S]*?i2c_power_confirmed\(specs::paperdiy::pmic::i2c_freq, paperdiy_pmic_variants,\n\s*pmic_ops::pm1_devices\)[\s\S]*?internal_i2c\(wiring::paperdiy::internal_i2c_sda, wiring::paperdiy::internal_i2c_scl, -1\)/);
  // PM1 first (skipped only when hinted PaperS3), then GT911 (skipped only when hinted PaperDIY).
  const confirm = /class paper_family_detector_t[\s\S]*?bool confirm[\s\S]*?\n  private:/.exec(detector)[0];
  assert.match(confirm, /ctx\.hint != desc_papers3\.def\.id[\s\S]*?probe_i2c_read\([\s\S]*?pmic_ops::pm1_i2c_addr, 0, pm1_id, sizeof\(pm1_id\),\n\s*pmic_ops::pm1_i2c_freq, 200\)[\s\S]*?== pmic_ops::pm1_device_id[\s\S]*?assign\(&desc_paperdiy\)[\s\S]*?ctx\.hint != desc_paperdiy\.def\.id[\s\S]*?gt911_addresses\[\] = \{ 0x14, 0x5D \}[\s\S]*?gt911_product_id_reg = 0x8140[\s\S]*?probe_i2c_read\([\s\S]*?specs::papers3::touch::i2c_freq, 0, true\)[\s\S]*?'9'[\s\S]*?assign\(&desc_papers3\)/);
  assert.match(detector, /paper_family_detector_t::members_\[\] = \{\n\s*&desc_paperdiy.def, &desc_papers3.def, nullptr,/);
  assert.match(detector, /&pm1_ext_family_detector, &paper_family_detector,/);
  assert.match(detector, /\{ &desc_papers3, construct_papers3, "board_M5PaperS3", nullptr \},\n\s*\{ &desc_paperdiy, construct_paperdiy, "board_M5PaperDIY", nullptr \},/);

  const setup = await fs.readFile(esp32s3SetupPath, "utf8");
  assert.match(setup, /panel_papers3 =[\s\S]*?with_memory\(specs::papers3::panel_ed047tc1::memory_width[\s\S]*?with_bus_shared\(false\);/);
  assert.match(setup, /touch_papers3 =\n\s*i2c_touch_default_addr\(\)/);
  assert.match(setup, /construct_papers3[\s\S]*?make_epd_panel\(panel_papers3, specs::papers3::panel_ed047tc1::line_padding[\s\S]*?make_i2c_touch<lgfx::Touch_GT911>\(touch_papers3\)/);
  assert.match(setup, /construct_paperdiy[\s\S]*?make_epd_panel\(panel_paperdiy, specs::paperdiy::panel_ed047tc1::line_padding/);
  assert.doesNotMatch(setup, /construct_paperdiy[\s\S]*?make_i2c_touch[\s\S]*?construct_status_t setup_detected_board/);
  assert.match(detector, /esp32s3_detectors_qfn56\[\][\s\S]*?&pm1_ext_family_detector, &paper_family_detector, &spi_id_detector/);
});

test("CoreS3 family catalog keeps shared wiring and option power variants", async () => {
  const members = ["m5stack_cores3", "m5stack_cores3se", "m5stack_stackchan"]
    .map((id) => catalogBoards.find((item) => item.id === id));
  assert.ok(members.every(Boolean));
  const emitted = members.map((item) => wiringAssignments(resolveCatalog(item)[0].board,
    parts, m5gfxBoardMapping(target, item.id)));
  const shared = Object.fromEntries(Object.entries(emitted[0]).filter(([name]) => !name.startsWith("camera_")));
  assert.deepEqual(emitted[1], shared);
  assert.deepEqual(emitted[2], shared);
  assert.equal(emitted[0].display_miso, emitted[0].display_dc);
  assert.equal(emitted[0].display_miso, emitted[0].shared_sd_miso);
  const core = members[0];
  const releaseRoles = target.boards.m5stack_cores3.release_probe.pins;
  const releasePins = releaseRoles.map((role) => Number(Object.entries(core.pins)
    .find(([, row]) => row.roles.includes(role))[0]));
  assert.deepEqual(releasePins, [38, 39, 40, 41, 42, 46, 47, 48]);

  const coreSource = await fs.readFile(
    path.join(root, "../../src/board_detect/m5/esp32s3/cores3.inl"), "utf8");
  const opsSource = await fs.readFile(
    path.join(root, "../../src/board_detect/m5/pmic_ops.hpp"), "utf8");
  assert.match(coreSource, /pmic_variant_confirmed_option[\s\S]*?vbus_5v, 0\)[\s\S]*?pmic_variant_confirmed_option[\s\S]*?vbus_5v, vbus_5v\)/);
  assert.match(coreSource, /rollback restores GPIO and chip selects only[\s\S]*?AW9523\/AXP2101 writes are intentionally not rolled back/);
  assert.match(coreSource, /result->refine = cores3_detail::refine/);
  assert.match(coreSource, /bool signature\(probe_ctx_t& ctx\) const override\n\s*\{\n\s*\/\/[\s\S]*?if \(ctx\.conditional_pins_unavailable\) \{ return false; \}[\s\S]*?probe_i2c_ack/);
  assert.match(coreSource, /probe_i2c_read[\s\S]*?pmic::id_reg[\s\S]*?pmic::id_value[\s\S]*?probe_i2c_read[\s\S]*?i2c_io_expander::id_reg[\s\S]*?i2c_io_expander::id_value/);
  assert.match(coreSource, /i2c_camera::i2c_addr[\s\S]*?i2c_camera::id_reg[\s\S]*?i2c_camera::id_value/);
  assert.match(coreSource, /One post-power read is deliberately retained[\s\S]*?camera_id\(probe\)/);
  assert.match(coreSource, /camera_id\(ctx\)[\s\S]*?probe_dedicated_pin_release[\s\S]*?RELEASE_AMBIGUOUS[\s\S]*?biasing toward camera family/);
  assert.match(coreSource, /release_was_unavailable[\s\S]*?result\.assign\(&desc_cores3se\)/);
  assert.match(coreSource, /!release\.available[\s\S]*?release_probe_unavailable/);
  assert.match(coreSource, /internal_camera_confirmed[\s\S]*?if \(!confirmed_before_power\)/);
  assert.match(coreSource, /i2c_stackchan_ioe::i2c_addr[\s\S]*?i2c_stackchan_ioe::firmware_reg[\s\S]*?i2c_stackchan_ioe::firmware_min/);
  const generatedSpecs = await fs.readFile(
    path.join(root, "../../src/board_detect/m5/generated/esp32s3_specs.hpp"), "utf8");
  assert.match(generatedSpecs, /namespace i2c_io_expander \{[\s\S]*?i2c_addr = 0x58;[\s\S]*?id_reg = 0x10;[\s\S]*?id_value = 0x23;/);
  assert.match(generatedSpecs, /namespace i2c_camera \{[\s\S]*?i2c_addr = 0x21;[\s\S]*?id_reg = 0x0;[\s\S]*?id_value = 0x9B;/);
  assert.match(generatedSpecs, /namespace i2c_stackchan_ioe \{[\s\S]*?i2c_addr = 0x6F;[\s\S]*?i2c_freq = 100000;[\s\S]*?firmware_reg = 0x2;[\s\S]*?firmware_min = 0x4;/);
  assert.match(generatedSpecs, /namespace release_probe \{[\s\S]*?pins\[] = \{ 38, 39, 40, 41, 42, 46, 47, 48 \};[\s\S]*?reads = 256;[\s\S]*?samples = 9;[\s\S]*?settle_us = 10;[\s\S]*?short_max_ns = 260;[\s\S]*?long_min_ns = 330;/);
  const generatedWiring = await fs.readFile(
    path.join(root, "../../src/board_detect/m5/generated/esp32s3_wiring.hpp"), "utf8");
  assert.match(generatedWiring, /namespace cores3 \{[\s\S]*?camera_href = 38;[\s\S]*?camera_d7 = 47;[\s\S]*?camera_d6 = 48;/);
  assert.match(opsSource, /cores3_vbus_off_power_on\[] = \{[\s\S]*?cores3_vbus_5v_power_on\[] = \{/);
  const coreSetup = await fs.readFile(
    path.join(root, "../../src/board_detect/m5/esp32s3/cores3_setup.inl"), "utf8");
  assert.match(coreSource, /refine_panel[\s\S]*?soft_spi_read32[\s\S]*?identify_panel_variant[\s\S]*?cores3::lcd_e/);
  assert.match(coreSetup, /make_spi_bus\(bus_cores3\)[\s\S]*?result\.option & generated_options::cores3::lcd_e/);
  const detectorSource = await fs.readFile(
    path.join(root, "../../src/board_detect/board_detect.inl"), "utf8");
  assert.match(detectorSource, /beginRead[\s\S]*?pin_miso_ == pin_dc_[\s\S]*?pin_mode_t::input/);
  assert.match(detectorSource, /endRead[\s\S]*?pin_miso_ == pin_dc_[\s\S]*?pin_mode_t::output/);
  const main = await fs.readFile(path.join(root, "../../src/M5GFX.cpp"), "utf8");
  assert.match(main, /assigned before prepare\/refine[\s\S]*?representative family ID[\s\S]*?setup_board = static_cast<board_t>\(result\.def->id\)/);
  assert.match(main, /case 0: detectors = board_detect::m5::esp32s3_detectors_qfn56;[\s\S]*?case 1: detectors = board_detect::m5::esp32s3_detectors_lga56;[\s\S]*?try_setup_detected\(detectors, board/);
});
