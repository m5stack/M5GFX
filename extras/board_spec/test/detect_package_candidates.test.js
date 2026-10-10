import assert from "node:assert/strict";
import { promises as fs } from "node:fs";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { body, compileRun } from "./detect_host_helpers.js";
const src=path.resolve(path.dirname(fileURLToPath(import.meta.url)),"../../../src");
const read=(name)=>fs.readFile(path.join(src,name),"utf8");

test("package-only Stamp candidates stop probing, never prepare or persist, and preserve prior weak evidence", async()=>{
 const core=await read("board_detect/board_detect.inl"),main=await read("M5GFX.cpp");
 const c5=await read("board_detect/m5/esp32c5/toughc5.inl"),c6=await read("board_detect/m5/esp32c6/c6_displayless.inl");
 const signature=(s,name)=>body(s.slice(s.indexOf(`class ${name}`)),"bool signature(");
 const confirm=(s,name)=>body(s.slice(s.indexOf(`class ${name}`)),"bool confirm(");
 assert.match(c5,/"M5StampC5", def_flag_fallback/);assert.match(c6,/"M5StampC6", def_flag_fallback/);
 await compileRun(`
#include "board_detect/detect_session.hpp"
#include <cassert>
#include <initializer_list>
#include <cstring>
#include <cstdio>
#define CONFIG_IDF_TARGET_ESP32C6 1
#define ESP_LOGD(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
using namespace m5gfx::board_detect;
namespace board_detect=m5gfx::board_detect;
using board_t=board_id_t;
int prepares=0,constructs=0,adopts=0;
namespace lgfx {namespace i2c {bool isInitialized(int){return false;}}}
namespace m5gfx {namespace board_detect {
struct board_desc_t {board_def_t def;struct {const char* const* data=nullptr;unsigned size=0;} option_names;struct {int hw_port=-1;} internal_i2c;};
void board_result_t::assign(const board_desc_t* d){desc=d;def=&d->def;}
struct detection_transaction_t {detection_transaction_t(int,int,bool){}bool valid(){return true;}void rollback(){}struct buses_t {void opened_i2c(int){}} bus; buses_t& buses(){return bus;}};
struct prepare_ctx_t {bool allow_reset=false;board_id_t preferred=0;unsigned attempt=0;bool final_attempt=false;int i2c_port_probe=0;detection_transaction_t* transaction=nullptr;const board_id_t* enabled_ids=nullptr;};
struct probe_ctx_t:prepare_ctx_t {bool confirm_attempted=false,family_identified=false;board_id_t hint=0;const board_def_t* candidate=nullptr;};
struct board_detector_t {const board_def_t* const* members;board_detector_t(const board_def_t* const* m):members(m){}virtual bool signature(probe_ctx_t&)const=0;virtual bool confirm(probe_ctx_t&,board_result_t*)const=0;bool has_member(board_id_t id)const{for(auto p=members;*p;++p)if((*p)->id==id)return true;return false;}};
inline int pins(int){return 0;}inline int no_pins(){return 0;}
bool prepare(const board_desc_t&,board_result_t&,const prepare_ctx_t&){++prepares;return true;}
namespace m5 {namespace wiring {namespace detection {const int unconditional_pins=0;}}}
}}
#include "board_detect/m5/generated/detector_order_constraints.hpp"
const char* tag="host";const int probe_i2c_port=0;
bool enabled(const probe_ctx_t&,board_id_t){return true;}
bool fallback_only(const board_detector_t* detector) ${body(core,"bool fallback_only(")}
bool gpio_power_hold_family(const board_detector_t*){return false;}
bool run_detector(const board_detector_t* detector,probe_ctx_t& ctx,board_result_t* result) ${body(core,"bool run_detector(")}
namespace m5gfx {namespace board_detect {
board_result_t detect_board(const board_detector_t* const* list,board_id_t hint,probe_ctx_t& ctx) ${body(core,"board_result_t detect_board(")}
}}
template<class Setup>detect_outcome_t finish_detection_setup(board_result_t&,detect_outcome_t out,detection_transaction_t&,board_t,Setup){++constructs;return out;}
template<class Setup>detect_outcome_t run_detection_attempt(const board_detector_t* const* detectors,const detect_request_t& request,bool final_attempt,Setup setup) ${body(main,"static board_detect::detect_outcome_t run_detection_attempt(")}
unsigned flash,psram;
#define EFUSE_RD_MAC_SYS2_REG 0
#define EFUSE_FLASH_CAP_S 0
#define EFUSE_FLASH_CAP_V 7
#define EFUSE_PSRAM_CAP_S 3
#define EFUSE_PSRAM_CAP_V 3
#define REG_READ(x) (flash | (psram<<3))
namespace c6_displayless_detail {unsigned flash_capacity(){return flash;}}
const board_desc_t desc_stampc5={{153,"StampC5",def_flag_fallback}},desc_stampc6={{154,"StampC6",def_flag_fallback}};
const board_def_t c5later={33,"ToughC5",0},c6later={140,"NanoC6",0},earlier={999,"earlier",def_flag_fallback};
struct stamp_t:board_detector_t {bool c5;stamp_t(const board_def_t*const*m,bool five):board_detector_t(m),c5(five){}
 bool sig5(probe_ctx_t& ctx)const ${signature(c5,"stampc5_detector_t")}
 bool sig6(probe_ctx_t& ctx)const ${signature(c6,"stampc6_detector_t")}
 bool signature(probe_ctx_t& ctx)const override{return c5?sig5(ctx):sig6(ctx);}
 bool confirm(probe_ctx_t& ctx,board_result_t* result)const override{if(c5) ${confirm(c5,"stampc5_detector_t")} else ${confirm(c6,"stampc6_detector_t")}}
};
struct later_t:board_detector_t {mutable int calls=0;later_t(const board_def_t*const*m):board_detector_t(m){}bool signature(probe_ctx_t&)const override{++calls;return false;}bool confirm(probe_ctx_t&,board_result_t*)const override{return false;}};
struct seed_t:board_detector_t {seed_t(const board_def_t*const*m):board_detector_t(m){}bool signature(probe_ctx_t&ctx)const override{ctx.candidate=&earlier;return false;}bool confirm(probe_ctx_t&,board_result_t*)const override{return false;}};
int main(){for(bool five:{false,true})for(unsigned f=0;f<4;++f)for(unsigned p=0;p<4;++p)for(bool seeded:{false,true})for(unsigned start:{0u,4u})for(board_id_t pref:{board_id_t(0),board_id_t(888)}) {
 const auto* desc=five?&desc_stampc5:&desc_stampc6;const auto* next=five?&c5later:&c6later;
 const board_def_t* sm[]={&desc->def,nullptr};const board_def_t* lm[]={next,nullptr};const board_def_t* em[]={&earlier,nullptr};
 for(board_id_t hint:{board_id_t(0),desc->def.id,next->id}){
 flash=f;psram=p;stamp_t stamp(sm,five);later_t later(lm);seed_t seed(em);
 const board_detector_t* normal[]={&stamp,&later,nullptr};const board_detector_t* withseed[]={&seed,&stamp,&later,nullptr};
 detect_request_t req;req.attempt=start;req.max_attempts=5;req.hint=hint;req.preferred=pref;
 prepares=constructs=adopts=0;
 auto out=run_detection_session(req,[&](const detect_request_t&r,bool final){return run_detection_attempt(seeded?withseed:normal,r,final,[]{++adopts;});});
 const bool matches=five?(f==1&&p==0):f==2;
 assert(out.attempts==1&&!out.setup_succeeded);assert(later.calls==int(!matches));
 assert(!prepares&&!constructs&&!adopts&&!should_persist_detection(out,0));
 // Hinted later families are blocked by generated module-protection edges.
 const auto* expected=seeded?&earlier:matches?&desc->def:nullptr;
 assert(out.result.candidate==expected);assert(out.verdict==(expected?verdict_t::candidate:verdict_t::unknown));
 assert(out.candidate_kind==(expected?candidate_kind_t::weak:candidate_kind_t::none));
 }
}}
`,"Stamp package weak outcomes");
});

