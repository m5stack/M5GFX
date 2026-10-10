import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
test("detection session preserves legacy hint, reset, retries, candidates and NVS decisions", async (t) => {
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
    assert.match(ran.stdout, /equivalence cases=4800000/);
  } finally {
    await fs.rm(directory, { recursive: true, force: true });
  }
});

test("compatibility autodetect preserves the C5/C61 transient output sentinel", async (t) => {
  const compiler = process.env.CXX || "c++";
  if (spawnSync(compiler, ["--version"]).error?.code === "ENOENT") return t.skip("C++ compiler unavailable");
  const sourceRoot = path.resolve(here, "../../../src");
  const main = await fs.readFile(path.join(sourceRoot, "M5GFX.cpp"), "utf8");
  const assignment = /#if !defined\(CONFIG_IDF_TARGET_ESP32C5\) && !defined\(CONFIG_IDF_TARGET_ESP32C61\)([\s\S]*?)#endif/.exec(main)?.[0];
  assert.ok(assignment, "compile the actual compatibility output block");
  const directory = await fs.mkdtemp(path.join(os.tmpdir(), "m5gfx-transient-"));
  try {
    const source = path.join(directory, "transient.cpp");
    await fs.writeFile(source, `#include "board_detect/detect_types.hpp"\n#include <cassert>\n`
      + `void apply(const m5gfx::board_detect::detect_outcome_t& outcome, bool* transient_fallback) {\n${assignment}\n}\n`
      + `int main() { for (int successful = 0; successful < 2; ++successful) {\n`
      + `for (int result = 0; result < 2; ++result) { for (int initial = 0; initial < 2; ++initial) {\n`
      + `m5gfx::board_detect::detect_outcome_t out; out.setup_succeeded = successful; out.result.transient_fallback = result;\n`
      + `bool caller = initial; apply(out, &caller); apply(out, nullptr);\n`
      + `#if defined(CONFIG_IDF_TARGET_ESP32C5) || defined(CONFIG_IDF_TARGET_ESP32C61)\nassert(caller == bool(initial));\n`
      + `#else\nassert(caller == bool(successful ? result : initial));\n#endif\n} } } }\n`);
    for (const chip of ["ESP32C5", "ESP32C61", "ESP32S3"]) {
      const binary = path.join(directory, chip);
      const compiled = spawnSync(compiler, ["-std=c++11", `-I${sourceRoot}`, `-DCONFIG_IDF_TARGET_${chip}`,
        source, "-o", binary], { encoding: "utf8" });
      assert.equal(compiled.status, 0, compiled.stderr || compiled.stdout);
      const ran = spawnSync(binary, [], { encoding: "utf8" });
      assert.equal(ran.status, 0, ran.stderr || ran.stdout);
    }
  } finally {
    await fs.rm(directory, { recursive: true, force: true });
  }
});
