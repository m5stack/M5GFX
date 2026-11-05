import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { formatBoard } from "../lib/format.js";
import { validateSchema } from "../lib/schema.js";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const schema = JSON.parse(await fs.readFile(path.join(root, "schema/board.schema.json"), "utf8"));
const source = JSON.parse(await fs.readFile(path.join(root, "boards/m5nanoc6.json"), "utf8"));
const localDocument = ["docs", "investigations", "x.md"].join("/");
const description = "Internal-pull GPIO survey on M5NanoC6";

for (const value of [localDocument, "survey.md", "survey.MD", "docs/survey.txt", "Docs\\survey.txt"]) {
  test(`measured_by rejects local document reference ${value}`, () => {
    const board = structuredClone(source);
    board.pins[3].measured_by = { roles: value };
    assert.ok(validateSchema(board, schema).some((issue) =>
      issue.id === "E_SCHEMA_PATTERN" && issue.path === "/pins/3/measured_by/roles"));
  });
}

for (const value of [description, "https://example.com/docs/survey.md", "http://example.com/survey.md"]) {
  test(`measured_by accepts public provenance ${value}`, () => {
    const board = structuredClone(source);
    board.pins[3].measured_by = { roles: value };
    assert.deepEqual(validateSchema(board, schema), []);
  });
}

test("CLI validation rejects local provenance and accepts a GPIO survey description", async () => {
  const directory = await fs.mkdtemp(path.join(os.tmpdir(), "m5gfx-provenance-"));
  const filename = path.join(directory, "m5nanoc6.json");
  try {
    for (const [value, status] of [[localDocument, 1], [description, 0]]) {
      const board = structuredClone(source);
      board.pins[3].measured_by = { roles: value };
      await fs.writeFile(filename, formatBoard(board));
      const result = spawnSync(process.execPath, ["cli/spec.js", "validate", filename], { cwd: root, encoding: "utf8" });
      assert.equal(result.status, status, result.stderr || result.stdout);
      if (status) assert.match(result.stdout + result.stderr, /measured_by.*E_SCHEMA_PATTERN/);
    }
  } finally {
    await fs.rm(directory, { recursive: true, force: true });
  }
});