test("P4 generated families leave absent evidence unknown and retain a Tab5 weak candidate", async()=>{
 const order=await read("board_detect/m5/generated/esp32p4_detector_order.hpp");
 assert.doesNotMatch(order,/stampp4/);
 await compileRun(`
#include "board_detect/detect_session.hpp"
#include <cassert>
#include <initializer_list>
using namespace m5gfx::board_detect;
const board_def_t tab={22,"Tab5",def_flag_fallback};
struct probe_ctx_t {const board_def_t* candidate=nullptr;};
struct board_detector_t {bool tab;bool signature(probe_ctx_t&ctx)const{if(tab&&weak)ctx.candidate=&::tab;return false;}static bool weak;};
bool board_detector_t::weak=false;
const board_detector_t corep4x_detector={false},tab5_family_detector={true},unitpoep4_detector={false};
constexpr unsigned max_detector_families=12;
${order.replace('#pragma once','')}
int main(){for(bool weak:{false,true}){board_detector_t::weak=weak;probe_ctx_t ctx;
 detect_request_t req;req.max_attempts=5;
 auto out=run_detection_session(req,[&](const detect_request_t&,bool){for(auto p=esp32p4_detectors;*p;++p)(*p)->signature(ctx);
 detect_outcome_t out;out.reason=fail_reason_t::no_signature;out.result.candidate=ctx.candidate;
 if(ctx.candidate){out.verdict=verdict_t::candidate;out.candidate_kind=candidate_kind_t::weak;}return out;});
 assert(out.attempts==1&&out.result.candidate==(weak?&tab:nullptr));
 assert(out.verdict==(weak?verdict_t::candidate:verdict_t::unknown));assert(!should_persist_detection(out,0));
}}
`,"P4 has no module-only candidate");
});

