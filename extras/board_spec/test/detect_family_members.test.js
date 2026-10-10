import { promises as fs } from "node:fs";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { body, compileRun } from "./detect_host_helpers.js";
const src=path.resolve(path.dirname(fileURLToPath(import.meta.url)),"../../../src/board_detect");
const common=`
#include "board_detect/detect_session.hpp"
#include "board_detect/detect_class.hpp"
#include "board_detect/dedicated_release_probe.hpp"
#include <cassert>
#include <initializer_list>
#define ESP_LOGD(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
using namespace m5gfx::board_detect;
namespace m5gfx {namespace board_detect {
struct board_desc_t {board_def_t def;struct {int sda,scl;} internal_i2c;};
void board_result_t::assign(const board_desc_t* d){desc=d;def=&d->def;}
struct prepare_ctx_t {board_id_t preferred=0,hint=0;bool final_attempt=false;int* transaction=nullptr;int i2c_port_probe=0;};
struct probe_ctx_t:prepare_ctx_t {bool conditional_pins_unavailable=false;bool cached=false,ack=false;};
}}
`;

test("production CoreS3 refinement covers camera, release band and gated IOE outcomes", async()=>{
 const source=await fs.readFile(path.join(src,"m5/esp32s3/cores3.inl"),"utf8");
 const detector=await fs.readFile(path.join(src,"board_detect.inl"),"utf8");
 const helper=body(detector,'static bool select_provisional_member');
 const refine=body(source,'bool refine(board_result_t& result, const prepare_ctx_t& ctx)\n    {');
 const gate=body(source,'bool stackchan_base_gate(');
 const ack=body(source,'bool stackchan_ack(');
 await compileRun(common.replace('#define ESP_LOGW(...) ((void)0)', '#include <cstring>\nint gate_warnings;\n#define ESP_LOGW(tag, message, ...) (gate_warnings += std::strstr(message, "base gate without IOE ACK") != nullptr)')+`
const board_desc_t desc_cores3={{1,"CoreS3",0}},desc_cores3se={{2,"SE",0}},desc_stackchan={{3,"StackChan",0}};
namespace specs {namespace stackchan {namespace i2c_stackchan_ioe {constexpr int i2c_addr=0x6f,firmware_reg=2,i2c_freq=100000,firmware_min=4;}}}
constexpr unsigned release_unavailable=3,internal_camera_confirmed=1,vbus_5v=2;
constexpr int sda=12,scl=11;
unsigned clock_ms;int ack_at,panel_calls,camera_calls,gate_calls,ack_calls,firmware_calls,init_calls,release_calls,ack_end_calls;
bool post_camera,base_gate,firmware_ok;unsigned fw;
namespace lgfx {unsigned millis(){return clock_ms;}void delay(unsigned ms){clock_ms+=ms;}
 namespace i2c {struct response_t {bool ok;bool has_value(){return ok;}};
 response_t beginTransaction(int,int addr,int freq,bool){assert(addr==0x6f&&freq==100000);++ack_calls;return {ack_at>=0&&clock_ms>=unsigned(ack_at)};}
 response_t endTransaction(int){++ack_end_calls;return {true};}}}
namespace startup_detail {struct i2c_scope_t {bool opened=true;int port;
 i2c_scope_t(int&,int p,const decltype(board_desc_t::internal_i2c)&):port(p){++init_calls;}
 ~i2c_scope_t(){++release_calls;}};}
void enable_bus_out(const prepare_ctx_t&){}
bool camera_id(probe_ctx_t&){++camera_calls;return post_camera;}
bool refine_panel(board_result_t&,const prepare_ctx_t&){++panel_calls;return true;}
pin_pull_result_t probe_pin_pulls(probe_ctx_t&,std::uint64_t mask){++gate_calls;assert(mask==((1ull<<5)|(1ull<<6)|(1ull<<7)));
 pin_pull_result_t r;r.pulldown_high=base_gate?(1ull<<6):0;r.pullup_high=base_gate?(1ull<<6):mask;return r;}
bool probe_i2c_ack(probe_ctx_t& ctx,int,int,int addr){assert(addr==0x6f);++ack_calls;
 if(!ctx.cached){ctx.cached=true;ctx.ack=ack_at>=0&&clock_ms>=unsigned(ack_at);}return ctx.ack;}
bool read(const prepare_ctx_t&,int addr,int reg,std::uint8_t* value,int freq){++firmware_calls;assert(addr==0x6f&&reg==2&&freq==100000);*value=fw;return firmware_ok;}
bool select_provisional_member(const prepare_ctx_t& ctx,board_result_t* result,const board_desc_t* preferred_if_possible,const board_desc_t* hinted_if_possible,const board_desc_t* family_default,const char* why) ${helper}
bool stackchan_base_gate(probe_ctx_t& probe) ${gate}
bool stackchan_ack(const prepare_ctx_t& ctx,bool gate) ${ack}
bool refine_member(board_result_t& result,const prepare_ctx_t& ctx) ${refine}
int main(){int tx=0;
 for(bool before:{false,true})for(bool after:{false,true})for(unsigned band=0;band<4;++band)
 for(bool gate:{false,true})for(int at:{-1,0,10})for(bool read_ok:{false,true})for(unsigned firmware:{3u,4u})
 for(bool final:{false,true})for(board_id_t preference:{board_id_t(0),board_id_t(1),board_id_t(2),board_id_t(3),board_id_t(42)})
 for(board_id_t hint:{board_id_t(0),board_id_t(1),board_id_t(2),board_id_t(3),board_id_t(42)}){
 board_result_t r;r.assign(&desc_cores3);r.refine_state=band;r.option=before?1:0;
 prepare_ctx_t ctx;ctx.transaction=&tx;ctx.final_attempt=final;ctx.preferred=preference;ctx.hint=hint;
 clock_ms=panel_calls=camera_calls=gate_calls=ack_calls=firmware_calls=init_calls=release_calls=ack_end_calls=gate_warnings=0;post_camera=after;base_gate=gate;ack_at=at;firmware_ok=read_ok;fw=firmware;
 const bool camera=before||after;bool provisional=false;int expected=0;
 const bool acknowledged=at==0||(gate&&at>=0&&at<=50);
 if(!camera&&(band==0||band==3)){expected=2;}
 else if(!camera&&band==1){
 if(final){provisional=true;auto possible=[&](board_id_t id){return id==1||id==2||(id==3&&acknowledged);};expected=possible(preference)?preference:possible(hint)?hint:1;}}
 else{
 if(acknowledged&&!read_ok){if(final){provisional=true;auto possible=[](board_id_t id){return id==1||id==3;};expected=possible(preference)?preference:possible(hint)?hint:3;}}
 else expected=acknowledged&&read_ok&&firmware>=4?3:1;
 }
 const bool ok=refine_member(r,ctx);assert(ok==(expected!=0));assert(r.refine_state==0&&r.option<4);
 assert(gate_warnings==int(gate&&!acknowledged&&(camera||band==2)));
 assert(camera_calls==int(!before)&&panel_calls==int(ok)&&gate_calls==1);
 if(ok){assert(r.def->id==expected&&r.provisional==provisional);detect_outcome_t out;out.result=r;if(provisional)assert(!should_persist_detection(out,0));}
 assert(init_calls==1&&release_calls==1&&ack_end_calls==ack_calls);
 assert(firmware_calls==int(acknowledged));
 if(!gate)assert(ack_calls==1&&clock_ms==0);
 else assert(clock_ms==(at<0?50u:unsigned(at)));
 }
}
`,"CoreS3/SE/StackChan outcome table");
});

