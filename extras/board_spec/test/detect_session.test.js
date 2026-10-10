import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
test("detection session enforces provisional preference, stable hints, candidates and confirmed-only persistence", async (t) => {
  const compiler = process.env.CXX || "c++";
  if (spawnSync(compiler, ["--version"]).error?.code === "ENOENT") return t.skip("C++ compiler unavailable");
  const directory = await fs.mkdtemp(path.join(os.tmpdir(), "m5gfx-session-"));
  try {
    const binary = path.join(directory, "detect_session_host");
    const compiled = spawnSync(compiler, ["-std=c++11", "-O2", `-I${path.resolve(here, "../../../src")}`,
      path.join(here, "detect_session_host.cpp"), "-o", binary], { encoding: "utf8" });
    assert.equal(compiled.status, 0, compiled.stderr || compiled.stdout);
    const ran = spawnSync(binary, [], { encoding: "utf8" });
    assert.equal(ran.status, 0, ran.stderr || ran.stdout);
    assert.match(ran.stdout, /selection cases=32/);
  } finally {
    await fs.rm(directory, { recursive: true, force: true });
  }
});
