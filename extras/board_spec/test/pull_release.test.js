import { promises as fs } from "node:fs";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { body, compileRun } from "./detect_host_helpers.js";
const here = path.dirname(fileURLToPath(import.meta.url));
const src = path.resolve(here, "../../../src/board_detect");

test("release pull classifier covers every four-sample combination", async () => {
 await compileRun(`
#include "board_detect/detect_class.hpp"
#include <cassert>
#include <initializer_list>
using namespace m5gfx::board_detect;
int main(){
 // Index bits are PD, PU, released PD, released PU. Values follow measured U/D/u/d/F/X semantics.
 const pull_class_t expected[]={
 pull_class_t::down,pull_class_t::conflict,pull_class_t::weak_down,pull_class_t::up,
 pull_class_t::down,pull_class_t::conflict,pull_class_t::conflict,pull_class_t::up,
 pull_class_t::down,pull_class_t::conflict,pull_class_t::floating,pull_class_t::up,
 pull_class_t::down,pull_class_t::conflict,pull_class_t::weak_up,pull_class_t::up};
 for(int pin:{0,17,63})for(unsigned sample=0;sample<16;++sample)for(bool released:{false,true}){
 const auto bit=std::uint64_t(1)<<pin;pin_pull_result_t r;r.release_sampled=released;
 // Other bits must not affect classification of this pin.
 r.pulldown_high=(sample&1)?~std::uint64_t(0):~bit;
 r.pullup_high=(sample&2)?~std::uint64_t(0):~bit;
 r.pulldown_release_high=(sample&4)?~std::uint64_t(0):~bit;
 r.pullup_release_high=(sample&8)?~std::uint64_t(0):~bit;
 const auto value=classify_pin_pull(r,pin);
 assert(value==(!released&&(sample&3)==2?pull_class_t::pull_dependent:expected[sample]));
 if(!released)assert(value!=pull_class_t::weak_up&&value!=pull_class_t::weak_down&&value!=pull_class_t::floating);
 }
}
`,"release classifications");
});

