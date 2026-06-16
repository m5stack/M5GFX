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
#include "Panel_SSD1677.hpp"
#include "lgfx/v1/Bus.hpp"
#include "lgfx/v1/platforms/common.hpp"
#include "lgfx/v1/misc/pixelcopy.hpp"
#include "lgfx/v1/misc/colortype.hpp"

#include <string.h>

#ifdef min
#undef min
#endif

namespace lgfx
{
 inline namespace v1
 {
//----------------------------------------------------------------------------

  static constexpr int8_t Bayer[16] = { -30, 18, -22, 26, -14, 2, -6, 10, -18, 30, -26, 22, -2, 14, -10, 6 };
  // static constexpr int8_t Bayer[16] = { 0, };

  // SSD1677 commands
  static constexpr uint8_t CMD_DEEP_SLEEP        = 0x10;
  static constexpr uint8_t CMD_DATA_ENTRY        = 0x11;
  static constexpr uint8_t CMD_MASTER_ACTIVATION = 0x20;
  static constexpr uint8_t CMD_DISP_UPDATE_CTRL1 = 0x21;
  static constexpr uint8_t CMD_DISP_UPDATE_CTRL2 = 0x22;
  static constexpr uint8_t CMD_WRITE_RAM_BW      = 0x24; // current frame / LSB plane
  static constexpr uint8_t CMD_WRITE_RAM_RED     = 0x26; // previous frame / MSB plane
  static constexpr uint8_t CMD_WRITE_TEMP        = 0x1A;
  static constexpr uint8_t CMD_WRITE_LUT         = 0x32;
  static constexpr uint8_t CMD_GATE_VOLT         = 0x03; // VGH
  static constexpr uint8_t CMD_SOURCE_VOLT       = 0x04; // VSH1, VSH2, VSL
  static constexpr uint8_t CMD_WRITE_VCOM        = 0x2C;
  static constexpr uint8_t CMD_SET_RAM_X         = 0x44;
  static constexpr uint8_t CMD_SET_RAM_Y         = 0x45;
  static constexpr uint8_t CMD_SET_RAM_X_CNT     = 0x4E;
  static constexpr uint8_t CMD_SET_RAM_Y_CNT     = 0x4F;

  static constexpr uint8_t CTRL1_NORMAL     = 0x00;
  static constexpr uint8_t CTRL1_BYPASS_RED = 0x40;

  //--------------------------------------------------------------------------
  // LUTs (ported from community-sdk EInkDisplay, non-X3 / GDEQ0426T82).
  // Layout: VS patterns (5 groups x 10 bytes) + TP/RP timing (10 groups x 5 bytes)
  //         + frame rate (5 bytes) = 105 bytes -> command 0x32.
  // Then voltages [VGH, VSH1, VSH2, VSL, VCOM] (bytes 105..109) -> 0x03/0x04/0x2C.
  //--------------------------------------------------------------------------

  static constexpr uint8_t lut_fastest[110] = {
    0x00, 0x4A, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT0: 00 black
    0x80, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT1: 01 dark
    0x88, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT2: 10 light
    0xA8, 0x44, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT3: 11 white
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // VCOM
    0x09, 0x0C, 0x03, 0x03, 0x00,
    0x0F, 0x03, 0x07, 0x03, 0x00,
    0x03, 0x00, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x8F, 0x8F, 0x8F, 0x8F, 0x8F,       // frame rate
    0x17, 0x41, 0xA8, 0x32, 0x30,       // VGH, VSH1, VSH2, VSL, VCOM
  };

  // Factory absolute LUTs. 2-bit pixel encoding: BW=bit0(LSB), RED=bit1(MSB).
  //   00=black, 01=dark, 10=light, 11=white. (i.e. value v(0..3) = (MSB<<1)|LSB.)
  static constexpr uint8_t lut_factory_fast[110] = {
    0x00, 0x4A, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT0: 00 black
    0x80, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT1: 01 dark
    0x88, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT2: 10 light
    0xA8, 0x44, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT3: 11 white
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // VCOM
    0x09, 0x0C, 0x03, 0x03, 0x00,
    0x0F, 0x03, 0x07, 0x03, 0x00,
    0x03, 0x00, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x44, 0x44, 0x44, 0x44, 0x44,       // frame rate (faster clock)
    0x17, 0x41, 0xA8, 0x32, 0x50,       // VGH, VSH1, VSH2, VSL, VCOM(-2.0V)
  };

  static constexpr uint8_t lut_factory_quality[110] = {
    0x00, 0x4A, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT0: 00 black
    0x80, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT1: 01 dark
    0x88, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT2: 10 light
    0xA8, 0x44, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT3: 11 white
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // VCOM
    0x08, 0x0B, 0x02, 0x03, 0x00,
    0x0C, 0x02, 0x07, 0x02, 0x00,
    0x01, 0x00, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01,
    0x22, 0x22, 0x22, 0x22, 0x22,       // frame rate (slower clock)
    0x17, 0x41, 0xA8, 0x32, 0x30,       // VGH, VSH1, VSH2, VSL, VCOM(-1.2V)
  };

  // Write a 110-byte LUT: 105 waveform bytes -> 0x32, voltages -> 0x03/0x04/0x2C.
  static void send_lut(IBus* bus, const uint8_t* lut)
  {
    bus->writeCommand(CMD_WRITE_LUT, 8);
    bus->writeBytes(lut, 105, true, false);
    bus->writeCommand(CMD_GATE_VOLT, 8);
    bus->writeData(lut[105], 8);                 // VGH
    bus->writeCommand(CMD_SOURCE_VOLT, 8);
    bus->writeData(lut[106], 8);                 // VSH1
    bus->writeData(lut[107], 8);                 // VSH2
    bus->writeData(lut[108], 8);                 // VSL
    bus->writeCommand(CMD_WRITE_VCOM, 8);
    bus->writeData(lut[109], 8);                 // VCOM
  }

  //--------------------------------------------------------------------------

  Panel_SSD1677::Panel_SSD1677(void)
  {
    _cfg.dummy_read_bits = 0;
    _epd_mode = epd_mode_t::epd_quality;
  }

  Panel_SSD1677::~Panel_SSD1677(void)
  {
    if (_prev_buf) { heap_free(_prev_buf); _prev_buf = nullptr; }
  }

  color_depth_t Panel_SSD1677::setColorDepth(color_depth_t depth)
  {
    (void)depth;
    _write_depth = color_depth_t::grayscale_8bit;
    _read_depth = color_depth_t::grayscale_8bit;
    return color_depth_t::grayscale_8bit;
  }

  uint32_t Panel_SSD1677::_get_plane_length(void) const
  {
    // EPD native: row indexed by Y(gate), packed along X(source).
    // row_bytes = (panel_width+7)/8 ; rows = panel_height.
    return (((_cfg.panel_width + 7) & ~7) >> 3) * _cfg.panel_height;
  }

  size_t Panel_SSD1677::_get_buffer_length(void) const
  {
    return _get_plane_length() * 2; // planeL + planeM
  }

  bool Panel_SSD1677::init(bool use_reset)
  {
    pinMode(_cfg.pin_busy, pin_mode_t::input_pullup);

    if (!Panel_HasBuffer::init(use_reset))
    {
      return false;
    }
    _buf_x1_len = _get_plane_length();
    memset(_buf, 0xFF, _get_buffer_length()); // white
    _after_wake();
    return true;
  }

  void Panel_SSD1677::_after_wake(void)
  {
    startWrite(true);
    for (uint8_t i = 0; auto cmds = getInitCommands(i); i++)
    {
      _wait_busy();
      command_list(cmds);
    }

    // Full-screen RAM window + clear both RAM banks to white.
    _set_ram_area(0, 0, _cfg.panel_width, _cfg.panel_height);
    _wait_busy();
    _bus->writeCommand(0x46, 8); _bus->writeData(0xF7, 8); // auto write BW RAM
    _wait_busy();
    _bus->writeCommand(0x47, 8); _bus->writeData(0xF7, 8); // auto write RED RAM
    _wait_busy();

    _screen_on = false;
    _last_epd_mode = (epd_mode_t)~0u;
    _initialize_seq = true;

    setRotation(_rotation);

    _range_old.top = 0;
    _range_old.left = 0;
    _range_old.right = _cfg.panel_width - 1;
    _range_old.bottom = _cfg.panel_height - 1;
    _range_mod.top = INT16_MAX;
    _range_mod.left = INT16_MAX;
    _range_mod.right = 0;
    _range_mod.bottom = 0;

    endWrite();
  }

  void Panel_SSD1677::_power_on(void)
  {
    if (_screen_on) { return; }
    _bus->writeCommand(CMD_DISP_UPDATE_CTRL2, 8);
    _bus->writeData(0xC0, 8);
    _bus->writeCommand(CMD_MASTER_ACTIVATION, 8);
    _send_msec = millis();
    _wait_busy();
    _screen_on = true;
  }

  void Panel_SSD1677::waitDisplay(void)
  {
    _wait_busy();
  }

  bool Panel_SSD1677::displayBusy(void)
  {
    return _cfg.pin_busy >= 0 && gpio_in(_cfg.pin_busy);
  }

  void Panel_SSD1677::_set_ram_area(int32_t x, int32_t y, int32_t w, int32_t h)
  {
    // Native coords: X = source (0..799), Y = gate (0..479).
    // Gates are reversed on this panel -> reverse Y and use Y-decrement.
    int32_t yrev = _cfg.panel_height - y - h;

    _bus->writeCommand(CMD_DATA_ENTRY, 8);
    _bus->writeData(0x01, 8); // X increment, Y decrement

    _bus->writeCommand(CMD_SET_RAM_X, 8);
    _bus->writeData(x & 0xFF, 8);
    _bus->writeData((x >> 8) & 0xFF, 8);
    _bus->writeData((x + w - 1) & 0xFF, 8);
    _bus->writeData(((x + w - 1) >> 8) & 0xFF, 8);

    _bus->writeCommand(CMD_SET_RAM_Y, 8);
    _bus->writeData((yrev + h - 1) & 0xFF, 8);
    _bus->writeData(((yrev + h - 1) >> 8) & 0xFF, 8);
    _bus->writeData(yrev & 0xFF, 8);
    _bus->writeData((yrev >> 8) & 0xFF, 8);

    _bus->writeCommand(CMD_SET_RAM_X_CNT, 8);
    _bus->writeData(x & 0xFF, 8);
    _bus->writeData((x >> 8) & 0xFF, 8);

    _bus->writeCommand(CMD_SET_RAM_Y_CNT, 8);
    _bus->writeData((yrev + h - 1) & 0xFF, 8);
    _bus->writeData(((yrev + h - 1) >> 8) & 0xFF, 8);
  }

  void Panel_SSD1677::_send_plane(uint32_t cmd, const uint8_t* plane, const range_rect_t& range, bool extra_invert)
  {
    int32_t xs = range.left & ~7;
    int32_t xe = range.right | 7;
    if (xe >= (int32_t)_cfg.panel_width) { xe = _cfg.panel_width - 1; }
    int32_t ys = range.top;
    int32_t ye = range.bottom;
    if (ye >= (int32_t)_cfg.panel_height) { ye = _cfg.panel_height - 1; }

    _set_ram_area(xs, ys, xe - xs + 1, ye - ys + 1);
    _wait_busy();
    _bus->writeCommand(cmd, 8);

    int32_t row_bytes = ((_cfg.panel_width + 7) & ~7) >> 3;
    int32_t xbytes = (xe - xs + 1) >> 3;
    int32_t rows = ye - ys + 1;
    const uint8_t* b = &plane[ys * row_bytes + (xs >> 3)];

    bool inv = extra_invert ^ _invert ^ _cfg.invert;
    if (inv)
    {
      uint8_t tmp[128];
      for (int32_t row = 0; row < rows; row++)
      {
        for (int32_t i = 0; i < xbytes; i++) { tmp[i] = ~b[i]; }
        _bus->writeBytes(tmp, xbytes, true, false);
        b += row_bytes;
      }
    }
    else if (xbytes == row_bytes)
    {
      _bus->writeBytes(b, xbytes * rows, true, true);
    }
    else
    {
      for (int32_t row = 0; row < rows; row++)
      {
        _bus->writeBytes(b, xbytes, true, true);
        b += row_bytes;
      }
    }
  }

  void Panel_SSD1677::display(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h)
  {
    if (0 < w && 0 < h)
    {
      uint_fast16_t xs = x, ys = y, xe = x + w - 1, ye = y + h - 1;
      _rotate_pos(xs, ys, xe, ye);
      _range_mod.left   = std::min<int32_t>(_range_mod.left  , std::min(xs, xe));
      _range_mod.right  = std::max<int32_t>(_range_mod.right , std::max(xs, xe));
      _range_mod.top    = std::min<int32_t>(_range_mod.top   , std::min(ys, ye));
      _range_mod.bottom = std::max<int32_t>(_range_mod.bottom, std::max(ys, ye));
    }
    if (_range_mod.empty()) { return; }

    auto mode = getEpdMode();
    bool is_full = _initialize_seq || (_last_epd_mode != mode)
                || (mode == epd_mode_t::epd_quality) || (mode == epd_mode_t::epd_text);

    if (is_full)
    {
      _range_mod.left = 0;
      _range_mod.top = 0;
      _range_mod.right = _cfg.panel_width - 1;
      _range_mod.bottom = _cfg.panel_height - 1;
    }

    // For fast diff, include the previously-changed region too.
    range_rect_t tr = _range_mod;
    if (tr.top > _range_old.top) { tr.top = _range_old.top; }
    if (tr.left > _range_old.left) { tr.left = _range_old.left; }
    if (tr.right < _range_old.right) { tr.right = _range_old.right; }
    if (tr.bottom < _range_old.bottom) { tr.bottom = _range_old.bottom; }
    _range_old = _range_mod;

    startWrite();

    // Base panel renders B/W: use planeM (MSB) as the monochrome bit (white if v>=2).
    const uint8_t* img = &_buf[_buf_x1_len];

    _send_plane(CMD_WRITE_RAM_BW, img, tr);
    if (is_full)
    {
      _send_plane(CMD_WRITE_RAM_RED, img, tr); // full/half: same image to both
    }

    // Refresh sequence (community-sdk refreshDisplay, B/W).
    _wait_busy();
    _bus->writeCommand(CMD_DISP_UPDATE_CTRL1, 8);
    _bus->writeData(is_full ? CTRL1_BYPASS_RED : CTRL1_NORMAL, 8);

    uint8_t dm = 0;
    if (!_screen_on) { _screen_on = true; dm |= 0xC0; }
    if (mode == epd_mode_t::epd_quality) { dm |= 0x34; }       // FULL
    else if (mode == epd_mode_t::epd_text)                     // HALF
    {
      _bus->writeCommand(CMD_WRITE_TEMP, 8);
      _bus->writeData(0x5A, 8);
      dm |= 0xD4;
    }
    else { dm |= 0x1C; }                                        // FAST (built-in LUT)

    _bus->writeCommand(CMD_DISP_UPDATE_CTRL2, 8);
    _bus->writeData(dm, 8);
    _bus->writeCommand(CMD_MASTER_ACTIVATION, 8);
    _send_msec = millis();
    _wait_busy();

    if (!is_full)
    {
      // Sync RED RAM with current frame so it serves as "previous" next time.
      _send_plane(CMD_WRITE_RAM_RED, img, tr);
    }

    _initialize_seq = false;
    _last_epd_mode = mode;
    _range_mod.top = INT16_MAX;
    _range_mod.left = INT16_MAX;
    _range_mod.right = 0;
    _range_mod.bottom = 0;

    endWrite();
  }

  void Panel_SSD1677::setInvert(bool invert)
  {
    _invert = invert;
    _range_mod.top = 0;
    _range_mod.left = 0;
    _range_mod.right = _cfg.panel_width - 1;
    _range_mod.bottom = _cfg.panel_height - 1;
  }

  void Panel_SSD1677::setSleep(bool flg)
  {
    if (flg)
    {
      startWrite();
      _wait_busy();
      _bus->writeCommand(CMD_DISP_UPDATE_CTRL2, 8);
      _bus->writeData(0x03, 8); // analog off + clock off
      _bus->writeCommand(CMD_MASTER_ACTIVATION, 8);
      _wait_busy();
      _bus->writeCommand(CMD_DEEP_SLEEP, 8);
      _bus->writeData(0x01, 8);
      _screen_on = false;
      endWrite();
    }
    else
    {
      // Deep-sleep wake requires a hardware reset (RST may be external -> rst_control).
      rst_control(false);
      delay(10);
      rst_control(true);
      delay(10);
      _after_wake();
    }
  }

  void Panel_SSD1677::setPowerSave(bool flg)
  {
    startWrite();
    _wait_busy();
    _bus->writeCommand(CMD_DISP_UPDATE_CTRL2, 8);
    _bus->writeData(flg ? 0x03 : 0xC0, 8);
    _bus->writeCommand(CMD_MASTER_ACTIVATION, 8);
    _wait_busy();
    _screen_on = !flg;
    endWrite();
  }

  void Panel_SSD1677::writeFillRectPreclipped(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, uint32_t rawcolor)
  {
    uint_fast16_t xs = x, xe = x + w - 1;
    uint_fast16_t ys = y, ye = y + h - 1;
    _xs = xs; _ys = ys; _xe = xe; _ye = ye;
    _update_transferred_rect(xs, ys, xe, ye);

    int32_t value = rawcolor;
    int32_t row_bytes = ((_cfg.panel_width + 7) & ~7) >> 3;

    y = ys;
    do
    {
      x = xs;
      auto btbl = &Bayer[(y & 3) << 2];
      do
      {
        uint32_t byte_idx = y * row_bytes + (x >> 3);
        uint8_t bit_mask = 0x80 >> (x & 7);
        int_fast8_t v = (value + btbl[x & 3]) >> 6;
        v = (v < 0) ? 0 : (v > 3 ? 3 : v);
        if (v & 1) _buf[byte_idx] |=  bit_mask; else _buf[byte_idx] &= ~bit_mask;
        if (v & 2) _buf[byte_idx + _buf_x1_len] |=  bit_mask; else _buf[byte_idx + _buf_x1_len] &= ~bit_mask;
      } while (++x <= xe);
    } while (++y <= ye);
  }

  void Panel_SSD1677::writeImage(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, pixelcopy_t* param, bool use_dma)
  {
    (void)use_dma;
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
          do
          {
            auto color = readbuf[prev_pos];
            _draw_pixel(x + prev_pos, y, color.raw);
          } while (new_pos != ++prev_pos);
        }
      } while (w != new_pos && w != (prev_pos = param->fp_skip(new_pos, w, param)));
      param->src_x32 = sx;
      param->src_y++;
    } while (++y < h);
  }

  void Panel_SSD1677::writePixels(pixelcopy_t* param, uint32_t length, bool use_dma)
  {
    (void)use_dma;
    {
      uint_fast16_t xs = _xs, xe = _xe, ys = _ys, ye = _ye;
      _update_transferred_rect(xs, ys, xe, ye);
    }
    uint_fast16_t xs = _xs, ys = _ys, xe = _xe, ye = _ye;
    uint_fast16_t xpos = _xpos, ypos = _ypos;

    static constexpr uint32_t buflen = 16;
    grayscale_t colors[buflen];
    int bufpos = buflen;
    do
    {
      if (bufpos == (int)buflen) {
        param->fp_copy(colors, 0, std::min(length, buflen), param);
        bufpos = 0;
      }
      auto color = colors[bufpos++];
      _draw_pixel(xpos, ypos, color.raw);
      if (++xpos > xe)
      {
        xpos = xs;
        if (++ypos > ye) { ypos = ys; }
      }
    } while (--length);
    _xpos = xpos;
    _ypos = ypos;
  }

  void Panel_SSD1677::readRect(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, void* dst, pixelcopy_t* param)
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
        readbuf[idx] = _read_pixel(x + idx, y) * 0x55;
      } while (++idx != w);
      param->src_x32 = 0;
      readpos = param->fp_copy(dst, readpos, readpos + w, param);
    } while (++y < h);
  }

  bool Panel_SSD1677::_wait_busy(uint32_t timeout)
  {
    _bus->wait();
    if (_cfg.pin_busy >= 0 && gpio_in(_cfg.pin_busy))
    {
      uint32_t start_time = millis();
      uint32_t delay_msec = _refresh_msec - (start_time - _send_msec);
      if (delay_msec && delay_msec < timeout) { delay(delay_msec); }
      do
      {
        if (millis() - start_time > timeout) { return false; }
        delay(1);
      } while (gpio_in(_cfg.pin_busy));
    }
    return true;
  }

  void Panel_SSD1677::_draw_pixel(uint_fast16_t x, uint_fast16_t y, uint32_t value)
  {
    _rotate_pos(x, y);
    int32_t row_bytes = ((_cfg.panel_width + 7) & ~7) >> 3;
    uint32_t byte_idx = y * row_bytes + (x >> 3);
    uint8_t bit_mask = 0x80 >> (x & 7);
    int_fast8_t v = ((int32_t)value + (Bayer[(x & 3) + ((y & 3) << 2)])) >> 6;
    v = (v < 0) ? 0 : (v > 3 ? 3 : v);
    if (v & 1) _buf[byte_idx] |=  bit_mask; else _buf[byte_idx] &= ~bit_mask;
    if (v & 2) _buf[byte_idx + _buf_x1_len] |=  bit_mask; else _buf[byte_idx + _buf_x1_len] &= ~bit_mask;
  }

  uint8_t Panel_SSD1677::_read_pixel(uint_fast16_t x, uint_fast16_t y)
  {
    _rotate_pos(x, y);
    int32_t row_bytes = ((_cfg.panel_width + 7) & ~7) >> 3;
    uint32_t byte_idx = y * row_bytes + (x >> 3);
    uint8_t bit_mask = 0x80 >> (x & 7);
    uint_fast8_t result = (_buf[byte_idx] & bit_mask) ? 1 : 0;
    result += (_buf[byte_idx + _buf_x1_len] & bit_mask) ? 2 : 0;
    return result;
  }

  void Panel_SSD1677::_update_transferred_rect(uint_fast16_t &xs, uint_fast16_t &ys, uint_fast16_t &xe, uint_fast16_t &ye)
  {
    _rotate_pos(xs, ys, xe, ye);

    // X (source) direction is byte-packed -> align to 8.
    int32_t x1 = xs & ~7;
    int32_t x2 = (xe & ~7) + 7;
    if (x2 >= (int32_t)_cfg.panel_width) { x2 = _cfg.panel_width - 1; }
    if (ye >= _cfg.panel_height) { ye = _cfg.panel_height - 1; }

    _range_mod.left   = std::min<int32_t>(x1, _range_mod.left);
    _range_mod.right  = std::max<int32_t>(x2, _range_mod.right);
    _range_mod.top    = std::min<int32_t>(ys, _range_mod.top);
    _range_mod.bottom = std::max<int32_t>(ye, _range_mod.bottom);
  }

  //==========================================================================
  // Panel_SSD1677_4Gray
  //==========================================================================

  bool Panel_SSD1677_4Gray::_ensure_prev_buf(void)
  {
    if (_prev_buf) { return true; }
    size_t len = _get_plane_length() * 2;
    _prev_buf = (uint8_t*)heap_alloc_psram(len);
    if (!_prev_buf) { _prev_buf = (uint8_t*)heap_alloc(len); }
    if (!_prev_buf) { return false; }
    memset(_prev_buf, 0xFF, len); // white baseline
    _prev_valid = false;
    return true;
  }

  void Panel_SSD1677_4Gray::_store_prev(void)
  {
    if (!_prev_buf) { return; }
    memcpy(_prev_buf, _buf, _get_plane_length() * 2);
    _prev_valid = true;
  }

  void Panel_SSD1677_4Gray::display(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h)
  {
    if (0 < w && 0 < h)
    {
      uint_fast16_t xs = x, ys = y, xe = x + w - 1, ye = y + h - 1;
      _rotate_pos(xs, ys, xe, ye);
      _range_mod.left   = std::min<int32_t>(_range_mod.left  , std::min(xs, xe));
      _range_mod.right  = std::max<int32_t>(_range_mod.right , std::max(xs, xe));
      _range_mod.top    = std::min<int32_t>(_range_mod.top   , std::min(ys, ye));
      _range_mod.bottom = std::max<int32_t>(_range_mod.bottom, std::max(ys, ye));
    }
    if (_range_mod.empty()) { return; }

    auto mode = getEpdMode();

    // 4-gray always refreshes the full screen.
    range_rect_t full;
    full.left = 0; full.top = 0;
    full.right = _cfg.panel_width - 1; full.bottom = _cfg.panel_height - 1;

    const uint8_t* planeL = _buf;
    const uint8_t* planeM = &_buf[_buf_x1_len];

    startWrite();

    // Pick RAM source planes + LUT per mode. All use the factory absolute path
    // (Display Mode 1, self-contained 0xC7).
    //   quality / text : 4-level, factory_quality (cleanest, slow)
    //   fast           : 4-level, factory_fast
    //   fastest        : 1-bit B/W. lut_fastest (differential) cannot drive
    //                    full white<->black transitions, so fastest binarizes:
    //                    feed planeM (v>=2 -> white) to BOTH RAMs, making the
    //                    2-bit value 00 (black) or 11 (white) only.
    const uint8_t* bw_src = planeL;  // 4-level: LSB -> BW, MSB -> RED
    const uint8_t* red_src = planeM;
    const uint8_t* lut = lut_factory_quality;
    if (mode == epd_mode_t::epd_fastest)
    {
      lut = lut_fastest;
    } else
    if (mode == epd_mode_t::epd_fast)
    {
      lut = lut_factory_fast;
    }

    // The factory LUT groups are reversed in brightness on this panel
    // (HW group 00=white .. 11=black, opposite of the datasheet comment), so
    // send the complement of both planes to get a correctly-oriented image.
    _send_plane(CMD_WRITE_RAM_BW,  bw_src,  full, true);
    _send_plane(CMD_WRITE_RAM_RED, red_src, full, true);

    send_lut(_bus, lut);
    _wait_busy();
    _bus->writeCommand(CMD_DISP_UPDATE_CTRL1, 8);
    _bus->writeData(CTRL1_NORMAL, 8);
    _bus->writeCommand(CMD_DISP_UPDATE_CTRL2, 8);
    _bus->writeData(0xC7, 8); // Mode 1, self-contained power cycle (powers down after)
    _bus->writeCommand(CMD_MASTER_ACTIVATION, 8);
    _send_msec = millis();
    _wait_busy();
    _screen_on = false;

    _initialize_seq = false;
    _last_epd_mode = mode;
    _range_mod.top = INT16_MAX;
    _range_mod.left = INT16_MAX;
    _range_mod.right = 0;
    _range_mod.bottom = 0;

    endWrite();
  }

//----------------------------------------------------------------------------
 }
}
