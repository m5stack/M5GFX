import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
const here = path.dirname(fileURLToPath(import.meta.url));
const src = path.resolve(here, "../../../src");
// Deferred public-API tests retain their existing fixtures until a native class boundary is available.
async function readMain() {
  return (await Promise.all(["M5GFX.cpp", "board_detect/detection_package.inl", "board_detect/detection_integration.inl"].map((name) => fs.readFile(path.join(src, name), "utf8")))).join("\n");
}
import { body, compileRun } from "./detect_host_helpers.js";

test("production attempt yields before construction and preserves adoption and rollback semantics", async () => {
  const main = await readMain();
  const attempt = body(main, "static board_detect::detect_outcome_t run_detection_attempt(");
  const finish = body(main, "static board_detect::detect_outcome_t construct_and_adopt(");
  const projection = main.slice(main.indexOf("    const auto board = outcome.adopted"), main.indexOf("#if defined ( ARDUINO_M5STACK_ATOM )", main.indexOf("    const auto board = outcome.adopted")));
  const header = await fs.readFile(path.join(src, "M5GFX.h"), "utf8");
  const candidateGetter = body(header, "board_t getCandidateBoard(");
  await compileRun(`
#include "board_detect/detect_types.hpp"
#include <cassert>
#include <initializer_list>
#define CONFIG_IDF_TARGET_ESP32C6 1
#define M5GFX_HAS_BOARD_DETECT
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGI(...) ((void)0)
using namespace m5gfx;
enum class board_t : std::uint32_t {board_unknown=0,member=1};
int rollback_count, commit_count, construct_count, adopt_count, reset_count;
bool power_failed, prepare_ok, adopt_ok, construct_ok, no_display;
namespace lgfx { namespace i2c { bool isInitialized(int) { return false; } } }
namespace m5gfx { namespace board_detect {
struct board_desc_t { board_def_t def; struct { int hw_port; } internal_i2c; struct { const int* data; int size; } hold_high_pins; };
void board_result_t::assign(const board_desc_t* d) { desc=d; def=&d->def; }
const board_desc_t* refined_member=nullptr;
struct board_detector_t {};
struct prepare_ctx_t { bool allow_reset; board_id_t fallback; unsigned attempt; bool final_attempt; int probe_port; detection_transaction_t* transaction; const board_id_t* enabled_ids; };
struct probe_ctx_t : prepare_ctx_t { bool confirm_attempted; };
inline int pins(int) {return 0;} inline int no_pins() {return 0;}
struct detection_transaction_t {
 detection_transaction_t(int,int,bool) {} bool valid() {return true;}
 void rollback() {++rollback_count;} void commit() {++commit_count;}
 void restore_to_snapshot(const int*, int) {++reset_count;}
 struct buses_t {void opened_i2c(int) {}} buses_;
 buses_t& buses() {return buses_;}
};
board_result_t detected;
board_result_t detect_board(const board_detector_t* const*, board_id_t, probe_ctx_t& p) {p.confirm_attempted=true; return detected;}
bool prepare(const board_desc_t&, board_result_t& r, const prepare_ctx_t&) {if(power_failed) {r.provisional=true;} else if(refined_member) {r.assign(refined_member);}
 return prepare_ok;}
namespace m5 {
namespace wiring { namespace detection { const int unconditional_pins=0; } }
struct display_parts_t {int* bus=nullptr;int* panel=nullptr;int* light=nullptr;int* touch=nullptr;};
enum class construct_status_t {failed, no_display, success};
construct_status_t construct_detected_board(const board_result_t&, display_parts_t*) {++construct_count; return !construct_ok ? construct_status_t::failed : no_display ? construct_status_t::no_display : construct_status_t::success;}
void destroy_display_parts(display_parts_t*) {}
struct log_t {const char* name; const char* annotation;};
log_t success_log(const board_result_t&) {return {nullptr,nullptr};}
}
} }
const int detection_i2c_port=0;
template<class Adopt> board_detect::detect_outcome_t construct_and_adopt(board_detect::board_result_t& result, board_detect::detect_outcome_t outcome, board_detect::detection_transaction_t& transaction, board_t representative_board, Adopt adopt) ${finish}
template<class Adopt> board_detect::detect_outcome_t run_detection_attempt(const board_detect::board_detector_t* const* detectors, const board_detect::detect_request_t& request, bool final_attempt, Adopt adopt) ${attempt}
struct M5GFX {
 board_t _board=board_t::board_unknown,_board_candidate=board_t::board_unknown;
 board_t getBoard() const {return _board;}
 board_t getCandidateBoard() const ${candidateGetter}
 void project(const board_detect::detect_outcome_t& outcome) { ${projection} }
};
int main() {
 using namespace board_detect;
 const board_desc_t desc={{1,"member",0},{-1},{nullptr,0}};
 const board_detector_t d; const board_detector_t* list[]={&d,nullptr};
 for(int provisional=0; provisional<2; ++provisional)
 for(int fallback=0; fallback<3; ++fallback)
 for(int failure=0; failure<4; ++failure)
 for(int displayless=0; displayless<2; ++displayless) {
  rollback_count=commit_count=construct_count=adopt_count=reset_count=0;
  detected={}; detected.assign(&desc); detected.status=detect_status_t::matched;
  power_failed=provisional; prepare_ok=failure!=1; construct_ok=failure!=2; adopt_ok=failure!=3; no_display=displayless;
  detect_request_t req; req.fallback=fallback; req.max_attempts=5;
  auto out=run_detection_session(req,[&](const detect_request_t& r,bool final) {
   return run_detection_attempt(list,r,final,[](m5::display_parts_t&,board_t) {++adopt_count; return adopt_ok;});
  });
  const bool yields=provisional && fallback==2 && prepare_ok;
  const bool success=failure==0 && !yields;
  M5GFX standalone;standalone.project(out);
  assert(standalone.getBoard()==(success?board_t::member:board_t::board_unknown));
  assert(standalone.getCandidateBoard()==(provisional&&!success?board_t::member:board_t::board_unknown));
  assert(out.adopted==success);
  assert(out.verdict==(provisional ? verdict_t::candidate : verdict_t::confirmed));
  assert(out.candidate_kind==(provisional ? candidate_kind_t::provisional : candidate_kind_t::none));
  assert(out.attempts==(success ? 1:5));
  assert(construct_count==(failure==1||yields ? 0:out.attempts));
  assert(adopt_count==(failure==1||failure==2||yields ? 0:out.attempts));
  assert(commit_count==int(success)); assert(rollback_count==(success ? 0:out.attempts));
  assert(!yields || out.result.candidate==&desc.def);
  assert(should_persist_detection(out,0)==(success&&!provisional));
 }
 // A retained power failure retries without construction, then refinement can select the preference.
 const board_desc_t fallback_desc={{2,"fallback",0},{-1},{nullptr,0}};
 rollback_count=commit_count=construct_count=adopt_count=reset_count=0;
 detected={};detected.assign(&desc);detected.status=detect_status_t::matched;
 prepare_ok=construct_ok=adopt_ok=true;no_display=false;refined_member=&fallback_desc;
 detect_request_t recovery;recovery.fallback=2;recovery.max_attempts=5;
 auto recovered=run_detection_session(recovery,[&](const detect_request_t& r,bool final) {
  power_failed=r.attempt==0;
  return run_detection_attempt(list,r,final,[](m5::display_parts_t&,board_t){++adopt_count;return true;});
 });
 assert(recovered.attempts==2&&recovered.adopted&&recovered.result.def==&fallback_desc.def);
 assert(recovered.verdict==verdict_t::confirmed&&recovered.candidate_kind==candidate_kind_t::none);
 assert(construct_count==1&&adopt_count==1&&rollback_count==1&&commit_count==1);
 // The final refined provisional member replaces the initial retained representative.
 rollback_count=commit_count=construct_count=adopt_count=reset_count=0;
 recovery.fallback=3;recovery.hint=2;
 auto yielded=run_detection_session(recovery,[&](const detect_request_t& r,bool final) {
  detected={};detected.assign(&desc);detected.status=detect_status_t::matched;
  power_failed=r.attempt==0;prepare_ok=r.attempt==0||final;
  detected.provisional=final;refined_member=final?&fallback_desc:nullptr;
  return run_detection_attempt(list,r,final,[](m5::display_parts_t&,board_t){++adopt_count;return true;});
 });
 assert(yielded.attempts==5&&!yielded.adopted&&yielded.result.candidate==&fallback_desc.def);
 assert(yielded.candidate_kind==candidate_kind_t::provisional&&yielded.verdict==verdict_t::candidate);
 M5GFX projected;projected.project(yielded);
 assert(projected.getBoard()==board_t::board_unknown&&projected.getCandidateBoard()==static_cast<board_t>(2));
 assert(construct_count==0&&adopt_count==0&&rollback_count==5&&commit_count==0);
 assert(!should_persist_detection(yielded,0));
 refined_member=nullptr;
}
`, "attempt");
});