test("production pull probe preserves zero-release sequence and samples release per pin", async () => {
 const source=await fs.readFile(path.join(src,"board_detect.inl"),"utf8");
 const probe=body(source,'pin_pull_result_t probe_pin_pulls(');
 const header=await fs.readFile(path.join(src,"board_detect.hpp"),"utf8");
 const declaration=header.match(/pin_pull_result_t probe_pin_pulls\([^;]+;/)[0];
 await compileRun(`
#include "board_detect/detect_class.hpp"
#include <cassert>
#include <initializer_list>
#include <vector>
using namespace m5gfx::board_detect;
struct event_t {int pin,kind;unsigned value;bool operator==(const event_t& b)const{return pin==b.pin&&kind==b.kind&&value==b.value;}};
std::vector<event_t> events;std::vector<bool> samples;unsigned cursor;int current_pin;
struct tx_t {void restore_start(std::int8_t pin){events.push_back({pin,3,0});}};
struct probe_ctx_t {tx_t* transaction;};
${declaration}
namespace lgfx {
enum class pin_mode_t {input_pulldown,input_pullup,input};
void pinMode(int pin,pin_mode_t mode){current_pin=pin;events.push_back({pin,0,unsigned(mode)});}
void delayMicroseconds(unsigned us){events.push_back({current_pin,1,us});}
bool gpio_in(int pin){const bool value=samples.at(cursor++);events.push_back({pin,2,unsigned(value)});return value;}}
pin_pull_result_t probe_pin_pulls(probe_ctx_t& ctx,std::uint64_t pin_mask,std::uint32_t release_us) ${probe}
int main(){tx_t tx;probe_ctx_t ctx={&tx};const std::uint64_t mask=(1ull<<3)|(1ull<<9);
 samples={false,true,true,false};cursor=0;
 const auto legacy=probe_pin_pulls(ctx,mask);
 const std::vector<event_t> expected_legacy={
 {3,0,0},{3,1,10},{3,2,0},{3,0,1},{3,1,10},{3,2,1},{3,3,0},
 {9,0,0},{9,1,10},{9,2,1},{9,0,1},{9,1,10},{9,2,0},{9,3,0}};
 assert(events==expected_legacy&&cursor==4);
 assert(legacy.pulldown_high==(1ull<<9)&&legacy.pullup_high==(1ull<<3));
 assert(legacy.pulldown_release_high==0&&legacy.pullup_release_high==0&&!legacy.release_sampled);
 events.clear();cursor=0;probe_pin_pulls(ctx,mask,0);assert(events==expected_legacy&&cursor==4);
 events.clear();samples={false,false,true,true,true,false,false,true};cursor=0;
 const auto released=probe_pin_pulls(ctx,mask,128);
 const std::vector<event_t> expected_release={
 {3,0,0},{3,1,10},{3,2,0},{3,0,2},{3,1,128},{3,2,0},{3,0,1},{3,1,10},{3,2,1},{3,0,2},{3,1,128},{3,2,1},{3,3,0},
 {9,0,0},{9,1,10},{9,2,1},{9,0,2},{9,1,128},{9,2,0},{9,0,1},{9,1,10},{9,2,0},{9,0,2},{9,1,128},{9,2,1},{9,3,0}};
 assert(events==expected_release&&cursor==8);
 assert(released.pulldown_high==(1ull<<9)&&released.pullup_high==(1ull<<3));
 assert(released.pulldown_release_high==0&&released.pullup_release_high==mask&&released.release_sampled);
 assert(classify_pin_pull(released,3)==pull_class_t::floating);
 assert(classify_pin_pull(released,9)==pull_class_t::conflict);
 events.clear();cursor=0;probe_pin_pulls(ctx,0,128);assert(events.empty()&&cursor==0);
}
`,"per-pin release probe sequence");
});

test("production DualKey rejects weak pulls with the shared release delay and preserves OPI exclusion", async () => {
 const source=await fs.readFile(path.join(src,"m5/esp32s3/families.inl"),"utf8");
 const header=await fs.readFile(path.join(src,"board_detect.hpp"),"utf8");
 const constant=header.match(/static constexpr std::uint32_t pull_release_us = \d+;/)[0];
 const signature=body(source.slice(source.indexOf('class dualkey_detector_t')),'bool signature(');
 const fixture=`
#include "board_detect/detect_class.hpp"
#include <cassert>
#include <initializer_list>
using namespace m5gfx::board_detect;
${constant}
struct {int def=1;} desc_dualkey;
struct probe_ctx_t {const int* candidate=nullptr;};
pin_pull_result_t socket;int probes;bool pressed;
pin_pull_result_t probe_pin_pulls(probe_ctx_t&,std::uint64_t mask,std::uint32_t release=0){++probes;
 if(mask==((1ull<<38)|(1ull<<39))){assert(release==128);return socket;}
 assert(release==0&&(mask==(1ull<<21)||mask==(1ull<<17)));
 pin_pull_result_t r;r.pulldown_high=(mask==(1ull<<17)&&pressed)?0:mask;return r;}
bool signature_member(probe_ctx_t& ctx) ${signature}
int main(){
#ifdef CONFIG_SPIRAM_MODE_OCT
 probe_ctx_t ctx;assert(!signature_member(ctx)&&probes==0&&ctx.candidate==nullptr);
#else
 for(unsigned a=0;a<16;++a)for(unsigned b=0;b<16;++b)for(bool key:{false,true}){
 socket={};socket.release_sampled=true;for(int pin:{38,39}){const unsigned sample=pin==38?a:b;const auto bit=1ull<<pin;
 if(sample&1)socket.pulldown_high|=bit;if(sample&2)socket.pullup_high|=bit;
 if(sample&4)socket.pulldown_release_high|=bit;if(sample&8)socket.pullup_release_high|=bit;}
 probes=0;pressed=key;probe_ctx_t ctx;const bool floating=a==10&&b==10;
 assert(signature_member(ctx)==(floating&&!key));assert(probes==(floating?3:2));
 assert((ctx.candidate!=nullptr)==(floating&&key));
 }
#endif
}
`;
 await compileRun(fixture,"DualKey release classification");
 await compileRun("#define CONFIG_SPIRAM_MODE_OCT 1\n"+fixture,"DualKey OPI exclusion");
});