test("production Cardputer subdivision reads independent ICs and requires four released floating pins",async()=>{
 const source=await fs.readFile(path.join(src,"m5/esp32s3/families.inl"),"utf8");
 const confirm=body(source.slice(source.indexOf('class cardputer_family_detector_t')),'bool confirm(');
 await compileRun(common+`
const board_desc_t desc_cardputer={{1,"Cardputer",0}},desc_cardputer_adv={{2,"ADV",0}},desc_vameter={{3,"VAMeter",0}};
namespace wiring {namespace cardputer {namespace cardputer_subdivision {constexpr int sense_pins[]={0,1,2,3},vameter_i2c_sda=0,vameter_i2c_scl=1,vameter_i2c_addrs[]={0x40,0x41};}}
namespace cardputer_adv {constexpr int internal_i2c_sda=2,internal_i2c_scl=3;}}
namespace specs {namespace cardputer {constexpr bool bus_three_wire=true;}}
const int cardputer_probes[]={1};constexpr unsigned pull_release_us=128;
int samples[4],recovery_calls,probe_calls,vameter_reads,adv_reads,companion_reads;bool vameter_ack,adv_ack,spi_ok=true;
bool probe_spi_id(probe_ctx_t&,const board_desc_t& d,const int*,int,board_result_t* r,bool){if(spi_ok)r->assign(&d);return spi_ok;}
pin_pull_result_t probe_pin_pulls(probe_ctx_t&,std::uint64_t mask,unsigned release=0){++probe_calls;assert(mask==15);
 if(release)assert(release==128&&recovery_calls==2);pin_pull_result_t r;r.release_sampled=release!=0;
 for(unsigned pin=0;pin<4;++pin){const unsigned sample=samples[pin];const auto bit=1ull<<pin;
 if(sample&1)r.pulldown_high|=bit;if(sample&2)r.pullup_high|=bit;
 if(sample&4)r.pulldown_release_high|=bit;if(sample&8)r.pullup_release_high|=bit;}return r;}
pin_pull_result_t recover_held_sda_and_resample(probe_ctx_t&,pin_pull_result_t r,std::uint64_t,int,int){++recovery_calls;r.release_sampled=false;return r;}
bool probe_i2c_read(probe_ctx_t&,int,int,int addr,int,std::uint8_t* data,int,int,int,bool){++vameter_reads;assert(addr==0x40);data[0]=0x54;data[1]=0x49;return vameter_ack;}
bool probe_i2c_ack(probe_ctx_t&,int,int,int addr){if(addr==0x34){++adv_reads;return adv_ack;}++companion_reads;assert(addr==0x41);return vameter_ack;}
bool confirm_member(probe_ctx_t& ctx,board_result_t* result) ${confirm}
int main(){
 for(int a:{0,15,10,14,2,1})for(int b:{0,15,10,14,2,1})for(int c:{0,15,10,14,2,1})for(int d:{0,15,10,14,2,1})
 for(bool vam:{false,true})for(bool adv:{false,true})for(bool final:{false,true})
 for(board_id_t preference:{board_id_t(0),board_id_t(1),board_id_t(2),board_id_t(3),board_id_t(42)})
 for(board_id_t hint:{board_id_t(0),board_id_t(1),board_id_t(2),board_id_t(3),board_id_t(42)}){
 samples[0]=a;samples[1]=b;samples[2]=c;samples[3]=d;vameter_ack=vam;adv_ack=adv;
 recovery_calls=probe_calls=vameter_reads=adv_reads=companion_reads=0;
 probe_ctx_t ctx;ctx.final_attempt=final;ctx.preferred=preference;ctx.hint=hint;board_result_t r;
 const bool vu=a==15&&b==15,au=c==15&&d==15,floating=a==10&&b==10&&c==10&&d==10;
 const bool v=vu&&vam,w=au&&adv;bool provisional=false;int expected=0;
 if(v&&w)expected=0;else if(v)expected=3;else if(w)expected=2;else if(floating)expected=1;
 else if(final){provisional=true;const auto possible=[&](board_id_t id){return id==1||(id==3&&(a&1)&&(b&1))||(id==2&&(c&1)&&(d&1));};expected=possible(preference)?preference:possible(hint)?hint:1;}
 const bool ok=confirm_member(ctx,&r);assert(ok==(expected!=0));assert(probe_calls==2&&recovery_calls==2);
 assert(vameter_reads==int(vu)&&adv_reads==int(au)&&companion_reads==int(vu&&vam));
 if(ok){assert(r.def->id==expected&&r.provisional==provisional);detect_outcome_t out;out.result=r;if(provisional)assert(!should_persist_detection(out,0));}
 }
 spi_ok=false;probe_calls=recovery_calls=0;probe_ctx_t ctx;board_result_t r;assert(!confirm_member(ctx,&r)&&probe_calls==0&&recovery_calls==0);
}
`,"Cardputer subdivision table");
});

