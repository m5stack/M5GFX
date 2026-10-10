import { promises as fs } from "node:fs";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { body, compileRun } from "./detect_host_helpers.js";
const here = path.dirname(fileURLToPath(import.meta.url));
const src = path.resolve(here, "../../../src/board_detect");
const common = `
#include "board_detect/detect_session.hpp"
#include <cassert>
#include <initializer_list>
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGD(...) ((void)0)
#define ESP_LOGI(...) ((void)0)
using namespace m5gfx::board_detect;
namespace m5gfx { namespace board_detect {
struct board_desc_t {board_def_t def;};
void board_result_t::assign(const board_desc_t* d) {desc=d; def=&d->def;}
struct prepare_ctx_t {board_id_t preferred=0,hint=0; unsigned attempt=0; bool final_attempt=false;};
struct probe_ctx_t : prepare_ctx_t {const board_def_t* candidate=nullptr; bool conditional_pins_unavailable=false;};
} }
const board_desc_t desc_core2={{1,"Core2",0}},desc_tough={{2,"Tough",0}},desc_cardputer={{3,"Cardputer",0}},desc_cardputer_adv={{4,"ADV",0}},desc_vameter={{5,"VAMeter",0}},desc_unitc6l={{6,"UnitC6L",0}},desc_nesson1={{7,"NessoN1",0}},desc_papers3={{8,"PaperS3",0}},desc_paperdiy={{9,"PaperDIY",0}};
`;

test("production Core2 and Cardputer selectors cover preference, hint and observed compatibility", async () => {
  const detector = await fs.readFile(path.join(src, "board_detect.inl"),"utf8");
  const core = await fs.readFile(path.join(src,"m5/esp32_d0wdq6.inl"),"utf8");
  const families = await fs.readFile(path.join(src,"m5/esp32s3/families.inl"),"utf8");
  const helper = body(detector,"static bool select_provisional_member");
  const begin=core.indexOf('const auto possible = [&](board_id_t id)'), end=core.indexOf('\n      log_panel_variant',begin);
  const selectCore=core.slice(begin, end).replace(/\n      }\s*$/, "");
  const card=body(families.slice(families.indexOf('class cardputer_family_detector_t')),"bool confirm(");
  await compileRun(common+`
namespace wiring {
namespace cardputer { namespace cardputer_subdivision {constexpr int sense_pins[]={0,1,2,3}; constexpr int vameter_i2c_sda=0,vameter_i2c_scl=1; constexpr int vameter_i2c_addrs[]={0x40,0x41};} }
namespace cardputer_adv {constexpr int internal_i2c_sda=2,internal_i2c_scl=3;}
}
namespace specs {namespace cardputer {constexpr bool bus_three_wire=true;}}
const int cardputer_probes[]={1};
struct pulls_t {std::uint64_t pulldown_high;};
std::uint64_t observed;
bool probe_spi_id(probe_ctx_t&,const board_desc_t& d,const int*,int,board_result_t* r,bool) {r->assign(&d); return true;}
pulls_t probe_pin_pulls(probe_ctx_t&,std::uint64_t) {return {observed};}
pulls_t recover_held_sda_and_resample(probe_ctx_t&,pulls_t p,std::uint64_t,int,int) {return p;}
bool probe_i2c_read(probe_ctx_t&,int,int,int,int,std::uint8_t*,int,int,int,bool) {return false;}
bool probe_i2c_ack(probe_ctx_t&,int,int,int) {return false;}
bool select_provisional_member(const prepare_ctx_t& ctx,board_result_t* result,const board_desc_t* preferred_if_possible,const board_desc_t* hinted_if_possible,const board_desc_t* family_default,const char* why) ${helper}
namespace generated_options {namespace core2 {constexpr unsigned new_pmic=1;}}
struct tx_t {void restore_start(int) {}};
bool core_select(prepare_ctx_t ctx,board_result_t& result) {tx_t tx; struct wrapper: prepare_ctx_t {tx_t* transaction;}; wrapper wrapped; static_cast<prepare_ctx_t&>(wrapped)=ctx; wrapped.transaction=&tx; const auto signals=0;
 ${selectCore.replaceAll('ctx.transaction','wrapped.transaction')}
 return true;
}
bool card_select(probe_ctx_t& ctx,board_result_t* result) ${card}
int main() {
 for(bool new_pmic: {false,true}) for(bool hint_present: {false,true}) for(board_id_t preferred: {board_id_t(0),board_id_t(1),board_id_t(2),board_id_t(42)}) {
  prepare_ctx_t ctx;ctx.preferred=preferred;ctx.hint=hint_present ? 2:0;board_result_t r;r.option=new_pmic?generated_options::core2::new_pmic:0;
  ctx.final_attempt=false;assert(!core_select(ctx,r));ctx.final_attempt=true;assert(core_select(ctx,r));
  assert(r.provisional);assert(r.def->id==(preferred==1 ? 1:preferred==2&&!new_pmic ? 2:hint_present&&!new_pmic ? 2:1));
  detect_outcome_t out;out.result=r;const bool setup=finalize_prepared_result(out,preferred);
  assert(setup==(preferred==0||preferred==r.def->id));assert(!should_persist_detection(out,0));
 }
 for(std::uint64_t mask: {std::uint64_t(3),std::uint64_t(12)})
 for(board_id_t preferred: {board_id_t(0),board_id_t(3),board_id_t(4),board_id_t(5),board_id_t(42)})
 for(board_id_t hint: {board_id_t(0),board_id_t(3),board_id_t(4),board_id_t(5)}) {
  observed=mask;probe_ctx_t ctx;ctx.hint=hint;ctx.preferred=preferred;board_result_t r;
  assert(!card_select(ctx,&r));ctx.final_attempt=true;assert(card_select(ctx,&r));assert(r.provisional);
  const auto compatible=[&](board_id_t id){return id==3||(id==4&&mask==12)||(id==5&&mask==3);};
  const auto expected=compatible(preferred)?preferred:compatible(hint)?hint:3;
  assert(r.def->id==expected);detect_outcome_t out;out.result=r;
  assert(finalize_prepared_result(out,preferred)==(preferred==0||preferred==expected));
 }
}
`,"provisional selectors");
});