test("public config stores the next preference and warns after either successful or failed init", async () => {
 const main=await readMain();
 const header=await fs.readFile(path.join(src,"M5GFX.h"),"utf8");
 const setter=body(main,"void M5GFX::setDetectConfig(");
 const config=body(header,"struct detect_config_t");
 const getter=body(header,"const detect_config_t& getDetectConfig(");
 await compileRun(`
#include <cassert>
#define ESP_PLATFORM 1
int warnings=0;
#define ESP_LOGW(...) (++warnings)
enum class board_t {board_unknown,member,other};
struct detect_config_t ${config};
struct M5GFX {detect_config_t _detect_config;bool _detect_started=false;board_t adopted=board_t::board_unknown;
 void setDetectConfig(const detect_config_t& config) ${setter}
 const detect_config_t& getDetectConfig() const ${getter}
};
int main(){M5GFX gfx;detect_config_t cfg;assert(gfx.getDetectConfig().fallback_board==board_t::board_unknown);
 cfg.fallback_board=board_t::member;gfx.setDetectConfig(cfg);assert(warnings==0);
 gfx._detect_started=true;gfx.setDetectConfig(cfg);assert(warnings==0);assert(gfx.adopted==board_t::board_unknown);
 gfx.adopted=board_t::member;cfg.fallback_board=board_t::other;gfx.setDetectConfig(cfg);
 assert(warnings==1);assert(gfx.adopted==board_t::member);assert(gfx.getDetectConfig().fallback_board==board_t::other);
 cfg.fallback_board=board_t::board_unknown;gfx.setDetectConfig(cfg);assert(gfx.getDetectConfig().fallback_board==board_t::board_unknown);assert(warnings==2);
 cfg.fixed_board=board_t::member;gfx.setDetectConfig(cfg);assert(warnings==3);
 gfx.setDetectConfig(cfg);assert(warnings==3);assert(gfx.adopted==board_t::member);
 cfg.fixed_board=board_t::other;gfx.setDetectConfig(cfg);assert(warnings==4);
}
`,"public config");
});

