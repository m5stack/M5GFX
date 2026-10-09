#!/usr/bin/env node
import { promises as fs } from "node:fs";
import { spawnSync } from "node:child_process";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

import { renderBundle } from "../build/bundle.js";
import { renderCompositionDoc, validateAccessory } from "../lib/compose.js";
import { validateConnectorTypes } from "../lib/ctypes.js";
import { validatePartCatalog } from "../lib/parts.js";
import { renderRevisionDoc, validateTargets } from "../lib/targets.js";
import { formatBoard } from "../lib/format.js";
import { emitPinTable, PIN_NAMES, PIN_TABLE_TARGETS, renderPinTableInl, renderPinTableJson } from "../lib/emit/m5unified_pin_table.js";
import { emitM5GFXWiring, m5gfxBoardMapping, renderM5GFXWiringHeader, selectM5GFXWiringBoards } from "../lib/emit/m5gfx_board_wiring.js";
import { emitM5GFXSpecs, renderM5GFXSpecsHeader } from "../lib/emit/m5gfx_board_specs.js";
import { renderDetectorOrderHeaders } from "../lib/emit/m5gfx_detector_order.js";
import { validateSchema } from "../lib/schema.js";
import { assertBoard, assertChip, assertSchema, parseJson } from "../lib/model.js";
import { resolveAll, resolveBoard } from "../lib/resolve.js";
import { validateBoard, validateCatalog, validateResolvedVariants } from "../lib/validate.js";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");

async function readJson(filename, assertion) {
  const text = await fs.readFile(filename, "utf8");
  return assertion(parseJson(text, filename), filename);
}

async function loadSchema() {
  return readJson(path.join(root, "schema/board.schema.json"), assertSchema);
}

async function loadChip(chipId) {
  return readJson(path.join(root, "chips", `${chipId}.json`), assertChip);
}

async function loadConnectorTypes() {
  const directory = path.join(root, "connector_types");
  const names = (await fs.readdir(directory)).filter((name) => name.endsWith(".json")).sort();
  const entries = await Promise.all(names.map(async (name) => {
    const value = await readJson(path.join(directory, name), (item) => item);
    return [value.id, value];
  }));
  return Object.fromEntries(entries);
}

async function loadParts() {
  const directory = path.join(root, "parts");
  const names = (await fs.readdir(directory)).filter((name) => name.endsWith(".json")).sort();
  return Object.fromEntries(await Promise.all(names.map(async (name) => {
    const value = await readJson(path.join(directory, name), (item) => item);
    return [value.id, value];
  })));
}

async function loadTargets() {
  return readJson(path.join(root, "targets.json"), (item) => item);
}

async function loadDetectorOrder() {
  const schema = await readJson(path.join(root, "schema/detector_order.schema.json"), (item) => item);
  return readJson(path.join(root, "detector_order.json"), (item) => {
    const issues = validateSchema(item, schema);
    if (issues.length) throw new Error(`detector_order.json: ${JSON.stringify(issues)}`);
    return item;
  });
}

async function loadAccessories() {
  const directory = path.join(root, "accessories");
  const names = (await fs.readdir(directory)).filter((name) => name.endsWith(".json")).sort();
  return Object.fromEntries(await Promise.all(names.map(async (name) => {
    const value = await readJson(path.join(directory, name), (item) => item);
    return [value.id, value];
  })));
}

async function context(board, schema, connectorTypes, parts, targets, accessories) {
  const compositionTarget = targets.m5unified_pin_table ?? {};
  return {
    schema, chip: await loadChip(board.chip), connectorTypes, parts, accessories,
    target: targets.m5gfx_board_desc,
    pinTableTarget: compositionTarget,
    composition: compositionTarget.compositions?.[board.id]?.default,
    allow_origins: compositionTarget.allow_origins,
  };
}

function resolveCatalogs(ctx) {
  return { chip: ctx.chip, parts: ctx.parts, accessories: ctx.accessories, composition: ctx.composition, allow_origins: ctx.allow_origins };
}

