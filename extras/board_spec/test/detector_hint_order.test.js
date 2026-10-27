import assert from "node:assert/strict";
import { promises as fs } from "node:fs";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { hasGpioPowerHold } from "../lib/emit/m5gfx_detector_order.js";
import { compileRun } from "./detect_host_helpers.js";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const src = path.resolve(root, "../../src");

// Reference for the constraint-ready selection rule. Input order is the actual
// generated array; membership and GPIO power hold come from the board catalog.
function referenceOrder(list, families, edges, hint) {
  const tried = [];
  while (tried.length < list.length) {
    let chosen = null, best = 4;
    for (const name of list) {
      if (tried.includes(name) || edges.some(e => e.after === name && !tried.includes(e.before))) continue;
      const rank = families.get(name).hold ? (name === hint ? 0 : 1) : (name === hint ? 2 : 3);
      if (rank < best) { chosen = name; best = rank; }
    }
    assert.notEqual(chosen, null, "detector constraints must be acyclic");
    tried.push(chosen);
  }
  return tried;
}

test("production detector order matches the reference for every package and board hint", async t => {
  const filenames = (await fs.readdir(path.join(root, "boards"))).filter(n => n.endsWith(".json"));
  const boards = await Promise.all(filenames.map(async name =>
    JSON.parse(await fs.readFile(path.join(root, "boards", name), "utf8"))));
  const byId = new Map(boards.map(board => [board.id, board]));
  const groups = JSON.parse(await fs.readFile(path.join(root, "detector_order.json"), "utf8"));
  // Derive candidate-only flags from the production descriptions, including
  // mixed families where a candidate-only member must not suppress promotion.
  const descriptorFiles = ["esp32c3.inl", "esp32c5/toughc5.inl", "esp32c6/c6_displayless.inl", "esp32s3/families.inl"];
  const descriptors = (await Promise.all(descriptorFiles.map(n => fs.readFile(path.join(src, "board_detect/m5", n), "utf8")))).join("\n");
  const candidateNames = new Set([...descriptors.matchAll(/"([^"]+)", def_flag_candidate_only/g)].map(m => m[1]));
  assert.equal(candidateNames.size, 4);
  let cases = 0;
  const sections = [], calls = [];
  for (const [name, group] of Object.entries(groups)) {
    const header = await fs.readFile(path.join(src, "board_detect/m5/generated", group.output), "utf8");
    const array = header.match(new RegExp(`static const board_detector_t\\* const ${name}\\[\\] = \\{([\\s\\S]*?)\\n\\};`));
    assert.ok(array, name);
    const list = [...array[1].matchAll(/&([a-z0-9_]+),/g)].map(m => m[1]);
    const families = new Map(group.families.map(f => [f.detector, {
      members: f.boards.map(id => byId.get(id)),
      hold: f.boards.some(id => hasGpioPowerHold(byId.get(id))),
    }]));
    const edges = group.constraints ?? [];
    assert.deepEqual(referenceOrder(list, families, edges, null), list, `${name}: no hint follows generated order`);
    const declarations = group.families.map(({ detector }) => {
      const members = families.get(detector).members;
      return members.map((b, i) => `static const board_def_t ${detector}_${i} = {${b.legacy_board_id}, "${b.name}", ${candidateNames.has(b.name) ? "def_flag_candidate_only" : "0"}};`).join("\n")
        + `\nstatic const board_def_t* const ${detector}_members[] = {${members.map((_, i) => `&${detector}_${i}`).join(",")},nullptr};`
        + `\nstatic const recording_detector_t ${detector}(${detector}_members, ${group.families.findIndex(f => f.detector === detector)});`;
    });
    const checks = [];
    for (const hintBoard of [null, ...boards]) {
      let hint = group.families.find(f => f.boards.includes(hintBoard?.id))?.detector ?? null;
      if (hint && families.get(hint).members.every(b => candidateNames.has(b.name))) hint = null;
      const expected = referenceOrder(list, families, edges, hint).map(n => group.families.findIndex(f => f.detector === n));
      checks.push(`seen.clear(); detect_board(${name}, ${hintBoard?.legacy_board_id ?? 0}, ctx); assert((seen == std::vector<int>{${expected.join(",")}}));`);
      ++cases;
    }
    sections.push(`namespace ${name}_case {\n${declarations.join("\n")}\n${array[0]}\nvoid check() { detection_transaction_t tx; probe_ctx_t ctx; ctx.transaction=&tx; ${checks.join("\n")} }\n}`);
    calls.push(`${name}_case::check();`);
  }
  await compileRun(`
#include "board_detect/detect_types.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#define ESP_LOGE(...) ((void)0)
#define ESP_LOGD(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
#include "board_detect/m5/generated/gpio_power_hold_board_ids.hpp"
#include "board_detect/m5/generated/detector_order_constraints.hpp"
namespace m5gfx { namespace board_detect {
struct board_desc_t { board_def_t def; struct {const char* const* data; unsigned size;} option_names; };
class detection_transaction_t { public: void rollback() {} };
struct probe_ctx_t { const board_id_t* enabled_ids=nullptr; detection_transaction_t* transaction=nullptr; board_id_t hint=0; bool confirm_attempted=false, family_identified=false; const board_def_t* candidate=nullptr; };
class board_detector_t {
public: const board_def_t* const* members;
explicit board_detector_t(const board_def_t* const* m):members(m) {}
virtual bool signature(probe_ctx_t&) const=0;
virtual bool confirm(probe_ctx_t&,board_result_t*) const=0;
bool has_member(board_id_t) const;
};
#include "board_detect/detector_runner.inl"
static std::vector<int> seen;
struct recording_detector_t:board_detector_t {
int index;
recording_detector_t(const board_def_t* const* m,int n):board_detector_t(m),index(n) {}
bool signature(probe_ctx_t&) const override {seen.push_back(index);return false;}
bool confirm(probe_ctx_t&,board_result_t*) const override {assert(false);return false;}
};
${sections.join("\n")}
}}
int main() {using namespace m5gfx::board_detect; ${calls.join("\n")} }
`, "package and hint detector order");
  t.diagnostic(`detector order comparisons=${cases}; packages=${Object.keys(groups).length}; hints=${boards.length + 1}`);
});