test("production SPI and Paper hint restrictions recover a different member on attempt one", async () => {
 const detector=await fs.readFile(path.join(src,"board_detect.inl"),"utf8");
 const families=await fs.readFile(path.join(src,"m5/esp32s3/families.inl"),"utf8");
 const spi=body(detector.slice(detector.indexOf('class spi_id_detector_t')),'bool confirm(');
 const paper=body(families.slice(families.indexOf('class paper_family_detector_t')),'bool confirm(');
 const main=await fs.readFile(path.join(src,"../M5GFX.cpp"),"utf8");
 const compat=body(main,"board_t M5GFX::autodetect(").replaceAll("board_t::board_unknown","board_id_unknown");
 await compileRun(common+`
struct member_t {const board_desc_t* desc;};
namespace wiring {namespace papers3 {constexpr int internal_i2c_sda=0,internal_i2c_scl=1;}}
namespace pmic_ops {constexpr int pm1_i2c_addr=0x6e,pm1_i2c_freq=100000,pm1_device_id=0x2050;}
namespace specs {namespace papers3 {namespace touch {constexpr int i2c_freq=400000;}}}
board_id_t actual;
bool probe_i2c_read(probe_ctx_t&,int,int,int addr,int,std::uint8_t* p,int,int,int,bool=false) {
 if(actual==9&&addr==0x6e) {p[0]=0x50;p[1]=0x20;return true;}
 if(actual==8&&addr==0x14) {p[0]='9';p[1]='1';p[2]='1';p[3]=0;return true;}return false;
}
struct spi_t {
 bool shared_id_read_=false;std::uint8_t member_count_=2;
 member_t members_desc_[2]={{&desc_core2},{&desc_tough}};
 bool probe_family(probe_ctx_t&,board_result_t*) const {return false;}
 bool probe_member(probe_ctx_t&,const member_t& m,board_result_t* r) const {if(m.desc->def.id!=actual) {return false;}r->assign(m.desc);return true;}
 bool confirm(probe_ctx_t& ctx,board_result_t* result) const ${spi}
};
bool paper_confirm(probe_ctx_t& ctx,board_result_t* result) ${paper}
using board_t=board_id_t;
namespace board_detect=m5gfx::board_detect;
namespace m5gfx {namespace board_detect {namespace m5 {
 struct display_parts_t {int* bus=nullptr;int* panel=nullptr;int* light=nullptr;int* touch=nullptr;};
}}}
struct package_t {const int* detectors=nullptr;bool conditional_pins_unavailable=false;};
package_t select_detection_package(bool) {static const int family=1;package_t p;p.detectors=&family;return p;}
bool reject_detected_setup(board_t) {return false;}
template<class Setup> detect_outcome_t run_detection_attempt(const int*,const detect_request_t& request,bool final,Setup) {
 probe_ctx_t ctx;ctx.hint=request.hint;ctx.attempt=request.attempt;ctx.final_attempt=final;detect_outcome_t out;
 if(actual<3 ? spi_t{}.confirm(ctx,&out.result):paper_confirm(ctx,&out.result)) {
  finalize_prepared_result(out,request.preferred);out.setup_succeeded=true;
 }
 return out;
}
struct M5GFX {
 struct {board_t fallback_board=0;} _detect_config;
 struct {int* get() {return nullptr;}} _panel_last;
 void panel(int*) {}
 bool _adopt_detected_parts(int*,int*,int*,int*) {return true;}
 board_t autodetect(bool use_reset,board_t board,bool final_attempt,bool* transient_fallback,bool* no_signature,board_t* candidate_board) ${compat}
};
int main() {
 for(board_id_t target: {board_id_t(1),board_id_t(2),board_id_t(8),board_id_t(9)}) {
  actual=target;probe_ctx_t ctx;ctx.hint=target<3 ? 3-target:17-target;board_result_t r;
  const auto confirm=[&](const detect_request_t& req,bool) {ctx.attempt=req.attempt;ctx.hint=req.hint;ctx.final_attempt=req.attempt+1==req.max_attempts;detect_outcome_t out;
    bool match=target<3 ? spi_t{}.confirm(ctx,&out.result):paper_confirm(ctx,&out.result);
    if(match) {finalize_prepared_result(out,0);out.setup_succeeded=true;}return out;};
  detect_request_t req;req.max_attempts=5;req.hint=ctx.hint;auto out=run_detection_session(req,confirm);
  assert(out.setup_succeeded&&out.result.def->id==target&&out.attempts==2);assert(out.verdict==verdict_t::confirmed);
  ctx.attempt=0;ctx.final_attempt=true;board_result_t final_result;
  assert(target<3 ? spi_t{}.confirm(ctx,&final_result):paper_confirm(ctx,&final_result));
  assert(final_result.def->id==target);
  M5GFX gfx;board_t candidate=0;bool transient=false,no_signature=false;
  assert(gfx.autodetect(false,ctx.hint,false,&transient,&no_signature,&candidate)==0);
  assert(gfx.autodetect(false,ctx.hint,true,&transient,&no_signature,&candidate)==target);
 }
}
`,"retry hint restrictions");
});

