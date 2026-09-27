import assert from "node:assert/strict";
import { promises as fs } from "node:fs";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

import {
  addBus, addChoice, addConnector, addDevice, addOwnerRole, addRole, boardRoleOptions, createBoard, duplicateBoard,
  removeBus, removeConnector, removeDevice, removeRoleFromBoard, setBusField, setDeviceField,
  setDeviceSpec, setPin, setSpec,
} from "../editor/board_ops.js";
import { formatBoard } from "../lib/format.js";
import { resolveBoard } from "../lib/resolve.js";
import { validateBoard } from "../lib/validate.js";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const readJson = async (relative) => JSON.parse(await fs.readFile(path.join(root, relative), "utf8"));
const schema = await readJson("schema/board.schema.json");
const chip = await readJson("chips/esp32s3.json");
const connectorTypes = Object.fromEntries(await Promise.all((await fs.readdir(path.join(root, "connector_types"))).filter((name) => name.endsWith(".json")).map(async (name) => {
  const value = await readJson(`connector_types/${name}`);
  return [value.id, value];
})));
const parts = Object.fromEntries(await Promise.all((await fs.readdir(path.join(root, "parts"))).filter((name) => name.endsWith(".json")).map(async (name) => {
  const value = await readJson(`parts/${name}`);
  return [value.id, value];
})));
const atoms3Text = await fs.readFile(path.join(root, "boards/m5atoms3.json"), "utf8");
const sticks3Text = await fs.readFile(path.join(root, "boards/m5sticks3.json"), "utf8");
const context = { schema, chip, connectorTypes, parts };

function buildAtomS3(onMidway) {
  const board = createBoard({ id: "m5atoms3", name: "M5AtomS3", official_name: "M5Stack AtomS3", official_name_verified: true, aliases: ["atoms3", "atom s3"], legacy_board_id: 11, chip: "esp32s3" });
  setSpec(board, "display.resolution", [128, 128]);
  setSpec(board, "display.touch", false);
  setSpec(board, "storage.psram_mb", 0);
  addBus(board, "main_spi", { kind: "spi", signals: ["sclk", "mosi"], preferred_host: "SPI3_HOST", freq: 40000000, freq_read: 16000000 });
  addBus(board, "internal_i2c", { kind: "i2c", signals: ["sda", "scl"], preferred_host: 1 });
  addBus(board, "port_a_i2c", { kind: "i2c", signals: ["sda", "scl"], preferred_host: 0 });
  addDevice(board, "lcd", { kind: "display", part: "st7735s", bus: "main_spi" });
  for (const [key, value] of Object.entries({ width: 128, height: 128, memory_height: 132, offset_x: 2, offset_y: 1, rotation_offset: 2, invert: true, readable: true, panel_type: "st7735s" })) setDeviceSpec(board, "lcd", null, key, value);
  addChoice(board, "lcd", "gc9107", { part: "gc9107", selected_by: "runtime", default: "st7735s" });
  for (const [key, value] of Object.entries({ width: 128, height: 128, offset_x: 0, offset_y: 32, rotation_offset: 0, invert: false, readable: false, panel_type: "gc9107" })) setDeviceSpec(board, "lcd", "gc9107", key, value);
  addDevice(board, "backlight", { kind: "backlight" });
  for (const [key, value] of Object.entries({ freq: 256, channel: 7, invert: false, offset: 48 })) setDeviceSpec(board, "backlight", null, key, value);
  addDevice(board, "btn_a", { kind: "button", part: "button" });
  for (const [key, value] of Object.entries({ active_low: true, pull: "external_up" })) setDeviceSpec(board, "btn_a", null, key, value);
  addConnector(board, "port_a", { type: "hy2_4p", standard: "port_i2c" });
  onMidway?.(board);
  for (const [gpio, roles] of Object.entries({
    1: ["bus:port_a_i2c.scl", "conn:port_a.1"], 2: ["bus:port_a_i2c.sda", "conn:port_a.2"],
    15: ["dev:lcd.cs"], 16: ["dev:backlight.pwm"], 17: ["bus:main_spi.sclk"], 21: ["bus:main_spi.mosi"],
    33: ["dev:lcd.dc"], 34: ["dev:lcd.rst"], 38: ["bus:internal_i2c.sda"], 39: ["bus:internal_i2c.scl"],
    41: ["dev:btn_a.in"],
  })) for (const role of roles) addRole(board, gpio, role, { chip });
  return board;
}

