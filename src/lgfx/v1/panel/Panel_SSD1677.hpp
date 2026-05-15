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
#pragma once

#include "lgfx/v1/panel/Panel_HasBuffer.hpp"
#include "lgfx/v1/misc/range.hpp"

namespace lgfx
{
 inline namespace v1
 {
//----------------------------------------------------------------------------

  struct Panel_SSD1677 : public Panel_HasBuffer
  {
    Panel_SSD1677(void);

    bool init(bool use_reset) override;

    color_depth_t setColorDepth(color_depth_t depth) override;

    void setInvert(bool invert) override;
    void setSleep(bool flg) override;
    void setPowerSave(bool flg) override;

    void waitDisplay(void) override;
    bool displayBusy(void) override;
    void display(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h) override;

    void writeFillRectPreclipped(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, uint32_t rawcolor) override;
    void writeImage(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, pixelcopy_t* param, bool use_dma) override;
    void writePixels(pixelcopy_t* param, uint32_t len, bool use_dma) override;

    uint32_t readCommand(uint_fast16_t, uint_fast8_t, uint_fast8_t) override { return 0; }
    uint32_t readData(uint_fast8_t, uint_fast8_t) override { return 0; }

    void readRect(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, void* dst, pixelcopy_t* param) override;

  private:

    static constexpr unsigned long _refresh_msec = 500;  // Longer for 480x800 display

    range_rect_t _range_old;
    unsigned long _send_msec = 0;
    epd_mode_t _last_epd_mode;
    uint32_t _buf_x1_len;
    bool _initialize_seq;
    bool _need_flip_draw;
    bool _epd_frame_switching = false;
    bool _epd_frame_back = false;

    size_t _get_buffer_length(void) const override;

    bool _wait_busy(uint32_t timeout = 4096);
    void _draw_pixel(uint_fast16_t x, uint_fast16_t y, uint32_t value);
    uint8_t _read_pixel(uint_fast16_t x, uint_fast16_t y);
    void _update_transferred_rect(uint_fast16_t &xs, uint_fast16_t &ys, uint_fast16_t &xe, uint_fast16_t &ye);
    void _exec_transfer(uint32_t cmd, const uint8_t* data, const range_rect_t& range, bool invert = false);
    void _after_wake(void);

    const uint8_t* getInitCommands(uint8_t listno) const override
    {
      // SSD1677 initialization sequence
      // LovyanGFX: panel_width=480, panel_height=800 (portrait)
      // EPD RAM: X=800 (LovyanGFX Y), Y=480 (LovyanGFX X)
      // 0x01 Driver output: 480 gates (EPD physical width)
      // 0x44 RAM X: 0 to 799 (maps to LovyanGFX Y)
      // 0x45 RAM Y: 0 to 479 (maps to LovyanGFX X)
      static constexpr uint8_t list0[] = {
          0x12, 0 + CMD_INIT_DELAY, 10,   // SW Reset + 10 msec delay
          0x18, 1, 0x80,                  // Read built-in temperature sensor
          0x0C, 5, 0xAE, 0xC7, 0xC3, 0xC0, 0x80,  // Booster soft start
          0x01, 3, (480-1) & 0xFF, ((480-1) >> 8) & 0xFF, 0x02,  // Driver output control: 480 gates
          0x3C, 1, 0x01,                  // BorderWaveform
          0x11, 1, 0x03,                  // Data entry mode: X+, Y+
          0x44, 4, 0x00, 0x00, (800-1) & 0xFF, ((800-1) >> 8) & 0xFF,  // RAM X: 0 to 799
          0x45, 4, 0x00, 0x00, (480-1) & 0xFF, ((480-1) >> 8) & 0xFF,  // RAM Y: 0 to 479
          0x4E, 2, 0x00, 0x00,            // RAM X address count = 0
          0x4F, 2, 0x00, 0x00,            // RAM Y address count = 0
          0xFF, 0xFF, // end
      };
      switch (listno) {
      case 0: return list0;
      default: return nullptr;
      }
    }
  };

//----------------------------------------------------------------------------
 }
}
