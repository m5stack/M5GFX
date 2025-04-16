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

  // 0 == neutral / 1 == to black / 2 == to white
  static constexpr const uint8_t lut_quality[] = {
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
    0, 2, 2, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1,
    2, 2, 2, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1,
    2, 2, 2, 2, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1,
    2, 2, 2, 2, 2, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1,
    2, 2, 2, 2, 2, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1,
    0, 2, 2, 2, 2, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1,
    0, 2, 2, 2, 2, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1,
    1, 1, 1, 2, 2, 2, 2, 0, 2, 0, 0, 0, 2, 2, 2, 2,
    1, 1, 1, 1, 0, 2, 2, 0, 2, 0, 2, 0, 2, 2, 2, 2,
    1, 1, 1, 1, 0, 2, 2, 0, 2, 0, 2, 0, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 0, 2, 0, 1, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 0, 1, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 0, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 0, 2, 2, 2, 2, 0, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 0, 2, 2, 2, 2, 1, 2, 2, 2, 2,
    1, 1, 1, 0, 1, 1, 0, 2, 2, 2, 2, 1, 2, 2, 2, 2,
    1, 1, 0, 0, 0, 0, 1, 1, 0, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
    1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
    1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 2, 2, 2,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 2,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };

  static constexpr const uint8_t lut_text[] = {
    0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 2, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 2, 2, 2, 2, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1,
    1, 1, 1, 2, 2, 2, 2, 0, 2, 0, 0, 0, 2, 2, 2, 2,
    1, 1, 1, 1, 0, 2, 2, 0, 2, 0, 2, 0, 2, 2, 2, 2,
    1, 1, 1, 1, 0, 2, 2, 0, 2, 0, 2, 0, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 0, 2, 0, 1, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 0, 1, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 0, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 0, 2, 2, 2, 2, 0, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 0, 2, 2, 2, 2, 1, 2, 2, 2, 2,
    1, 1, 1, 0, 1, 1, 0, 2, 2, 2, 2, 1, 2, 2, 2, 2,
    1, 1, 0, 0, 0, 0, 1, 1, 0, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
    1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
    1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 2, 2, 2,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 2,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };

  static constexpr const uint8_t lut_fast[] = {
    2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 
    2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };
  
  static constexpr const uint8_t lut_fastest[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };
  
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
      cfg_detail.lut_quality_step = sizeof(lut_quality) >> 4;
    }
    if (cfg_detail.lut_text == nullptr) {
      cfg_detail.lut_text = lut_text;
      cfg_detail.lut_text_step = sizeof(lut_text) >> 4;
    }
    if (cfg_detail.lut_fast == nullptr) {
      cfg_detail.lut_fast = lut_fast;
      cfg_detail.lut_fast_step = sizeof(lut_fast) >> 4;
    }
    if (cfg_detail.lut_fastest == nullptr) {
      cfg_detail.lut_fastest = lut_fastest;
      cfg_detail.lut_fastest_step = sizeof(lut_fastest) >> 4;;
    }
    config_detail(cfg_detail);

    Panel_HasBuffer::init(use_reset);

    if (init_intenal()) {
      _buf = _frame_buffer;
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

    // 面積分のフレームバッファ (1Byte=2pixel)
    _frame_buffer = (uint8_t *)heap_caps_aligned_alloc(16, (memory_w * memory_h) / 2, MALLOC_CAP_SPIRAM); // current pixels
    _step_table = (uint16_t *)heap_caps_malloc(memory_w + 16, MALLOC_CAP_DMA);
    _dma_bufs[0] = (uint8_t *)heap_caps_malloc((memory_w / 4) + _config_detail.line_padding + 16, MALLOC_CAP_DMA);
    _dma_bufs[1] = (uint8_t *)heap_caps_malloc((memory_w / 4) + _config_detail.line_padding + 16, MALLOC_CAP_DMA);

    if (!_frame_buffer || !_step_table || !_dma_bufs[0] || !_dma_bufs[1] || !_lut_2pixel) {
      if (_frame_buffer) { heap_caps_free(_frame_buffer); _frame_buffer = nullptr; }
      if (_step_table) { heap_caps_free(_step_table); _step_table = nullptr; }
      if (_dma_bufs[0]) { heap_caps_free(_dma_bufs[0]); _dma_bufs[0] = nullptr; }
      if (_dma_bufs[1]) { heap_caps_free(_dma_bufs[1]); _dma_bufs[1] = nullptr; }
      if (_lut_2pixel) { heap_caps_free(_lut_2pixel); _lut_2pixel = nullptr; }

      return false;
    }

    auto dst = _lut_2pixel;
    memset(dst, 0x0F, 256);
    size_t lindex = 256;
    for (int epd_mode = 1; epd_mode < 5; ++epd_mode) {
      const uint8_t* lut_src = nullptr;
      size_t lut_step = 0;
      switch (epd_mode) {
        default: continue;
        case epd_mode_t::epd_quality: lut_src = _config_detail.lut_quality; lut_step = _config_detail.lut_quality_step; break;
        case epd_mode_t::epd_text:    lut_src = _config_detail.lut_text;    lut_step = _config_detail.lut_text_step;    break;
        case epd_mode_t::epd_fast:    lut_src = _config_detail.lut_fast;    lut_step = _config_detail.lut_fast_step;    break;
        case epd_mode_t::epd_fastest: lut_src = _config_detail.lut_fastest; lut_step = _config_detail.lut_fastest_step; break;
      }
      if (lut_src == nullptr) { continue; }
      _lut_step_offset[epd_mode] = lindex >> 8;
      _lut_mode_remain[epd_mode] = lut_step;

      for (int step = 0; step < lut_step; ++step) {
        for (int lv = 0; lv < 256; ++lv) {
          dst[lindex] = (lut_src[lv >> 4] << 2) + (lut_src[lv & 0xf]);
          ++lindex;
        }
        lut_src += 16;
      }
    }

    _update_queue_handle = xQueueCreate(16, sizeof(update_data_t));
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
  }

  void Panel_EPD::setInvert(bool invert)
  {
    // unimplemented
    _invert = invert;
  }

  void Panel_EPD::setSleep(bool flg)
  {
  }

  void Panel_EPD::setPowerSave(bool flg)
  {
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

    uint_fast16_t xs = (_range_mod.left + _cfg.offset_x) & ~ 3;
    uint_fast16_t xe = (_range_mod.right + _cfg.offset_x + 3) | 3;
    uint_fast16_t ys = _range_mod.top    + _cfg.offset_y;
    uint_fast16_t ye = _range_mod.bottom + _cfg.offset_y;


    update_data_t upd;

    upd.lut_offset = _lut_step_offset[_epd_mode];
    upd.remain = _lut_mode_remain[_epd_mode];
    upd.x = xs;
    upd.w = xe - xs + 1;
    upd.y = ys;
    upd.h = ye - ys + 1;

    cacheWriteBack(&_frame_buffer[y * _cfg.memory_width >> 1], h * _cfg.memory_width >> 1);
    bool res = xQueueSend(_update_queue_handle, &upd, 128 / portTICK_PERIOD_MS) == pdTRUE;
    vTaskDelay(1);
    if (res)
    {
      _range_mod.top    = INT16_MAX;
      _range_mod.left   = INT16_MAX;
      _range_mod.right  = 0;
      _range_mod.bottom = 0;
    }
  }