test("board operations create AtomS3 byte-for-byte", () => {
  assert.equal(formatBoard(buildAtomS3()), atoms3Text);
});

test("required part signals are missing midway and complete at the end", () => {
  let midway;
  const complete = buildAtomS3((board) => { midway = structuredClone(board); });
  const resolvedMidway = resolveBoard(midway, { lcd: "st7735s" }, connectorTypes, { chip, parts });
  assert.ok(validateBoard(resolvedMidway, { ...context, resolved: true }).some((item) => item.id === "E_PART_SIGNAL_MISSING"));
  const resolvedComplete = resolveBoard(complete, { lcd: "st7735s" }, connectorTypes, { chip, parts });
  assert.equal(validateBoard(resolvedComplete, { ...context, resolved: true }).filter((item) => item.id === "E_PART_SIGNAL_MISSING").length, 0);
});

test("explicit values equal to part defaults remain explicit", () => {
  const board = createBoard({ id: "sample", name: "Sample", legacy_board_id: 999, chip: "esp32s3" });
  addDevice(board, "lcd", { kind: "display", part: "st7735s" });
  setDeviceSpec(board, "lcd", null, "memory_height", parts.st7735s.spec_keys.memory_height.default);
  assert.equal(board.devices.lcd.spec.memory_height, 162);
  setDeviceSpec(board, "lcd", null, "memory_height", undefined);
  assert.equal(board.devices.lcd.spec, undefined);
});

test("reserved GPIO roles are rejected", () => {
  const board = createBoard({ id: "sample", name: "Sample", legacy_board_id: 999, chip: "esp32s3" });
  assert.throws(() => addRole(board, 26, "dev:lcd.cs", { chip }), (error) => error.code === "E_CHIP_RESERVED");
  setSpec(board, "storage.psram_mode", "opi");
  assert.throws(() => addRole(board, 33, "dev:lcd.cs", { chip }), (error) => error.code === "E_CHIP_RESERVED_COND");
});

test("new buses, devices, and connectors immediately become role candidates", () => {
  const board = createBoard({ id: "sample", name: "Sample", legacy_board_id: 999, chip: "esp32s3" });
  addBus(board, "panel_spi", { kind: "spi", signals: ["sclk", "mosi"] });
  addDevice(board, "lcd", { kind: "display", part: "st7735s", bus: "panel_spi" });
  addConnector(board, "port_a", { type: "hy2_4p" });
  const roles = boardRoleOptions(board, { schema, chip, parts, connectorTypes, currentGPIO: 4 });
  for (const role of ["bus:panel_spi.sclk", "bus:panel_spi.mosi", "dev:lcd.cs", "dev:lcd.dc", "conn:port_a.1"]) assert.ok(roles.includes(role), role);
  addRole(board, 4, "dev:lcd.cs", { chip });
  assert.equal(boardRoleOptions(board, { schema, chip, parts, connectorTypes, currentGPIO: 5 }).includes("dev:lcd.cs"), false);
});