function printErrors(filename, errors) {
  for (const item of errors) console.error(`${filename}:${item.path || "/"}: ${item.id}: ${item.message}`);
}

function failureCount(errors) {
  return errors.filter((item) => item.severity !== "warning").length;
}

function safeOutputPath(directory, filename) {
  const base = path.resolve(directory);
  const output = path.resolve(base, filename);
  if (!filename || path.basename(filename) !== filename || path.dirname(output) !== base) {
    throw new Error(`unsafe generated filename: ${filename}`);
  }
  return output;
}

function validateGenerationInputs(boards, contexts, connectorTypes, parts, targets, accessories) {
  const errors = [...validateConnectorTypes(connectorTypes), ...validatePartCatalog(parts)];
  for (const accessory of Object.values(accessories)) errors.push(...validateAccessory(accessory, { connectorTypes, parts }));
  errors.push(...validateCatalog(boards, (board) => contexts.get(board.id)));
  errors.push(...validateTargets(boards, targets.m5gfx_board_desc));
  for (const board of boards) errors.push(...validateResolvedVariants(board, contexts.get(board.id)));
  if (failureCount(errors)) {
    printErrors("catalog", errors);
    throw new Error(`${failureCount(errors)} validation error(s)`);
  }
}

async function boardFiles() {
  return (await fs.readdir(path.join(root, "boards")))
    .filter((name) => name.endsWith(".json"))
    .sort()
    .map((name) => path.join(root, "boards", name));
}

async function pinTableEntries(boards, connectorTypes, contexts, targets) {
  const target = targets.m5unified_pin_table ?? {};
  return boards.map((board) => {
    const ctx = contexts.get(board.id);
    const resolved = resolveBoard(board, {}, connectorTypes, resolveCatalogs(ctx));
    return { board, emitted: emitPinTable(resolved, target) };
  });
}

async function pinTableOutputs(boards, connectorTypes, contexts, targets, targetId = "esp32") {
  const entries = await pinTableEntries(boards, connectorTypes, contexts, targets);
  return {
    json: renderPinTableJson(entries),
    inl: renderPinTableInl(entries, targetId, targets.m5gfx_board_desc),
    entries,
  };
}

function pinTableBoards(boards, contexts, targetId) {
  const target = PIN_TABLE_TARGETS[targetId];
  if (!target) throw new Error(`unknown pin-table target ${targetId}`);
  return boards.filter((board) => contexts.get(board.id).chip.soc === target.soc);
}

async function writePinTableOutputs() {
  const schema = await loadSchema();
  const connectorTypes = await loadConnectorTypes();
  const parts = await loadParts();
  const targets = await loadTargets();
  const accessories = await loadAccessories();
  const boards = await Promise.all((await boardFiles()).map((filename) => readJson(filename, assertBoard)));
  const contexts = new Map();
  for (const board of boards) contexts.set(board.id, await context(board, schema, connectorTypes, parts, targets, accessories));
  validateGenerationInputs(boards, contexts, connectorTypes, parts, targets, accessories);
  const directory = path.join(root, "generated/m5unified_pin_table");
  await fs.mkdir(directory, { recursive: true });
  let count = 0;
  for (const targetId of Object.keys(PIN_TABLE_TARGETS)) {
    const selected = pinTableBoards(boards, contexts, targetId);
    const output = await pinTableOutputs(selected, connectorTypes, contexts, targets, targetId);
    await fs.writeFile(safeOutputPath(directory, `${targetId}.json`), output.json);
    await fs.writeFile(safeOutputPath(directory, `${targetId}.inl`), output.inl);
    count += selected.length;
  }
  console.log(`generated M5Unified pin tables for ${count} board(s) across ${Object.keys(PIN_TABLE_TARGETS).length} target(s)`);
}

async function assertGeneratedFile(filename, expected) {
  let actual;
  try { actual = await fs.readFile(filename, "utf8"); }
  catch { throw new Error(`missing generated artifact: ${path.relative(root, filename)}`); }
  if (actual !== expected) throw new Error(`stale generated artifact: ${path.relative(root, filename)}`);
}