test("production C6 display signature returns an unsaved weak UnitC6L independently of hint", async () => {
 const source=await fs.readFile(path.join(src,"m5/esp32c6/c6_display.inl"),"utf8");
 const family=source.slice(source.indexOf('class c6_display_family_detector_t'));
 const signature=body(family,'bool signature('),confirm=body(family,'bool confirm(');
 await compileRun(common+`
namespace c6_display_detail {constexpr int sda=0,scl=1,pi4io_freq=100000;constexpr std::uint64_t signature_bit=1ull<<18;enum class candidate_t {none,unitc6l,nesson1};}
namespace wiring {namespace unitc6l {constexpr int internal_i2c_sda=0,internal_i2c_scl=1;}}
namespace specs {namespace unitc6l {namespace i2c_ioe {constexpr int i2c_addr=0x43,id_reg=0,i2c_freq=100000;}}
namespace nesson1 {namespace i2c_pi4io1 {constexpr int i2c_addr=0x43,id_reg=0;}namespace i2c_pi4io2 {constexpr int i2c_addr=0x44,id_reg=0;}}}
bool present, responds, high;
bool probe_i2c_bus_present(probe_ctx_t&,int,int) {return present;}
struct pulls_t {std::uint64_t pullup_high;};
pulls_t probe_pin_pulls(probe_ctx_t&,std::uint64_t) {return {high?c6_display_detail::signature_bit:0};}
bool probe_i2c_read(probe_ctx_t&,int,int,int,int,std::uint8_t* p,int,int,int) {*p=0xa0;return responds;}
bool is_pi4io(std::uint8_t p) {return (p&0xe0)==0xa0;}
struct c6_t {mutable c6_display_detail::candidate_t candidate_;
 bool signature(probe_ctx_t& ctx) const ${signature}
 bool confirm(probe_ctx_t& ctx,board_result_t* result) const ${confirm}
};
int main() {
 for(bool bus:{false,true}) for(bool response:{false,true}) for(board_id_t hint:{board_id_t(0),board_id_t(6),board_id_t(42)}) {
 present=bus;responds=response;high=true;c6_t detector;detect_request_t req;req.hint=hint;req.max_attempts=5;
 auto out=run_detection_session(req,[&](const detect_request_t& r,bool last){probe_ctx_t ctx;ctx.hint=r.hint;ctx.final_attempt=last;detect_outcome_t out;
  if(!detector.signature(ctx)) {out.reason=fail_reason_t::no_signature;return out;}
  if(detector.confirm(ctx,&out.result)) {finalize_prepared_result(out,0);out.setup_succeeded=true;}
  else if(ctx.candidate) {out.verdict=verdict_t::candidate;out.candidate_kind=candidate_kind_t::weak;out.result.candidate=ctx.candidate;}
  return out;
 });
 assert(out.verdict==(!bus?verdict_t::unknown:response?verdict_t::confirmed:verdict_t::candidate));
 assert(out.setup_succeeded==(bus&&response));assert(!bus||response||out.result.candidate==&desc_unitc6l.def);
 assert(should_persist_detection(out,0)==(bus&&response));assert(out.attempts==(!bus||response?1:5));
 }
 present=true;responds=false;high=true;c6_t detector;probe_ctx_t ctx;board_result_t result;
 ctx.candidate=&desc_core2.def;
 assert(detector.signature(ctx));assert(!detector.confirm(ctx,&result));
 assert(ctx.candidate==&desc_core2.def);
 ctx.candidate=nullptr;assert(!detector.confirm(ctx,&result));
 assert(ctx.candidate==&desc_unitc6l.def);
}
`,"C6 weak candidate");
});