//----------------------------------------------------------------------------
  void Panel_EPD::task_update(Panel_EPD* me)
  {
    std::vector<update_data_t> _update_data;
    update_data_t new_data;

    const size_t data_len = me->_cfg.memory_width;
    const size_t write_len = (data_len / 4) + me->_config_detail.line_padding;
    const size_t mh = me->_cfg.memory_height;

    auto bus = me->getBusEPD();

    uint16_t* step_buf = me->_step_table;
    for (;;) {
      vTaskDelay(1);
      if (_update_data.empty())
      { // 処理対象が無い場合は EPDの電力停止
        // 全ピクセル ニュートラル化して電源を切る
        uint8_t *dma_buf = me->_dma_bufs[0];
        memset(dma_buf, 0, write_len);
        bus->powerControl(true);
        bus->beginTransaction();
        for (uint_fast16_t y = 0; y < mh; y++) {
          bus->writeBytes(dma_buf, write_len);
          bus->scanlineDone();
        }
        bus->endTransaction();
        bus->powerControl(false);
        me->_display_busy = false;
      }

      if (_update_data.size() < 64)
      {
        TickType_t wait_tick = _update_data.empty() ? portMAX_DELAY : 0;
        if (xQueueReceive(me->_update_queue_handle, &new_data, wait_tick)) {
          me->_display_busy = true;
          do {
  // printf("\n new_data: x:%d y:%d w:%d h:%d \n", new_data.x, new_data.y, new_data.w, new_data.h);
            _update_data.push_back(new_data);
          } while (xQueueReceive(me->_update_queue_handle, &new_data, 0));
          // remainが小さい順にソート
          std::stable_sort(_update_data.begin(), _update_data.end(), [](const update_data_t& a, const update_data_t& b) {
            return a.remain < b.remain;
          });
        }
      }
      if (_update_data.empty()) { continue; }

      bus->powerControl(true);
      for (uint_fast16_t y = 0; y < mh; y++) {
        memset(step_buf, 0, data_len);
        bool hit = false;
        int x = UINT16_MAX;
        int x2 = 0;
        for (const auto& data: _update_data) {
          if (y < data.y || y >= data.y + data.h) { continue; }
          hit = true;
          int tx = data.x >> 1;
          int tw = (data.w + 1) >> 1;
          if (x > tx) { x = tx; }
          if (x2 < tx + tw) { x2 = tx + tw; }
          uint16_t value = data.lut_offset << 8;
          while (tw--) {
            step_buf[tx++] = value;
          }
        }
        uint8_t *dma_buf = me->_dma_bufs[y & 1];
        memset(dma_buf, 0xFF, write_len);
        if (hit) {
          auto lut = me->_lut_2pixel;
          auto sb = step_buf;
          auto dst = dma_buf;
          x >>= 1;
          size_t w = ((x2 + 1) >> 1) - x;
          uint8_t* fb = &me->_frame_buffer[y * data_len / 2];
          fb += x << 1;
          sb += x << 1;
          dst += x;
          while (w--) {
            auto fb0 = fb[0];
            auto sb0 = sb[0];
            auto fb1 = fb[1];
            auto sb1 = sb[1];
            fb0 = lut[fb0 + sb0];
            fb1 = lut[fb1 + sb1];
            sb += 2;
            fb += 2;
            dst[0] = (fb0 << 4) + fb1;
            dst++;
          }
        }
        if (y == 0) {
          bus->beginTransaction();
        } else {
          bus->scanlineDone();
        }
        bus->writeBytes(dma_buf, write_len);
      }

      // _update_dataの処理を進め、remainが 0になったものを除去する
      for (auto it = _update_data.begin(); it != _update_data.end();) {
        if (it->remain != 0) {
          it->remain--;
          if (it->remain != 0) {
            it->lut_offset++;
            ++it;
            continue;
          }
        }
        it = _update_data.erase(it);
      }
      bus->scanlineDone();
      bus->endTransaction();
    }
  }

//----------------------------------------------------------------------------
 }
}

#endif
