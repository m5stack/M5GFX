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

#define LUT_MAKE(d0,d1,d2,d3,d4,d5,d6,d7,d8,d9,da,db,dc,dd,de,df) (uint32_t)((d0<< 0)|(d1<< 2)|(d2<< 4)|(d3<< 6)|(d4<< 8)|(d5<<10)|(d6<<12)|(d7<<14)|(d8<<16)|(d9<<18)|(da<<20)|(db<<22)|(dc<<24)|(dd<<26)|(de<<28)|(df<<30))

// LUTの横軸は色の濃さ。左端が 黒、右端が白の16段階のグレースケール。
// LUTの縦軸は時間軸。上から順に下に向かって処理が進んでいく。
// 値の意味は 0 == end of data / 1 == to black / 2 == to white / 3 == no operation
// LUT_MAKE１行あたり 1フレーム分の16階調それぞれの動作が定義される。
  static constexpr const uint32_t lut_quality[] = {
    LUT_MAKE(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
    LUT_MAKE(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 1, 1, 1, 1, 1),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2),
    LUT_MAKE(1, 1, 2, 2, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2),
    LUT_MAKE(1, 1, 1, 1, 1, 2, 1, 1, 2, 2, 1, 2, 1, 2, 2, 2),
    LUT_MAKE(1, 1, 3, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2),
    LUT_MAKE(3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 1, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 2, 2, 2, 3),
    LUT_MAKE(3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3),
    LUT_MAKE(3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3),
    LUT_MAKE(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
  };
  static constexpr const uint32_t lut_text[] = {
    LUT_MAKE(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
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
    LUT_MAKE(1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 2, 2, 1, 1, 2, 2),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 2, 2, 2, 2, 2, 2),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3),
    LUT_MAKE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 2, 2, 2, 3),
    LUT_MAKE(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
  };

  static constexpr const uint32_t lut_fast[] = {
    LUT_MAKE(1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2),
    LUT_MAKE(2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1),
    LUT_MAKE(1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2),
    LUT_MAKE(1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2),
    LUT_MAKE(1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2),
    LUT_MAKE(1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2),
    LUT_MAKE(1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2),
    LUT_MAKE(1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2),
    LUT_MAKE(1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2),
    LUT_MAKE(1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2),
    LUT_MAKE(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
  };

  static constexpr const uint32_t lut_fastest[] = {
    LUT_MAKE(1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2),
    LUT_MAKE(1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2),
    LUT_MAKE(1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2),
    LUT_MAKE(1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2),
    LUT_MAKE(1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2),
    LUT_MAKE(1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2),
    LUT_MAKE(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
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
    _write_depth = color_depth_t::grayscale_8bit;
    _read_depth = color_depth_t::grayscale_8bit;
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
      memset(_buf, 0xFF, _cfg.panel_width * _cfg.panel_height / 2);
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
    auto panel_w = _cfg.panel_width;
    auto panel_h = _cfg.panel_height;
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
    _buf = (uint8_t *)heap_caps_aligned_alloc(16, (panel_w * panel_h) / 2, MALLOC_CAP_SPIRAM); // current pixels

    // DMA転送用バッファx2 (1Byte=4pixel)
    const auto dma_len = memory_w / 4 + _config_detail.line_padding;
    _dma_bufs[0] = (uint8_t *)heap_caps_malloc(dma_len, MALLOC_CAP_DMA);
    _dma_bufs[1] = (uint8_t *)heap_caps_malloc(dma_len, MALLOC_CAP_DMA);

    if (!_step_framebuf || !_buf || !_dma_bufs[0] || !_dma_bufs[1] || !_lut_2pixel) {
      if (_buf) { heap_caps_free(_buf); _buf = nullptr; }
      if (_dma_bufs[0]) { heap_caps_free(_dma_bufs[0]); _dma_bufs[0] = nullptr; }
      if (_dma_bufs[1]) { heap_caps_free(_dma_bufs[1]); _dma_bufs[1] = nullptr; }
      if (_lut_2pixel) { heap_caps_free(_lut_2pixel); _lut_2pixel = nullptr; }

      return false;
    }
    memset(_dma_bufs[0], 0, dma_len);
    memset(_dma_bufs[1], 0, dma_len);

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

    // 白ピクセルの初期値をセット
    for (int i = 0; i < memory_w * memory_h >> 1; ++i) {
      _step_framebuf[i] = 0xFFFFu;
    }

    _update_queue_handle = xQueueCreate(8, sizeof(update_data_t));
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

    int32_t sum = rawcolor << 2;

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

    auto readbuf = (grayscale_t*)alloca(w * sizeof(grayscale_t));
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
          _draw_pixels(x + prev_pos, y, &readbuf[prev_pos], new_pos - prev_pos);
          prev_pos = new_pos;
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
    grayscale_t colors[buflen];
    int bufpos = buflen;

    do
    {
      if (bufpos == buflen) {
        param->fp_copy(colors, 0, std::min(length, buflen), param);
        bufpos = 0;
      }

      uint32_t len = std::min(length, buflen - bufpos);
      len = std::min<uint32_t>(len, 1u + xe - xpos);
      _draw_pixels(xpos, ypos, &colors[bufpos], len);
      bufpos += len;
      length -= len;
      xpos += len;
      if (xpos > xe)
      {
        xpos = xs;
        if (++ypos > ye)
        {
          ypos = ys;
        }
      }
    } while (length);

    _xpos = xpos;
    _ypos = ypos;
  }

  void Panel_EPD::readRect(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, void* __restrict dst, pixelcopy_t* param)
  {
    auto readbuf = (grayscale_t*)alloca(w * sizeof(grayscale_t));
    param->src_data = readbuf;
    int32_t readpos = 0;
    h += y;
    do
    {
      uint32_t idx = 0;
      do
      {
        readbuf[idx] = _read_pixel(x + idx, y);
      } while (++idx != w);
      param->src_x32 = 0;
      readpos = param->fp_copy(dst, readpos, readpos + w, param);
    } while (++y < h);
  }

  __attribute((optimize("-O3")))
  void Panel_EPD::_draw_pixels(uint_fast16_t x, uint_fast16_t y, const grayscale_t* values, size_t len)
  {
    int ax = 1;
    int ay = 0;
    uint_fast8_t r = _internal_rotation;
    if (r)
    {
      if (r & 1) { std::swap(x, y); std::swap(ax, ay); }
      uint_fast8_t rb = 1 << r;
      if (rb & 0b11000110) { x = _cfg.panel_width  - 1 - x; ax = -ax; } // case 1:2:6:7:
      if (rb & 0b10011100) { y = _cfg.panel_height - 1 - y; ay = -ay; } // case 2:3:4:7:
    }

    const bool fast = _epd_mode == epd_mode_t::epd_fast || _epd_mode == epd_mode_t::epd_fastest;

    if (ax) {
      const auto btbl = &Bayer[(y & 3) << 2];
      size_t idx = x + (y * _cfg.panel_width);

      uint_fast8_t value = _buf[idx >> 1] & 0xF0;
      if (fast) {
        if (idx & 1) {
          values--;
          goto LABEL_FAST_ODD_START;
        }
        do {
          {
            int32_t sum = values[0].get();
            value = ((sum + (btbl[idx & 3] << 2)) < 128 ? 0 : 0xF0);
            idx += ax;
          }
          if (--len == 0) { goto LABEL_ODD_END; }

LABEL_FAST_ODD_START:

          {
            int32_t sum = values[1].get();
            values += 2;
            value += ((sum + (btbl[idx & 3] << 2)) < 128 ? 0 : 0x0F);
            _buf[idx >> 1] = value;
            idx += ax;
          }
        } while (--len);
        return;
      } else {
        if (idx & 1) {
          values--;
          goto LABEL_ODD_START;
        }
        do {
          {
            int32_t sum = values[0].get();
            value = (std::min<int32_t>(15, std::max<int32_t>(0, (sum << 2) + btbl[idx & 3]) >> 6) & 0x0F) << 4;
            idx += ax;
          }
          if (--len == 0) { goto LABEL_ODD_END; }
LABEL_ODD_START:
          {
            int32_t sum = values[1].get();
            values += 2;
            value += (std::min<int32_t>(15, std::max<int32_t>(0, (sum << 2) + btbl[idx & 3]) >> 6) & 0x0F);
            _buf[idx >> 1] = value;
            idx += ax;
          }
        } while (--len);
        return;
      }

      {
LABEL_ODD_END:
        _buf[idx >> 1] = (_buf[idx >> 1] & 0x0F) + value;
        return;
      }
    } else {
      size_t idx = (x >> 1) + (y * ((_cfg.panel_width + 1) >> 1));
      const uint_fast8_t shift = (x & 1) ? 0 : 4;
      do {
        int32_t sum = values->get() << 2;
        ++values;
        auto btbl = &Bayer[(y & 3) << 2];
        uint_fast8_t value;
        if (fast) {
          value = ((int32_t)sum + btbl[x & 3] * 16 < 512 ? 0 : 0xF) << shift;
        } else {
          value = (std::min<int32_t>(15, std::max<int32_t>(0, sum + btbl[x & 3]) >> 6) & 0x0F) << shift;
        }
        _buf[idx] = (_buf[idx] & (0xF0 >> shift)) | value;
        idx += ay * ((_cfg.panel_width + 1) >> 1);
        y += ay;
      } while (--len);
    }
  }

  uint8_t Panel_EPD::_read_pixel(uint_fast16_t x, uint_fast16_t y)
  {
    _rotate_pos(x, y);
    size_t idx = (x >> 1) + (y * ((_cfg.panel_width + 1) >> 1));
    return 0x11u * ((x & 1)
         ? (_buf[idx] & 0x0F)
         : (_buf[idx] >> 4))
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

    uint_fast16_t xs = (_range_mod.left + _cfg.offset_x) & ~1u;
    uint_fast16_t xe = (_range_mod.right + _cfg.offset_x) & ~1u;
    uint_fast16_t ys = _range_mod.top    + _cfg.offset_y;
    uint_fast16_t ye = _range_mod.bottom + _cfg.offset_y;

    update_data_t upd;
    upd.x = xs;
    upd.w = xe - xs + 2;
    upd.y = ys;
    upd.h = ye - ys + 1;
    upd.mode = _epd_mode;

    cacheWriteBack(&_buf[y * _cfg.panel_width >> 1], h * _cfg.panel_width >> 1);
    delay(1);
    bool res = xQueueSend(_update_queue_handle, &upd, 128 / portTICK_PERIOD_MS) == pdTRUE;
// printf("\nres: %d, xs: %d, xe: %d, ys: %d, ye: %d\n", res, xs, xe, ys, ye);
    if (res)
    {
      _range_mod.top    = INT16_MAX;
      _range_mod.left   = INT16_MAX;
      _range_mod.right  = 0;
      _range_mod.bottom = 0;
    }
  }

#if defined( __XTENSA__ )
  __attribute__((noinline,noclone))
  static void blit_dmabuf(uint32_t* dst, uint16_t* src, const uint8_t* lut, size_t len)
  {
#define DST "a2"  // a2 == dst
#define SRC "a3"  // a3 == src
#define LUT "a4"  // a4 == lut
                  // a5 == len
#define VAL "a5"  // value tmp
#define S_0 "a6"  // pixel section0 value
#define S_1 "a7"  // pixel section1 value
#define S_2 "a8"  // pixel section2 value
#define S_3 "a9"  // pixel section3 value
#define S_4 "a10" // pixel section4 value
#define S_5 "a11" // pixel section5 value
#define S_6 "a12" // pixel section6 value
#define S_7 "a13" // pixel section7 value
#define LPX "a14" // lut pixel data
#define BUF "a15" // pixel result (uint32) value

  __asm__ __volatile(
    " loop     a5, BLT_BUFFER_END          \n"  // lenの回数だけループ命令で処理

    " movi   " BUF ", 0                    \n"  // 出力用バッファを0クリア
    " l16si  " S_0 "," SRC ", 0            \n"  // S_0 = src[0]; // 元データを 8セット分 取得
    " l16si  " S_1 "," SRC ", 2            \n"  // S_1 = src[1];
    " l16si  " S_2 "," SRC ", 4            \n"  // S_2 = src[2];
    " l16si  " S_3 "," SRC ", 6            \n"  // S_3 = src[3];
    " l16si  " S_4 "," SRC ", 8            \n"  // S_4 = src[4];
    " l16si  " S_5 "," SRC ", 10           \n"  // S_5 = src[5];
    " l16si  " S_6 "," SRC ", 12           \n"  // S_6 = src[6];
    " l16si  " S_7 "," SRC ", 14           \n"  // S_7 = src[7];
    " bgei   " S_0 ",  0    , BLT_SECTION0 \n"  // データ値が負でない場合は更新処理を行うためジャンプ
    " bgei   " S_1 ",  0    , BLT_SECTION1 \n"
    "BLT_RETURN1:                          \n"
    " bgei   " S_2 ",  0    , BLT_SECTION2 \n"
    "BLT_RETURN2:                          \n"
    " bgei   " S_3 ",  0    , BLT_SECTION3 \n"
    "BLT_RETURN3:                          \n"
    " bgei   " S_4 ",  0    , BLT_SECTION4 \n"
    "BLT_RETURN4:                          \n"
    " bgei   " S_5 ",  0    , BLT_SECTION5 \n"
    "BLT_RETURN5:                          \n"
    " bgei   " S_6 ",  0    , BLT_SECTION6 \n"
    "BLT_RETURN6:                          \n"
    " bgei   " S_7 ",  0    , BLT_SECTION7 \n"
    "BLT_RETURN7:                          \n"
    " s32i   " BUF "," DST ",  0           \n"  // データを出力
    " addi   " SRC "," SRC ",  16          \n"  // 元データのポインタを進める
    " addi   " DST "," DST ",  4           \n"  // 出力先のポインタを進める
    "BLT_BUFFER_END:                       \n"  // ループ終端
    " retw                                 \n"  // 関数終了

    "BLT_SECTION0:                         \n"
    " add    " LPX "," S_0 "," LUT "       \n"  // LPX = &lut[S_0]
    " l8ui   " LPX "," LPX ", 0            \n"  // LPX = *LPX
    " addmi  " VAL "," S_0 ", -32768       \n"  // VAL = S_0 - 32768
    " addmi  " S_0 "," S_0 ", 256          \n"  // S_0 += 256
    " moveqz " S_0 "," VAL "," LPX "       \n"  // if (LPX == 0) S_0 = VAL
    " s16i   " S_0 "," SRC ", 0            \n"  // src[0] = S_0;
    " slli   " LPX "," LPX ", 4            \n"  // LPX <<= 4
    " add    " BUF "," BUF "," LPX "       \n"  // buf += LPX
    " blti   " S_1 ",  0    , BLT_RETURN1  \n"

    "BLT_SECTION1:                         \n"
    " add    " LPX "," S_1 "," LUT "       \n"  // LPX = &lut[S_1]
    " l8ui   " LPX "," LPX ", 0            \n"  // LPX = *LPX
    " addmi  " VAL "," S_1 ", -32768       \n"  // VAL = S_1 - 32768
    " addmi  " S_1 "," S_1 ", 256          \n"  // S_1 += 256
    " moveqz " S_1 "," VAL "," LPX "       \n"  // if (LPX == 0) S_1 = VAL
    " s16i   " S_1 "," SRC ", 2            \n"  // src[1] = S_1;
  //" slli   " LPX "," LPX ", 0            \n"  // LPX <<= 0
    " add    " BUF "," BUF "," LPX "       \n"  // buf += LPX
    " blti   " S_2 ",  0    , BLT_RETURN2  \n"

    "BLT_SECTION2:                         \n"
    " add    " LPX "," S_2 "," LUT "       \n"  // LPX = &lut[S_1]
    " l8ui   " LPX "," LPX ", 0            \n"  // LPX = *LPX
    " addmi  " VAL "," S_2 ", -32768       \n"  // VAL = S_2 - 32768
    " addmi  " S_2 "," S_2 ", 256          \n"  // S_2 += 256
    " moveqz " S_2 "," VAL "," LPX "       \n"  // if (LPX == 0) S_2 = VAL
    " s16i   " S_2 "," SRC ", 4            \n"  // src[2] = S_2;
    " slli   " LPX "," LPX ", 12           \n"  // LPX <<= 12
    " add    " BUF "," BUF "," LPX "       \n"  // buf += LPX
    " blti   " S_3 ",  0    , BLT_RETURN3  \n"

    "BLT_SECTION3:                         \n"
    " add    " LPX "," S_3 "," LUT "       \n"  // LPX = &lut[S_3]
    " l8ui   " LPX "," LPX ", 0            \n"  // LPX = *LPX
    " addmi  " VAL "," S_3 ", -32768       \n"  // VAL = S_3 - 32768
    " addmi  " S_3 "," S_3 ", 256          \n"  // S_3 += 256
    " moveqz " S_3 "," VAL "," LPX "       \n"  // if (LPX == 0) S_3 = VAL
    " s16i   " S_3 "," SRC ", 6            \n"  // src[3] = S_3;
    " slli   " LPX "," LPX ", 8            \n"  // LPX <<= 8
    " add    " BUF "," BUF "," LPX "       \n"  // buf += LPX
    " blti   " S_4 ",  0    , BLT_RETURN4  \n"

    "BLT_SECTION4:                         \n"
    " add    " LPX "," S_4 "," LUT "       \n"  // LPX = &lut[S_4]
    " l8ui   " LPX "," LPX ", 0            \n"  // LPX = *LPX
    " addmi  " VAL "," S_4 ", -32768       \n"  // VAL = S_4 - 32768
    " addmi  " S_4 "," S_4 ", 256          \n"  // S_4 += 256
    " moveqz " S_4 "," VAL "," LPX "       \n"  // if (LPX == 0) S_4 = VAL
    " s16i   " S_4 "," SRC ", 8            \n"  // src[4] = S_4;
    " slli   " LPX "," LPX ", 20           \n"  // LPX <<= 20
    " add    " BUF "," BUF "," LPX "       \n"  // buf += LPX
    " blti   " S_5 ",  0    , BLT_RETURN5  \n"
  
    "BLT_SECTION5:                         \n"
    " add    " LPX "," S_5 "," LUT "       \n"  // LPX = &lut[S_5]
    " l8ui   " LPX "," LPX ", 0            \n"  // LPX = *LPX
    " addmi  " VAL "," S_5 ", -32768       \n"  // VAL = S_5 - 32768
    " addmi  " S_5 "," S_5 ", 256          \n"  // S_5 += 256
    " moveqz " S_5 "," VAL "," LPX "       \n"  // if (LPX == 0) S_5 = VAL
    " s16i   " S_5 "," SRC ", 10           \n"  // src[5] = S_5;
    " slli   " LPX "," LPX ", 16           \n"  // LPX <<= 16
    " add    " BUF "," BUF "," LPX "       \n"  // buf += LPX
    " blti   " S_6 ",  0    , BLT_RETURN6  \n"

    "BLT_SECTION6:                         \n"
    " add    " LPX "," S_6 "," LUT "       \n"  // LPX = &lut[S_6]
    " l8ui   " LPX "," LPX ", 0            \n"  // LPX = *LPX
    " addmi  " VAL "," S_6 ", -32768       \n"  // VAL = S_6 - 32768
    " addmi  " S_6 "," S_6 ", 256          \n"  // S_6 += 256
    " moveqz " S_6 "," VAL "," LPX "       \n"  // if (LPX == 0) S_6 = VAL
    " s16i   " S_6 "," SRC ", 12           \n"  // src[6] = S_6;
    " slli   " LPX "," LPX ", 28           \n"  // LPX <<= 28
    " add    " BUF "," BUF "," LPX "       \n"  // buf += LPX
    " blti   " S_7 ",  0    , BLT_RETURN7  \n"

    "BLT_SECTION7:                         \n"
    " add    " LPX "," S_7 "," LUT "       \n"  // LPX = &lut[S_7]
    " l8ui   " LPX "," LPX ", 0            \n"  // LPX = *LPX
    " addmi  " VAL "," S_7 ", -32768       \n"  // VAL = S_7 - 32768
    " addmi  " S_7 "," S_7 ", 256          \n"  // S_7 += 256
    " moveqz " S_7 "," VAL "," LPX "       \n"  // if (LPX == 0) S_7 = VAL
    " s16i   " S_7 "," SRC ", 14           \n"  // src[7] = S_7;
    " slli   " LPX "," LPX ", 24           \n"  // LPX <<= 24
    " add    " BUF "," BUF "," LPX "       \n"  // buf += LPX
    " j                       BLT_RETURN7  \n"

    "BLT_END:                              \n"
  );

#undef DST
#undef SRC
#undef LUT
#undef VAL
#undef S_0
#undef S_1
#undef S_2
#undef S_3
#undef S_4
#undef S_5
#undef S_6
#undef S_7
#undef LPX
#undef BUF

}
#else
  __attribute((optimize("-O3")))
  static void blit_dmabuf(uint32_t* dst, uint16_t* src, const uint8_t* lut, size_t len)
  {
    while (len--)
    {
      uint32_t buf = 0;

      int16_t s_0 = src[0];
      int16_t s_1 = src[1];
      int16_t s_2 = src[2];
      int16_t s_3 = src[3];
      int16_t s_4 = src[4];
      int16_t s_5 = src[5];
      int16_t s_6 = src[6];
      int16_t s_7 = src[7];
      if (s_0 >= 0) {
        auto lpx = lut[s_0];
        s_0 += 256;
        if (lpx == 0) { s_0 -= 32768; }
        buf += lpx << 4;
        src[0] = s_0;
      }
      if (s_1 >= 0) {
        auto lpx = lut[s_1];
        s_1 += 256;
        if (lpx == 0) { s_1 -= 32768; }
        buf += lpx << 0;
        src[1] = s_1;
      }
      if (s_2 >= 0) {
        auto lpx = lut[s_2];
        s_2 += 256;
        if (lpx == 0) { s_2 -= 32768; }
        buf += lpx << 12;
        src[2] = s_2;
      }
      if (s_3 >= 0) {
        auto lpx = lut[s_3];
        s_3 += 256;
        if (lpx == 0) { s_3 -= 32768; }
        buf += lpx << 8;
        src[3] = s_3;
      }

      if (s_4 >= 0) {
        auto lpx = lut[s_4];
        s_4 += 256;
        if (lpx == 0) { s_4 -= 32768; }
        buf += lpx << 20;
        src[4] = s_4;
      }
      if (s_5 >= 0) {
        auto lpx = lut[s_5];
        s_5 += 256;
        if (lpx == 0) { s_5 -= 32768; }
        buf += lpx << 16;
        src[5] = s_5;
      }
      if (s_6 >= 0) {
        auto lpx = lut[s_6];
        s_6 += 256;
        if (lpx == 0) { s_6 -= 32768; }
        buf += lpx << 28;
        src[6] = s_6;
      }
      if (s_7 >= 0) {
        auto lpx = lut[s_7];
        s_7 += 256;
        if (lpx == 0) { s_7 -= 32768; }
        buf += lpx << 24;
        src[7] = s_7;
      }
      dst[0] = buf;
      src += 8;
      dst ++;
    }
  }

#endif

  void Panel_EPD::task_update(Panel_EPD* me)
  {
    update_data_t new_data;

    const size_t panel_w = me->_cfg.panel_width;
    const size_t memory_w = me->_cfg.memory_width;
    const size_t write_len = (memory_w / 4) + me->_config_detail.line_padding;

    const int magni_h = (me->_cfg.memory_height > me->_cfg.panel_height)
                      ? (me->_cfg.memory_height / me->_cfg.panel_height)
                      : 1;
    const size_t mh = (me->_cfg.memory_height + magni_h - 1) / magni_h;

    auto bus = me->getBusEPD();

    uint32_t remain = 0;

    for (;;) {
      vTaskDelay(1);
      me->_display_busy = (remain != 0);
      TickType_t wait_tick = remain ? 0 : portMAX_DELAY;
      if (xQueueReceive(me->_update_queue_handle, &new_data, wait_tick)) {
        me->_display_busy = true;
        uint32_t usec = lgfx::micros();
        bool refresh = ( remain == 0 );
        do {
// printf("\n new_data: x:%d y:%d w:%d h:%d \n", new_data.x, new_data.y, new_data.w, new_data.h);
          auto lut_remain = me->_lut_remain_table[new_data.mode];
          if (remain < lut_remain) {
            remain = lut_remain;
          }
          size_t panel_idx = ((new_data.x + new_data.y * panel_w) >> 1);
          size_t memory_idx = ((new_data.x + new_data.y * memory_w) >> 1);
          auto src = &me->_buf[panel_idx];
          auto dst = &me->_step_framebuf[memory_idx];
          size_t h = new_data.h;
          uint_fast16_t lut_offset = me->_lut_offset_table[new_data.mode] << 8;

          if (refresh && (new_data.mode != epd_mode_t::epd_fastest)) {
            do {
              size_t w = new_data.w >> 1;
              auto s = src;
              auto d = dst;
              src += panel_w >> 1;
              dst += memory_w >> 1;
              if (w & 1) {
                uint_fast16_t s0 = s[0];
                ++s;
                s0 += lut_offset;
                d[0] = s0;
                ++d;
                --w;
              }
              w >>= 1;
              for (int i = 0; i < w; ++i) {
                uint_fast16_t s0 = s[0];
                uint_fast16_t s1 = s[1];
                d[0] = s0 + lut_offset;
                d[1] = s1 + lut_offset;
                d += 2;
                s += 2;
              }
            } while (--h);
          } else {
            do {
              size_t w = new_data.w >> 1;
              auto s = src;
              auto d = dst;
              src += panel_w >> 1;
              dst += memory_w >> 1;
              if (w & 1) {
                auto d0 = *((uint8_t*)&d[0]);
                uint16_t s0 = s[0];
                ++s;
                if (d0 != s0) { d[0] = s0 + lut_offset; }
                ++d;
                --w;
              }
              w >>= 1;
              for (int i = 0; i < w; ++i) {
                auto d0 = *((uint8_t*)&d[0]);
                auto d1 = *((uint8_t*)&d[1]);
                uint16_t s0 = s[0];
                uint16_t s1 = s[1];
                s += 2;
                if (d0 != s0) { d[0] = s0 + lut_offset; }
                if (d1 != s1) { d[1] = s1 + lut_offset; }
                d += 2;
              }
            } while (--h);
          }
          if (lgfx::micros() - usec >= 2048) {
            break;
          }
        } while (xQueueReceive(me->_update_queue_handle, &new_data, 0));
      }

      bus->powerControl(true);
      int w = (memory_w + 15) >> 4;
      for (uint_fast16_t y = 0; y < mh; y++) {
        auto dma_buf = (uint32_t*)me->_dma_bufs[y & 1];
        blit_dmabuf(dma_buf, &me->_step_framebuf[y * memory_w >> 1], me->_lut_2pixel, w);
        for (int m = 0; m < magni_h; ++m) {
          if (y == 0 && m == 0) {
            bus->beginTransaction();
          } else {
            bus->scanlineDone();
          }
          bus->writeScanLine((const uint8_t*)dma_buf, write_len);
        }
      }

      bus->scanlineDone();
      bus->endTransaction();

      if (remain == 0) {
        bus->powerControl(false);
      } else {
        --remain;
      }
    }
  }

//----------------------------------------------------------------------------
 }
}

#endif