test("production Atom pull mismatch stays provisional on attempt zero and Stack bypass keeps its retry timing", async () => {
 const atomSource=await fs.readFile(path.join(src,"m5/esp32_pico.inl"),"utf8");
 const stackSource=await fs.readFile(path.join(src,"m5/esp32_d0wdq6.inl"),"utf8");
 const atom=body(atomSource.slice(atomSource.indexOf('class atom_family_detector_t')),'bool probe(');
 const stack=body(stackSource.slice(stackSource.indexOf('class stack_family_detector_t')),'bool signature(');
 const bypass=body(stackSource.slice(stackSource.indexOf('class stack_family_detector_t')),'if (values[2] & 2u)');
 await compileRun(common+`
struct tx_t {void restore_start(std::initializer_list<int>){} void restore_start(int){} } tx;
struct hw_ctx_t:probe_ctx_t {tx_t* transaction=&tx;struct {std::uint64_t values[3];} detector_workspace;};
const board_desc_t desc_atomvoice={{10,"Voice",0}},desc_atommatrix={{11,"Matrix",0}},desc_atomlite={{12,"Lite",0}},desc_atomu={{13,"U",0}};
namespace lgfx {enum class pin_mode_t {input,input_pullup,input_pulldown};pin_mode_t mode;void pinMode(int pin,pin_mode_t m){if(pin==23)mode=m;}bool gpio_in(int pin) {if(pin==2)return true;if(pin==34)return mode==pin_mode_t::input_pullup;return false;}}
void esp_rom_delay_us(int){}
namespace atom_touch {bool measure(std::uint32_t* a,std::uint32_t* b){*a=100;*b=25;return true;}void clear_led(){}}
struct atom_t {mutable bool measured_=false,touch_ok_=false;mutable std::uint32_t t4_=0,t7_=0;
 bool probe(hw_ctx_t& ctx,board_result_t* result) const ${atom}
};
struct stack_desc_t {board_def_t def;struct {int dc,cs;} display;};
const stack_desc_t desc_stack={{14,"Stack",0},{0,1}};
namespace startup_detail {bool description_valid(const stack_desc_t&){return true;}bool gpio_valid(int){return true;}void pin_level(int,bool){}}
namespace detail {bool sd_pull_mask(const stack_desc_t&,std::uint64_t* mask){*mask=3;return true;}}
struct stack_pulls_t {std::uint64_t pulldown_high,pullup_high;};
stack_pulls_t probe_pin_pulls(hw_ctx_t&,std::uint64_t){return {0,3};}
bool stack_signature(hw_ctx_t& ctx) ${stack}
int main() {
 atom_t detector;detect_request_t req;req.max_attempts=5;
 auto out=run_detection_session(req,[&](const detect_request_t& r,bool){hw_ctx_t ctx;ctx.attempt=r.attempt;detect_outcome_t out;
 assert(detector.probe(ctx,&out.result));assert(out.result.def==&desc_atommatrix.def);assert(out.result.provisional);
 finalize_prepared_result(out,0);out.setup_succeeded=true;return out;});
 assert(out.attempts==1&&out.candidate_kind==candidate_kind_t::provisional);assert(!should_persist_detection(out,0));
 for(bool hinted:{false,true}) {
 req.hint=hinted?14:0;
 out=run_detection_session(req,[&](const detect_request_t& r,bool last){hw_ctx_t ctx;ctx.hint=r.hint;ctx.final_attempt=last;detect_outcome_t out;
 if(!stack_signature(ctx)){return out;}
 const auto& values=ctx.detector_workspace.values;auto* result=&out.result;result->def=&desc_stack.def;
 if(values[2]&2u) ${bypass}
 finalize_prepared_result(out,0);out.setup_succeeded=true;return out;});
 assert(out.attempts==(hinted?1:5));assert(out.result.provisional);assert(!should_persist_detection(out,0));
 req.preferred=desc_core2.def.id;
 out=run_detection_session(req,[&](const detect_request_t& r,bool last){hw_ctx_t ctx;ctx.hint=r.hint;ctx.final_attempt=last;detect_outcome_t out;
 if(!stack_signature(ctx)){return out;}
 const auto& values=ctx.detector_workspace.values;auto* result=&out.result;result->def=&desc_stack.def;
 if(values[2]&2u) ${bypass}
 if(finalize_prepared_result(out,r.preferred)) {out.setup_succeeded=true;}return out;});
 assert(out.attempts==5&&!out.setup_succeeded&&out.result.candidate==&desc_stack.def);
 req.preferred=0;
 }
}
`,"Atom and Stack timing");
});

