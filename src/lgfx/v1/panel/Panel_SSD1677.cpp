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

#ifdef min
#undef min
#endif

namespace lgfx
{
 inline namespace v1
 {
//----------------------------------------------------------------------------

  static constexpr uint8_t Bayer[16] = { 8, 200, 40, 232, 72, 136, 104, 168, 56, 248, 24, 216, 120, 184, 88, 152 };

  static constexpr uint8_t CMD_DEEP_SLEEP_MODE   = 0x10; // スリープの設定。スリープからの復帰にはハードウェアリセットが必要
  static constexpr uint8_t CMD_MASTER_ACTIVATION = 0x20; // 画面の描画更新を実施する
  static constexpr uint8_t CMD_DISPLAY_UPDATE_CONTROL_1 = 0x21;
  static constexpr uint8_t CMD_DISPLAY_UPDATE_CONTROL_2 = 0x22;
  static constexpr uint8_t CMD_WRITE_RAM_BW  = 0x24;
  static constexpr uint8_t CMD_WRITE_RAM_RED = 0x26;


  Panel_SSD1677::Panel_SSD1677(void)
  {
    _cfg.dummy_read_bits = 0;
    _epd_mode = epd_mode_t::epd_quality;
  }

  color_depth_t Panel_SSD1677::setColorDepth(color_depth_t depth)
  {
    (void)depth;
    _write_depth = color_depth_t::grayscale_8bit;
    _read_depth = color_depth_t::grayscale_8bit;
    return color_depth_t::grayscale_8bit;
  }

  size_t Panel_SSD1677::_get_buffer_length(void) const
  {
    // Buffer layout:
    // LovyanGFX: panel_width=480 (X), panel_height=800 (Y)
    // EPD RAM: RAM Y=480 (rows), RAM X=800 (bits, 100 bytes per row)
    // LovyanGFX X (0-479) -> RAM Y (row index)
    // LovyanGFX Y (0-799) -> RAM X (bit position)
    // Buffer: panel_width rows * (panel_height+7)/8 bytes = 480 * 100 = 48000 bytes
    auto buf_x1_len = ((_cfg.panel_height + 7) & ~7) * _cfg.panel_width >> 3;
    return buf_x1_len * 2;
  }

  bool Panel_SSD1677::init(bool use_reset)
  {
    pinMode(_cfg.pin_busy, pin_mode_t::input_pullup);

    if (!Panel_HasBuffer::init(use_reset))
    {
      return false;
    }
    auto buf_len = _get_buffer_length();
    _buf_x1_len = buf_len >> 1;
    memset(_buf, 0xFF, buf_len);
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

    _last_epd_mode = (epd_mode_t)~0u;
    _initialize_seq = true;
    _need_flip_draw = false;
    _epd_frame_back = false;

    setInvert(_invert);

    setRotation(_rotation);

    _range_old.top = 0;
    _range_old.left = 0;
    _range_old.right = _width - 1;
    _range_old.bottom = _height - 1;
    _range_mod.top    = INT16_MAX;
    _range_mod.left   = INT16_MAX;
    _range_mod.right  = 0;
    _range_mod.bottom = 0;

    endWrite();
  }

  void Panel_SSD1677::waitDisplay(void)
  {
    _wait_busy();
  }

  bool Panel_SSD1677::displayBusy(void)
  {
    return _cfg.pin_busy >= 0 && gpio_in(_cfg.pin_busy);
  }

  void Panel_SSD1677::display(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h)
  {
    if (0 < w && 0 < h)
    {
      _range_mod.left   = std::min<int16_t>(_range_mod.left  , x        );
      _range_mod.right  = std::max<int16_t>(_range_mod.right , x + w - 1);
      _range_mod.top    = std::min<int16_t>(_range_mod.top   , y        );
      _range_mod.bottom = std::max<int16_t>(_range_mod.bottom, y + h - 1);
    }
    if (_range_mod.empty()) { return; }
    auto epd_mode = getEpdMode();
    bool need_flip_draw = _need_flip_draw || (epd_mode_t::epd_quality < epd_mode && epd_mode < epd_mode_t::epd_fast);
    _need_flip_draw = false;

    bool flg_mode_changed = (_last_epd_mode != epd_mode);

    if (_initialize_seq || flg_mode_changed)
    {
    // CMD_DISPLAY_UPDATE_CONTROL_2 parameter
    // 0b10000000 = Enable Clock signal
    // 0b01000000 = Enable Analog
    // 0b00100000 = Load temperature value
    // 0b00010000 = Load LUT with DISPLAY Mode 1
    // 0b00011000 = Load LUT with DISPLAY Mode 2
    // 0b00000100 = Display with DISPLAY Mode 1
    // 0b00001100 = Display with DISPLAY Mode 2
    // 0b00000010 = Disable Analog
    // 0b00000001 = Disable clock signal
    // epd_quality高品質モードではフリッキング更新を行う
      _range_mod.left = 0;
      _range_mod.right = _width - 1;
      _range_mod.top = 0;
      _range_mod.bottom = _height - 1;

      if (_initialize_seq) {
        _initialize_seq = false;
        // リセット直後は起動シーケンス設定およびフレームバッファの転送を行う。ここではリフレッシュは行わない。
        _bus->writeCommand(CMD_DISPLAY_UPDATE_CONTROL_2, 8);
        _bus->writeData(0xF8, 8);
        _exec_transfer(CMD_WRITE_RAM_BW, _buf, _range_mod, true);
        _exec_transfer(CMD_WRITE_RAM_RED, &_buf[_buf_x1_len], _range_mod, true);
        _bus->writeCommand(CMD_MASTER_ACTIVATION, 8);
        _send_msec = millis();
      }

      // epd_qualityの場合は反転描画は不要になる。
      // 他のモードに変更した直後は反転描画を行う。
      need_flip_draw = (epd_mode != epd_mode_t::epd_quality);
      _epd_frame_switching = need_flip_draw;
      if (!need_flip_draw)
      {
        if (_epd_frame_back)
        {  // フレームバッファ2番に送信される場合はモード変更前に一度描画更新を行う
          _epd_frame_back = false;
          _exec_transfer(CMD_WRITE_RAM_BW, _buf, _range_mod);
          _exec_transfer(CMD_WRITE_RAM_RED, &_buf[_buf_x1_len], _range_mod);
          _bus->writeCommand(CMD_MASTER_ACTIVATION, 8); // Active Display update
          _send_msec = millis();
        }
      }
      _wait_busy();
      _bus->writeCommand(CMD_DISPLAY_UPDATE_CONTROL_2, 8); // Display update seq opt
      uint8_t refresh_param = (epd_mode == epd_mode_t::epd_quality)
                          ? 0x14   // DISPLAY Mode1 (flicking)
                          : 0x1C;  // DISPLAY Mode2 (no flick)
      _bus->writeData(refresh_param, 8);
      _last_epd_mode = epd_mode;
    }
    range_rect_t tr = _range_mod;
    if (tr.top > _range_old.top) { tr.top = _range_old.top; }
    if (tr.left > _range_old.left) { tr.left = _range_old.left; }
    if (tr.right < _range_old.right) { tr.right = _range_old.right; }
    if (tr.bottom < _range_old.bottom) { tr.bottom = _range_old.bottom; }
    _range_old = _range_mod;

    _exec_transfer(CMD_WRITE_RAM_BW, _buf, tr, need_flip_draw);
    _exec_transfer(CMD_WRITE_RAM_RED, &_buf[_buf_x1_len], tr, need_flip_draw);
    _bus->writeCommand(CMD_MASTER_ACTIVATION, 8); // Active Display update
    _send_msec = millis();
    if (need_flip_draw)
    { // 反転リフレッシュを自前でやる場合
      _exec_transfer(CMD_WRITE_RAM_BW, _buf, tr);
      _exec_transfer(CMD_WRITE_RAM_RED, &_buf[_buf_x1_len], tr);
      _bus->writeCommand(CMD_MASTER_ACTIVATION, 8); // Active Display update
      _send_msec = millis();
    } else {
      if (_epd_frame_switching) { _epd_frame_back = !_epd_frame_back; }
      else { _epd_frame_back = false; }
    }

    _range_mod.top    = INT16_MAX;
    _range_mod.left   = INT16_MAX;
    _range_mod.right  = 0;
    _range_mod.bottom = 0;
  }

  void Panel_SSD1677::setInvert(bool invert)
  {
    if (_invert == invert && !_initialize_seq) { return; }
    _invert = invert;
    startWrite();
    _wait_busy();
    _bus->writeCommand(CMD_DISPLAY_UPDATE_CONTROL_1, 8);
    _bus->writeData((invert ^ _cfg.invert) ? 0x88 : 0x00, 8);
    _need_flip_draw = true;
    _range_mod.top = 0;
    _range_mod.left = 0;
    _range_mod.right = _width - 1;
    _range_mod.bottom = _height - 1;
    endWrite();
  }

  void Panel_SSD1677::setSleep(bool flg)
  {
    if (flg)
    {
      startWrite();
      _wait_busy();
      _bus->writeCommand(CMD_DISPLAY_UPDATE_CONTROL_2, 8);
      _bus->writeData(0x03, 8); // Disable Analog , Disable clock signal
      _bus->writeCommand(CMD_DEEP_SLEEP_MODE, 8);
      _bus->writeData(0x03, 8);
      endWrite();
    }
    else
    {
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
    _bus->writeCommand(CMD_DISPLAY_UPDATE_CONTROL_2, 8);
    _bus->writeData(flg ? 0x03 : 0xE0, 8);
    _wait_busy();
    _bus->writeCommand(CMD_MASTER_ACTIVATION, 8);
    endWrite();
  }

  void Panel_SSD1677::writeFillRectPreclipped(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, uint32_t rawcolor)
  {
    uint_fast16_t xs = x, xe = x + w - 1;
    uint_fast16_t ys = y, ye = y + h - 1;
    _xs = xs;
    _ys = ys;
    _xe = xe;
    _ye = ye;
    _update_transferred_rect(xs, ys, xe, ye);

    grayscale_t color;
    color.raw = rawcolor;
    uint32_t value = rawcolor;

    // Buffer layout: LovyanGFX X (0-479) = row index, LovyanGFX Y (0-799) = bit position
    // Each row is (panel_height+7)/8 = 100 bytes
    int32_t row_bytes = ((_cfg.panel_height + 7) & ~7) >> 3;

    y = ys;
    do
    {
      x = xs;
      auto btbl = &Bayer[(y & 3) << 2];
      do
      {
        // Buffer index: x * row_bytes + y / 8, bit position: y % 8
        uint32_t byte_idx = x * row_bytes + (y >> 3);
        uint8_t bit_mask = 0x80 >> (y & 7);
        bool flg = 256 <= value + btbl[x & 3];
        if (flg) _buf[byte_idx] |=  bit_mask;
        else     _buf[byte_idx] &= ~bit_mask;
      } while (++x <= xe);
    } while (++y <= ye);
  }

  void Panel_SSD1677::writeImage(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, pixelcopy_t* param, bool use_dma)
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
      auto color = colors[bufpos++];
      _draw_pixel(xpos, ypos, color.raw);
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
        if (millis() - start_time > timeout) {
// printf("TIMEOUT\n");
          return false;
        }
        delay(1);
      } while (gpio_in(_cfg.pin_busy));
// printf("time:%d\n", millis() - start_time);
    }
    return true;
  }

  void Panel_SSD1677::_draw_pixel(uint_fast16_t x, uint_fast16_t y, uint32_t value)
  {
    _rotate_pos(x, y);
    // Buffer layout: LovyanGFX X (0-479) = row index, LovyanGFX Y (0-799) = bit position
    // row_bytes = (panel_height+7)/8 = 100 bytes per row
    int32_t row_bytes = ((_cfg.panel_height + 7) & ~7) >> 3;
    uint32_t byte_idx = x * row_bytes + (y >> 3);
    uint8_t bit_mask = 0x80 >> (y & 7);
    // bool flg = 256 <= value + Bayer[(x & 3) | (y & 3) << 2];
    uint_fast8_t v = (value + Bayer[(x & 3) | (y & 3) << 2]) >> 7;
    v = (v < 4 ? v : 3);
    if (v & 2) _buf[byte_idx] |=  bit_mask;
    else       _buf[byte_idx] &= ~bit_mask;
    if (v & 1) _buf[byte_idx + _buf_x1_len] |=  bit_mask;
    else       _buf[byte_idx + _buf_x1_len] &= ~bit_mask;
  }

  uint8_t Panel_SSD1677::_read_pixel(uint_fast16_t x, uint_fast16_t y)
  {
    _rotate_pos(x, y);
    // Buffer layout: LovyanGFX X (0-479) = row index, LovyanGFX Y (0-799) = bit position
    int32_t row_bytes = ((_cfg.panel_height + 7) & ~7) >> 3;
    uint32_t byte_idx = x * row_bytes + (y >> 3);
    uint8_t bit_mask = 0x80 >> (y & 7);
    uint_fast8_t result = (_buf[byte_idx] & bit_mask) ? 2 : 0;
    result += (_buf[byte_idx + _buf_x1_len] & bit_mask) ? 1 : 0;
    return result;
  }

  void Panel_SSD1677::_exec_transfer(uint32_t cmd, const uint8_t* buf, const range_rect_t& range, bool invert)
  {
    // LovyanGFX: panel_width=480 (X), panel_height=800 (Y)
    // EPD RAM: X=800, Y=480
    // Coordinate mapping: LovyanGFX X -> RAM Y, LovyanGFX Y -> RAM X
    // range.left/right = LovyanGFX X (0-479) -> RAM Y
    // range.top/bottom = LovyanGFX Y (0-799) -> RAM X
    //
    // Buffer layout: row = LovyanGFX X (480 rows), column bits = LovyanGFX Y (100 bytes per row)
    // row_bytes = (panel_height+7)/8 = 100 bytes

    // LovyanGFX Y needs byte alignment (maps to RAM X direction, stored as bits in buffer)
    int32_t lgfx_xs = range.left;
    int32_t lgfx_xe = range.right;
    int32_t lgfx_ys = range.top & ~7;
    int32_t lgfx_ye = (range.bottom & ~7) + 7;

    // Map to EPD RAM coordinates
    int32_t ram_xs = lgfx_ys;        // RAM X start = LovyanGFX Y start
    int32_t ram_xe = lgfx_ye;        // RAM X end = LovyanGFX Y end
    int32_t ram_ys = lgfx_xs;        // RAM Y start = LovyanGFX X start
    int32_t ram_ye = lgfx_xe;        // RAM Y end = LovyanGFX X end

    _wait_busy();

    // 0x44: Set RAM X address start/end position (0-799, from LovyanGFX Y)
    _bus->writeCommand(0x44, 8);
    _bus->writeData(ram_xs & 0xFF, 8);
    _bus->writeData((ram_xs >> 8) & 0xFF, 8);
    _bus->writeData(ram_xe & 0xFF, 8);
    _bus->writeData((ram_xe >> 8) & 0xFF, 8);

    // 0x45: Set RAM Y address start/end position (0-479, from LovyanGFX X)
    _bus->writeCommand(0x45, 8);
    _bus->writeData(ram_ys & 0xFF, 8);
    _bus->writeData((ram_ys >> 8) & 0xFF, 8);
    _bus->writeData(ram_ye & 0xFF, 8);
    _bus->writeData((ram_ye >> 8) & 0xFF, 8);

    // 0x4E: Set RAM X address count to start
    _bus->writeCommand(0x4E, 8);
    _bus->writeData(ram_xs & 0xFF, 8);
    _bus->writeData((ram_xs >> 8) & 0xFF, 8);

    // 0x4F: Set RAM Y address count to start
    _bus->writeCommand(0x4F, 8);
    _bus->writeData(ram_ys & 0xFF, 8);
    _bus->writeData((ram_ys >> 8) & 0xFF, 8);

    _wait_busy();

    _bus->writeCommand(cmd, 8);

    // Buffer layout: row = LovyanGFX X (480 rows), bytes per row = (panel_height+7)/8 = 100
    int32_t row_bytes = ((_cfg.panel_height + 7) & ~7) >> 3;
    int32_t transfer_bytes = ((lgfx_ye - lgfx_ys) >> 3) + 1;  // bytes to transfer per row (Y direction)
    int32_t rows = lgfx_xe - lgfx_xs + 1;                      // number of rows (X direction)

    auto b = &buf[lgfx_xs * row_bytes + (lgfx_ys >> 3)];

    if (invert)
    {
      for (int32_t row = 0; row < rows; row++)
      {
        for (int32_t i = 0; i < transfer_bytes; i++)
        {
          _bus->writeData(~b[i], 8);
        }
        b += row_bytes;
      }
    }
    else
    {
      if (row_bytes == transfer_bytes) {
        // Full width transfer - can send all at once
        _bus->writeBytes(b, transfer_bytes * rows, true, true);
      }
      else
      {
        // Partial width transfer - send row by row
        for (int32_t row = 0; row < rows; row++)
        {
          _bus->writeBytes(b, transfer_bytes, true, true);
          b += row_bytes;
        }
      }
    }
  }

  void Panel_SSD1677::_update_transferred_rect(uint_fast16_t &xs, uint_fast16_t &ys, uint_fast16_t &xe, uint_fast16_t &ye)
  {
    _rotate_pos(xs, ys, xe, ye);

    // LovyanGFX Y direction (0-799) needs byte alignment for buffer access
    int32_t y1 = ys & ~7;
    int32_t y2 = (ye & ~7) + 7;

    // Clamp to valid range (panel_width=480, panel_height=800)
    if (xe >= _cfg.panel_width) xe = _cfg.panel_width - 1;
    if (y2 >= _cfg.panel_height) y2 = _cfg.panel_height - 1;

    _range_mod.left   = std::min<int32_t>(xs, _range_mod.left);
    _range_mod.right  = std::max<int32_t>(xe, _range_mod.right);
    _range_mod.top    = std::min<int32_t>(y1, _range_mod.top);
    _range_mod.bottom = std::max<int32_t>(y2, _range_mod.bottom);
  }

//----------------------------------------------------------------------------
 }
}