test("production init restores panel according to adoption independently of verdict", async () => {
  const main = await readMain();
  const start = main.indexOf("        if (!package.keep_panel_during_attempt ||");
  assert.ok(start >= 0);
  const restore = main.slice(start, main.indexOf("        return result;", start));
  await compileRun([
    '#include "board_detect/detect_types.hpp"',
    '#include <cassert>',
    'using namespace m5gfx;',
    'int calls, clears;',
    'void panel(int* p) {++calls; if (!p) ++clears;}',
    'struct holder_t {int value; int* get() {return &value;}} _panel_last;',
    'void restore(bool direct, const board_detect::detect_outcome_t& result) {',
    'struct {bool keep_panel_during_attempt;} package={direct};',
    restore,
    '}',
    'int main() {for(int verdict=0;verdict<3;++verdict) for(int adopted=0;adopted<2;++adopted) for(int direct=0;direct<2;++direct) {',
    'calls=clears=0;board_detect::detect_outcome_t result;',
    'result.verdict=static_cast<board_detect::verdict_t>(verdict);result.adopted=adopted;',
    'restore(direct,result);',
    'assert(calls==(!direct?1:adopted?0:2));assert(clears==(direct&&!adopted));',
    '}}',
  ].join("\n"), "panel restoration");
});

test("setup rejection lists preserve all historical board groups", async () => {
  const reject = body(await readMain(), "static bool reject_detected_setup(");
  const groups = [
    ["board_M5StickCPlus"],
    ["board_M5StopWatch", "board_M5PaperMono"],
    ["board_M5ChainCaptain"],
    ["board_M5PaperColor"],
    ["board_M5PaperS3"],
    ["board_M5PaperDIY"],
    ["board_M5Cardputer", "board_M5CardputerADV", "board_M5VAMeter"],
    ["board_M5AirQ"],
    ["board_M5StamPLC"],
    ["board_M5CoreP4X"],
    ["board_M5Tab5", "board_M5Tab5X"],
    ["board_M5StampC5", "board_M5ToughC5"],
    ["board_M5CoreMatrix"],
  ];
  const names = ["board_unknown", ...new Set(groups.flat())];
  await compileRun(`
#include <cassert>
#include <initializer_list>
enum class board_t {${names.join(",")}};
bool reject_default(board_t representative_board) ${reject}
${groups.map((boards, index) => `
#define M5GFX_AUTODETECT_TEST_FAIL_SETUP_BOARDS ${boards.map((name) => `board_t::${name}`).join(",")}
bool reject_${index}(board_t representative_board) ${reject}
#undef M5GFX_AUTODETECT_TEST_FAIL_SETUP_BOARDS
`).join("\n")}
int main() {
 for (const auto board : {${names.map((name) => `board_t::${name}`).join(",")}}) {
  assert(!reject_default(board));
  ${groups.map((boards, index) => `assert(reject_${index}(board) == (${boards.map((name) => `board == board_t::${name}`).join(" || ")}));`).join("\n  ")}
 }
}
`, "setup rejection");
});