function wiringEntries(boards, connectorTypes, contexts, mappings, target) {
  return selectM5GFXWiringBoards(boards, mappings).map((board) => {
    const ctx = contexts.get(board.id);
    const resolved = resolveBoard(board, {}, connectorTypes, { chip: ctx.chip, parts: ctx.parts });
    return { board, chip: ctx.chip, emitted: emitM5GFXWiring(resolved, ctx.parts, target) };
  });
}

async function wiringOutputs(boards, connectorTypes, contexts, targets) {
  const outputs = new Map();
  const target = targets.m5gfx_board_desc ?? {};
  const mappings = Object.keys(target.boards ?? {}).map((id) => m5gfxBoardMapping(target, id)).filter(Boolean);
  for (const filename of new Set(mappings.map((mapping) => mapping.wiringOutput))) {
    const selected = mappings.filter((mapping) => mapping.wiringOutput === filename);
    outputs.set(filename, renderM5GFXWiringHeader(wiringEntries(boards, connectorTypes, contexts, selected, target)));
  }
  return outputs;
}

function specsOutputs(boards, connectorTypes, contexts, targets) {
  const outputs = new Map();
  const target = targets.m5gfx_board_desc ?? {};
  const mappings = Object.keys(target.boards ?? {}).map((id) => m5gfxBoardMapping(target, id))
    .filter((mapping) => mapping?.specsOutput);
  for (const filename of new Set(mappings.map((mapping) => mapping.specsOutput))) {
    const selected = mappings.filter((mapping) => mapping.specsOutput === filename).map((mapping) => {
      const board = boards.find((item) => item.id === mapping.boardId);
      if (!board) throw new Error(`${mapping.boardId} board is required for M5GFX specs`);
      const ctx = contexts.get(board.id);
      const variants = resolveAll(board, connectorTypes, resolveCatalogs(ctx)).map((item) => item.board);
      return emitM5GFXSpecs(board, variants, ctx.parts, mapping);
    });
    outputs.set(filename, renderM5GFXSpecsHeader(selected));
  }
  return outputs;
}

async function writeWiringOutput() {
  const schema = await loadSchema();
  const connectorTypes = await loadConnectorTypes();
  const parts = await loadParts();
  const targets = await loadTargets();
  const detectorOrder = await loadDetectorOrder();
  const accessories = await loadAccessories();
  const boards = await Promise.all((await boardFiles()).map((filename) => readJson(filename, assertBoard)));
  const contexts = new Map();
  for (const board of boards) contexts.set(board.id, await context(board, schema, connectorTypes, parts, targets, accessories));
  validateGenerationInputs(boards, contexts, connectorTypes, parts, targets, accessories);
  const directory = path.resolve(root, "../../src/board_detect/m5/generated");
  await fs.mkdir(directory, { recursive: true });
  for (const [filename, output] of await wiringOutputs(boards, connectorTypes, contexts, targets)) {
    await fs.writeFile(safeOutputPath(directory, filename), output);
  }
  for (const [filename, output] of specsOutputs(boards, connectorTypes, contexts, targets)) {
    await fs.writeFile(safeOutputPath(directory, filename), output);
  }
  for (const [filename, output] of renderDetectorOrderHeaders(boards, detectorOrder)) {
    await fs.writeFile(safeOutputPath(directory, filename), output);
  }
  const count = Object.values(targets.m5gfx_board_desc?.boards ?? {}).filter((entry) => entry.wiring_output).length;
  console.log(`generated M5GFX wiring for ${count} board(s)`);
}

