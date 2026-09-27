import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));

test("board-detection operation IR passes its C++11 host suite", async (t) => {
  const compiler = process.env.CXX || "c++";
  const probe = spawnSync(compiler, ["--version"], { encoding: "utf8" });
  if (probe.error?.code === "ENOENT") return t.skip(`C++ compiler not found (${compiler})`);
  const directory = await fs.mkdtemp(path.join(os.tmpdir(), "m5gfx-ops-"));
  const binary = path.join(directory, "ops_host");
  try {
    const sourceRoot = path.resolve(here, "../../../src");
    const source = path.join(here, "ops_host.cpp");
    const compiled = spawnSync(compiler, ["-std=c++11", `-I${sourceRoot}`, source, "-o", binary], { encoding: "utf8" });
    assert.equal(compiled.status, 0, compiled.stderr || compiled.stdout);
    const ran = spawnSync(binary, [], { encoding: "utf8" });
    assert.equal(ran.status, 0, ran.stderr || ran.stdout);
  } finally {
    await fs.rm(directory, { recursive: true, force: true });
  }
});
