import assert from "node:assert/strict";
import { promises as fs } from "node:fs";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

// Every lgfx::board_t value must have a catalog entry (boards/*.json legacy_board_id)
// and a board definition (the board id field of an entry in src/board_detect/m5,
// which makes the board selectable through fixed_board), unless it is listed
// below with the parts it lacks and a reason. Whether a defined board is also
// auto-detected is checked by the detector order tests, not here.

const here = path.dirname(fileURLToPath(import.meta.url));
const specRoot = path.resolve(here, "..");
const src = path.resolve(here, "../../../src");

const NONE = [];
const BOTH = ["catalog", "definition"];

// Values that are intentionally outside the catalog and the board definitions.
const NOT_COVERED = new Map([
  [0, { lacks: BOTH, reason: "board_unknown is not a board" }],
  [20, { lacks: BOTH, reason: "CoreMP135 is not an ESP32 board" }],
  [131, { lacks: BOTH, reason: "M5Camera is a legacy board without a display" }],
  ...[192, 193, 194, 195, 196, 197, 198, 199, 200, 201].map((v) => [v, { lacks: BOTH, reason: "external display" }]),
  [512, { lacks: BOTH, reason: "frame buffer" }],
]);

// Boards not yet released. Recheck their status when it may have changed and
// add what they lack once released.
const PRE_RELEASE = new Map([
  [156, { lacks: ["definition"], reason: "M5StampP4X: no board definition in M5GFX yet" }],
]);

// Remove comments and string / character literals so that only code is matched.
// Raw string literals are not handled and fail the test instead.
function stripCode(text) {
  assert.ok(!/(?:^|[^\w])(?:u8|u|U|L)?R"/.test(text), "raw string literals are not supported by this test");
  return text.replace(/\/\*[\s\S]*?\*\/|\/\/[^\n]*|"(?:\\.|[^"\\\n])*"|'(?:\\.|[^'\\\n])*'/g, (m) => (m[0] === "/" ? " " : '""'));
}