async function validateFiles(files) {
  const schema = await loadSchema();
  const connectorTypes = await loadConnectorTypes();
  const parts = await loadParts();
  const targets = await loadTargets();
  const accessories = await loadAccessories();
  let count = 0;
  const typeErrors = validateConnectorTypes(connectorTypes);
  printErrors("connector_types", typeErrors);
  count += failureCount(typeErrors);
  for (const accessory of Object.values(accessories)) {
    const accessoryErrors = validateAccessory(accessory, { connectorTypes, parts });
    printErrors(`accessories/${accessory.id}.json`, accessoryErrors);
    count += failureCount(accessoryErrors);
  }
  for (const filename of files) {
    const board = await readJson(filename, assertBoard);
    const ctx = await context(board, schema, connectorTypes, parts, targets, accessories);
    const errors = [...validateBoard(board, ctx), ...validateResolvedVariants(board, ctx)];
    printErrors(filename, errors);
    count += failureCount(errors);
  }
  if (count) throw new Error(`${count} validation error(s)`);
  console.log(`validated ${files.length} board file(s)`);
}

async function formatFiles(files, write) {
  for (const filename of files) {
    const board = await readJson(filename, assertBoard);
    const formatted = formatBoard(board);
    if (write) await fs.writeFile(filename, formatted);
    else process.stdout.write(formatted);
  }
}

async function resolveFile(filename) {
  const schema = await loadSchema();
  const connectorTypes = await loadConnectorTypes();
  const parts = await loadParts();
  const targets = await loadTargets();
  const accessories = await loadAccessories();
  const board = await readJson(filename, assertBoard);
  const ctx = await context(board, schema, connectorTypes, parts, targets, accessories);
  const errors = [...validateBoard(board, ctx), ...validateResolvedVariants(board, ctx)];
  if (failureCount(errors)) {
    printErrors(filename, errors);
    throw new Error(`${failureCount(errors)} validation error(s)`);
  }
  const outputDir = path.join(root, "generated/resolved");
  await fs.mkdir(outputDir, { recursive: true });
  const outputs = resolveAll(board, connectorTypes, resolveCatalogs(ctx));
  for (const output of outputs) await fs.writeFile(safeOutputPath(outputDir, output.filename), formatBoard(output.board));
  const revisionDoc = renderRevisionDoc(board, ctx.target);
  if (revisionDoc) {
    const docsDir = path.join(root, "generated/docs");
    await fs.mkdir(docsDir, { recursive: true });
    await fs.writeFile(safeOutputPath(docsDir, `${board.id}_revisions.md`), revisionDoc);
  }
  const compositionDoc = renderCompositionDoc(board, ctx.composition, accessories);
  if (compositionDoc) {
    const docsDir = path.join(root, "generated/docs");
    await fs.mkdir(docsDir, { recursive: true });
    await fs.writeFile(safeOutputPath(docsDir, `${board.id}_composition.md`), compositionDoc);
  }
  console.log(`resolved ${outputs.length} combination(s)`);
}

