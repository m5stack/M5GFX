import assert from "node:assert/strict";
import { promises as fs } from "node:fs";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { body, compileRun } from "./detect_host_helpers.js";
const src = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../../../src");
const readPanel = () => fs.readFile(path.join(src, "lgfx/v1/panel/Panel_M5HDMI.cpp"), "utf8");

test("write-only init propagates transfer failures from reset and init without truncating writes", async () => {
  const source = await readPanel();
  const method = (name) => body(source, `Panel_M5HDMI::HDMI_Trans::${name}(`);
  await compileRun(`
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>
#define ESP_LOGD(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
std::vector<std::pair<int,int>> writes;
int failed_at=-1,reads=0,wait_ms=0;
void delay(int ms) {wait_ms+=ms;}
namespace lgfx {
struct Bus_I2C {struct config_t {int i2c_port=1,i2c_addr=0x39,pin_sda=11,pin_scl=9;uint32_t freq_write=100000,freq_read=100000;};};
namespace i2c {
struct result_t {bool failed;bool has_error()const{return failed;}};
result_t transactionWrite(int,int,const uint8_t* b,uint8_t,uint32_t) {writes.emplace_back(b[0],b[1]);return {int(writes.size())==failed_at};}
result_t transactionWriteIgnoreAck(int a,int b,const uint8_t* c,uint8_t d,uint32_t e) {return transactionWrite(a,b,c,d,e);}
result_t transactionWriteRead(int,int,const uint8_t* reg,uint8_t,uint8_t* out,size_t len,uint32_t) {
 ++reads;for(size_t i=0;i<len;++i)out[i]=*reg==0?0x17:*reg==1?0x02:*reg==2?0xE2:(*reg==0x15||*reg==0xeb)?0x80:0;return {false};
}
struct i2c_temporary_switcher_t {i2c_temporary_switcher_t(int,int,int){}void restore(){}};
}
}
struct Panel_M5HDMI {struct HDMI_Trans {
 lgfx::Bus_I2C::config_t HDMI_Trans_config;bool _write_only=false,_write_failed=false;
 struct ChipID {uint8_t id[3];};
 HDMI_Trans(const lgfx::Bus_I2C::config_t&,bool=false);
 uint8_t readRegister(uint8_t);bool writeRegister(uint8_t,uint8_t);bool writeRegisterSet(const uint8_t*,size_t);
 ChipID readChipID();void reset();bool init();
};};
Panel_M5HDMI::HDMI_Trans::HDMI_Trans(const lgfx::Bus_I2C::config_t& i2c_config,bool write_only) ${method("HDMI_Trans")}
uint8_t Panel_M5HDMI::HDMI_Trans::readRegister(uint8_t register_address) ${method("readRegister")}
bool Panel_M5HDMI::HDMI_Trans::writeRegister(uint8_t register_address,uint8_t value) ${method("writeRegister")}
bool Panel_M5HDMI::HDMI_Trans::writeRegisterSet(const uint8_t* reg_data_pair,size_t len) ${method("writeRegisterSet")}
Panel_M5HDMI::HDMI_Trans::ChipID Panel_M5HDMI::HDMI_Trans::readChipID() ${method("readChipID")}
void Panel_M5HDMI::HDMI_Trans::reset() ${method("reset")}
bool Panel_M5HDMI::HDMI_Trans::init() ${method("init")}
int main() {
 lgfx::Bus_I2C::config_t cfg;
 for(bool write_only:{false,true}) {
  writes.clear();reads=wait_ms=0;failed_at=-1;
  Panel_M5HDMI::HDMI_Trans good(cfg,write_only);good.reset();const int reset_count=writes.size();
  assert(good.init());const auto full=writes;const int total=writes.size();
  assert(total>reset_count+2);assert((reads==0)==write_only);assert(wait_ms==(write_only?100:0));
  for(int at:{1,reset_count+1,total}) {
   writes.clear();reads=wait_ms=0;failed_at=at;
   Panel_M5HDMI::HDMI_Trans failed(cfg,write_only);failed.reset();
   assert(failed.init()==!write_only);assert(writes==full);
  }
  writes.clear();reads=wait_ms=0;failed_at=-1;
  Panel_M5HDMI::HDMI_Trans wake(cfg,write_only);assert(wake.init());
  assert(writes[0]==std::make_pair(0xff,0x80));assert(writes[1]==std::make_pair(0xee,0x01));
  assert((reads==0)==write_only);
 }
}
`, "transmitter transfer failures");
});