test("production CoreS3 confirm transports measured release band to refinement",async()=>{
 const source=await fs.readFile(path.join(src,"m5/esp32s3/cores3.inl"),"utf8");
 const confirm=body(source.slice(source.indexOf('class cores3_family_detector_t')),'bool confirm(');
 await compileRun(common+`
const board_desc_t desc_cores3={{1,"CoreS3",0}},desc_cores3se={{2,"SE",0}};
namespace specs {namespace cores3 {
 namespace pmic {constexpr int id_reg=3,id_value=0x4a;}
 namespace i2c_io_expander {constexpr int id_reg=0x10,id_value=0x23,i2c_freq=400000;}
 namespace release_probe {constexpr std::int8_t pins[]={38,39,40,41,42,45,47,48};constexpr unsigned reads=64,samples=8,settle_us=1,short_max_ns=260,long_min_ns=330;}}}
int band;bool available,before;int release_calls;
struct release_t {bool available;};struct summary_t {pin_release_band_t band;};
release_t probe_dedicated_pin_release(probe_ctx_t&,const std::int8_t*,int,int,int,int){++release_calls;return {available};}
summary_t summarize_dedicated_release(release_t,unsigned,unsigned){return {available?static_cast<pin_release_band_t>(band):pin_release_band_t::ambiguous};}
bool probe_i2c_read(probe_ctx_t&,int,int,int addr,int,std::uint8_t* value,int,int,int){*value=addr==0x34?0x4a:0x23;return true;}
namespace cores3_detail {
 constexpr int sda=12,scl=11,axp_addr=0x34,aw_addr=0x58,i2c_freq=400000;
 constexpr unsigned release_unavailable=3,internal_camera_confirmed=1;
 bool camera_id(probe_ctx_t&){return before;}
 unsigned observe_vbus(probe_ctx_t&){return 2;}
 bool refine(board_result_t&,const prepare_ctx_t&){return true;}}
bool confirm_member(probe_ctx_t& ctx,board_result_t* result) ${confirm}
int main(){for(band=0;band<3;++band)for(bool avail:{false,true})for(bool camera:{false,true}){
 available=avail;before=camera;release_calls=0;probe_ctx_t ctx;board_result_t r;
 assert(confirm_member(ctx,&r));const unsigned transported=camera?2:avail?band:3;
 assert(r.refine_state==transported&&release_calls==int(!camera));assert(r.option&2);assert(r.option<4);
 assert(bool(r.option&1)==camera&&r.refine==cores3_detail::refine);
 assert(r.def==(transported==0?&desc_cores3se.def:&desc_cores3.def));
}}
`,"CoreS3 release-band transport");
});
