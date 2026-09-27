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
import { validateChoices } from "../lib/choices.js";
import { compose, validateAccessory, validateComposition } from "../lib/compose.js";
import { isCompatible, validateConnectorTypes } from "../lib/ctypes.js";
import { deriveSd } from "../lib/derive/sd.js";
import { consumesPinTableRole, emitPinTable, PIN_NAMES, pinNameForRole } from "../lib/emit/m5unified_pin_table.js";
import { emitM5GFXWiring, renderM5GFXWiringHeader, selectM5GFXWiringBoards, wiringFieldsForRole } from "../lib/emit/m5gfx_board_wiring.js";
import { formatBoard } from "../lib/format.js";
import { clone } from "../lib/model.js";
import { isGeneratedField, pintableAssignments } from "../lib/pintable_roles.js";
import { validateOwners } from "../lib/owners.js";
import { validatePartCatalog, validateParts } from "../lib/parts.js";
import { resolveAll, resolveBoard } from "../lib/resolve.js";
import { validateTarget } from "../lib/targets.js";
import { validateBoard, validateCatalog, validateFormat, validateResolvedVariants } from "../lib/validate.js";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const board = JSON.parse(await fs.readFile(path.join(root, "boards/m5stack_core2.json"), "utf8"));
const schema = JSON.parse(await fs.readFile(path.join(root, "schema/board.schema.json"), "utf8"));
const chip = JSON.parse(await fs.readFile(path.join(root, "chips/esp32_d0wdq6.json"), "utf8"));
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
const i18nMatch = /<script type="application\/json" id="board-spec-i18n">([\s\S]*?)<\/script>/.exec(editorHtml);
const messages = JSON.parse(i18nMatch[1]);
const compositionTarget = targets.m5unified_pin_table;
const resolveCatalog = (item) => resolveAll(item, connectorTypes, {
  chip, parts, accessories,
  composition: compositionTarget.compositions?.[item.id]?.default,
  allow_origins: compositionTarget.allow_origins,
});

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
  assert.deepEqual(validateBoard(board, context).map((item) => item.id), ["W_TGT_REV_INDISTINGUISHABLE"]);
});

test("all six D0WDQ6 boards validate", () => {
  assert.deepEqual(catalogFiles, ["m5paper.json", "m5stack.json", "m5stack_core2.json", "m5station.json", "m5timercam.json", "m5tough.json"]);
  assert.deepEqual(validateCatalog(catalogBoards, () => context).filter((item) => item.severity !== "warning"), []);
});