async function check() {
  const schema = await loadSchema();
  const connectorTypes = await loadConnectorTypes();
  const parts = await loadParts();
  const targets = await loadTargets();
  const detectorOrder = await loadDetectorOrder();
  const accessories = await loadAccessories();
  const files = await boardFiles();
  const boards = await Promise.all(files.map((filename) => readJson(filename, assertBoard)));
  const contexts = new Map();
  for (const board of boards) contexts.set(board.id, await context(board, schema, connectorTypes, parts, targets, accessories));

  const errors = validateConnectorTypes(connectorTypes);
  errors.push(...validatePartCatalog(parts));
  for (const accessory of Object.values(accessories)) errors.push(...validateAccessory(accessory, { connectorTypes, parts }));
  errors.push(...validateCatalog(boards, (board) => contexts.get(board.id)));
  errors.push(...validateTargets(boards, targets.m5gfx_board_desc));
  for (const board of boards) errors.push(...validateResolvedVariants(board, contexts.get(board.id)));
  if (failureCount(errors)) {
    printErrors("catalog", errors);
    throw new Error(`${failureCount(errors)} validation error(s)`);
  }

  for (let index = 0; index < boards.length; index += 1) {
    const source = await fs.readFile(files[index], "utf8");
    if (source !== formatBoard(boards[index])) throw new Error(`${path.relative(root, files[index])} is not formatted`);
    const ctx = contexts.get(boards[index].id);
    for (const output of resolveAll(boards[index], connectorTypes, resolveCatalogs(ctx))) {
      const filename = path.join(root, "generated/resolved", output.filename);
      let actual;
      try { actual = await fs.readFile(filename, "utf8"); }
      catch { throw new Error(`missing resolved snapshot: ${path.relative(root, filename)}`); }
      if (actual !== formatBoard(output.board)) throw new Error(`stale resolved snapshot: ${path.relative(root, filename)}`);
    }
    const revisionDoc = renderRevisionDoc(boards[index], ctx.target);
    if (revisionDoc) {
      const filename = path.join(root, "generated/docs", `${boards[index].id}_revisions.md`);
      let actual;
      try { actual = await fs.readFile(filename, "utf8"); }
      catch { throw new Error(`missing revision doc: ${path.relative(root, filename)}`); }
      if (actual !== revisionDoc) throw new Error(`stale revision doc: ${path.relative(root, filename)}`);
    }
    const compositionDoc = renderCompositionDoc(boards[index], ctx.composition, accessories);
    if (compositionDoc) {
      const filename = path.join(root, "generated/docs", `${boards[index].id}_composition.md`);
      let actual;
      try { actual = await fs.readFile(filename, "utf8"); }
      catch { throw new Error(`missing composition doc: ${path.relative(root, filename)}`); }
      if (actual !== compositionDoc) throw new Error(`stale composition doc: ${path.relative(root, filename)}`);
    }
  }
  const expectedSnapshots = new Set(boards.flatMap((board) => resolveAll(board, connectorTypes, resolveCatalogs(contexts.get(board.id))).map((output) => output.filename)));
  const actualSnapshots = (await fs.readdir(path.join(root, "generated/resolved"))).filter((name) => name.endsWith(".json"));
  const unexpected = actualSnapshots.filter((name) => !expectedSnapshots.has(name));
  if (unexpected.length) throw new Error(`unexpected resolved snapshot(s): ${unexpected.join(", ")}`);
  const expectedDocs = new Set([
    ...boards.filter((board) => board.revisions?.length).map((board) => `${board.id}_revisions.md`),
    ...boards.filter((board) => contexts.get(board.id).composition).map((board) => `${board.id}_composition.md`),
  ]);
  const actualDocs = (await fs.readdir(path.join(root, "generated/docs"))).filter((name) => name.endsWith(".md"));
  const unexpectedDocs = actualDocs.filter((name) => !expectedDocs.has(name));
  if (unexpectedDocs.length) throw new Error(`unexpected revision doc(s): ${unexpectedDocs.join(", ")}`);

  const pinTableDir = path.join(root, "generated/m5unified_pin_table");
  for (const targetId of Object.keys(PIN_TABLE_TARGETS)) {
    const selected = pinTableBoards(boards, contexts, targetId);
    const pinTables = await pinTableOutputs(selected, connectorTypes, contexts, targets, targetId);
    await assertGeneratedFile(path.join(pinTableDir, `${targetId}.json`), pinTables.json);
    await assertGeneratedFile(path.join(pinTableDir, `${targetId}.inl`), pinTables.inl);
  }
  const wiringDirectory = path.resolve(root, "../../src/board_detect/m5/generated");
  for (const [filename, output] of await wiringOutputs(boards, connectorTypes, contexts, targets)) {
    await assertGeneratedFile(path.join(wiringDirectory, filename), output);
  }
  for (const [filename, output] of specsOutputs(boards, connectorTypes, contexts, targets)) {
    await assertGeneratedFile(path.join(wiringDirectory, filename), output);
  }
  for (const [filename, output] of renderDetectorOrderHeaders(boards, detectorOrder)) {
    await assertGeneratedFile(path.join(wiringDirectory, filename), output);
  }

  const distFilename = path.join(root, "dist/board_spec_editor.html");
  let dist;
  try { dist = await fs.readFile(distFilename, "utf8"); }
  catch { throw new Error(`missing editor bundle: ${path.relative(root, distFilename)}`); }
  if (dist !== await renderBundle()) throw new Error(`stale editor bundle: ${path.relative(root, distFilename)}`);
  console.log(`check passed: ${boards.length} board(s), ${boards.reduce((sum, board) => sum + resolveAll(board, connectorTypes, resolveCatalogs(contexts.get(board.id))).length, 0)} resolved snapshot(s)`);
}