test("duplicate operations can produce StickS3 byte-for-byte", () => {
  const board = duplicateBoard(buildAtomS3(), { id: "m5sticks3", name: "M5StickS3", official_name: "M5Stack StickS3", official_name_verified: true, aliases: ["sticks3", "m5sticks3", "stick s3"], legacy_board_id: 26 });
  for (const [gpio, pin] of Object.entries(structuredClone(board.pins))) for (const role of pin.roles ?? []) removeRoleFromBoard(board, gpio, role);
  removeConnector(board, "port_a");
  removeDevice(board, "lcd");
  removeDevice(board, "backlight");
  removeDevice(board, "btn_a");
  for (const id of ["main_spi", "internal_i2c", "port_a_i2c"]) removeBus(board, id);
  setSpec(board, "display.resolution", [135, 240]);
  setSpec(board, "storage.psram_mb", 8);
  setSpec(board, "storage.psram_mode", "opi");

  addBus(board, "main_spi", { kind: "spi", signals: ["sclk", "mosi"], preferred_host: "SPI3_HOST", freq: 40000000, freq_read: 16000000 });
  addBus(board, "internal_i2c", { kind: "i2c", signals: ["sda", "scl"], preferred_host: 1, freq: 100000 });
  addBus(board, "port_a_i2c", { kind: "i2c", signals: ["sda", "scl"], preferred_host: 0 });
  addBus(board, "i2s_audio", { kind: "i2s", signals: ["mck", "bck", "ws", "data_out", "data_in"], freq: 22050 });
  setBusField(board, "i2s_audio", "note", "Speaker uses I2S0 and microphone uses I2S1; magnification is 1.");
  addDevice(board, "lcd", { kind: "display", part: "st7789v2", bus: "main_spi" });
  for (const [key, value] of Object.entries({ width: 135, height: 240, offset_x: 52, offset_y: 40, rotation_offset: 0, invert: true, readable: true })) setDeviceSpec(board, "lcd", null, key, value);
  addDevice(board, "backlight", { kind: "backlight" });
  for (const [key, value] of Object.entries({ freq: 256, channel: 7, invert: false, offset: 16 })) setDeviceSpec(board, "backlight", null, key, value);
  addDevice(board, "pmic", { kind: "pmic", part: "m5pm1", bus: "internal_i2c" });
  addOwnerRole(board, "pmic", "gpio2", "dev:lcd.power");
  addOwnerRole(board, "pmic", "gpio3", "dev:speaker.enable");
  addOwnerRole(board, "pmic", "pwrkey", "dev:btn_pwr.in");
  addDevice(board, "speaker", { kind: "speaker", bus: "i2s_audio" });
  setDeviceField(board, "speaker", "verified", { bus: "datasheet" });
  addDevice(board, "mic", { kind: "mic", bus: "i2s_audio" });
  setDeviceField(board, "mic", "verified", { bus: "datasheet" });
  addDevice(board, "btn_a", { kind: "button", part: "button" });
  for (const [key, value] of Object.entries({ active_low: true, pull: "external_up" })) setDeviceSpec(board, "btn_a", null, key, value);
  addDevice(board, "btn_b", { kind: "button", part: "button" });
  for (const [key, value] of Object.entries({ active_low: true, pull: "external_up" })) setDeviceSpec(board, "btn_b", null, key, value);
  addDevice(board, "btn_pwr", { kind: "button", part: "button" });
  for (const [key, value] of Object.entries({ active_low: true, pull: "pmic" })) setDeviceSpec(board, "btn_pwr", null, key, value);
  addDevice(board, "psram", { kind: "psram", part: "psram" });
  addConnector(board, "port_a", { type: "hy2_4p", standard: "port_i2c" });
  const roles = {
    9: ["bus:port_a_i2c.sda", "conn:port_a.2"], 10: ["bus:port_a_i2c.scl", "conn:port_a.1"], 11: ["dev:btn_a.in"], 12: ["dev:btn_b.in"],
    14: ["bus:i2s_audio.data_out"], 15: ["bus:i2s_audio.ws"], 16: ["bus:i2s_audio.data_in"], 17: ["bus:i2s_audio.bck"], 18: ["bus:i2s_audio.mck"],
    21: ["dev:lcd.rst"], 26: ["dev:psram.cs"], 27: ["dev:psram.d3"], 28: ["dev:psram.d2"], 30: ["dev:psram.clk"], 31: ["dev:psram.d1"], 32: ["dev:psram.d0"],
    33: ["dev:psram.d4"], 34: ["dev:psram.d5"], 35: ["dev:psram.d6"], 36: ["dev:psram.d7"], 37: ["dev:psram.dqs"],
    38: ["dev:backlight.pwm"], 39: ["bus:main_spi.mosi"], 40: ["bus:main_spi.sclk"], 41: ["dev:lcd.cs"], 45: ["dev:lcd.dc"],
    47: ["bus:internal_i2c.sda"], 48: ["bus:internal_i2c.scl"],
  };
  for (const [gpio, items] of Object.entries(roles)) for (const role of items) addRole(board, gpio, role, { chip });
  setPin(board, 47, { pull: "up", pull_reliable: true });
  setPin(board, 48, { pull: "up", pull_reliable: true });
  assert.equal(formatBoard(board), sticks3Text);
});