test("connection check reads IDCODE without programming or status retries", async () => {
  const source = await readPanel();
  const method = (name) => body(source, `Panel_M5HDMI::LOAD_FPGA::${name}(`);
  await compileRun(`
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>
#define ESP_LOGI(...) ((void)0)
uint32_t pin_reg=0,idcode=0;int statuses=0,resets=0,erases=0,waits=0,ones=0,id_bits=0,id_commands=0,tap_moves=0;
bool checking=false;
void delay(int){++waits;}
namespace lgfx {
volatile uint32_t* get_gpio_lo_reg(int){return &pin_reg;}volatile uint32_t* get_gpio_hi_reg(int){return &pin_reg;}
enum class pin_mode_t {output,input,input_pullup};
std::vector<std::pair<int,pin_mode_t>> modes;
void pinMode(int pin,pin_mode_t mode){modes.emplace_back(pin,mode);}
bool gpio_in(int pin){assert(checking&&pin==2&&id_bits<32);return (idcode>>id_bits++)&1u;}
}
const uint8_t fs_bitstream_rle[]={0,0};
enum {ISC_ENABLE,ISC_ERASE,ISC_NOOP,ERASE_DONE,ISC_DISABLE,FAST_PROGRAM,READ_ID_CODE};
struct Panel_M5HDMI {struct LOAD_FPGA {
 volatile uint32_t* _tdi_reg[2];volatile uint32_t* _tck_reg[2];volatile uint32_t* _tms_reg[2];
 uint32_t TCK_MASK,TDI_MASK,TMS_MASK;uint8_t TDO_PIN;
 enum TAP_TypeDef {TAP_UNKNOWN,TAP_IDLE,TAP_DRSHIFT,TAP_DREXIT1};
 LOAD_FPGA()=default;
 LOAD_FPGA(uint_fast8_t,uint_fast8_t,uint_fast8_t,uint_fast8_t);
 void initPins(uint_fast8_t,uint_fast8_t,uint_fast8_t,uint_fast8_t);
 uint32_t waitStatus();
 uint32_t JTAG_ReadIdCode();
 static bool isConnected(uint_fast8_t,uint_fast8_t,uint_fast8_t,uint_fast8_t);
 void JTAG_MoveTap(TAP_TypeDef from,TAP_TypeDef to){
  if(from==TAP_UNKNOWN&&to==TAP_IDLE)++resets;
  if(checking){
   if(tap_moves==0)assert(from==TAP_UNKNOWN&&to==TAP_IDLE);
   else if(tap_moves==1)assert(from==TAP_IDLE&&to==TAP_DRSHIFT);
   else {assert(tap_moves==2);assert(from==TAP_DREXIT1&&to==TAP_IDLE);}
   ++tap_moves;
  }
 }
 uint32_t JTAG_ReadStatus(){assert(!checking&&resets==0);return ++statuses<=ones?0xFFFFFFFFu:0;}
 void JTAG_WriteInst(int op){
  if(op==ISC_ERASE)++erases;
  if(op==READ_ID_CODE){assert(checking&&resets==1&&tap_moves==1);++id_commands;}
 }
 void JTAG_DUMMY_CLOCK(int){}void JTAG_Write(uint_fast8_t,bool,bool,size_t=1){}
};};
void Panel_M5HDMI::LOAD_FPGA::initPins(uint_fast8_t _TCK_PIN,uint_fast8_t _TDI_PIN,uint_fast8_t _TDO_PIN,uint_fast8_t _TMS_PIN) ${method("initPins")}
uint32_t Panel_M5HDMI::LOAD_FPGA::waitStatus() ${method("waitStatus")}
uint32_t Panel_M5HDMI::LOAD_FPGA::JTAG_ReadIdCode() ${method("JTAG_ReadIdCode")}
bool Panel_M5HDMI::LOAD_FPGA::isConnected(uint_fast8_t tck,uint_fast8_t tdi,uint_fast8_t tdo,uint_fast8_t tms) ${method("isConnected")}
Panel_M5HDMI::LOAD_FPGA::LOAD_FPGA(uint_fast8_t _TCK_PIN,uint_fast8_t _TDI_PIN,uint_fast8_t _TDO_PIN,uint_fast8_t _TMS_PIN) ${method("LOAD_FPGA")}
int main(){
 checking=true;
 for(uint32_t code:{0u,0xFFFFFFFFu,0x1100481bu,0x12345678u,1u,0x80000000u}){
  statuses=resets=erases=waits=id_bits=id_commands=tap_moves=0;idcode=code;lgfx::modes.clear();
  assert(Panel_M5HDMI::LOAD_FPGA::isConnected(0,1,2,3)==(code!=0&&code!=0xFFFFFFFFu));
  assert(statuses==0&&waits==0&&erases==0);
  assert(resets==1&&id_commands==1&&id_bits==32&&tap_moves==3);
  assert(lgfx::modes.size()==4);
  assert(lgfx::modes.back()==std::make_pair(2,lgfx::pin_mode_t::input));
 }
 checking=false;
 for(int all_ones:{0,2,128}) {
  statuses=resets=erases=waits=id_bits=id_commands=0;ones=all_ones;lgfx::modes.clear();
  Panel_M5HDMI::LOAD_FPGA fpga(0,1,2,3);
  assert(statuses==(all_ones==128?128:all_ones+1));
  assert(waits==all_ones);
  assert(erases==1&&resets==1&&id_bits==0&&id_commands==0);
  assert(lgfx::modes.size()==4);
  assert(lgfx::modes.back()==std::make_pair(2,lgfx::pin_mode_t::input));
 }
}
`, "transmitter IDCODE presence and loader status retries");
});