test("production PM1 unresolved tail keeps PaperMono without running hinted StopWatch power", async () => {
 const source=await fs.readFile(path.join(src,"m5/esp32s3/families.inl"),"utf8");
 const begin=source.indexOf('      if (!ctx.final_attempt)');
 const end=source.indexOf('\n    }',begin);
 const tail=source.slice(begin,end);
 await compileRun(common+`
const board_desc_t desc_stopwatch={{20,"StopWatch",0}},desc_papermono={{21,"PaperMono",0}};
int power_calls=0;
namespace startup_detail {bool prepare_power(const board_desc_t&,board_result_t&,int,bool) {++power_calls;return true;}}
bool refine_tail(board_result_t& result,const prepare_ctx_t& ctx) {
 bool stopwatch_touch=false;struct {bool opened;int port;} i2c={true,0};
 ${tail}
}
int main(){for(board_id_t hint:{board_id_t(0),board_id_t(20),board_id_t(21)}) for(board_id_t preferred:{board_id_t(0),board_id_t(20),board_id_t(21)}) {
 prepare_ctx_t ctx;ctx.hint=hint;ctx.preferred=preferred;board_result_t r;r.assign(&desc_papermono);
 assert(!refine_tail(r,ctx));ctx.final_attempt=true;assert(refine_tail(r,ctx));
 assert(r.def==&desc_papermono.def&&r.provisional);assert(power_calls==0);
 detect_outcome_t out;out.result=r;assert(finalize_prepared_result(out,preferred)==(preferred!=20));
}}
`,"PM1 fixed representative");
});