const PIN_TABLE_START = "static constexpr const uint8_t _pin_table_i2c_ex_in[][5] = {";
const PIN_TABLE_END = "\n#endif\n\n  /// @return true when every write";

function extractPinTables(source, filename) {
  const start = source.indexOf(PIN_TABLE_START);
  if (start === -1) throw new Error(`${filename}: pin-table start anchor not found: ${PIN_TABLE_START}`);
  const end = source.indexOf(PIN_TABLE_END, start);
  if (end === -1) throw new Error(`${filename}: pin-table end anchor not found: ${PIN_TABLE_END.trim()}`);
  return source.slice(start, end);
}

function gpioDefines(source) {
  const numbers = [...source.matchAll(/\bGPIO_NUM_(\d+)\b/g)].map((match) => Number(match[1]));
  return [...new Set(numbers)].sort((left, right) => left - right).map((number) => `#define GPIO_NUM_${number} ${number}`).join("\n");
}

function comparisonHarness(tableSource, targetId, selectedBoards, boardTarget) {
  const rows = selectedBoards.map(({ id }) => {
    const boardEnum = boardTarget.boards?.[id]?.board_enum;
    if (!boardEnum) throw new Error(`missing board enum for ${id}`);
    return `  { "${id}", lgfx::board_t::${boardEnum} },`;
  }).join("\n");
  const target = PIN_TABLE_TARGETS[targetId];
  return `#include <cstdint>
#include <cstdio>
#include <cstring>
#include <utility>
#include "src/lgfx/boards.hpp"
#define CONFIG_IDF_TARGET 1
#define ${target.define} 1
${gpioDefines(tableSource)}
namespace m5 {
using board_t = lgfx::board_t;
class M5Unified {
public:
  static int8_t _get_pin_table[${PIN_NAMES.length}];
  static void _setup_pinmap(board_t);
};
int8_t M5Unified::_get_pin_table[${PIN_NAMES.length}];
${tableSource}
}
int main() {
  struct row_t { const char* id; lgfx::board_t board; };
  const row_t rows[] = {
${rows}
  };
  for (const auto& row : rows) {
    m5::M5Unified::_setup_pinmap(row.board);
    std::printf("%s", row.id);
    for (const auto value : m5::M5Unified::_get_pin_table) std::printf(" %u", static_cast<unsigned char>(value));
    std::putchar('\\n');
  }
}
`;
}

function parseActualPinTables(output) {
  const result = {};
  for (const line of output.trim().split("\n")) {
    const [id, ...values] = line.trim().split(/\s+/);
    if (!id) continue;
    if (values.length !== PIN_NAMES.length) throw new Error(`host harness returned ${values.length} pin values for ${id}; expected ${PIN_NAMES.length}`);
    result[id] = Object.fromEntries(PIN_NAMES.map((name, index) => [name, Number(values[index])]));
  }
  return result;
}

