import assert from "node:assert/strict";
import { promises as fs } from "node:fs";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { body, compileRun } from "./detect_host_helpers.js";
const src = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../../../src");
// Public-API tests use the original fixtures.
async function readMain() {
  return (await Promise.all(["M5GFX.cpp", "board_detect/detection_package.inl", "board_detect/detection_integration.inl"].map((name) => fs.readFile(path.join(src, name), "utf8")))).join("\n");
}

test("fixed acceptance bypasses NVS and detection and retries clean startup after failure", async () => {
  const main = await readMain();
  const registry = await fs.readFile(path.join(src, "board_detect/m5/board_registry.inl"), "utf8");
  const fixed = body(main, "static board_detect::detect_outcome_t run_fixed_startup(");
  const session = body(main, "static board_detect::detect_outcome_t run_fixed_startup_session(");
  const select = body(main, "const board_detect::m5::board_entry_t* select_fixed_board(");
  const finish = body(main, "static board_detect::detect_outcome_t construct_and_adopt(");
  const find = body(registry, "const board_entry_t* find_board(");
  const start = main.indexOf("    _detect_started = true;", main.indexOf("bool M5GFX::init_impl("));
  const end = main.indexOf("\n#if defined (M5GFX_HAS_BOARD_DETECT)\n    static constexpr char NVS_KEY", start);
  assert.ok(end > start, "fixed startup prefix ends before the guarded NVS path");
  const prefix = main.slice(start, end);
  const getterSource = await fs.readFile(path.join(src, "M5GFX.h"), "utf8");
  const fixedGetter = body(getterSource, "board_t getFixedBoard(");
  await compileRun(`
#include "board_detect/detect_types.hpp"
#include <cassert>
#include <cstddef>
#include <cstddef>
#include <initializer_list>
#include <vector>
#define CONFIG_IDF_TARGET 1
#define CONFIG_IDF_TARGET_ESP32S3 1
#define M5GFX_HAS_BOARD_DETECT
#define ESP_LOGW(...) (++warnings)
#define ESP_LOGI(...) ((void)0)
using namespace m5gfx;
enum class board_t:std::uint32_t {board_unknown=0,member=1,unsupported=2};
int warnings,nvs_calls,detect_calls,construct_calls,adopt_calls,finish_calls,rollback_calls,commit_calls,start_calls;
bool prepare_ok,construct_ok,adopt_ok,displayless;
bool opi_active=false,description_ok=true,capture_ok=true;int captures;
bool conditional_detection_pins_unavailable(){return opi_active;}
std::vector<bool> reset_flags;int startup_countdown=0;
namespace lgfx {namespace i2c {bool isInitialized(int) {return false;}}}
namespace m5gfx {namespace board_detect {
constexpr int max_detection_pins=64;
struct list_t {const std::int8_t* data;std::size_t size;};
using pin_list_t=list_t;
struct board_desc_t {
 board_def_t def;
 struct {int hold_pin;} power;
 struct {int pin;} reset;
 struct {int sclk,mosi,miso,dc,cs,rst,busy;} display;
 struct {int sclk,mosi,miso,sd_cs,other_cs;} sd;
 struct {int sda,scl,hw_port;} internal_i2c;
 list_t hold_high_pins,op_gpio_pins;
};
void board_result_t::assign(const board_desc_t* value) {desc=value;def=value?&value->def:&board_def_unknown;}
struct prepare_ctx_t {bool allow_reset=true;int probe_port=-1;detection_transaction_t* transaction=nullptr;};
inline pin_list_t no_pins() {return {nullptr,0};}
struct detection_transaction_t {
 detection_transaction_t(pin_list_t pins,pin_list_t,bool) {
  ++captures;for(std::size_t i=0;i<pins.size;++i) {assert(pins.data[i]>=0);}
 }
 bool valid() {return capture_ok;}
 void rollback() {++rollback_calls;}void commit(){++commit_calls;}
 void restore_to_snapshot(const std::int8_t*,std::size_t) {}
 struct buses_t {void opened_i2c(int){}} buses_;
 buses_t& buses(){return buses_;}
};
namespace prepare_detail {bool description_valid(const board_desc_t&) {return description_ok;}}
bool prepare(const board_desc_t&,board_result_t& result,const prepare_ctx_t& ctx) {
 reset_flags.push_back(ctx.allow_reset);
 assert(result.option==0&&result.prepared==0&&!result.provisional&&result.refine==nullptr&&result.candidate==nullptr);
 ++start_calls;result.option=4;result.prepared=prepared_power;return prepare_ok||(startup_countdown>0&&--startup_countdown==0);
}
namespace m5 {
namespace wiring {namespace detection {const std::int8_t opi_pins[]={33,34,35,36,37};}}
struct display_parts_t {int* bus=nullptr;int* panel=nullptr;int* light=nullptr;int* touch=nullptr;};
enum class construct_status_t {ok,no_display,failed};
using start_t=bool(*)(board_result_t&,const prepare_ctx_t&);
struct board_entry_t {const board_desc_t* desc;start_t fixed_prepare;};
template<std::size_t BoardCount> const board_entry_t* find_board(const board_entry_t (&boards)[BoardCount],board_id_t id) ${find}
board_desc_t desc={{1,"member",0},{0},{1},{2,3,4,5,6,1,7},{-1,-1,-1,-1,-1},{-1,-1,-1},{nullptr,0},{nullptr,0}};
const board_entry_t entries[]={{&desc,nullptr}};
const board_entry_t (&esp32s3_boards)[1]=entries;
construct_status_t construct_detected_board(const board_result_t& result,display_parts_t*) {
 assert(result.desc==&desc&&result.def==&desc.def&&result.option==4&&result.prepared==prepared_power&&!result.provisional&&result.refine==nullptr);
 ++construct_calls;return !construct_ok?construct_status_t::failed:displayless?construct_status_t::no_display:construct_status_t::ok;
}
void destroy_display_parts(display_parts_t*) {}
struct log_t {const char* name;const char* annotation;};
log_t success_log(const board_result_t&){return {nullptr,nullptr};}
}
}}
const int detection_i2c_port=-1;
template<class Adopt> board_detect::detect_outcome_t construct_and_adopt(board_detect::board_result_t& result,board_detect::detect_outcome_t outcome,board_detect::detection_transaction_t& transaction,board_t representative_board,Adopt adopt) ${finish}
template<class Adopt> board_detect::detect_outcome_t run_fixed_startup(const board_detect::m5::board_entry_t& entry,bool allow_reset,Adopt adopt) ${fixed}
template<class Adopt> board_detect::detect_outcome_t run_fixed_startup_session(const board_detect::m5::board_entry_t& entry,bool allow_reset,Adopt adopt) ${session}
const board_detect::m5::board_entry_t* select_fixed_board(board_t board) ${select}
bool reject_detected_setup(board_t){return false;}
struct M5GFX {
 bool _detect_started=false;
 board_t _board=board_t::board_unknown,_board_candidate=board_t::member,_fixed_board=board_t::board_unknown;
 struct {board_t fixed_board=board_t::board_unknown,fallback_board=board_t::board_unknown;} _detect_config;
 board_t getBoard() const {return _board;}
 board_t getFixedBoard() const ${fixedGetter}
 bool _adopt_detected_parts(int*,int*,int*,int*){++adopt_calls;return adopt_ok;}
 bool _init_adopted_panel(bool){++finish_calls;return true;}
 bool init(bool use_reset=true,bool use_clear=true) {
 ${prefix}
 ++nvs_calls;++detect_calls;return false;
 }
};
int main() {
 using namespace board_detect;
 using board_detect::m5::desc;
 prepare_ok=construct_ok=adopt_ok=true;
 M5GFX unsupported;unsupported._detect_config.fixed_board=board_t::unsupported;
 assert(!unsupported.init()&&unsupported.getFixedBoard()==board_t::board_unknown);
 assert(nvs_calls==0&&detect_calls==0&&start_calls==0);
 for(int failure=0;failure<3;++failure) {
 M5GFX gfx;gfx._detect_config.fixed_board=board_t::member;
 prepare_ok=failure!=0;construct_ok=failure!=1;adopt_ok=failure!=2;
 assert(!gfx.init());assert(gfx.getBoard()==board_t::board_unknown&&gfx.getFixedBoard()==board_t::member);
 prepare_ok=construct_ok=adopt_ok=true;
 assert(gfx.init());assert(gfx.getBoard()==board_t::member&&gfx.getFixedBoard()==board_t::member);
 assert(gfx._board_candidate==board_t::board_unknown);
 assert(nvs_calls==0&&detect_calls==0);
 }
 assert(commit_calls==3&&rollback_calls==15);
 for(bool reset : {false,true}) for(int recovered_at=0;recovered_at<=5;++recovered_at) {
  prepare_ok=false;startup_countdown=recovered_at;reset_flags.clear();
  M5GFX retry;retry._detect_config.fixed_board=board_t::member;
  assert(retry.init(reset)==(recovered_at!=0));
  assert(retry.getBoard()==(recovered_at?board_t::member:board_t::board_unknown));
  const int count=recovered_at?recovered_at:5;
  assert(reset_flags.size()==static_cast<unsigned>(count));
  for(int attempt=0;attempt<count;++attempt) {assert(reset_flags[attempt]==(reset||attempt>=3));}
 }
 prepare_ok=true;
 const int old_captures=captures,old_starts=start_calls;
 opi_active=true;desc.display.cs=33;M5GFX opi;opi._detect_config.fixed_board=board_t::member;
 assert(!opi.init()&&opi.getFixedBoard()==board_t::member&&opi.getBoard()==board_t::board_unknown);
 assert(captures==old_captures&&start_calls==old_starts);desc.display.cs=6;
 assert(opi.init());opi_active=false;
 description_ok=false;M5GFX invalid;invalid._detect_config.fixed_board=board_t::member;
 int before_warnings=warnings;assert(!invalid.init());assert(warnings==before_warnings+5);description_ok=true;
 capture_ok=false;M5GFX snapshot;snapshot._detect_config.fixed_board=board_t::member;
 before_warnings=warnings;assert(!snapshot.init());assert(warnings==before_warnings+5);capture_ok=true;
 std::int8_t excess[65];for(int i=0;i<65;++i)excess[i]=i;desc.op_gpio_pins={excess,65};
 M5GFX overflow;overflow._detect_config.fixed_board=board_t::member;before_warnings=warnings;
 assert(!overflow.init());assert(warnings==before_warnings+5);desc.op_gpio_pins={nullptr,0};
 M5GFX wide;wide._detect_config.fixed_board=static_cast<board_t>(0x10001);
 assert(!wide.init()&&wide.getFixedBoard()==board_t::board_unknown);
 displayless=true;M5GFX no_display;no_display._detect_config.fixed_board=board_t::member;
 assert(no_display.init()&&no_display.getBoard()==board_t::member&&no_display.getFixedBoard()==board_t::member);
 M5GFX initialized;initialized._board=board_t::member;initialized._detect_config.fixed_board=board_t::unsupported;
 const int before=start_calls;assert(initialized.init()&&initialized.getFixedBoard()==board_t::board_unknown&&start_calls==before);
 assert(nvs_calls==0&&detect_calls==0);
 M5GFX auto_gfx;assert(!auto_gfx.init());assert(nvs_calls==1&&detect_calls==1);
}
`, "fixed entry and startup");
});

