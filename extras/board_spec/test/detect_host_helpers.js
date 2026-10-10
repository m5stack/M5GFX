import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
const here = path.dirname(fileURLToPath(import.meta.url));
const src = path.resolve(here, "../../../src");
export function body(source, marker) {
  const begin = source.indexOf(marker); assert.ok(begin >= 0, marker);
  const brace = source.indexOf("{", begin); let depth = 1, end = brace + 1;
  while (depth && end < source.length) { if (source[end] === "{") ++depth; if (source[end] === "}") --depth; ++end; }
  assert.equal(depth, 0); return source.slice(brace, end);
}
export async function compileRun(source, label) {
  const directory = await fs.mkdtemp(path.join(os.tmpdir(), "m5gfx-s1-"));
  try {
    const file = path.join(directory, "host.cpp"), binary = path.join(directory, "host");
    await fs.writeFile(file, source);
    const result = spawnSync(process.env.CXX || "c++", ["-std=c++11", "-O2", `-I${src}`, file, "-o", binary], {encoding: "utf8"});
    assert.equal(result.status, 0, `${label}: ${result.stderr}`);
    const ran = spawnSync(binary, [], {encoding: "utf8"});
    assert.equal(ran.status, 0, `${label}: ${ran.stderr}`);
  } finally { await fs.rm(directory, {recursive:true, force:true}); }
}