test("all 11 revision combinations match snapshots", async () => {
  const outputs = catalogBoards.flatMap(resolveCatalog);
  assert.equal(outputs.length, 11);
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
  const referenced = new Set([
    ...[...editorSource.matchAll(/\bt\("([^"]+)"/g)].map((match) => match[1]),
    ...[...editorHtml.matchAll(/data-i18n(?:-aria-label)?="([^"]+)"/g)].map((match) => match[1]),
  ]);
  assert.deepEqual([...referenced].filter((key) => !messages[key]), []);
  assert.equal(/[ぁ-んァ-ン一-龯]/.test(editorSource), false, "editor UI source has no hard-coded Japanese");

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
    m5paper: { sd_mmc_clk: 14, sd_mmc_cmd: 12, sd_mmc_d0: 13, sd_mmc_d3: 4 },
    m5stack: { sd_mmc_clk: 18, sd_mmc_cmd: 23, sd_mmc_d0: 19, sd_mmc_d3: 4 },
    m5stack_core2: { sd_mmc_clk: 18, sd_mmc_cmd: 23, sd_mmc_d0: 38, sd_mmc_d3: 4 },
    m5tough: { sd_mmc_clk: 18, sd_mmc_cmd: 23, sd_mmc_d0: 38, sd_mmc_d3: 4 },
  };
  for (const source of catalogBoards.filter((item) => item.devices.sd)) {
    const actual = Object.fromEntries(Object.entries(pintableAssignments(source)).filter(([name]) => name.startsWith("sd_mmc_")));
    assert.deepEqual(actual, expected[source.id], source.id);
    for (const output of resolveCatalog(source)) {
      assert.deepEqual(output.board.devices.sd.derived, { modes: ["spi"], bus: "main_spi", cs: "gpio:4" }, output.filename);
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

test("M5Unified live pin tables match generated values", async (t) => {
  const compiler = process.env.CXX || "c++";
  const probe = spawnSync(compiler, ["--version"], { encoding: "utf8" });
  if (probe.error?.code === "ENOENT") return t.skip(`C++ compiler not found (${compiler})`);
  const m5unified = process.env.M5UNIFIED_PATH || path.resolve(root, "../../../M5Unified");
  try { await fs.access(path.join(m5unified, "src/M5Unified.inl")); }
  catch { return t.skip(`M5Unified checkout not found (${m5unified})`); }
  const compared = spawnSync(process.execPath, ["cli/spec.js", "compare-pintable", "--m5unified", m5unified], { cwd: root, encoding: "utf8" });
  assert.equal(compared.status, 0, compared.stderr || compared.stdout);
  assert.match(compared.stdout, /comparison passed: 6 board\(s\), 300 value\(s\)/);
});

test("M5GFX wiring emitter maps every board-description GPIO", () => {
  const expected = {
    m5station: { display: [18, 23, -1, 19, 5, 15, -1], sd: null, i2c: [21, 22, 1], reset: 15, power: -1, hold: [5] },
    m5stack_core2: { display: [18, 23, 38, 15, 5, -1, -1], sd: [18, 23, 38, 4, 5], i2c: [21, 22, 1], reset: -1, power: -1, hold: [4, 5] },
    m5tough: { display: [18, 23, 38, 15, 5, -1, -1], sd: [18, 23, 38, 4, 5], i2c: [21, 22, 1], reset: -1, power: -1, hold: [4, 5] },
    m5stack: { display: [18, 23, 19, 27, 14, 33, -1], sd: [18, 23, 19, 4, 14], i2c: [21, 22, 0], reset: 33, power: -1, hold: [4, 14] },
    m5paper: { display: [14, 12, 13, -1, 15, 23, 27], sd: [14, 12, 13, 4, 15], i2c: [21, 22, 1], reset: 23, power: 2, hold: [4, 15] },
  };
  const entries = [];
  for (const source of catalogBoards.filter((item) => expected[item.id])) {
    const resolved = resolveAll(source, connectorTypes, { chip, parts })[0].board;
    const emitted = emitM5GFXWiring(resolved, parts);
    const flattened = {
      display: Object.values(emitted.display),
      sd: emitted.sharedSd && Object.values(emitted.sharedSd),
      i2c: emitted.i2c && Object.values(emitted.i2c),
      reset: emitted.resetGpio,
      power: emitted.powerGpio,
      hold: emitted.hold,
    };
    assert.deepEqual(flattened, expected[source.id], source.id);
    entries.push({ board: source, emitted });
  }
  assert.match(renderM5GFXWiringHeader(entries), /constexpr std::int8_t display_sclk = 18;/);
  assert.deepEqual(wiringFieldsForRole(board, "bus:main_spi.sclk", parts), ["display_sclk", "shared_sd_sclk"]);
  assert.deepEqual(wiringFieldsForRole(board, "dev:lcd.rst", parts), ["display_rst"]);
});

test("M5GFX reset GPIO requires an explicit display-reset declaration", () => {
  const station = catalogBoards.find((item) => item.id === "m5station");
  const resolved = resolveBoard(station, {}, connectorTypes, { chip, parts });
  const mapping = emitM5GFXWiring(resolved, parts).mapping;
  assert.equal(mapping.reset, "display_rst");
  for (const pin of Object.values(resolved.pins)) pin.roles = pin.roles.filter((role) => role !== "dev:lcd.rst");
  assert.throws(() => emitM5GFXWiring(resolved, parts), /reset uses display_rst.*unavailable/);
});

test("M5GFX shared SD requires a derived SPI mode", () => {
  const resolved = resolveAll(board, connectorTypes, { chip, parts })[0].board;
  resolved.devices.sd.derived.modes = resolved.devices.sd.derived.modes.filter((mode) => mode !== "spi");
  assert.throws(() => emitM5GFXWiring(resolved, parts), /SD shares the display bus but does not support SPI/);
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
  assert.throws(() => selectM5GFXWiringBoards(catalogBoards.filter((item) => item.id !== "m5paper")), /m5paper.*found 0/);
  const paper = catalogBoards.find((item) => item.id === "m5paper");
  assert.throws(() => selectM5GFXWiringBoards([...catalogBoards, clone(paper)]), /m5paper.*found 2/);
});

test("M5GFX wiring rejects conflicting assignments to one emitted field", () => {
  const resolved = resolveAll(board, connectorTypes, { chip, parts })[0].board;
  resolved.pins[38].roles = resolved.pins[38].roles.filter((role) => role !== "dev:sd.d0");
  resolved.pins[19].roles.push("dev:sd.d0");
  assert.throws(() => emitM5GFXWiring(resolved, parts), /shared_sd_miso is assigned to both GPIO (38 and GPIO 19|19 and GPIO 38)/);
});

test("M5GFX wiring selects one display device without mixing another display", () => {
  const resolved = resolveAll(board, connectorTypes, { chip, parts })[0].board;
  resolved.devices.aux_panel = { kind: "display", bus: "main_spi" };
  resolved.pins[12].roles.push("dev:aux_panel.cs");
  resolved.pins[13].roles.push("dev:aux_panel.dc");
  const emitted = emitM5GFXWiring(resolved, parts);
  assert.equal(emitted.display.cs, 5);
  assert.equal(emitted.display.dc, 15);
  assert.deepEqual(emitted.hold, [4, 5]);
});

test("M5GFX wiring does not share an SD device on another bus", () => {
  const resolved = resolveAll(board, connectorTypes, { chip, parts })[0].board;
  resolved.devices.sd.bus = "sd_spi";
  resolved.buses.sd_spi = { kind: "spi", signals: ["sclk", "mosi", "miso"] };
  assert.equal(emitM5GFXWiring(resolved, parts).sharedSd, null);
});

test("M5GFX generated I2C host is an integer in the supported range", () => {
  const resolved = resolveAll(board, connectorTypes, { chip, parts })[0].board;
  resolved.buses.internal_i2c.preferred_host = "I2C_NUM_1";
  assert.throws(() => emitM5GFXWiring(resolved, parts), /integer from 0 to 127/);
  resolved.buses.internal_i2c.preferred_host = 128;
  assert.throws(() => emitM5GFXWiring(resolved, parts), /integer from 0 to 127/);
});

test("M5GFX generated namespaces contain only fields consumed by their descriptor", () => {
  const entries = selectM5GFXWiringBoards(catalogBoards).map((source) => ({
    board: source,
    emitted: emitM5GFXWiring(resolveBoard(source, {}, connectorTypes, { chip, parts }), parts),
  }));
  const header = renderM5GFXWiringHeader(entries);
  const scope = (name) => new RegExp(`namespace ${name} \\{([\\s\\S]*?)\\n\\} // namespace ${name}`).exec(header)[1];
  assert.doesNotMatch(scope("station"), /shared_sd_|power_gpio/);
  assert.doesNotMatch(scope("core2"), /reset_gpio|power_gpio/);
  assert.doesNotMatch(scope("tough"), /reset_gpio|power_gpio/);
  assert.doesNotMatch(scope("stack"), /internal_i2c_|power_gpio/);
  assert.doesNotMatch(scope("paper"), /internal_i2c_/);
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

test("E_ROLE_DUP", () => assertSingle("E_ROLE_DUP", fixture((value) => value.pins[1].roles.push("bus:port_a_i2c.sda"))));

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
  assert.ok(errors.some((item) => item.id === "E_VARIANT_PINTABLE"), JSON.stringify(errors, null, 2));
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

test("E_HEX_FORMAT", () => assertSingle("E_HEX_FORMAT", fixture((value) => { value.devices.touch.i2c_addr = 56; })));

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
  const duplicate = clone(target);
  duplicate.options.m5stack_core2.push({ name: "duplicate", bit: 2, select: { pmic: "axp192" } });
  assert.ok(validateTarget(board, duplicate).some((item) => item.id === "E_TGT_SLOT_DUP"));
  duplicate.options.m5stack_core2.push({ name: "unknown", bit: 3, select: { lcd: "missing" } });
  assert.ok(validateTarget(board, duplicate).some((item) => item.id === "E_TGT_SELECT_UNKNOWN"));
  assert.ok(validateTarget(board, target).some((item) => item.id === "W_TGT_REV_INDISTINGUISHABLE"));
  const runtime = clone(board);
  runtime.devices.imu.selected_by = "runtime";
  assert.ok(validateTarget(runtime, target).some((item) => item.id === "W_TGT_SLOT_UNOBSERVED"));
});

test("M5Stack and Tough LCD choices are runtime-selected", () => {
  const stack = catalogBoards.find((item) => item.id === "m5stack");
  const tough = catalogBoards.find((item) => item.id === "m5tough");
  assert.equal(stack.devices.lcd.selected_by, "runtime");
  assert.equal(tough.devices.lcd.selected_by, "runtime");
  assert.equal(stack.revisions, undefined);
  assert.equal(tough.revisions, undefined);
  assert.deepEqual(resolveCatalog(stack).map((item) => item.filename), ["m5stack+lcd=tn.json", "m5stack+lcd=ips.json"]);
  assert.deepEqual(resolveCatalog(tough).map((item) => item.filename), ["m5tough+lcd=ili9342c.json", "m5tough+lcd=ili9342e.json"]);
  assert.equal(validateTarget(stack, target).some((item) => item.id === "W_TGT_SLOT_UNOBSERVED"), false);
  assert.equal(validateTarget(tough, target).some((item) => item.id === "W_TGT_SLOT_UNOBSERVED"), false);
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

test("W_CONN_MULTI_UNUSED is a warning only", () => {
  const value = fixture((item) => { item.connectors.port_a.multi_gpio = ["1"]; });
  assert.deepEqual(validateBoard(value, context).filter((item) => item.id === "W_CONN_MULTI_UNUSED").map((item) => [item.id, item.severity]), [["W_CONN_MULTI_UNUSED", "warning"]]);
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

test("E_ID_FORMAT rejects invalid IDs", () => assertSingle("E_ID_FORMAT", fixture((value) => { value.id = "M5Stack-Core2"; })));

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
