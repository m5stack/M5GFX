#!/usr/bin/env node
import { spawnSync } from "node:child_process";
import { readdirSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
// Verification never updates fixtures, even when the caller has opted into updates.
const env = { ...process.env };
delete env.M5GFX_UPDATE_SNAPSHOTS;
delete env.NODE_TEST_CONTEXT;
let failed = false;
const results = [];
function report(label, status) {
  const line = `${label}: ${status}`;
  results.push(line);
  console.log(line);
}
function run(label, command, args) {
  const result = spawnSync(command, args, { cwd: root, env, stdio: "inherit" });
  const passed = !result.error && result.status === 0;
  report(label, passed ? "passed" : `failed (${result.error?.message ?? result.signal ?? `exit ${result.status}`})`);
  failed ||= !passed;
}
run("Catalog check", process.execPath, ["cli/spec.js", "check"]);
const tests = readdirSync(path.join(root, "test")).filter((name) => name.endsWith(".test.js")).sort();
run("Node tests", process.execPath, ["--test", "--test-reporter=tap", ...tests.map((name) => `test/${name}`)]);
const compiler = spawnSync(env.CXX || "c++", ["--version"], { env, stdio: "ignore" });
let hostSkipped = false;
if (compiler.error?.code === "ENOENT") {
  if (env.CI || Object.hasOwn(env, "CXX")) {
    report("C++ host tests", "failed (C++ compiler unavailable; CI and an explicit CXX require a working compiler)");
    failed = true;
  } else {
    report("C++ host tests", "skipped (no C++ compiler)");
    hostSkipped = true;
  }
}
else run("C++ host tests", "sh", ["../../test/board_detect/run.sh"]);
console.log("\nVerification summary:");
for (const line of results) console.log(line);
if (hostSkipped) console.log("\nATTENTION: C++ host tests were skipped; run them with a compiler before submitting");
process.exitCode = failed ? 1 : 0;