test("M5Unified BOARD_ID guard accepts positive values without M5GFX_BOARD and fixed selection wins", async (t) => {
 const unified=process.env.M5UNIFIED_PATH||path.resolve(src,"../../M5Unified");
 let header;try{header=await fs.readFile(path.join(unified,"src/M5Unified.hpp"),"utf8");}catch{return t.skip("M5Unified checkout absent");}
 const guard=[...header.matchAll(/#if defined \(BOARD_ID\) && \(\(BOARD_ID \+ 0\) > 0\)[\s\S]*?#endif/g)].map(m=>m[0]).find(s=>s.includes("detect_config.fixed_board"));
 assert.ok(guard);
 const start=header.indexOf("      const auto fixed_board = Display.getFixedBoard();");
 const end=header.indexOf("      _board = board;",start)+"      _board = board;".length;
 const selection=header.slice(start,end);
 const definitions=["","#define BOARD_ID","#define BOARD_ID 0","#define BOARD_ID 10"];
 for(const define of definitions){
  await compileRun(`
#include <cassert>
#include <cstddef>
${define}
enum class board_t{board_unknown=0,fixed=1,fallback=2,candidate=3,default_board=4};
struct config_t {board_t fixed_board=board_t::board_unknown;};
int main(){config_t detect_config;
${guard}
assert(static_cast<int>(detect_config.fixed_board)==${define.includes("10")?10:0});}
`,"BOARD_ID guard");
 }
 await compileRun(`
#include <cassert>
#include <cstddef>
#define ESP_LOG_WARN 1
enum class board_t{board_unknown=0,fixed=1,fallback=2,candidate=3,default_board=4};
int warnings;void Log(int,const char*,unsigned,unsigned){++warnings;}
struct display_t {
 board_t fixed,adopted,candidate;
 board_t getFixedBoard(){return fixed;}board_t getBoard(){return adopted;}board_t getCandidateBoard(){return candidate;}
};
struct unified_t {
 display_t Display;struct {board_t fallback_board;} cfg;
 board_t _board;
 board_t _default_fallback_board(){return board_t::default_board;}
 void select(){
 ${selection}
 }
};
int main(){for(int fixed=0;fixed<2;++fixed)for(int adopted=0;adopted<2;++adopted)for(int fallback=0;fallback<2;++fallback)for(int candidate=0;candidate<2;++candidate){
 warnings=0;unified_t u;u.Display={fixed?board_t::fixed:board_t::board_unknown,adopted?board_t::fallback:board_t::board_unknown,candidate?board_t::candidate:board_t::board_unknown};u.cfg.fallback_board=fallback?board_t::fallback:board_t::board_unknown;
 u.select();assert(u._board==(fixed?board_t::fixed:adopted||fallback?board_t::fallback:candidate?board_t::candidate:board_t::default_board));
 assert(warnings==(fixed&&fallback));
}}
`,"fixed and fallback selection");
});

test("fixed GPIO46 pre-hold follows the existing power pin table", async (t) => {
 const unified=process.env.M5UNIFIED_PATH||path.resolve(src,"../../M5Unified");
 let header, impl;
 try {
  header=await fs.readFile(path.join(unified,"src/M5Unified.hpp"),"utf8");
  impl=await fs.readFile(path.join(unified,"src/M5Unified.inl"),"utf8");
 } catch { return t.skip("M5Unified checkout not found"); }
 const table=/static constexpr const uint8_t _pin_table_other1\[\]\[2\] = \{[\s\S]*?\n\};/.exec(impl)[0];
 const getter=body(impl,"int8_t M5Unified::_get_power_hold_pin(board_t id)");
 const guard=[...header.matchAll(/#if defined \(BOARD_ID\) && \(\(BOARD_ID \+ 0\) > 0\)[\s\S]*?#endif/g)].map(m=>m[0]).find(s=>s.includes("gpio46_hold"));
 for(const fixed of [undefined,0,1,2,3,4,5,6]) {
 await compileRun(`
#include <cstdint>
#include <cassert>
#define CONFIG_IDF_TARGET_ESP32S3 1
#define GPIO_NUM_46 46
#define GPIO_NUM_44 44
${fixed===undefined?"":"#define BOARD_ID "+fixed}
struct board_t {enum value {board_unknown=0,board_M5Dial=1,board_M5Capsule=2,board_M5AirQ=3,board_M5DinMeter=4,board_M5PaperS3=5,board_M5AtomS3=6};uint8_t id;board_t(int v):id(v){} operator uint8_t() const{return id;}};
namespace m5gfx {using ::board_t;}
${table}
struct M5Unified {
 static int8_t _get_power_hold_pin(board_t id) ${getter}
 struct {int board=0;int getBoard(){return board;}} Display;
 bool hold(){
 ${guard}
 return gpio46_hold;
 }
};
int main(){M5Unified u;assert(u.hold()==${fixed===undefined||fixed===0||(fixed>=1&&fixed<=4)?"true":"false"});u.Display.board=6;assert(!u.hold());assert(M5Unified::_get_power_hold_pin(board_t(99))==-1);}
`,"fixed power hold");
 }
});