test("production PM1 pre-power table gates writes and fixes observed members", async () => {
 const source=await fs.readFile(path.join(src,"m5/esp32s3/families.inl"),"utf8");
 const detector=await fs.readFile(path.join(src,"board_detect.inl"),"utf8");
 const confirm=body(source.slice(source.indexOf('class pm1_family_detector_t')),'bool confirm(');
 const helper=body(detector,'static bool select_provisional_member');
 await compileRun(common+`
const board_desc_t desc_stopwatch={{20,"StopWatch",0}},desc_papermono={{21,"PaperMono",0}};
namespace wiring {namespace stopwatch {constexpr int internal_i2c_sda=1,internal_i2c_scl=2;}
namespace papermono {constexpr int internal_i2c_sda=1,internal_i2c_scl=2;}}
namespace detail {constexpr int stopwatch_probe_addr=0x15,papermono_probe_addr=0x38;
bool refine_papermono_touch(board_result_t&,const prepare_ctx_t&){return true;}}
struct pulls_t {std::uint64_t pulldown_high,pullup_high;};
int sw,pm,g12,g13,reads;
pulls_t probe_pin_pulls(probe_ctx_t&,std::uint64_t){return {std::uint64_t(g12&1)<<12|std::uint64_t(g13&1)<<13,std::uint64_t((g12>>1)&1)<<12|std::uint64_t((g13>>1)&1)<<13};}
bool probe_i2c_ack(probe_ctx_t&,int,int,int addr){++reads;assert(addr==0x15||addr==0x38);return addr==0x15?sw:pm;}
bool probe_i2c_read(probe_ctx_t&,int,int,int addr,int,std::uint8_t* data,int,int,int){data[0]=0x50;data[1]=0x20;assert(addr==0x6e);return true;}
bool select_provisional_member(const prepare_ctx_t& ctx,board_result_t* result,const board_desc_t* preferred_if_possible,const board_desc_t* hinted_if_possible,const board_desc_t* family_default,const char* why) ${helper}
bool confirm_member(probe_ctx_t& ctx,board_result_t* result) ${confirm}
int main(){
 for(sw=0;sw<=1;++sw)for(pm=0;pm<=1;++pm)for(g12=0;g12<4;++g12)for(g13=0;g13<4;++g13)
 for(bool final:{false,true})for(board_id_t preferred:{board_id_t(0),board_id_t(20),board_id_t(21),board_id_t(42)})
 for(board_id_t hint:{board_id_t(0),board_id_t(20),board_id_t(21)}){
 probe_ctx_t ctx;ctx.final_attempt=final;ctx.attempt=final?4:0;ctx.preferred=preferred;ctx.hint=hint;
 board_result_t r;reads=0;bool ok=confirm_member(ctx,&r);assert(reads==2);
 int expected=0;bool provisional=false,refine=false;
 if(sw!=pm)expected=sw?20:21;
 else if(!sw&&((g12==0&&g13==0)||(g12==3&&g13==3))){expected=21;refine=true;}
 else if(final){provisional=true;
 if(!sw&&(g12==2||(g12==0&&g13==3)))expected=20;
 else expected=(preferred==20||preferred==21)?preferred:(hint==20||hint==21)?hint:0;}
 assert(ok==(expected!=0));int stopwatch_power=0,papermono_power=0;
 if(ok){assert(r.def->id==expected&&r.provisional==provisional);
 assert(bool(r.refine)==refine);if(expected==20)++stopwatch_power;else ++papermono_power;
 detect_outcome_t out;out.result=r;if(provisional)assert(!should_persist_detection(out,0));}
 assert(stopwatch_power==(ok&&expected==20));assert(papermono_power==(ok&&expected==21));
 }
}
`,"PM1 pre-power table");
});

