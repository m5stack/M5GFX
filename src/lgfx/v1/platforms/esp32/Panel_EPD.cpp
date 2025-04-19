/*----------------------------------------------------------------------------/
  Lovyan GFX - Graphics library for embedded devices.

Original Source:
 https://github.com/lovyan03/LovyanGFX/

Licence:
 [FreeBSD](https://github.com/lovyan03/LovyanGFX/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)

Contributors:
 [ciniml](https://github.com/ciniml)
 [mongonta0716](https://github.com/mongonta0716)
 [tobozo](https://github.com/tobozo)
/----------------------------------------------------------------------------*/

#include "Panel_EPD.hpp"
#if SOC_LCD_I80_SUPPORTED && defined (CONFIG_IDF_TARGET_ESP32S3)

#include "lgfx/v1/Bus.hpp"
#include "lgfx/v1/platforms/common.hpp"
#include "lgfx/v1/misc/pixelcopy.hpp"
#include "lgfx/v1/misc/colortype.hpp"

#if __has_include(<esp_cache.h>)
#include <esp_cache.h>
#endif
#if defined (ESP_CACHE_MSYNC_FLAG_DIR_C2M)
__attribute__((weak))
int Cache_WriteBack_Addr(uint32_t addr, uint32_t size)
{
  uintptr_t start = addr & ~63u;
  uintptr_t end = (addr + size + 63u) & ~63u;
  if (start >= end) return 0;
  // return esp_cache_msync((void*)start, end - start, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_TYPE_DATA);
  auto res = esp_cache_msync((void*)start, end - start, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_TYPE_DATA);
  if (res != ESP_OK){
    printf("start: %08x, end: %08x\n", start, end);
  }
  return res;
}
#define LGFX_USE_CACHE_WRITEBACK_ADDR
#else
#if defined (CONFIG_IDF_TARGET_ESP32S3)
 #if __has_include(<esp32s3/rom/cache.h>)
  #include <esp32s3/rom/cache.h>
  extern int Cache_WriteBack_Addr(uint32_t addr, uint32_t size);
  #define LGFX_USE_CACHE_WRITEBACK_ADDR
 #endif
#endif
#endif

