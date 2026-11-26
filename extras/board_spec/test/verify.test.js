import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import test from "node:test";

async function fixture(t, { checkExit = 0, testFails = false, compiler = process.execPath, ci = "", searchPath } = {}) {
  const directory = await fs.mkdtemp(path.join(os.tmpdir(), "board-spec-verify-"));
  t.after(() => fs.rm(directory, { recursive: true, force: true }));
  const root = path.join(directory, "extras/board_spec");
  for (const dir of ["cli", "test", "../../test/board_detect"]) await fs.mkdir(path.join(root, dir), { recursive: true });
  await fs.writeFile(path.join(root, "package.json"), '{"type":"module"}');
  await fs.copyFile(new URL("../cli/verify.js", import.meta.url), path.join(root, "cli/verify.js"));
  await fs.writeFile(path.join(root, "cli/spec.js"), `process.exitCode = ${checkExit};`);
  await fs.writeFile(path.join(root, "test/sample.test.js"), `import test from "node:test"; test("fixture", () => { ${testFails ? 'throw new Error("fixture failure");' : ""} });`);
  await fs.writeFile(path.join(directory, "test/board_detect/run.sh"), 'test "${M5GFX_UPDATE_SNAPSHOTS+x}" != x || exit 7\necho host-ran\n');
  const env = { ...process.env, CXX: compiler, M5GFX_UPDATE_SNAPSHOTS: "1" };
  if (compiler === null) delete env.CXX;
  env.CI = ci;
  if (searchPath !== undefined) env.PATH = searchPath;
  env.NODE_TEST_CONTEXT = "child-v8";
  return spawnSync(process.execPath, [path.join(root, "cli/verify.js")], { encoding: "utf8", env });
}

test("verify runs every gate after failures and never updates snapshots", async (t) => {
  const result = await fixture(t, { checkExit: 2, testFails: true });
  assert.equal(result.status, 1, result.stderr);
  assert.match(result.stdout, /Catalog check: failed \(exit 2\)/);
  assert.match(result.stdout, /Node tests: failed/);
  assert.match(result.stdout, /host-ran\nC\+\+ host tests: passed/);
});

test("verify reports successful gates", async (t) => {
  const result = await fixture(t);
  assert.equal(result.status, 0, result.stderr);
  assert.ok(result.stdout.endsWith("Verification summary:\nCatalog check: passed\nNode tests: passed\nC++ host tests: passed\n"));
});

test("verify explicitly skips unavailable default host tests locally", async (t) => {
  const result = await fixture(t, { compiler: null, searchPath: path.join(os.tmpdir(), "no-board-spec-tools") });
  assert.equal(result.status, 0, result.stderr);
  assert.match(result.stdout, /C\+\+ host tests: skipped \(no C\+\+ compiler\)/);
  assert.ok(!result.stdout.includes("host-ran"));
  assert.match(result.stdout, /C\+\+ host tests were skipped; run them with a compiler before submitting/);
});

test("verify requires a host compiler in CI", async (t) => {
  const result = await fixture(t, { compiler: null, ci: "1", searchPath: path.join(os.tmpdir(), "no-board-spec-tools") });
  assert.equal(result.status, 1, result.stderr);
  assert.match(result.stdout, /C\+\+ host tests: failed .*compiler unavailable/);
  assert.ok(!result.stdout.includes("C++ host tests: skipped"));
});

test("verify rejects an unavailable explicit CXX locally", async (t) => {
  const result = await fixture(t, { compiler: path.join(os.tmpdir(), "no-board-spec-compiler") });
  assert.equal(result.status, 1, result.stderr);
  assert.match(result.stdout, /C\+\+ host tests: failed .*explicit CXX/);
});
