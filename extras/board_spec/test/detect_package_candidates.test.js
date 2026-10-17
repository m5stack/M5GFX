import assert from "node:assert/strict";
import { promises as fs } from "node:fs";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { body, compileRun } from "./detect_host_helpers.js";
const src=path.resolve(path.dirname(fileURLToPath(import.meta.url)),"../../../src");

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