namespace lgfx
{
 inline namespace v1
 {
//----------------------------------------------------------------------------
#if defined ( LGFX_USE_CACHE_WRITEBACK_ADDR )
  static void cacheWriteBack(const void* ptr, uint32_t size)
  {
    if (!isEmbeddedMemory(ptr))
    {
      Cache_WriteBack_Addr((uint32_t)ptr, size);
    }
  }
#else
  static inline void cacheWriteBack(const void*, uint32_t) {}
#endif

//----------------------------------------------------------------------------

  static constexpr int8_t Bayer[16] = {-30, 2, -22, 10, 18, -14, 26, -6, -18, 14, -26, 6, 30, -2, 22, -10};
  // static constexpr int8_t Bayer[16] = { 0, };

#define LUT_MAKE(d0,d1,d2,d3,d4,d5,d6,d7,d8,d9,da,db,dc,dd,de,df) (uint32_t)((d0<< 0)|(d1<< 2)|(d2<< 4)|(d3<< 6)|(d4<< 8)|(d5<<10)|(d6<<12)|(d7<<14)|(d8<<16)|(d9<<18)|(da<<20)|(db<<22)|(dc<<24)|(dd<<26)|(de<<28)|(df<<30))

// LUTの横軸は色の濃さ。左端が 黒、右端が白の16段階のグレースケール。
// LUTの縦軸は時間軸。上から順に下に向かって処理が進んでいく。
// 値の意味は 0 == no operation / 1 == to black / 2 == to white / 3 == end of data
  static constexpr const uint32_t lut_quality[] = {
    LUT_MAKE(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
    LUT_MAKE(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),  
    LUT_MAKE(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 2, 1, 1, 2, 2, 2, 1, 2, 1, 1, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 2, 1, 1, 2, 2, 1, 2, 2, 2, 2, 1, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 2, 2, 1, 1, 2, 2),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 2, 2, 2, 0),
    LUT_MAKE(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
    LUT_MAKE(3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3),
  };

  static constexpr const uint32_t lut_text[] = {
    LUT_MAKE(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
    LUT_MAKE(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 2, 1, 1, 2, 2, 2, 1, 2, 1, 1, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 2, 1, 1, 2, 2, 1, 2, 2, 2, 2, 1, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 2, 2, 1, 1, 2, 2),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 2, 2, 2, 0),
    LUT_MAKE(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
    LUT_MAKE(3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3),
  };

  static constexpr const uint32_t lut_fast[] = {
    LUT_MAKE(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2),
    LUT_MAKE(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2),
    LUT_MAKE(2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1),
    LUT_MAKE(2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1),
    LUT_MAKE(2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1),
    LUT_MAKE(2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1),
    LUT_MAKE(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2),
    LUT_MAKE(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2),
    LUT_MAKE(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2),
    LUT_MAKE(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2),
    LUT_MAKE(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2),
    LUT_MAKE(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2),
    LUT_MAKE(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2),
    LUT_MAKE(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2),
    LUT_MAKE(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
    LUT_MAKE(3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3),
  };

  static constexpr const uint32_t lut_fastest[] = {
    LUT_MAKE(2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1),
    LUT_MAKE(2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1),
    LUT_MAKE(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2),
    LUT_MAKE(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2),
    LUT_MAKE(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2),
    LUT_MAKE(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2),
    LUT_MAKE(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2),
    LUT_MAKE(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2),
    LUT_MAKE(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
    LUT_MAKE(3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3),
  };
#undef LUT_MAKE

  Panel_EPD::Panel_EPD(void)
  {
    _epd_mode = epd_mode_t::epd_quality;
    _auto_display = true;
  }

  Panel_EPD::~Panel_EPD(void)
  {
  }

  color_depth_t Panel_EPD::setColorDepth(color_depth_t depth)
  {
    _write_depth = color_depth_t::rgb888_3Byte;
    _read_depth = color_depth_t::rgb888_3Byte;
    return depth;
  }

  bool Panel_EPD::init(bool use_reset)
  {
    auto cfg_detail = config_detail();
    if (cfg_detail.lut_quality == nullptr) {
      cfg_detail.lut_quality = lut_quality;
      cfg_detail.lut_quality_step = sizeof(lut_quality) / sizeof(uint32_t);
    }
    if (cfg_detail.lut_text == nullptr) {
      cfg_detail.lut_text = lut_text;
      cfg_detail.lut_text_step = sizeof(lut_text) / sizeof(uint32_t);
    }
    if (cfg_detail.lut_fast == nullptr) {
      cfg_detail.lut_fast = lut_fast;
      cfg_detail.lut_fast_step = sizeof(lut_fast) / sizeof(uint32_t);
    }
    if (cfg_detail.lut_fastest == nullptr) {
      cfg_detail.lut_fastest = lut_fastest;
      cfg_detail.lut_fastest_step = sizeof(lut_fastest) / sizeof(uint32_t);
    }
    config_detail(cfg_detail);

    Panel_HasBuffer::init(use_reset);

    if (init_intenal()) {
      memset(_buf, 0xFF, _cfg.memory_width * _cfg.memory_height / 2);
      _range_mod.top    = INT16_MAX;
      _range_mod.left   = INT16_MAX;
      _range_mod.right  = 0;
      _range_mod.bottom = 0;
      return true;
    }
    return false;
  }


  bool Panel_EPD::init_intenal(void)
  {
    auto memory_w = _cfg.memory_width;
    auto memory_h = _cfg.memory_height;
    if (memory_w == 0 || memory_h == 0) { return false; }

    size_t lut_total_step = 1;
    lut_total_step += _config_detail.lut_quality_step;
    lut_total_step += _config_detail.lut_text_step;
    lut_total_step += _config_detail.lut_fast_step;
    lut_total_step += _config_detail.lut_fastest_step;

    // EPD制御用LUT (2ピクセルセット)
    _lut_2pixel = (uint8_t *)heap_caps_malloc(lut_total_step * 256 * sizeof(uint16_t), MALLOC_CAP_DMA);

    // リフレッシュの進行状況付きフレームバッファ (下位8bitはピクセル2個分の16階調値そのまま)
    _step_framebuf = (uint16_t *)heap_caps_aligned_alloc(16, (memory_w * memory_h), MALLOC_CAP_SPIRAM); // current pixels

    // 面積分のフレームバッファ (1Byte=2pixel)
    _buf = (uint8_t *)heap_caps_aligned_alloc(16, (memory_w * memory_h) / 2, MALLOC_CAP_SPIRAM); // current pixels

    // DMA転送用バッファx2 (1Byte=4pixel)
    _dma_bufs[0] = (uint8_t *)heap_caps_malloc((memory_w / 4) + _config_detail.line_padding + 16, MALLOC_CAP_DMA);
    _dma_bufs[1] = (uint8_t *)heap_caps_malloc((memory_w / 4) + _config_detail.line_padding + 16, MALLOC_CAP_DMA);

    if (!_step_framebuf || !_buf || !_dma_bufs[0] || !_dma_bufs[1] || !_lut_2pixel) {
      if (_buf) { heap_caps_free(_buf); _buf = nullptr; }
      if (_dma_bufs[0]) { heap_caps_free(_dma_bufs[0]); _dma_bufs[0] = nullptr; }
      if (_dma_bufs[1]) { heap_caps_free(_dma_bufs[1]); _dma_bufs[1] = nullptr; }
      if (_lut_2pixel) { heap_caps_free(_lut_2pixel); _lut_2pixel = nullptr; }

      return false;
    }
    // リフレッシュの進行状況付きフレームバッファ初期値を中間色相当にしておく
    for (int i = 0; i < memory_w * memory_h >> 1; ++i) {
      _step_framebuf[i] = 0x0088u;
    }

    auto dst = _lut_2pixel;
    memset(dst, 0x0F, 256);
    size_t lindex = 256;
    for (int epd_mode = 1; epd_mode < 5; ++epd_mode) {
      const uint32_t* lut_src = nullptr;
      size_t lut_step = 0;
      switch (epd_mode) {
        default: continue;
        case epd_mode_t::epd_quality: lut_src = _config_detail.lut_quality; lut_step = _config_detail.lut_quality_step; break;
        case epd_mode_t::epd_text:    lut_src = _config_detail.lut_text;    lut_step = _config_detail.lut_text_step;    break;
        case epd_mode_t::epd_fast:    lut_src = _config_detail.lut_fast;    lut_step = _config_detail.lut_fast_step;    break;
        case epd_mode_t::epd_fastest: lut_src = _config_detail.lut_fastest; lut_step = _config_detail.lut_fastest_step; break;
      }
      if (lut_src == nullptr) { continue; }
      _lut_offset_table[epd_mode] = lindex >> 8;
      _lut_remain_table[epd_mode] = lut_step;
// ESP_LOGV("dbg", "\n\nepd_mode: %d, offset: %d, remain: %d\n", epd_mode, _lut_offset_table[epd_mode], _lut_remain_table[epd_mode]);
// printf("\nepd_mode: %d, offset: %d, remain: %d\n\n", epd_mode, _lut_offset_table[epd_mode], _lut_remain_table[epd_mode]);
      for (int step = 0; step < lut_step; ++step) {
        auto lu = lut_src[0];
        for (int lv = 0; lv < 256; ++lv) {
          dst[lindex] = (((lu >> ((lv >> 4) << 1)) & 3) << 2) + ((lu >> ((lv & 15) << 1)) & 3);
          ++lindex;
        }
        ++lut_src;
      }
    }

    _update_queue_handle = xQueueCreate(4, sizeof(update_data_t));
    auto task_priority = _config_detail.task_priority;
    auto task_pinned_core = _config_detail.task_pinned_core;
    if (task_pinned_core >= portNUM_PROCESSORS)
    {
      task_pinned_core = (xPortGetCoreID() + 1) % portNUM_PROCESSORS;
    }
    xTaskCreatePinnedToCore((TaskFunction_t)task_update, "epd", 2048, this, task_priority, &_task_update_handle, task_pinned_core);
    // タスク側とメイン側の処理CPUコアが異なる場合、PSRAMのキャッシュ同期をしないとフレームバッファが即時反映されない点に注意

    return true;
  }

  void Panel_EPD::beginTransaction(void)
  {
  }

  void Panel_EPD::endTransaction(void)
  {
  }

  void Panel_EPD::waitDisplay(void)
  {
    while (_display_busy) { vTaskDelay(1); }
  }

  bool Panel_EPD::displayBusy(void)
  {
// キュー _update_queue_handle に余裕があるか調べる
    if (_update_queue_handle && uxQueueSpacesAvailable(_update_queue_handle) == 0) {
      return true;
    }
    return false;
  };


  void Panel_EPD::setInvert(bool invert)
  {
    // unimplemented
    _invert = invert;
  }

  void Panel_EPD::setSleep(bool flg)
  {
    getBusEPD()->powerControl(!flg);
  }

  void Panel_EPD::setPowerSave(bool flg)
  {
    getBusEPD()->powerControl(!flg);
  }

  void Panel_EPD::writeFillRectPreclipped(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, uint32_t rawcolor)
  {
    uint_fast16_t xs = x, xe = x + w - 1;
    uint_fast16_t ys = y, ye = y + h - 1;
    _xs = xs;
    _ys = ys;
    _xe = xe;
    _ye = ye;
    _update_transferred_rect(xs, ys, xe, ye);

    bgr888_t color { rawcolor };
    int32_t sum = (color.R8() + (color.G8() << 1) + color.B8());

    bool fast = _epd_mode == epd_mode_t::epd_fast || _epd_mode == epd_mode_t::epd_fastest;
    y = ys;
    do
    {
      x = xs;
      auto btbl = &Bayer[(y & 3) << 2];
      auto buf = &_buf[y * ((_cfg.panel_width + 1) >> 1)];

      if (fast) {
        do
        {
          size_t idx = x >> 1;
          uint_fast8_t shift = (x & 1) ? 0 : 4;
          uint_fast8_t value = (sum + btbl[x & 3] * 16 < 512 ? 0 : 0xF) << shift;
          buf[idx] = (buf[idx] & (0xF0 >> shift)) | value;
        } while (++x <= xe);
      } else {
        do
        {
          size_t idx = x >> 1;
          uint_fast8_t shift = (x & 1) ? 0 : 4;
          uint_fast8_t value = (std::min<int32_t>(15, std::max<int32_t>(0, sum + btbl[x & 3]) >> 6) & 0x0F) << shift;
          buf[idx] = (buf[idx] & (0xF0 >> shift)) | value;
        } while (++x <= xe);
      }
    } while (++y <= ye);
  }

  void Panel_EPD::writeImage(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, pixelcopy_t* param, bool use_dma)
  {
    uint_fast16_t xs = x, xe = x + w - 1;
    uint_fast16_t ys = y, ye = y + h - 1;
    _update_transferred_rect(xs, ys, xe, ye);

    auto readbuf = (bgr888_t*)alloca(w * sizeof(bgr888_t));
    auto sx = param->src_x32;
    h += y;
    do
    {
      uint32_t prev_pos = 0, new_pos = 0;
      do
      {
        new_pos = param->fp_copy(readbuf, prev_pos, w, param);
        if (new_pos != prev_pos)
        {
          do
          {
            auto color = readbuf[prev_pos];
            _draw_pixel(x + prev_pos, y, (color.R8() + (color.G8() << 1) + color.B8()));
          } while (new_pos != ++prev_pos);
        }
      } while (w != new_pos && w != (prev_pos = param->fp_skip(new_pos, w, param)));
      param->src_x32 = sx;
      param->src_y++;
    } while (++y < h);
  }

  void Panel_EPD::writePixels(pixelcopy_t* param, uint32_t length, bool use_dma)
  {
    {
      uint_fast16_t xs = _xs;
      uint_fast16_t xe = _xe;
      uint_fast16_t ys = _ys;
      uint_fast16_t ye = _ye;
      _update_transferred_rect(xs, ys, xe, ye);
    }
    uint_fast16_t xs   = _xs  ;
    uint_fast16_t ys   = _ys  ;
    uint_fast16_t xe   = _xe  ;
    uint_fast16_t ye   = _ye  ;
    uint_fast16_t xpos = _xpos;
    uint_fast16_t ypos = _ypos;

    static constexpr uint32_t buflen = 16;
    bgr888_t colors[buflen];
    int bufpos = buflen;
    do
    {
      if (bufpos == buflen) {
        param->fp_copy(colors, 0, std::min(length, buflen), param);
        bufpos = 0;
      }
      auto color = colors[bufpos++];
      _draw_pixel(xpos, ypos, (color.R8() + (color.G8() << 1) + color.B8()));
      if (++xpos > xe)
      {
        xpos = xs;
        if (++ypos > ye)
        {
          ypos = ys;
        }
      }
    } while (--length);
    _xpos = xpos;
    _ypos = ypos;
  }

  void Panel_EPD::readRect(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, void* __restrict dst, pixelcopy_t* param)
  {
    auto readbuf = (bgr888_t*)alloca(w * sizeof(bgr888_t));
    param->src_data = readbuf;
    int32_t readpos = 0;
    h += y;
    do
    {
      uint32_t idx = 0;
      do
      {
        readbuf[idx] = 0x010101u * (_read_pixel(x + idx, y) * 16 + 8);
      } while (++idx != w);
      param->src_x32 = 0;
      readpos = param->fp_copy(dst, readpos, readpos + w, param);
    } while (++y < h);
  }

  void Panel_EPD::_draw_pixel(uint_fast16_t x, uint_fast16_t y, uint32_t sum)
  {
    _rotate_pos(x, y);

    auto btbl = &Bayer[(y & 3) << 2];
    size_t idx = (x >> 1) + (y * ((_cfg.panel_width + 1) >> 1));
    uint_fast8_t shift = (x & 1) ? 0 : 4;
    uint_fast8_t value;
    bool fast = _epd_mode == epd_mode_t::epd_fast || _epd_mode == epd_mode_t::epd_fastest;
    if (fast) {
      value = ((int32_t)sum + btbl[x & 3] * 16 < 512 ? 0 : 0xF) << shift;
    } else {
      value = (std::min<int32_t>(15, std::max<int32_t>(0, sum + btbl[x & 3]) >> 6) & 0x0F) << shift;
    }
    _buf[idx] = (_buf[idx] & (0xF0 >> shift)) | value;
  }

  uint8_t Panel_EPD::_read_pixel(uint_fast16_t x, uint_fast16_t y)
  {
    _rotate_pos(x, y);
    size_t idx = (x >> 1) + (y * ((_cfg.panel_width + 1) >> 1));
    return (x & 1)
         ? (_buf[idx] & 0x0F)
         : (_buf[idx] >> 4)
         ;
  }

  void Panel_EPD::_update_transferred_rect(uint_fast16_t &xs, uint_fast16_t &ys, uint_fast16_t &xe, uint_fast16_t &ye)
  {
    _rotate_pos(xs, ys, xe, ye);
    _range_mod.left   = std::min<int32_t>(xs, _range_mod.left);
    _range_mod.right  = std::max<int32_t>(xe, _range_mod.right);
    _range_mod.top    = std::min<int32_t>(ys, _range_mod.top);
    _range_mod.bottom = std::max<int32_t>(ye, _range_mod.bottom);
  }

  void Panel_EPD::display(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h)
  {
    if (0 < w && 0 < h)
    {
      _range_mod.left   = std::min<int16_t>(_range_mod.left  , x        );
      _range_mod.right  = std::max<int16_t>(_range_mod.right , x + w - 1);
      _range_mod.top    = std::min<int16_t>(_range_mod.top   , y        );
      _range_mod.bottom = std::max<int16_t>(_range_mod.bottom, y + h - 1);
    }

    if (_range_mod.empty()) { return; }

    uint_fast16_t xs = _range_mod.left + _cfg.offset_x;
    uint_fast16_t xe = _range_mod.right + _cfg.offset_x;
    uint_fast16_t ys = _range_mod.top    + _cfg.offset_y;
    uint_fast16_t ye = _range_mod.bottom + _cfg.offset_y;

    update_data_t upd;
    upd.x = xs;
    upd.w = xe - xs + 1;
    upd.y = ys;
    upd.h = ye - ys + 1;
    upd.mode = _epd_mode;

    cacheWriteBack(&_buf[y * _cfg.memory_width >> 1], h * _cfg.memory_width >> 1);
    vTaskDelay(1);
    bool res = xQueueSend(_update_queue_handle, &upd, 128 / portTICK_PERIOD_MS) == pdTRUE;
    vTaskDelay(1);
// printf("\nres: %d, xs: %d, xe: %d, ys: %d, ye: %d\n", res, xs, xe, ys, ye);
    if (res)
    {
      _range_mod.top    = INT16_MAX;
      _range_mod.left   = INT16_MAX;
      _range_mod.right  = 0;
      _range_mod.bottom = 0;
    }
  }

  void Panel_EPD::task_update(Panel_EPD* me)
  {
    me->_display_busy = true;
    update_data_t new_data;

    const size_t data_len = me->_cfg.memory_width;
    const size_t write_len = (data_len / 4) + me->_config_detail.line_padding;
    const size_t mh = me->_cfg.memory_height;

    auto bus = me->getBusEPD();

    bus->powerControl(true);

    uint32_t remain = 0;

    for (;;) {
      vTaskDelay(1);
      me->_display_busy = (remain != 0);
      TickType_t wait_tick = remain ? 0 : portMAX_DELAY;
      if (xQueueReceive(me->_update_queue_handle, &new_data, wait_tick)) {
        me->_display_busy = true;
        uint32_t retry = 4;
        do {
          auto lut_remain = me->_lut_remain_table[new_data.mode];
          if (remain < lut_remain) {
            remain = lut_remain;
          }
          // 範囲内のピクセルをすべて操作し変化分を反映する
          size_t idx = (new_data.x + new_data.y * data_len) >> 1;
          auto src = &me->_buf[idx];
          auto dst = &me->_step_framebuf[idx];
          size_t h = new_data.h;
          size_t w = (((new_data.x & 1) + new_data.w) >> 1) + 1;
          uint_fast16_t lut_offset = me->_lut_offset_table[new_data.mode] << 8;
          uint_fast16_t lut_last = lut_offset + ((me->_lut_remain_table[new_data.mode]) << 8);
          if (new_data.mode != epd_mode_t::epd_fastest) { lut_last -= 256; }
// printf("\n new_data: x:%d y:%d w:%d h:%d \n", new_data.x, new_data.y, new_data.w, new_data.h);
          do {
            auto s = src - 1;
            auto d = dst - 1;
            src += data_len >> 1;
            dst += data_len >> 1;
            {
              for (int i = 0; i < w; ++i) {
                auto dval = d[1];
                auto sval = s[1];
                d++;
                s++;
                if ((dval >= lut_last) || (dval < lut_offset) || ((dval & 0xFF) != sval))
                {
                  *d = sval + lut_offset;
                }
              }
            }
          } while (--h);
          if (--retry == 0) {
            break;
          }
        } while (xQueueReceive(me->_update_queue_handle, &new_data, 0));
      }
      if (remain == 0) {
        bus->powerControl(false);
        continue;
      }
      --remain;

      bus->powerControl(true);

      auto lut = me->_lut_2pixel;
      for (uint_fast16_t y = 0; y < mh; y++) {
        int x = 0;
        int w = data_len >> 2;
        uint8_t *dma_buf = me->_dma_bufs[y & 1];
        {
          auto sb = &me->_step_framebuf[y * data_len >> 1];
          auto dst = dma_buf;
          do {
            auto sb0 = sb[0];
            auto sb1 = sb[1];
            auto fb0 = lut[sb0];
            auto fb1 = lut[sb1];
            if (fb0 != 0x0F) {
              sb[0] = sb0 + 256;
            }
            if (fb1 != 0x0F) {
              sb[1] = sb1 + 256;
            }
            dst[0] = (fb0 << 4) + fb1;
            sb += 2;
            dst += 1;
          } while (--w);
        }
        if (y == 0) {
          bus->beginTransaction();
        } else {
          bus->scanlineDone();
        }
        bus->writeScanLine(dma_buf, write_len);
      }

      bus->scanlineDone();
      bus->endTransaction();
    }
  }

//----------------------------------------------------------------------------
 }
}

#endif