test("production PM1 D/D refinement polls only PaperMono and never switches power", async () => {
 const source=await fs.readFile(path.join(src,"m5/esp32s3/families.inl"),"utf8");
 const refine=body(source,'bool refine_papermono_touch');
 await compileRun(common.replace('bool final_attempt=false;','bool final_attempt=false; int* transaction=nullptr; int i2c_port_probe=0;')
 .replace('#define ESP_LOGW(...) ((void)0)', '#include <cstring>\nconst char* last_reason;\n#define ESP_LOGW(tag, format, reason, ...) (last_reason=reason)')+`
const board_desc_t desc_papermono={{21,"PaperMono",0}};
namespace specs {namespace papermono {namespace touch {constexpr int i2c_freq=100000;}}}
constexpr int papermono_probe_addr=0x38;
bool ack,opened;unsigned clock_ms,probes;
namespace lgfx {unsigned millis(){return clock_ms;}void delay(unsigned ms){clock_ms+=ms;}
namespace i2c {struct response {bool ok;bool has_value(){return ok;}};
response beginTransaction(int,int addr,int,bool){assert(addr==0x38);++probes;return {ack};}
response endTransaction(int){return {true};}}}
namespace startup_detail {struct i2c_scope_t {bool opened;int port=0;
i2c_scope_t(int&,int,const board_desc_t&):opened(::opened){}};}
bool refine_member(board_result_t& result,const prepare_ctx_t& ctx) ${refine.replace('desc_papermono.internal_i2c','desc_papermono')}
int main(){int tx=0;for(bool final:{false,true})for(bool response:{false,true})for(bool bus:{false,true}){
 ack=response;opened=bus;clock_ms=probes=0;prepare_ctx_t ctx;ctx.final_attempt=final;ctx.transaction=&tx;
 board_result_t r;r.assign(&desc_papermono);int papermono_power=1,stopwatch_power=0;
 bool ok=refine_member(r,ctx);assert(ok==((bus&&response)||final));
 assert(r.def==&desc_papermono.def&&r.provisional==(final&&!(bus&&response)));
 assert(probes==(bus?(response?1:200):0));assert(papermono_power==1&&stopwatch_power==0);
 if(final&&!(bus&&response))assert(std::strcmp(last_reason,bus?"PaperMono touch unanswered":"PaperMono internal I2C unavailable")==0);
}}
`,"PM1 post-power refine");
});