// Parse the body of `enum board_t { ... };` strictly. Every entry must be
// `name = <decimal without leading zero|hex>` or `name = <earlier name>` (an alias), optionally with
// __attribute__ ((...)) after the name; anything else fails the test so that a
// new syntax is not skipped silently.
function parseBoardEnum(text) {
  const code = stripCode(text);
  const m = /\benum\s+board_t\b[^{]*\{([\s\S]*?)\}\s*;/.exec(code);
  assert.ok(m, "enum board_t not found");
  const values = new Map();
  const numbers = new Set();
  for (const raw of m[1].split(",")) {
    const entry = raw.replace(/__attribute__\s*\(\((?:[^()]|\([^()]*\))*\)\)/g, " ").trim();
    if (entry === "") continue;
    const e = /^(board_\w+)\s*=\s*(0[xX][0-9a-fA-F]+|0|[1-9]\d*|board_\w+)$/.exec(entry);
    assert.ok(e, `unsupported board_t entry: ${raw.trim()}`);
    const [, name, init] = e;
    const alias = /^board_/.test(init);
    const value = alias ? values.get(init) : Number(init);
    assert.ok(value !== undefined, `${name} refers to unknown ${init}`);
    assert.ok(!values.has(name), `duplicate ${name}`);
    // Only an alias may share a value; a repeated number is a copying mistake.
    assert.ok(alias || !numbers.has(value), `${name} = ${init} repeats the value of another board`);
    numbers.add(value);
    values.set(name, value);
  }
  return values;
}

async function readTree(dir) {
  const files = await fs.readdir(dir, { recursive: true });
  const texts = await Promise.all(files.filter((f) => /\.(inl|hpp|h|cpp)$/.test(f))
    .map((f) => fs.readFile(path.join(dir, f), "utf8")));
  return texts.join("\n");
}

test("every board_t value has a catalog entry and a board definition", async (t) => {
  const names = parseBoardEnum(await fs.readFile(path.join(src, "lgfx/boards.hpp"), "utf8"));

  // board id -> chips the catalog gives for it
  const catalog = new Map();
  const boardDir = path.join(specRoot, "boards");
  for (const f of (await fs.readdir(boardDir)).filter((f) => f.endsWith(".json"))) {
    const { legacy_board_id: id, chip } = JSON.parse(await fs.readFile(path.join(boardDir, f), "utf8"));
    if (id === undefined) continue;
    assert.ok(typeof chip === "string", `${f}: no chip`);
    if (!catalog.has(id)) catalog.set(id, new Set());
    catalog.get(id).add(chip);
  }

  // board id -> chips whose table defines it.
  // A board definition is a board_desc_t whose address is listed in the
  // board_entry_t <chip>_boards[] table that select_fixed_board() searches on
  // that chip; <chip> must match the catalog chip of the board.
  const detected = new Map();
  const detect = stripCode(await readTree(path.join(src, "board_detect/m5")));
  const descId = new Map();
  for (const m of detect.matchAll(/\bboard_desc_t\s+(desc_\w+)\s*=\s*\{\s*\{\s*(?:id|static_cast\s*<\s*board_id_t\s*>)\s*\(\s*lgfx::(board_\w+)\s*\)/g)) {
    assert.ok(names.has(m[2]), `${m[1]} refers to unknown ${m[2]}`);
    descId.set(m[1], names.get(m[2]));
  }
  const tables = [...detect.matchAll(/\bboard_entry_t\s+(\w+)_boards\s*\[\s*\]\s*=\s*\{([\s\S]*?)\n\s*\}\s*;/g)];
  assert.ok(tables.length > 0, "no board_entry_t tables found");
  for (const table of tables) {
    for (const m of table[2].matchAll(/\{\s*&\s*(\w+)\s*,/g)) {
      assert.ok(descId.has(m[1]), `cannot find the board id of ${m[1]}`);
      const id = descId.get(m[1]);
      if (!detected.has(id)) detected.set(id, new Set());
      detected.get(id).add(table[1]);
    }
  }

  const missing = [];
  const stale = [];
  const wrongChip = [];
  for (const value of new Set(names.values())) {
    const label = [...names].filter(([, v]) => v === value).map(([n]) => n).join(" / ");
    const lacks = [!catalog.has(value) && "catalog", !detected.has(value) && "definition"].filter(Boolean);
    if (catalog.has(value) && detected.has(value)) {
      const want = [...catalog.get(value)].sort().join(", ");
      const have = [...detected.get(value)].sort().join(", ");
      if (want !== have) wrongChip.push(`${value} ${label}: catalog chip [${want}], defined in the tables of [${have}]`);
    }
    const listed = NOT_COVERED.get(value) ?? PRE_RELEASE.get(value);
    const expected = listed ? listed.lacks : NONE;
    if (lacks.join() === expected.join()) continue;
    if (!listed) missing.push(`${value} ${label}: no ${lacks.join(", no ")}`);
    else stale.push(`${value} ${label}: listed as lacking [${expected.join(", ")}], actually lacks [${lacks.join(", ")}]`);
  }
  for (const [value, { lacks }] of [...NOT_COVERED, ...PRE_RELEASE]) {
    if (lacks.length === 0) stale.push(`${value} is listed but lacks nothing`);
    if (NOT_COVERED.has(value) && PRE_RELEASE.has(value)) stale.push(`${value} is listed twice`);
  }
  const known = new Set(names.values());
  for (const value of [...NOT_COVERED.keys(), ...PRE_RELEASE.keys()]) {
    if (!known.has(value)) stale.push(`${value} is no longer in board_t`);
  }
  for (const [value, { reason }] of PRE_RELEASE) t.diagnostic(`pre-release ${value}: ${reason}`);

  assert.deepEqual(missing, [], "add the catalog entry / board definition, or list the board with what it lacks and a reason");
  assert.deepEqual(stale, [], "update the exclusion lists to match");
  assert.deepEqual(wrongChip, [], "define the board in the table of its catalog chip");
});