async function comparePinTable(m5unifiedPath) {
  const compiler = process.env.CXX || "c++";
  const probe = spawnSync(compiler, ["--version"], { encoding: "utf8" });
  if (probe.error?.code === "ENOENT") {
    console.log(`compare-pintable skipped: C++ compiler not found (${compiler})`);
    return;
  }
  if (probe.error || probe.status !== 0) throw new Error(`C++ compiler probe failed: ${probe.error?.message ?? probe.stderr.trim()}`);

  const inlFilename = path.join(m5unifiedPath, "src/M5Unified.inl");
  const tableSource = extractPinTables(await fs.readFile(inlFilename, "utf8"), inlFilename);
  const temporary = await fs.mkdtemp(path.join(os.tmpdir(), "m5unified-pintable-"));
  try {
    const schema = await loadSchema();
    const connectorTypes = await loadConnectorTypes();
    const parts = await loadParts();
    const targets = await loadTargets();
    const accessories = await loadAccessories();
    const boards = await Promise.all((await boardFiles()).map((filename) => readJson(filename, assertBoard)));
    const contexts = new Map();
    for (const board of boards) contexts.set(board.id, await context(board, schema, connectorTypes, parts, targets, accessories));
    const differences = [];
    let comparedBoards = 0;
    for (const targetId of Object.keys(PIN_TABLE_TARGETS)) {
      const selected = pinTableBoards(boards, contexts, targetId);
      const source = path.join(temporary, `compare-${targetId}.cpp`);
      const executable = path.join(temporary, `compare-${targetId}`);
      await fs.writeFile(source, comparisonHarness(tableSource, targetId, selected, targets.m5gfx_board_desc));
      const compiled = spawnSync(compiler, ["-std=c++17", "-I", path.resolve(root, "../.."), source, "-o", executable], { encoding: "utf8" });
      if (compiled.error || compiled.status !== 0) throw new Error(`${targetId} pin-table harness compile failed:\n${compiled.error?.message ?? compiled.stderr.trim()}`);
      const ran = spawnSync(executable, [], { encoding: "utf8" });
      if (ran.error || ran.status !== 0) throw new Error(`${targetId} pin-table harness failed:\n${ran.error?.message ?? ran.stderr.trim()}`);
      const actual = parseActualPinTables(ran.stdout);
      const generated = await pinTableEntries(selected, connectorTypes, contexts, targets);
      comparedBoards += generated.length;
      for (const { board, emitted } of generated) for (const name of PIN_NAMES) {
        if (emitted.values[name] !== actual[board.id]?.[name]) differences.push([board.id, name, emitted.values[name], actual[board.id]?.[name] ?? "missing"]);
      }
    }
    if (differences.length) {
      console.log("board | pin_name | generated | actual");
      console.log("--- | --- | ---: | ---:");
      for (const row of differences) console.log(row.join(" | "));
      throw new Error(`M5Unified pin-table comparison failed: ${differences.length} difference(s)`);
    }
    console.log(`M5Unified pin-table comparison passed: ${comparedBoards} board(s), ${comparedBoards * PIN_NAMES.length} value(s), ${Object.keys(PIN_TABLE_TARGETS).length} target(s)`);
  } finally {
    await fs.rm(temporary, { recursive: true, force: true });
  }
}

async function main() {
  const [command, ...args] = process.argv.slice(2);
  if (command === "validate") return validateFiles(args.length ? args.map((file) => path.resolve(file)) : await boardFiles());
  if (command === "format") {
    const write = args.includes("--write");
    const files = args.filter((arg) => arg !== "--write").map((file) => path.resolve(file));
    if (!files.length) throw new Error("format requires at least one file");
    return formatFiles(files, write);
  }
  if (command === "resolve") {
    if (args.length !== 1) throw new Error("resolve requires exactly one board file");
    return resolveFile(path.resolve(args[0]));
  }
  if (command === "emit-pintable") return writePinTableOutputs();
  if (command === "emit-wiring") return writeWiringOutput();
  if (command === "compare-pintable") {
    if (args.length !== 2 || args[0] !== "--m5unified") throw new Error("compare-pintable requires --m5unified <path>");
    return comparePinTable(path.resolve(args[1]));
  }
  if (command === "check") return check();
  throw new Error("usage: spec.js validate <file...> | format <file...> [--write] | resolve <file> | emit-pintable | emit-wiring | compare-pintable --m5unified <path> | check");
}

main().catch((failure) => {
  console.error(failure.message);
  process.exitCode = 1;
});