test("production prepare retries failed power before pending member refinement", async () => {
 const source=await fs.readFile(path.join(src,"board_detect.inl"),"utf8");
 const prepare=body(source,'bool prepare(const board_desc_t& desc,');
 const extended=common.replace('#define ESP_LOGW', '#define ESP_LOGE(...) ((void)0)\n#define ESP_LOGW')
 .replace('struct board_desc_t {board_def_t def;};',`enum class reset_kind_t {none,i2c_regs};
 struct board_desc_t {board_def_t def;struct {const int* variants;} power;
 struct {int hw_port,sda,scl;} internal_i2c;struct {reset_kind_t kind;} reset;};`)
 .replace('bool final_attempt=false;','bool final_attempt=false; int* transaction=nullptr; int i2c_port_probe=0;');
 await compileRun(extended+`
int power_calls,refine_calls,sd_calls,construct_calls;bool fail_power;
namespace lgfx {namespace i2c {struct value_t {bool has_value() const{return true;}int value() const{return 0;}};
 bool isInitialized(int){return false;}value_t getPinSDA(int){return {};}
 value_t getPinSCL(int){return {};}value_t init(int,int,int){return {};}}}
namespace startup_detail {
 bool description_valid(const board_desc_t&){return true;}
 struct i2c_scope_t {bool opened=true;int port=0;i2c_scope_t(int&,int,decltype(board_desc_t::internal_i2c)) {}};
 bool prepare_power(const board_desc_t&,board_result_t& r,int,bool){++power_calls;r.prepared|=prepared_power;
 if(fail_power)r.prepared|=prepared_power_failed;return true;}
 bool prepare_sd_spi(const board_desc_t&,board_result_t&,const prepare_ctx_t&){++sd_calls;return true;}
 void hold_chip_selects(const board_desc_t&){}
}
bool prepare_reset(const board_desc_t&,board_result_t&,const prepare_ctx_t&,int,void*,bool){return true;}
bool refine_member(board_result_t&,const prepare_ctx_t&){++refine_calls;return true;}
bool prepare_member(const board_desc_t& desc,board_result_t& result,const prepare_ctx_t& ctx) ${prepare}
int main(){int tx=0,variant=0;
 // Pending refine represents both D/D and U/U; no refine represents touch-confirmed members.
 for(int row:{0,1,2})for(bool final:{false,true})for(bool failed:{false,true})for(bool scoped:{false,true}){
 board_desc_t desc={{21,"PaperMono",0},{scoped?&variant:nullptr},{-1,47,48},{reset_kind_t::none}};
 board_result_t r;r.assign(&desc);r.refine=row<2?refine_member:nullptr;
 prepare_ctx_t ctx;ctx.transaction=&tx;ctx.final_attempt=final;
 power_calls=refine_calls=sd_calls=construct_calls=0;fail_power=failed;
 const bool ok=prepare_member(desc,r,ctx);if(ok)++construct_calls;
 const bool retry=failed&&row<2&&!final;
 assert(ok==!retry&&construct_calls==int(!retry));assert(power_calls==1);
 assert(refine_calls==int(row<2&&!failed));assert(sd_calls==int(!retry));
 assert(r.provisional==(failed&&row<2&&final));
 assert(bool(r.prepared&prepared_refine)==(row<2&&!retry));
 if(failed)assert(r.prepared&prepared_power_failed);
 }
 // Completed refinement does not turn a confirmed member's power failure into a retry.
 board_desc_t desc={{21,"PaperMono",0},{nullptr},{-1,47,48},{reset_kind_t::none}};
 board_result_t r;r.assign(&desc);r.refine=refine_member;r.prepared=prepared_power|prepared_power_failed|prepared_refine;
 prepare_ctx_t ctx;ctx.transaction=&tx;assert(prepare_member(desc,r,ctx));assert(!r.provisional);
}
`,"pending-refinement power failure");
});