test("Unified defaults preserve build selections and supported package defaults only", async(t)=>{
 const unified=process.env.M5UNIFIED_PATH||path.resolve(src,"../../M5Unified");
 let impl;
 try { impl=await fs.readFile(path.join(unified,"src/M5Unified.inl"),"utf8"); }
 catch { return t.skip("M5Unified checkout not found"); }
 const fallback=body(impl,"board_t M5Unified::_default_fallback_board(");
 const names=[...new Set(fallback.match(/board_[A-Za-z0-9_]+/g))];
 const base=`#include <cassert>\nenum class board_t {${names.join(',')}};\nunsigned pkg=0,revision=0,pkg_reads=0;namespace m5gfx {unsigned get_pkg_ver(){++pkg_reads;return pkg;}}\nstruct esp_chip_info_t {unsigned revision;};void esp_chip_info(esp_chip_info_t*p){p->revision=revision;}\nstruct M5Unified {static board_t fallback() ${fallback}};\n`;
 const cases=[['M5UNIFIED_PC_BUILD','for(pkg=0;pkg<8;++pkg)assert(M5Unified::fallback()==board_t::board_unknown);'],
 ['CONFIG_IDF_TARGET_ESP32','for(pkg=0;pkg<8;++pkg)assert(M5Unified::fallback()==(pkg==6?board_t::board_M5AtomPsram:pkg==5?board_t::board_M5StampPico:board_t::board_unknown));'],
 ['CONFIG_IDF_TARGET_ESP32S3','for(pkg=0;pkg<2;++pkg)assert(M5Unified::fallback()==(pkg==1?board_t::board_M5StampS3Mini:board_t::board_M5StampS3));'],
 ['CONFIG_IDF_TARGET_ESP32C3','assert(M5Unified::fallback()==board_t::board_M5StampC3U);'],
 ...['ESP32C5','ESP32C6','ESP32H2'].map(chip=>['CONFIG_IDF_TARGET_'+chip,'for(pkg=0;pkg<8;++pkg)assert(M5Unified::fallback()==board_t::board_unknown);assert(pkg_reads==0);']),
 ['CONFIG_IDF_TARGET_ESP32P4','for(revision=299;revision<302;++revision)assert(M5Unified::fallback()==(revision>=300?board_t::board_M5StampP4X:board_t::board_M5StampP4));']];
 await Promise.all(cases.map(([macro,checks])=>compileRun(`#define CONFIG_IDF_TARGET 1\n#define ${macro} 1\n${base}int main(){${checks}}`,macro)));
 const buildSelections=[['ARDUINO_M5STACK_CORE_ESP32','M5Stack'],['ARDUINO_M5STACK_FIRE','M5Stack'],['ARDUINO_M5Stack_Core_ESP32','M5Stack'],['ARDUINO_M5STACK_CORE2','M5StackCore2'],['ARDUINO_M5STACK_Core2','M5StackCore2'],['ARDUINO_M5STICK_C','M5StickC'],['ARDUINO_M5Stick_C','M5StickC'],['ARDUINO_M5STICK_C_PLUS','M5StickCPlus'],['ARDUINO_M5Stick_C_Plus','M5StickCPlus'],['ARDUINO_M5STACK_COREINK','M5StackCoreInk'],['ARDUINO_M5Stack_CoreInk','M5StackCoreInk'],['ARDUINO_M5STACK_PAPER','M5Paper'],['ARDUINO_M5STACK_Paper','M5Paper'],['ARDUINO_M5STACK_TOUGH','M5Tough'],['ARDUINO_M5STACK_ATOM','M5AtomLite'],['ARDUINO_M5Stack_ATOM','M5AtomLite'],['ARDUINO_M5STACK_TIMER_CAM','M5TimerCam'],['ARDUINO_M5Stack_Timer_CAM','M5TimerCam']];
 await Promise.all(buildSelections.map(([macro,board])=>compileRun(`#define CONFIG_IDF_TARGET_ESP32 1\n#define ${macro} 1\n${base}int main(){assert(M5Unified::fallback()==board_t::board_${board});assert(pkg_reads==0);}`,macro)));
});
