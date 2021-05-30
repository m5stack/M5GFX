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

#include "LGFXBase.hpp"
#include "../utility/miniz.h"
#include "../utility/lgfx_pngle.h"
#include "../utility/lgfx_qrcode.h"
#include "../utility/lgfx_tjpgd.h"
#include "panel/Panel_Device.hpp"
#include "misc/bitmap.hpp"
//#include "lgfx_TTFfont.hpp"

#include <cstdarg>
#include <cmath>
#include <limits>
#include <list>

namespace lgfx
{
 inline namespace v1
 {
//----------------------------------------------------------------------------
  static constexpr float deg_to_rad = 0.017453292519943295769236907684886;
  static constexpr std::uint8_t FP_SCALE = 16;

  LGFXBase::LGFXBase(void)
  {
  }

  void LGFXBase::setColorDepth(color_depth_t depth)
  {
    _panel->setColorDepth(depth);
    _write_conv.setColorDepth(_panel->getWriteDepth());
    _read_conv.setColorDepth(_panel->getReadDepth());
  }

  void LGFXBase::setRotation(std::uint_fast8_t rotation)
  {
    if (_panel) _panel->setRotation(rotation);
    clearClipRect();
  }

  void LGFXBase::setAddrWindow(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h)
  {
    if (_adjust_abs(x, w)||_adjust_abs(y, h)) return;
    if (x < 0) { w += x; x = 0; }
    if (w > width() - x)  w = width()  - x;
    if (w < 1) { x = 0; w = 0; }
    if (y < 0) { h += y; y = 0; }
    if (h > height() - y) h = height() - y;
    if (h < 1) { y = 0; h = 0; }

    startWrite();
    setWindow(x, y, x + w - 1, y + h - 1);
    endWrite();
  }

  void LGFXBase::setClipRect(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h)
  {
    if (x < 0) { w += x; x = 0; }
    if (w > width() - x)  w = width()  - x;
    if (w < 1) { x = 0; w = 0; }
    _clip_l = x;
    _clip_r = x + w - 1;

    if (y < 0) { h += y; y = 0; }
    if (h > height() - y) h = height() - y;
    if (h < 1) { y = 0; h = 0; }
    _clip_t = y;
    _clip_b = y + h - 1;
  }

  void LGFXBase::display(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h)
  {
    if (x < 0) { w += x; x = 0; }
    if (w > width() - x)  w = width()  - x;
    if (w < 1) { x = 0; w = 0; }
    if (y < 0) { h += y; y = 0; }
    if (h > height() - y) h = height() - y;
    if (h < 1) { y = 0; h = 0; }
    _panel->display(x, y, w, h);
  }

  void LGFXBase::getClipRect(std::int32_t *x, std::int32_t *y, std::int32_t *w, std::int32_t *h)
  {
    *x = _clip_l;
    *w = _clip_r - *x + 1;
    *y = _clip_t;
    *h = _clip_b - *y + 1;
  }

  void LGFXBase::clearClipRect(void)
  {
    _clip_l = 0;
    _clip_r = width() - 1;
    _clip_t = 0;
    _clip_b = height() - 1;
  }

  void LGFXBase::setScrollRect(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h)
  {
    _adjust_abs(x, w);
    if (x < 0) { w += x; x = 0; }
    if (w > width() - x)  w = width()  - x;
    if (w < 0) w = 0;
    _sx = x;
    _sw = w;

    _adjust_abs(y, h);
    if (y < 0) { h += y; y = 0; }
    if (h > height() - y) h = height() - y;
    if (h < 0) h = 0;
    _sy = y;
    _sh = h;
  }

  void LGFXBase::getScrollRect(std::int32_t *x, std::int32_t *y, std::int32_t *w, std::int32_t *h)
  {
    *x = _sx;
    *y = _sy;
    *w = _sw;
    *h = _sh;
  }

  void LGFXBase::clearScrollRect(void)
  {
    _sx = 0;
    _sw = width();
    _sy = 0;
    _sh = height();
  }

  void LGFXBase::drawFastVLine(std::int32_t x, std::int32_t y, std::int32_t h)
  {
    _adjust_abs(y, h);
    startWrite();
    writeFastVLine(x, y, h);
    endWrite();
  }

  void LGFXBase::writeFastVLine(std::int32_t x, std::int32_t y, std::int32_t h)
  {
    if (x < _clip_l || x > _clip_r) return;
    auto ct = _clip_t;
    if (y < ct) { h += y - ct; y = ct; }
    auto cb = _clip_b + 1 - y;
    if (h > cb) h = cb;
    if (h < 1) return;

    writeFillRectPreclipped(x, y, 1, h);
  }

  void LGFXBase::drawFastHLine(std::int32_t x, std::int32_t y, std::int32_t w)
  {
    _adjust_abs(x, w);
    startWrite();
    writeFastHLine(x, y, w);
    endWrite();
  }

  void LGFXBase::writeFastHLine(std::int32_t x, std::int32_t y, std::int32_t w)
  {
    if (y < _clip_t || y > _clip_b) return;
    auto cl = _clip_l;
    if (x < cl) { w += x - cl; x = cl; }
    auto cr = _clip_r + 1 - x;
    if (w > cr) w = cr;
    if (w < 1) return;

    writeFillRectPreclipped(x, y, w, 1);
  }

  void LGFXBase::fillRect(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h)
  {
    _adjust_abs(x, w);
    _adjust_abs(y, h);
    startWrite();
    writeFillRect(x, y, w, h);
    endWrite();
  }

  void LGFXBase::writeFillRect(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h)
  {
    if (_clipping(x, y, w, h))
    {
      writeFillRectPreclipped(x, y, w, h);
    }
  }

  void LGFXBase::drawRect(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h)
  {
    if (_adjust_abs(x, w)||_adjust_abs(y, h)) return;
    startWrite();
    writeFastHLine(x, y        , w);
    if (--h) {
      writeFastHLine(x, y + h    , w);
      if (--h) {
        writeFastVLine(x        , ++y, h);
        writeFastVLine(x + w - 1,   y, h);
      }
    }
    endWrite();
  }

  void LGFXBase::drawCircle(std::int32_t x, std::int32_t y, std::int32_t r)
  {
    if ( r <= 0 ) {
      drawPixel(x, y);
      return;
    }

    startWrite();
    std::int32_t f = 1 - r;
    std::int32_t ddF_y = - (r << 1);
    std::int32_t ddF_x = 1;
    std::int32_t i = 0;
    std::int32_t j = -1;
    do {
      while (f < 0) {
        ++i;
        f += (ddF_x += 2);
      }
      f += (ddF_y += 2);

      writeFastHLine(x - i    , y + r, i - j);
      writeFastHLine(x - i    , y - r, i - j);
      writeFastHLine(x + j + 1, y - r, i - j);
      writeFastHLine(x + j + 1, y + r, i - j);

      writeFastVLine(x + r, y + j + 1, i - j);
      writeFastVLine(x + r, y - i    , i - j);
      writeFastVLine(x - r, y - i    , i - j);
      writeFastVLine(x - r, y + j + 1, i - j);
      j = i;
    } while (i < --r);
    endWrite();
  }

  void LGFXBase::drawCircleHelper(std::int32_t x, std::int32_t y, std::int32_t r, std::uint_fast8_t cornername)
  {
    if (r <= 0) return;
    std::int32_t f     = 1 - r;
    std::int32_t ddF_y = - (r << 1);
    std::int32_t ddF_x = 1;
    std::int32_t i     = 0;
    std::int32_t j     = 0;

    startWrite();
    do {
      while (f < 0) {
        ++i;
        f += (ddF_x += 2);
      }
      f += (ddF_y += 2);

      if (cornername & 0x1) { // left top
        writeFastHLine(x - i, y - r, i - j);
        writeFastVLine(x - r, y - i, i - j);
      }
      if (cornername & 0x2) { // right top
        writeFastVLine(x + r    , y - i, i - j);
        writeFastHLine(x + j + 1, y - r, i - j);
      }
      if (cornername & 0x4) { // right bottom
        writeFastHLine(x + j + 1, y + r    , i - j);
        writeFastVLine(x + r    , y + j + 1, i - j);
      }
      if (cornername & 0x8) { // left bottom
        writeFastVLine(x - r, y + j + 1, i - j);
        writeFastHLine(x - i, y + r    , i - j);
      }
      j = i;
    } while (i < --r);
    endWrite();
  }

  void LGFXBase::fillCircle(std::int32_t x, std::int32_t y, std::int32_t r) {
    startWrite();
    writeFastHLine(x - r, y, (r << 1) + 1);
    fillCircleHelper(x, y, r, 3, 0);
    endWrite();
  }

  void LGFXBase::fillCircleHelper(std::int32_t x, std::int32_t y, std::int32_t r, std::uint_fast8_t corners, std::int32_t delta)
  {
    if (r <= 0) return;

    ++delta;

    std::int32_t f     = 1 - r;
    std::int32_t ddF_y = - (r << 1);
    std::int32_t ddF_x = 1;
    std::int32_t i     = 0;

    startWrite();
    do {
      std::int32_t len = 0;
      while (f < 0) {
        f += (ddF_x += 2);
        ++len;
      }
      i += len;
      f += (ddF_y += 2);

      if (corners & 0x1) {
        if (len) writeFillRect(x - r, y + i - len + 1, (r << 1) + delta, len);
        writeFastHLine(x - i, y + r, (i << 1) + delta);
      }
      if (corners & 0x2) {
        writeFastHLine(x - i, y - r, (i << 1) + delta);
        if (len) writeFillRect(x - r, y - i, (r << 1) + delta, len);
      }
    } while (i < --r);
    endWrite();
  }

  void LGFXBase::drawEllipse(std::int32_t x, std::int32_t y, std::int32_t rx, std::int32_t ry)
  {
    if (ry == 0) {
      drawFastHLine(x - rx, y, (ry << 2) + 1);
      return;
    }
    if (rx == 0) {
      drawFastVLine(x, y - ry, (rx << 2) + 1);
      return;
    }
    if (rx < 0 || ry < 0) return;

    std::int32_t xt, yt, s, i;
    std::int32_t rx2 = rx * rx;
    std::int32_t ry2 = ry * ry;

    startWrite();

    i = -1;
    xt = 0;
    yt = ry;
    s = (ry2 << 1) + rx2 * (1 - (ry << 1));
    do {
      while ( s < 0 ) s += ry2 * ((++xt << 2) + 2);
      writeFastHLine(x - xt   , y - yt, xt - i);
      writeFastHLine(x + i + 1, y - yt, xt - i);
      writeFastHLine(x + i + 1, y + yt, xt - i);
      writeFastHLine(x - xt   , y + yt, xt - i);
      i = xt;
      s -= (--yt) * rx2 << 2;
    } while (ry2 * xt <= rx2 * yt);

    i = -1;
    yt = 0;
    xt = rx;
    s = (rx2 << 1) + ry2 * (1 - (rx << 1));
    do {
      while ( s < 0 ) s += rx2 * ((++yt << 2) + 2);
      writeFastVLine(x - xt, y - yt   , yt - i);
      writeFastVLine(x - xt, y + i + 1, yt - i);
      writeFastVLine(x + xt, y + i + 1, yt - i);
      writeFastVLine(x + xt, y - yt   , yt - i);
      i = yt;
      s -= (--xt) * ry2 << 2;
    } while (rx2 * yt <= ry2 * xt);

    endWrite();
  }

  void LGFXBase::fillEllipse(std::int32_t x, std::int32_t y, std::int32_t rx, std::int32_t ry)
  {
    if (ry == 0) {
      drawFastHLine(x - rx, y, (ry << 2) + 1);
      return;
    }
    if (rx == 0) {
      drawFastVLine(x, y - ry, (rx << 2) + 1);
      return;
    }
    if (rx < 0 || ry < 0) return;

    std::int32_t xt, yt, i;
    std::int32_t rx2 = rx * rx;
    std::int32_t ry2 = ry * ry;
    std::int32_t s;

    startWrite();

    writeFastHLine(x - rx, y, (rx << 1) + 1);
    i = 0;
    yt = 0;
    xt = rx;
    s = (rx2 << 1) + ry2 * (1 - (rx << 1));
    do {
      while (s < 0) s += rx2 * ((++yt << 2) + 2);
      writeFillRect(x - xt, y - yt   , (xt << 1) + 1, yt - i);
      writeFillRect(x - xt, y + i + 1, (xt << 1) + 1, yt - i);
      i = yt;
      s -= (--xt) * ry2 << 2;
    } while (rx2 * yt <= ry2 * xt);

    xt = 0;
    yt = ry;
    s = (ry2 << 1) + rx2 * (1 - (ry << 1));
    do {
      while (s < 0) s += ry2 * ((++xt << 2) + 2);
      writeFastHLine(x - xt, y - yt, (xt << 1) + 1);
      writeFastHLine(x - xt, y + yt, (xt << 1) + 1);
      s -= (--yt) * rx2 << 2;
    } while(ry2 * xt <= rx2 * yt);

    endWrite();
  }

  void LGFXBase::drawRoundRect(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h, std::int32_t r)
  {
    if (_adjust_abs(x, w)||_adjust_abs(y, h)) return;
    startWrite();

    w--;
    h--;
    std::int32_t len = (r << 1) + 1;
    std::int32_t y1 = y + h - r;
    std::int32_t y0 = y + r;
    writeFastVLine(x      , y0 + 1, h - len);
    writeFastVLine(x + w  , y0 + 1, h - len);

    std::int32_t x1 = x + w - r;
    std::int32_t x0 = x + r;
    writeFastHLine(x0 + 1, y      , w - len);
    writeFastHLine(x0 + 1, y + h  , w - len);

    std::int32_t f     = 1 - r;
    std::int32_t ddF_y = -(r << 1);
    std::int32_t ddF_x = 1;

    len = 0;
    for (std::int32_t i = 0; i <= r; i++) {
      len++;
      if (f >= 0) {
        writeFastHLine(x0 - i          , y0 - r, len);
        writeFastHLine(x0 - i          , y1 + r, len);
        writeFastHLine(x1 + i - len + 1, y1 + r, len);
        writeFastHLine(x1 + i - len + 1, y0 - r, len);
        writeFastVLine(x1 + r, y1 + i - len + 1, len);
        writeFastVLine(x0 - r, y1 + i - len + 1, len);
        writeFastVLine(x1 + r, y0 - i, len);
        writeFastVLine(x0 - r, y0 - i, len);
        len = 0;
        r--;
        ddF_y += 2;
        f     += ddF_y;
      }
      ddF_x += 2;
      f     += ddF_x;
    }
    endWrite();
  }

  void LGFXBase::fillRoundRect(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h, std::int32_t r)
  {
    if (_adjust_abs(x, w)||_adjust_abs(y, h)) return;
    startWrite();
    std::int32_t y2 = y + r;
    std::int32_t y1 = y + h - r - 1;
    std::int32_t ddF_y = - (r << 1);
    std::int32_t delta = w + ddF_y;
    writeFillRect(x, y2, w, h + ddF_y);
    std::int32_t x0 = x + r;
    std::int32_t f     = 1 - r;
    std::int32_t ddF_x = 1;
    std::int32_t len = 0;
    for (std::int32_t i = 0; i <= r; i++) {
      len++;
      if (f >= 0) {
        writeFillRect(x0 - r, y2 - i          , (r << 1) + delta, len);
        writeFillRect(x0 - r, y1 + i - len + 1, (r << 1) + delta, len);
        if (i == r) break;
        len = 0;
        writeFastHLine(x0 - i, y1 + r, (i << 1) + delta);
        ddF_y += 2;
        f     += ddF_y;
        writeFastHLine(x0 - i, y2 - r, (i << 1) + delta);
        r--;
      }
      ddF_x += 2;
      f     += ddF_x;
    }
    endWrite();
  }

  void LGFXBase::drawLine(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1)
  {
    bool steep = abs(y1 - y0) > abs(x1 - x0);

    std::int32_t xstart = _clip_l;
    std::int32_t ystart = _clip_t;
    std::int32_t xend   = _clip_r;
    std::int32_t yend   = _clip_b;

    if (steep)
    {
      std::swap(xstart, ystart);
      std::swap(xend, yend);
      std::swap(x0, y0);
      std::swap(x1, y1);
    }
    if (x0 > x1)
    {
      std::swap(x0, x1);
      std::swap(y0, y1);
    }
    if (x0 > xend || x1 < xstart) return;
    xend = std::min(x1, xend);

    std::int32_t dy = abs(y1 - y0);
    std::int32_t ystep = (y1 > y0) ? 1 : -1;
    std::int32_t dx = x1 - x0;
    std::int32_t err = dx >> 1;

    while (x0 < xstart || y0 < ystart || y0 > yend)
    {
      err -= dy;
      if (err < 0)
      {
        err += dx;
        y0 += ystep;
      }
      if (++x0 > xend) return;
    }
    std::int32_t xs = x0;
    std::int32_t dlen = 0;
    if (ystep < 0) std::swap(ystart, yend);
    yend += ystep;

    startWrite();
    if (steep)
    {
      do
      {
        ++dlen;
        if ((err -= dy) < 0)
        {
          writeFillRectPreclipped(y0, xs, 1, dlen);
          err += dx;
          xs = x0 + 1; dlen = 0; y0 += ystep;
          if (y0 == yend) break;
        }
      } while (++x0 <= xend);
      if (dlen) writeFillRectPreclipped(y0, xs, 1, dlen);
    }
    else
    {
      do
      {
        ++dlen;
        if ((err -= dy) < 0)
        {
          writeFillRectPreclipped(xs, y0, dlen, 1);
          err += dx;
          xs = x0 + 1; dlen = 0; y0 += ystep;
          if (y0 == yend) break;
        }
      } while (++x0 <= xend);
      if (dlen) writeFillRectPreclipped(xs, y0, dlen, 1);
    }
    endWrite();
  }

  void LGFXBase::drawTriangle(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2)
  {
    startWrite();
    drawLine(x0, y0, x1, y1);
    drawLine(x1, y1, x2, y2);
    drawLine(x2, y2, x0, y0);
    endWrite();
  }

  void LGFXBase::fillTriangle(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2)
  {
    std::int32_t a, b;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
    if (y1 > y2) { std::swap(y2, y1); std::swap(x2, x1); }
    if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }

    if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
      a = b = x0;
      if (x1 < a)      a = x1;
      else if (x1 > b) b = x1;
      if (x2 < a)      a = x2;
      else if (x2 > b) b = x2;
      drawFastHLine(a, y0, b - a + 1);
      return;
    }
    if ((x1-x0) * (y2-y0) == (x2-x0) * (y1-y0)) {
      drawLine(x0,y0,x2,y2);
      return;
    }

    std::int32_t dy1 = y1 - y0;
    std::int32_t dy2 = y2 - y0;
    bool change = ((x1 - x0) * dy2 > (x2 - x0) * dy1);
    std::int32_t dx1 = abs(x1 - x0);
    std::int32_t dx2 = abs(x2 - x0);
    std::int32_t xstep1 = x1 < x0 ? -1 : 1;
    std::int32_t xstep2 = x2 < x0 ? -1 : 1;
    a = b = x0;
    if (change) {
      std::swap(dx1, dx2);
      std::swap(dy1, dy2);
      std::swap(xstep1, xstep2);
    }
    std::int32_t err1 = (std::max(dx1, dy1) >> 1)
                 + (xstep1 < 0
                   ? std::min(dx1, dy1)
                   : dx1);
    std::int32_t err2 = (std::max(dx2, dy2) >> 1)
                 + (xstep2 > 0
                   ? std::min(dx2, dy2)
                   : dx2);
    startWrite();
    if (y0 != y1) {
      do {
        err1 -= dx1;
        while (err1 < 0) { err1 += dy1; a += xstep1; }
        err2 -= dx2;
        while (err2 < 0) { err2 += dy2; b += xstep2; }
        writeFastHLine(a, y0, b - a + 1);
      } while (++y0 < y1);
    }

    if (change) {
      b = x1;
      xstep2 = x2 < x1 ? -1 : 1;
      dx2 = abs(x2 - x1);
      dy2 = y2 - y1;
      err2 = (std::max(dx2, dy2) >> 1)
           + (xstep2 > 0
             ? std::min(dx2, dy2)
             : dx2);
    } else {
      a = x1;
      dx1 = abs(x2 - x1);
      dy1 = y2 - y1;
      xstep1 = x2 < x1 ? -1 : 1;
      err1 = (std::max(dx1, dy1) >> 1)
           + (xstep1 < 0
             ? std::min(dx1, dy1)
             : dx1);
    }
    do {
      err1 -= dx1;
      while (err1 < 0) { err1 += dy1; if ((a += xstep1) == x2) break; }
      err2 -= dx2;
      while (err2 < 0) { err2 += dy2; if ((b += xstep2) == x2) break; }
      writeFastHLine(a, y0, b - a + 1);
    } while (++y0 <= y2);
    endWrite();
  }

  void LGFXBase::drawBezier( std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2)
  {
    std::int32_t x = x0 - x1, y = y0 - y1;
    double t = x0 - 2 * x1 + x2, r;

    startWrite();

    if (x * (x2 - x1) > 0) {
      if (y * (y2 - y1) > 0)
        if (fabs((y0 - 2 * y1 + y2) / t * x) > abs(y)) {
          x0 = x2; x2 = x + x1; y0 = y2; y2 = y + y1;
        }
      t = (x0 - x1) / t;
      r = (1 - t) * ((1 - t) * y0 + 2.0 * t * y1) + t * t * y2;
      t = (x0 * x2 - x1 * x1) * t / (x0 - x1);
      x = floor(t + 0.5); y = floor(r + 0.5);
      r = (y1 - y0) * (t - x0) / (x1 - x0) + y0;
      draw_bezier_helper(x0, y0, x, floor(r + 0.5), x, y);
      r = (y1 - y2) * (t - x2) / (x1 - x2) + y2;
      x0 = x1 = x; y0 = y; y1 = floor(r + 0.5);
    }
    if ((y0 - y1) * (y2 - y1) > 0) {
      t = y0 - 2 * y1 + y2; t = (y0 - y1) / t;
      r = (1 - t) * ((1 - t) * x0 + 2.0 * t * x1) + t * t * x2;
      t = (y0 * y2 - y1 * y1) * t / (y0 - y1);
      x = floor(r + 0.5); y = floor(t + 0.5);
      r = (x1 - x0) * (t - y0) / (y1 - y0) + x0;
      draw_bezier_helper(x0, y0, floor(r + 0.5), y, x, y);
      r = (x1 - x2) * (t - y2) / (y1 - y2) + x2;
      x0 = x; x1 = floor(r + 0.5); y0 = y1 = y;
    }
    draw_bezier_helper(x0, y0, x1, y1, x2, y2);

    endWrite();
  }

  void LGFXBase::draw_bezier_helper( std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2)
  {
    // Check if coordinates are sequential (replaces assert)
    if (((x2 >= x1 && x1 >= x0) || (x2 <= x1 && x1 <= x0))
        && ((y2 >= y1 && y1 >= y0) || (y2 <= y1 && y1 <= y0)))
    {
      // Coordinates are sequential
      std::int32_t sx = x2 - x1, sy = y2 - y1;
      std::int32_t xx = x0 - x1, yy = y0 - y1, xy;
      float dx, dy, err, cur = xx * sy - yy * sx;

      if (sx * (std::int32_t)sx + sy * (std::int32_t)sy > xx * xx + yy * yy) {
        x2 = x0; x0 = sx + x1; y2 = y0; y0 = sy + y1; cur = -cur;
      }
      if (cur != 0) {
        xx += sx; xx *= sx = x0 < x2 ? 1 : -1;
        yy += sy; yy *= sy = y0 < y2 ? 1 : -1;
        xy = 2 * xx * yy; xx *= xx; yy *= yy;
        if (cur * sx * sy < 0) {
          xx = -xx; yy = -yy; xy = -xy; cur = -cur;
        }
        dx = 4.0 * sy * cur * (x1 - x0) + xx - xy;
        dy = 4.0 * sx * cur * (y0 - y1) + yy - xy;
        xx += xx; yy += yy; err = dx + dy + xy;
        do {
          drawPixel(x0, y0);
          if (x0 == x2 && y0 == y2)
          {
            return;
          }
          y1 = 2 * err < dx;
          if (2 * err > dy) {
            x0 += sx;
            dx -= xy;
            err += dy += yy;
          }
          if (    y1    ) {
            y0 += sy;
            dy -= xy;
            err += dx += xx;
          }
        } while (dy < dx );
      }
      drawLine(x0, y0, x2, y2);
    }
  }

  void LGFXBase::drawBezier( std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::int32_t x3, std::int32_t y3)
  {
    std::int32_t w = x0-x1;
    std::int32_t h = y0-y1;
    std::int32_t len = w*w+h*h;
    w = x1-x2;
    h = y1-y2;
    std::int32_t len2 = w*w+h*h;
    if (len < len2) len = len2;
    w = x2-x3;
    h = y2-y3;
    len2 = w*w+h*h;
    if (len < len2) len = len2;
    len = (std::int32_t)round(sqrt(len)) >> 2;

    float fx0 = x0;
    float fy0 = y0;
    float fx1 = x1;
    float fy1 = y1;
    float fx2 = x2;
    float fy2 = y2;
    float fx3 = x3;
    float fy3 = y3;

    std::int32_t i = 0;
    startWrite();
//drawLine(x0, y0, x1, y1);
//drawLine(x1, y1, x2, y2);
//drawLine(x2, y2, x3, y3);
//drawCircle(x0, y0, 3);
//drawCircle(x1, y1, 3);
//drawCircle(x2, y2, 3);
//drawCircle(x3, y3, 3);
    do {
      float t = i;
      t = t / (len<<1);
      float tr = 1 - t;
      float f0 = tr * tr;
      float f1 = f0 * t * 3;
      f0 = f0 * tr;
      float f3 = t * t;
      float f2 = tr * f3 * 3;
      f3 = f3 * t;
      x1 = roundf( fx0 * f0 + fx1 * f1 + fx2 * f2 + fx3 * f3);
      y1 = roundf( fy0 * f0 + fy1 * f1 + fy2 * f2 + fy3 * f3);
      if (x0 != x1 || y0 != y1) {
        drawLine(x0, y0, x1, y1);
//drawCircle(x1, y1, 3);
        x0 = x1;
        y0 = y1;
      }
      x2 = roundf( fx0 * f3 + fx1 * f2 + fx2 * f1 + fx3 * f0);
      y2 = roundf( fy0 * f3 + fy1 * f2 + fy2 * f1 + fy3 * f0);
      if (x3 != x2 || y3 != y2) {
        drawLine(x3, y3, x2, y2);
//drawCircle(x2, y2, 3);
        x3 = x2;
        y3 = y2;
      }
    } while (++i <= len);
    endWrite();
  }

  void LGFXBase::draw_gradient_line( std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1, uint32_t colorstart, uint32_t colorend )
  {
    if ( colorstart == colorend || (x0 == x1 && y0 == y1)) {
      setColor(colorstart);
      drawLine( x0, y0, x1, y1);
      return;
    }

    bool steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) { // swap axis
      std::swap(x0, y0);
      std::swap(x1, y1);
    }

    if (x0 > x1) { // swap points
      std::swap(x0, x1);
      std::swap(y0, y1);
      std::swap(colorstart, colorend);
    }

    std::int32_t dx = x1 - x0;
    std::int32_t err = dx >> 1;
    std::int32_t dy = abs(y1 - y0);
    std::int32_t ystep = (y0 < y1) ? 1 : -1;

    std::int32_t r = (colorstart >> 16)&0xFF;
    std::int32_t g = (colorstart >> 8 )&0xFF;
    std::int32_t b = (colorstart      )&0xFF;

    std::int32_t diff_r = ((colorend >> 16)&0xFF) - r;
    std::int32_t diff_g = ((colorend >> 8 )&0xFF) - g;
    std::int32_t diff_b = ((colorend      )&0xFF) - b;

    startWrite();
    for (std::int32_t x = x0; x <= x1; x++) {
      setColor(color888( (x - x0) * diff_r / dx + r
                       , (x - x0) * diff_g / dx + g
                       , (x - x0) * diff_b / dx + b));
      if (steep) writePixel(y0, x);
      else       writePixel(x, y0);
      err -= dy;
      if (err < 0) {
        err += dx;
        y0 += ystep;
      }
    }
    endWrite();
  }

  void LGFXBase::drawEllipseArc(std::int32_t x, std::int32_t y, std::int32_t r0x, std::int32_t r1x, std::int32_t r0y, std::int32_t r1y, float start, float end)
  {
    if (r0x < r1x) std::swap(r0x, r1x);
    if (r0y < r1y) std::swap(r0y, r1y);
    if (r1x < 0) return;
    if (r1y < 0) return;

    bool equal = fabsf(start - end) < std::numeric_limits<float>::epsilon();
    start = fmodf(start, 360);
    end = fmodf(end, 360);
    if (start < 0) start += 360.0;
    if (end < 0) end += 360.0;

    startWrite();
    fill_arc_helper(x, y, r0x, r1x, r0y, r1y, start, start);
    fill_arc_helper(x, y, r0x, r1x, r0y, r1y, end, end);
    if (!equal && (fabsf(start - end) <= 0.0001)) { start = .0; end = 360.0; }
    fill_arc_helper(x, y, r0x, r0x, r0y, r0y, start, end);
    fill_arc_helper(x, y, r1x, r1x, r1y, r1y, start, end);
    endWrite();
  }

  void LGFXBase::fillEllipseArc(std::int32_t x, std::int32_t y, std::int32_t r0x, std::int32_t r1x, std::int32_t r0y, std::int32_t r1y, float start, float end)
  {
    if (r0x < r1x) std::swap(r0x, r1x);
    if (r0y < r1y) std::swap(r0y, r1y);
    if (r1x < 0) return;
    if (r1y < 0) return;

    bool equal = fabsf(start - end) < std::numeric_limits<float>::epsilon();
    start = fmodf(start, 360);
    end = fmodf(end, 360);
    if (start < 0) start += 360.0;
    if (end < 0) end += 360.0;
    if (!equal && (fabsf(start - end) <= 0.0001)) { start = .0; end = 360.0; }

    startWrite();
    fill_arc_helper(x, y, r0x, r1x, r0y, r1y, start, end);
    endWrite();
  }

  void LGFXBase::fill_arc_helper(std::int32_t cx, std::int32_t cy, std::int32_t oradius_x, std::int32_t iradius_x, std::int32_t oradius_y, std::int32_t iradius_y, float start, float end)
  {
    float s_cos = (cosf(start * deg_to_rad));
    float e_cos = (cosf(end * deg_to_rad));
    float sslope = s_cos / (sinf(start * deg_to_rad));
    float eslope = -1000000;
    if (end != 360.0) eslope = e_cos / (sinf(end * deg_to_rad));
    float swidth =  0.5 / s_cos;
    float ewidth = -0.5 / e_cos;

    bool start180 = !(start < 180);
    bool end180 = end < 180;
    bool reversed = start + 180 < end || (end < start && start < end + 180);

    std::int32_t xleft  = -oradius_x;
    std::int32_t xright = oradius_x + 1;
    std::int32_t y = -oradius_y;
    std::int32_t ye = oradius_y;
    if (!reversed)
    {
      if (    (end >= 270 || end <  90) && (start >= 270 || start <  90)) xleft = 0;
      else if (end <  270 && end >= 90  &&  start <  270 && start >= 90) xright = 1;
      if (     end >= 180 && start >= 180) ye = 0;
      else if (end <  180 && start <  180) y = 0;
    }
    if (y  < _clip_t - cy    ) y  = _clip_t - cy;
    if (ye > _clip_b - cy + 1) ye = _clip_b - cy + 1;

    if (xleft  < _clip_l - cx    ) xleft  = _clip_l - cx;
    if (xright > _clip_r - cx + 1) xright = _clip_r - cx + 1;

    bool trueCircle = (oradius_x == oradius_y) && (iradius_x == iradius_y);

    std::int32_t iradius_y2 = iradius_y * (iradius_y - 1);
    std::int32_t iradius_x2 = iradius_x * (iradius_x - 1);
    float irad_rate = iradius_x2 && iradius_y2 ? (float)iradius_x2 / (float)iradius_y2 : 0;

    std::int32_t oradius_y2 = oradius_y * (oradius_y + 1);
    std::int32_t oradius_x2 = oradius_x * (oradius_x + 1);
    float orad_rate = oradius_x2 && oradius_y2 ? (float)oradius_x2 / (float)oradius_y2 : 0;

    do
    {
      std::int32_t y2 = y * y;
      std::int32_t compare_o = oradius_y2 - y2;
      std::int32_t compare_i = iradius_y2 - y2;
      if (!trueCircle)
      {
        compare_i = floorf(compare_i * irad_rate);
        compare_o = ceilf (compare_o * orad_rate);
      }
      std::int32_t xe = ceilf(sqrtf(compare_o));
      std::int32_t x = 1 - xe;

      if ( x < xleft )  x = xleft;
      if (xe > xright) xe = xright;
      float ysslope = (y + swidth) * sslope;
      float yeslope = (y + ewidth) * eslope;
      int len = 0;
      do
      {
        bool flg1 = start180 != (x <= ysslope);
        bool flg2 =   end180 != (x <= yeslope);
        std::int32_t x2 = x * x;
        if (x2 >= compare_i
         && ((flg1 && flg2) || (reversed && (flg1 || flg2)))
         && x != xe
         && x2 < compare_o)
        {
          ++len;
        }
        else
        {
          if (len)
          {
            writeFastHLine(cx + x - len, cy + y, len);
            len = 0;
          }
          if (x2 >= compare_o) break;
          if (x < 0 && x2 < compare_i) { x = -x; }
        }
      } while (++x <= xe);
    } while (++y <= ye);
  }

  void LGFXBase::draw_bitmap(std::int32_t x, std::int32_t y, const std::uint8_t *bitmap, std::int32_t w, std::int32_t h, std::uint32_t fg_rawcolor, std::uint32_t bg_rawcolor)
  {
    if (w < 1 || h < 1) return;
    setRawColor(fg_rawcolor);
    std::int32_t byteWidth = (w + 7) >> 3;
    std::uint_fast8_t byte = 0;

    bool fg = true;
    std::int32_t j = 0;
    startWrite();
    do {
      std::int32_t i = 0;
      do {
        std::int32_t ip = i;
        for (;;) {
          if (!(i & 7)) byte = bitmap[i >> 3];
          if (fg != (bool)(byte & 0x80) || (++i >= w)) break;
          byte <<= 1;
        }
        if ((ip != i) && (fg || bg_rawcolor != ~0u)) {
          writeFastHLine(x + ip, y + j, i - ip);
        }
        fg = !fg;
        if (bg_rawcolor != ~0u) setRawColor(fg ? fg_rawcolor : bg_rawcolor);
      } while (i < w);
      bitmap += byteWidth;
    } while (++j < h);
    endWrite();
  }

  void LGFXBase::draw_xbitmap(std::int32_t x, std::int32_t y, const std::uint8_t *bitmap, std::int32_t w, std::int32_t h, std::uint32_t fg_rawcolor, std::uint32_t bg_rawcolor)
  {
    if (w < 1 || h < 1) return;
    setRawColor(fg_rawcolor);
    std::int32_t byteWidth = (w + 7) >> 3;
    std::uint_fast8_t byte = 0;

    bool fg = true;
    std::int32_t j = 0;
    startWrite();
    do {
      std::int32_t i = 0;
      do {
        std::int32_t ip = i;
        for (;;) {
          if (!(i & 7)) byte = bitmap[i >> 3];
          if (fg != (bool)(byte & 0x01) || (++i >= w)) break;
          byte >>= 1;
        }
        if ((ip != i) && (fg || bg_rawcolor != ~0u)) {
          writeFastHLine(x + ip, y + j, i - ip);
        }
        fg = !fg;
        if (bg_rawcolor != ~0u) setRawColor(fg ? fg_rawcolor : bg_rawcolor);
      } while (i < w);
      bitmap += byteWidth;
    } while (++j < h);
    endWrite();
  }

  void LGFXBase::pushImage(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h, pixelcopy_t *param, bool use_dma)
  {
    param->src_bitwidth = w;
    if (param->src_bits < 8) {        // get bitwidth
//      std::uint32_t x_mask = (1 << (4 - __builtin_ffs(param->src_bits))) - 1;
//      std::uint32_t x_mask = (1 << ((~(param->src_bits>>1)) & 3)) - 1;
      std::uint32_t x_mask = (param->src_bits == 1) ? 7
                           : (param->src_bits == 2) ? 3
                                                    : 1;
      param->src_bitwidth = (w + x_mask) & (~x_mask);
    }

    std::int32_t dx=0, dw=w;
    if (0 < _clip_l - x) { dx = _clip_l - x; dw -= dx; x = _clip_l; }

    if (_adjust_width(x, dx, dw, _clip_l, _clip_r - _clip_l + 1)) return;
    param->src_x = dx;

    std::int32_t dy=0, dh=h;
    if (0 < _clip_t - y) { dy = _clip_t - y; dh -= dy; y = _clip_t; }
    if (_adjust_width(y, dy, dh, _clip_t, _clip_b - _clip_t + 1)) return;
    param->src_y = dy;

    startWrite();
    _panel->writeImage(x, y, dw, dh, param, use_dma);
    endWrite();
  }

  void LGFXBase::make_rotation_matrix(float* result, float dst_x, float dst_y, float src_x, float src_y, float angle, float zoom_x, float zoom_y)
  {
    float rad = fmodf(angle, 360) * deg_to_rad;
    float sin_f = sinf(rad);
    float cos_f = cosf(rad);
    result[0] =  cos_f * zoom_x;
    result[1] = -sin_f * zoom_y;
    result[2] =  dst_x - src_x * result[0] - src_y * result[1];
    result[3] =  sin_f * zoom_x;
    result[4] =  cos_f * zoom_y;
    result[5] =  dst_y - src_x * result[3] - src_y * result[4];
  }

  static bool make_invert_affine32(std::int32_t* __restrict__ result, const float* __restrict__ matrix)
  {
    float det = matrix[0] * matrix[4] - matrix[1] * matrix[3];
    if (det == 0.0f) return false;
    det = (1 << FP_SCALE) / det;
    result[0] = roundf(det *  matrix[4]);
    result[1] = roundf(det * -matrix[1]);
    result[2] = roundf(det * (matrix[1] * matrix[5] - matrix[2] * matrix[4]));
    result[3] = roundf(det * -matrix[3]);
    result[4] = roundf(det *  matrix[0]);
    result[5] = roundf(det * (matrix[2] * matrix[3] - matrix[0] * matrix[5]));
    return true;
  }

  void LGFXBase::push_image_rotate_zoom(float dst_x, float dst_y, float src_x, float src_y, float angle, float zoom_x, float zoom_y, std::int32_t w, std::int32_t h, pixelcopy_t* pc)
  {
    float matrix[6];
    make_rotation_matrix(matrix, dst_x + 0.5, dst_y + 0.5, src_x + 0.5, src_y + 0.5, angle, zoom_x, zoom_y);
    push_image_affine(matrix, w, h, pc);
  }

  void LGFXBase::push_image_rotate_zoom_aa(float dst_x, float dst_y, float src_x, float src_y, float angle, float zoom_x, float zoom_y, std::int32_t w, std::int32_t h, pixelcopy_t* pc)
  {
    float matrix[6];
    make_rotation_matrix(matrix, dst_x + 0.5, dst_y + 0.5, src_x + 0.5, src_y + 0.5, angle, zoom_x, zoom_y);
    push_image_affine_aa(matrix, w, h, pc);
  }

  void LGFXBase::push_image_affine(const float* matrix, std::int32_t w, std::int32_t h, pixelcopy_t* pc)
  {
    pc->no_convert = false;
    pc->src_height = h;
    pc->src_width = w;
    pc->src_bitwidth = w;
    if (pc->src_bits < 8) {
      std::uint32_t x_mask = (pc->src_bits == 1) ? 7
                           : (pc->src_bits == 2) ? 3
                                                 : 1;
      pc->src_bitwidth = (w + x_mask) & (~x_mask);
    }
    push_image_affine(matrix, pc);
  }

  void LGFXBase::push_image_affine_aa(const float* matrix, std::int32_t w, std::int32_t h, pixelcopy_t* pc)
  {
    pc->no_convert = false;
    pc->src_height = h;
    pc->src_width = w;
    pc->src_bitwidth = w;
    if (pc->src_bits < 8) {
      std::uint32_t x_mask = (pc->src_bits == 1) ? 7
                           : (pc->src_bits == 2) ? 3
                                                 : 1;
      pc->src_bitwidth = (w + x_mask) & (~x_mask);
    }
    pixelcopy_t pc_post;
    auto dst_depth = getColorDepth();
    pc_post.dst_bits = _write_conv.bits;
    pc_post.dst_mask = (1 << pc_post.dst_bits) - 1;
    if (hasPalette() || pc_post.dst_bits < 8)
    {
      pc_post.fp_copy = pixelcopy_t::blend_palette_fast;
    }
    else
    if (pc_post.dst_bits > 16) {
      if (dst_depth == rgb888_3Byte) {
        pc_post.fp_copy = pixelcopy_t::blend_rgb_fast<bgr888_t>;
      } else {
        pc_post.fp_copy = pixelcopy_t::blend_rgb_fast<bgr666_t>;
      }
    } else {
      if (dst_depth == rgb565_2Byte) {
        pc_post.fp_copy = pixelcopy_t::blend_rgb_fast<swap565_t>;
      } else { // src_depth == rgb332_1Byte:
        pc_post.fp_copy = pixelcopy_t::blend_rgb_fast<rgb332_t>;
      }
    }
    push_image_affine_aa(matrix, pc, &pc_post);
  }

  void LGFXBase::fillAffine(const float matrix[6], std::int32_t w, std::int32_t h)
  {
    std::int32_t min_y = matrix[3] * (w << FP_SCALE);
    std::int32_t max_y = matrix[4] * (h << FP_SCALE);
    if ((min_y < 0) == (max_y < 0))
    {
      max_y += min_y;
      min_y = 0;
    }
    if (min_y > max_y)
    {
      std::swap(min_y, max_y);
    }

    {
      std::int32_t offset_y32 = matrix[5] * (1 << FP_SCALE) + (1 << (FP_SCALE-1));
      min_y = std::max(_clip_t    , (offset_y32 + min_y - 1) >> FP_SCALE);
      max_y = std::min(_clip_b + 1, (offset_y32 + max_y + 1) >> FP_SCALE);
      if (min_y >= max_y) return;
    }


    std::int32_t iA[6];
    if (!make_invert_affine32(iA, matrix)) return;

    std::int32_t offset = (min_y << 1) - 1;
    iA[2] += ((iA[0] + iA[1] * offset) >> 1);
    iA[5] += ((iA[3] + iA[4] * offset) >> 1);

    std::int32_t scale_w = w << FP_SCALE;
    std::int32_t xs1 = (iA[0] < 0 ?   - scale_w :   1) - iA[0];
    std::int32_t xs2 = (iA[0] < 0 ? 0 : (1 - scale_w)) - iA[0];

    std::int32_t scale_h = h << FP_SCALE;
    std::int32_t ys1 = (iA[3] < 0 ?   - scale_h :   1) - iA[3];
    std::int32_t ys2 = (iA[3] < 0 ? 0 : (1 - scale_h)) - iA[3];

    std::int32_t cl = _clip_l    ;
    std::int32_t cr = _clip_r + 1;

    std::int32_t div1 = iA[0] ? - iA[0] : -1;
    std::int32_t div2 = iA[3] ? - iA[3] : -1;
    std::int32_t y = min_y - max_y;

    startWrite();
    do
    {
      iA[2] += iA[1];
      iA[5] += iA[4];
      std::int32_t left  = std::max(cl, std::max((iA[2] + xs1) / div1, (iA[5] + ys1) / div2));
      std::int32_t right = std::min(cr, std::min((iA[2] + xs2) / div1, (iA[5] + ys2) / div2));
      if (left < right)
      {
        writeFillRectPreclipped(left, y + max_y, right - left, 1);
      }
    } while (++y);
    endWrite();
  }

  void LGFXBase::push_image_affine(const float* matrix, pixelcopy_t* pc)
  {
    std::int32_t min_y = matrix[3] * (pc->src_width  << FP_SCALE);
    std::int32_t max_y = matrix[4] * (pc->src_height << FP_SCALE);
    if ((min_y < 0) == (max_y < 0))
    {
      max_y += min_y;
      min_y = 0;
    }
    if (min_y > max_y)
    {
      std::swap(min_y, max_y);
    }

    {
      std::int32_t offset_y32 = matrix[5] * (1 << FP_SCALE) + (1 << (FP_SCALE-1));
      min_y = std::max(_clip_t    , (offset_y32 + min_y - 1) >> FP_SCALE);
      max_y = std::min(_clip_b + 1, (offset_y32 + max_y + 1) >> FP_SCALE);
      if (min_y >= max_y) return;
    }


    std::int32_t iA[6];
    if (!make_invert_affine32(iA, matrix)) return;

    std::int32_t offset = (min_y << 1) - 1;
    iA[2] += ((iA[0] + iA[1] * offset) >> 1);
    iA[5] += ((iA[3] + iA[4] * offset) >> 1);

    std::int32_t scale_w = pc->src_width << FP_SCALE;
    std::int32_t xs1 = (iA[0] < 0 ?   - scale_w :   1) - iA[0];
    std::int32_t xs2 = (iA[0] < 0 ? 0 : (1 - scale_w)) - iA[0];

    std::int32_t scale_h = pc->src_height << FP_SCALE;
    std::int32_t ys1 = (iA[3] < 0 ?   - scale_h :   1) - iA[3];
    std::int32_t ys2 = (iA[3] < 0 ? 0 : (1 - scale_h)) - iA[3];

    std::int32_t cl = _clip_l    ;
    std::int32_t cr = _clip_r + 1;

    std::int32_t y = min_y - max_y;

    startWrite();
    do
    {
      iA[2] += iA[1];
      iA[5] += iA[4];
      std::int32_t left  = std::max(cl, std::max(iA[0] ? (iA[2] + xs1) / - iA[0] : cl, iA[3] ? (iA[5] + ys1) / - iA[3] : cl));
      std::int32_t right = std::min(cr, std::min(iA[0] ? (iA[2] + xs2) / - iA[0] : cr, iA[3] ? (iA[5] + ys2) / - iA[3] : cr));
      if (left < right)
      {
        pc->src_x32 = iA[2] + left * iA[0];
        if (static_cast<std::uint32_t>(pc->src_x) < pc->src_width)
        {
          pc->src_y32 = iA[5] + left * iA[3];
          if (static_cast<std::uint32_t>(pc->src_y) < pc->src_height)
          {
            pc->src_x32_add = iA[0];
            pc->src_y32_add = iA[3];
            _panel->writeImage(left, y + max_y, right - left, 1, pc, true);
          }
        }
      }
    } while (++y);
    endWrite();
  }

  void LGFXBase::push_image_affine_aa(const float* matrix, pixelcopy_t* pc, pixelcopy_t* pc2)
  {
    std::int32_t min_y = matrix[3] * (pc->src_width  << FP_SCALE);
    std::int32_t max_y = matrix[4] * (pc->src_height << FP_SCALE);
    if ((min_y < 0) == (max_y < 0))
    {
      max_y += min_y;
      min_y = 0;
    }
    if (min_y > max_y)
    {
      std::swap(min_y, max_y);
    }

    {
      std::int32_t offset_y32 = matrix[5] * (1 << FP_SCALE);
      min_y = std::max(_clip_t, (offset_y32 + min_y   ) >> FP_SCALE);
      max_y = std::min(_clip_b, (offset_y32 + max_y -1) >> FP_SCALE) + 1;
      if (min_y >= max_y) return;
    }

    std::int32_t iA[6];
    if (!make_invert_affine32(iA, matrix)) return;

    pc->src_x32_add = iA[0];
    pc->src_y32_add = iA[3];
    std::uint32_t x32_diff = std::min<std::uint32_t>(8 << FP_SCALE, std::max(abs(iA[0]), abs(iA[1])) - 1) >> 1;
    std::uint32_t y32_diff = std::min<std::uint32_t>(8 << FP_SCALE, std::max(abs(iA[3]), abs(iA[4])) - 1) >> 1;

    std::int32_t offset = (min_y << 1) - 1;
    iA[2] += ((iA[0] + iA[1] * offset) >> 1);
    iA[5] += ((iA[3] + iA[4] * offset) >> 1);

    std::int32_t scale_w = (pc->src_width << FP_SCALE) + (x32_diff << 1);
    std::int32_t xs1 = (iA[0] < 0 ?   - scale_w :   1) - iA[0] + x32_diff;
    std::int32_t xs2 = (iA[0] < 0 ? 0 : (1 - scale_w)) - iA[0] + x32_diff;

    std::int32_t scale_h = (pc->src_height << FP_SCALE) + (y32_diff << 1);
    std::int32_t ys1 = (iA[3] < 0 ?   - scale_h :   1) - iA[3] + y32_diff;
    std::int32_t ys2 = (iA[3] < 0 ? 0 : (1 - scale_h)) - iA[3] + y32_diff;

    std::int32_t cl = _clip_l    ;
    std::int32_t cr = _clip_r + 1;
    argb8888_t buffer[cr - cl];
    pc2->src_data = buffer;

    startWrite();
    do
    {
      iA[2] += iA[1];
      iA[5] += iA[4];
      std::int32_t left  = std::max(cl, std::max(iA[0] ? (iA[2] + xs1) / - iA[0] : cl, iA[3] ? (iA[5] + ys1) / - iA[3] : cl));
      std::int32_t right = std::min(cr, std::min(iA[0] ? (iA[2] + xs2) / - iA[0] : cr, iA[3] ? (iA[5] + ys2) / - iA[3] : cr));
      if (left < right)
      {
        std::int32_t len = right - left;

        std::uint32_t xs = iA[2] + left * iA[0];
        pc->src_x32 = xs - x32_diff;
        pc->src_xe32 = xs + x32_diff;
        std::uint32_t ys = iA[5] + left * iA[3];
        pc->src_y32 = ys - y32_diff;
        pc->src_ye32 = ys + y32_diff;

        pc->fp_copy(buffer, 0, len, pc);
        pc2->src_x32_add = 1 << pixelcopy_t::FP_SCALE;
        pc2->src_y32_add = 0;
        pc2->src_x32 = 0;
        pc2->src_y32 = 0;
        _panel->writeImageARGB(left, min_y, len, 1, pc2);
      }
    } while (++min_y != max_y);
    endWrite();
  }

  void LGFXBase::readRect(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h, std::uint8_t* data)
  {
    pixelcopy_t p(nullptr, rgb332_t::depth, _read_conv.depth, false, getPalette());
    read_rect(x, y, w, h, data, &p);
  }
  void LGFXBase::readRect(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h, std::uint16_t* data)
  {
    pixelcopy_t p(nullptr, swap565_t::depth, _read_conv.depth, false, getPalette());
    if (_swapBytes && !_palette_count && _read_conv.depth >= 8) {
      p.no_convert = false;
      p.fp_copy = pixelcopy_t::get_fp_copy_rgb_affine_dst<rgb565_t>(_read_conv.depth);
    }
    read_rect(x, y, w, h, data, &p);
  }
  void LGFXBase::readRect(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h, void* data)
  {
    pixelcopy_t p(nullptr, bgr888_t::depth, _read_conv.depth, false, getPalette());
    if (_swapBytes && !_palette_count && _read_conv.depth >= 8) {
      p.no_convert = false;
      p.fp_copy = pixelcopy_t::get_fp_copy_rgb_affine_dst<rgb888_t>(_read_conv.depth);
    }
    read_rect(x, y, w, h, data, &p);
  }

  void LGFXBase::scroll(std::int_fast16_t dx, std::int_fast16_t dy)
  {
    setColor(_base_rgb888);
    std::int32_t w  = _sw - abs(dx);
    std::int32_t h  = _sh - abs(dy);
    if (w < 0 || h < 0)
    {
      writeFillRect(_sx, _sy, _sw, _sh);
      return;
    }

    std::int32_t src_x = dx < 0 ? _sx - dx : _sx;
    std::int32_t dst_x = src_x + dx;
    std::int32_t src_y = dy < 0 ? _sy - dy : _sy;
    std::int32_t dst_y = src_y + dy;

    startWrite();
    _panel->copyRect(dst_x, dst_y, w, h, src_x, src_y);

    if (     dx > 0) writeFillRectPreclipped(_sx           , dst_y,  dx, h);
    else if (dx < 0) writeFillRectPreclipped(_sx + _sw + dx, dst_y, -dx, h);
    if (     dy > 0) writeFillRectPreclipped(_sx, _sy           , _sw,  dy);
    else if (dy < 0) writeFillRectPreclipped(_sx, _sy + _sh + dy, _sw, -dy);
    endWrite();
  }

  void LGFXBase::copyRect(std::int32_t dst_x, std::int32_t dst_y, std::int32_t w, std::int32_t h, std::int32_t src_x, std::int32_t src_y)
  {
    auto wid = width();
    if (src_x < dst_x) { if (src_x < 0) { w += src_x; dst_x -= src_x; src_x = 0; } if (w > wid  - dst_x)  w = wid  - dst_x; }
    else               { if (dst_x < 0) { w += dst_x; src_x -= dst_x; dst_x = 0; } if (w > wid  - src_x)  w = wid  - src_x; }
    if (w < 1) return;

    auto hei = height();
    if (src_y < dst_y) { if (src_y < 0) { h += src_y; dst_y -= src_y; src_y = 0; } if (h > hei - dst_y)  h = hei - dst_y; }
    else               { if (dst_y < 0) { h += dst_y; src_y -= dst_y; dst_y = 0; } if (h > hei - src_y)  h = hei - src_y; }
    if (h < 1) return;

    _panel->copyRect(dst_x, dst_y, w, h, src_x, src_y);
  }

  void LGFXBase::read_rect(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h, void* dst, pixelcopy_t* param)
  {
    _adjust_abs(x, w);
    if (x < 0) { w += x; x = 0; }
    if (w > width() - x)  w = width()  - x;
    if (w < 1) return;

    _adjust_abs(y, h);
    if (y < 0) { h += y; y = 0; }
    if (h > height() - y) h = height() - y;
    if (h < 1) return;

    _panel->readRect(x, y, w, h, dst, param);
  }

  struct paint_point_t { std::int32_t lx,rx,y,oy; };

  static void paint_add_points(std::list<paint_point_t>& points, int lx, int rx, int y, int oy, bool* linebuf)
  {
    paint_point_t pt { 0, 0, y, oy };
    while (lx <= rx) {
      while (lx < rx && !linebuf[lx]) ++lx;
      if (!linebuf[lx]) break;
      pt.lx = lx;
      while (++lx <= rx && linebuf[lx]);
      pt.rx = lx - 1;
      points.push_back(pt);
    }
  }

  void LGFXBase::floodFill(std::int32_t x, std::int32_t y)
  {
    if (x < _clip_l || x > _clip_r || y < _clip_t || y > _clip_b) return;
    bgr888_t target;
    readRectRGB(x, y, 1, 1, &target);
    if (_color.raw == _write_conv.convert(lgfx::color888(target.r, target.g, target.b))) return;

    pixelcopy_t p;
    p.transp = _read_conv.convert(lgfx::color888(target.r, target.g, target.b));
    switch (_read_conv.depth)
    {
    case color_depth_t::rgb888_3Byte: p.fp_copy = pixelcopy_t::compare_rgb_fast<bgr888_t>;  break;
    case color_depth_t::rgb666_3Byte: p.fp_copy = pixelcopy_t::compare_rgb_fast<bgr666_t>;  break;
    case color_depth_t::rgb565_2Byte: p.fp_copy = pixelcopy_t::compare_rgb_fast<swap565_t>; break;
    case color_depth_t::rgb332_1Byte: p.fp_copy = pixelcopy_t::compare_rgb_fast<rgb332_t>;  break;
    default: p.fp_copy = pixelcopy_t::compare_bit_fast;
      p.src_bits = _read_conv.depth;
      p.src_mask = (1 << p.src_bits) - 1;
      p.transp &= p.src_mask;
      break;
    }

    std::int32_t cl = _clip_l;
    int w = _clip_r - cl + 1;
    std::uint8_t bufIdx = 0;
    bool* linebufs[3] = { new bool[w], new bool[w], new bool[w] };
    std::int32_t bufY[3] = {-2, -2, -2};  // 3 line buffer (default: out of range.)
    bufY[0] = y;
    read_rect(cl, y, w, 1, linebufs[0], &p);
    std::list<paint_point_t> points;
    points.push_back({x, x, y, y});

    startWrite();
    while (!points.empty())
    {
      std::int32_t y0 = bufY[bufIdx];
      auto it = points.begin();
      std::int32_t counter = 0;
      while (it->y != y0 && ++it != points.end()) ++counter;
      if (it == points.end())
      {
        if (counter < 256)
        {
          ++bufIdx;
          std::int32_t y1 = bufY[(bufIdx  )%3];
          std::int32_t y2 = bufY[(bufIdx+1)%3];
          it = points.begin();
          while ((it->y != y1) && (it->y != y2) && (++it != points.end()));
        }
      }

      bufIdx = 0;
      if (it == points.end())
      {
        it = points.begin();
        bufY[0] = it->y;
        read_rect(cl, it->y, w, 1, linebufs[0], &p);
      }
      else
      {
        for (; bufIdx < 2; ++bufIdx) if (it->y == bufY[bufIdx]) break;
      }
      bool* linebuf = &linebufs[bufIdx][- cl];

      int lx = it->lx;
      int rx = it->rx;
      int ly = it->y;
      int oy = it->oy;
      points.erase(it);
      if (!linebuf[lx]) continue;

      int lxsav = lx - 1;
      int rxsav = rx + 1;

      int cr = _clip_r;
      while (lx > cl && linebuf[lx - 1]) --lx;
      while (rx < cr && linebuf[rx + 1]) ++rx;

      writeFastHLine(lx, ly, rx - lx + 1);
      memset(&linebuf[lx], 0, rx - lx + 1);

      int newy = ly - 1;
      do
      {
        if (newy == oy && lx >= lxsav && rxsav >= rx) continue;
        if (newy < _clip_t) continue;
        if (newy > _clip_b) continue;
        int bidx = 0;
        while (newy != bufY[bidx] && ++bidx != 3);
        if (bidx == 3) {
          for (bidx = 0; bidx < 2 && (abs(bufY[bidx] - ly) <= 1); ++bidx);
          bufY[bidx] = newy;
          read_rect(cl, newy, w, 1, linebufs[bidx], &p);
        }
        bool* linebuf = &linebufs[bidx][- cl];
        if (newy == oy)
        {
          paint_add_points(points, lx ,lxsav, newy, ly, linebuf);
          paint_add_points(points, rxsav ,rx, newy, ly, linebuf);
        } else {
          paint_add_points(points, lx ,rx, newy, ly, linebuf);
        }
      } while ((newy += 2) < ly + 2);
    }
    int i = 0;
    do { delete[] linebufs[i]; } while (++i != 3);
    endWrite();
  }

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

  static char* numberToStr(unsigned long n, char* buf, std::size_t buflen, std::uint8_t base)
  {
    char *str = &buf[buflen - 1];

    *str = '\0';

    if (base < 2) { base = 10; }  // prevent crash if called with base == 1
    do {
      unsigned long m = n;
      n /= base;
      char c = m - base * n;
      *--str = c < 10 ? c + '0' : c + 'A' - 10;
    } while (n);

    return str;
  }

  static char* numberToStr(long n, char* buf, std::size_t buflen, std::uint8_t base)
  {
    if (n >= 0) return numberToStr((unsigned long) n, buf, buflen, base);
    auto res = numberToStr(- n, buf, buflen, 10) - 1;
    res[0] = '-';
    return res;
  }

  static char* floatToStr(double number, char* buf, std::size_t /*buflen*/, std::uint8_t digits)
  {
    if (std::isnan(number))    { return strcpy(buf, "nan"); }
    if (std::isinf(number))    { return strcpy(buf, "inf"); }
    if (number > 4294967040.0) { return strcpy(buf, "ovf"); } // constant determined empirically
    if (number <-4294967040.0) { return strcpy(buf, "ovf"); } // constant determined empirically

    char* dst = buf;
    // Handle negative numbers
    //bool negative = (number < 0.0);
    if (number < 0.0) {
      number = -number;
      *dst++ = '-';
    }

    // Round correctly so that print(1.999, 2) prints as "2.00"
    double rounding = 0.5;
    for(std::uint8_t i = 0; i < digits; ++i) {
      rounding /= 10.0;
    }

    number += rounding;

    // Extract the integer part of the number and print it
    unsigned long int_part = (unsigned long) number;
    double remainder = number - (double) int_part;

    {
      constexpr std::size_t len = 14;
      char numstr[len];
      auto tmp = numberToStr(int_part, numstr, len, 10);
      auto slen = strlen(tmp);
      memcpy(dst, tmp, slen);
      dst += slen;
    }

    // Print the decimal point, but only if there are digits beyond
    if (digits > 0) {
      dst[0] = '.';
      ++dst;
    }
    // Extract digits from the remainder one at a time
    while (digits-- > 0) {
      remainder *= 10.0;
      unsigned int toPrint = (unsigned int)(remainder);
      dst[0] = '0' + toPrint;
      ++dst;
      remainder -= toPrint;
    }
    dst[0] = 0;
    return buf;
  }

  std::uint16_t LGFXBase::decodeUTF8(std::uint8_t c)
  {
    // 7 bit Unicode Code Point
    if (!(c & 0x80)) {
      _decoderState = utf8_decode_state_t::utf8_state0;
      return c;
    }

    if (_decoderState == utf8_decode_state_t::utf8_state0)
    {
      // 11 bit Unicode Code Point
      if ((c & 0xE0) == 0xC0)
      {
        _unicode_buffer = ((c & 0x1F)<<6);
        _decoderState = utf8_decode_state_t::utf8_state1;
        return 0;
      }

      // 16 bit Unicode Code Point
      if ((c & 0xF0) == 0xE0)
      {
        _unicode_buffer = ((c & 0x0F)<<12);
        _decoderState = utf8_decode_state_t::utf8_state2;
        return 0;
      }
      // 21 bit Unicode  Code Point not supported so fall-back to extended ASCII
      //if ((c & 0xF8) == 0xF0) return (std::uint16_t)c;
    }
    else
    {
      if (_decoderState == utf8_decode_state_t::utf8_state2)
      {
        _unicode_buffer |= ((c & 0x3F)<<6);
        _decoderState = utf8_decode_state_t::utf8_state1;
        return 0;
      }
      _unicode_buffer |= (c & 0x3F);
      _decoderState = utf8_decode_state_t::utf8_state0;
      return _unicode_buffer;
    }

    _decoderState = utf8_decode_state_t::utf8_state0;

    return c; // fall-back to extended ASCII
  }

  std::int32_t LGFXBase::fontHeight(const IFont* font) const
  {
    FontMetrics fm;
    font->getDefaultMetric(&fm);
    return fm.height * _text_style.size_y;
  }

  std::int32_t LGFXBase::textLength(const char *string, std::int32_t width)
  {
    if (!string || !string[0]) return 0;

    auto sx = _text_style.size_x;

    std::int32_t left = 0;
    std::int32_t right = 0;
    auto str = string;
    do {
      std::uint16_t uniCode = *string;
      if (_text_style.utf8) {
        do {
          uniCode = decodeUTF8(*string);
        } while (uniCode < 0x20 && *(++string));
        if (uniCode < 0x20) break;
      }

      //if (!_font->updateFontMetric(&_font_metrics, uniCode)) continue;
      _font->updateFontMetric(&_font_metrics, uniCode);
      if (left == 0 && right == 0 && _font_metrics.x_offset < 0) left = right = - (int)(_font_metrics.x_offset * sx);
      right = left + std::max<int>(_font_metrics.x_advance * sx, int(_font_metrics.width * sx) + int(_font_metrics.x_offset * sx));
      //right = left + (int)(std::max<int>(_font_metrics.x_advance, _font_metrics.width + _font_metrics.x_offset) * sx);
      left += (int)(_font_metrics.x_advance * sx);
      if (width <= right) return string - str;
    } while (*(++string));
    return string - str;
  }

  std::int32_t LGFXBase::textWidth(const char *string)
  {
    if (!string || !string[0]) return 0;

    auto sx = _text_style.size_x;

    std::int32_t left = 0;
    std::int32_t right = 0;
    do {
      std::uint16_t uniCode = *string;
      if (_text_style.utf8) {
        do {
          uniCode = decodeUTF8(*string);
        } while (uniCode < 0x20 && *(++string));
        if (uniCode < 0x20) break;
      }

      //if (!_font->updateFontMetric(&_font_metrics, uniCode)) continue;
      _font->updateFontMetric(&_font_metrics, uniCode);
      if (left == 0 && right == 0 && _font_metrics.x_offset < 0) left = right = - (int)(_font_metrics.x_offset * sx);
      right = left + std::max<int>(_font_metrics.x_advance*sx, int(_font_metrics.width*sx) + int(_font_metrics.x_offset * sx));
      //right = left + (int)(std::max<int>(_font_metrics.x_advance, _font_metrics.width + _font_metrics.x_offset) * sx);
      left += (int)(_font_metrics.x_advance * sx);
    } while (*(++string));
    return right;
  }


  std::size_t LGFXBase::drawNumber(long long_num, std::int32_t poX, std::int32_t poY)
  {
    constexpr std::size_t len = 8 * sizeof(long) + 1;
    char buf[len];
    return drawString(numberToStr(long_num, buf, len, 10), poX, poY);
  }

  std::size_t LGFXBase::drawFloat(float floatNumber, std::uint8_t dp, std::int32_t poX, std::int32_t poY)
  {
    std::size_t len = 14 + dp;
    char buf[len];
    return drawString(floatToStr(floatNumber, buf, len, dp), poX, poY);
  }

  std::size_t LGFXBase::drawChar(std::uint16_t uniCode, std::int32_t x, std::int32_t y, std::uint8_t font) {
    if (_font == fontdata[font]) return drawChar(uniCode, x, y);
    _filled_x = 0;
    return fontdata[font]->drawChar(this, x, y, uniCode, &_text_style);
  }


  std::size_t LGFXBase::draw_string(const char *string, std::int32_t x, std::int32_t y, textdatum_t datum)
  {
    std::int16_t sumX = 0;
    std::int32_t cwidth = textWidth(string); // Find the pixel width of the string in the font
    std::int32_t cheight = _font_metrics.height * _text_style.size_y;

    if (string && string[0]) {
      auto tmp = string;
      do {
        std::uint16_t uniCode = *tmp;
        if (_text_style.utf8) {
          do {
            uniCode = decodeUTF8(*tmp);
          } while (uniCode < 0x20 && *++tmp);
          if (uniCode < 0x20) break;
        }
        //if (_font->updateFontMetric(&_font_metrics, uniCode))
        {
          _font->updateFontMetric(&_font_metrics, uniCode);
          if (_font_metrics.x_offset < 0) sumX = - _font_metrics.x_offset * _text_style.size_x;
          break;
        }
      } while (*++tmp);
    }
    if (datum & middle_left) {          // vertical: middle
      y -= cheight >> 1;
    } else if (datum & bottom_left) {   // vertical: bottom
      y -= cheight;
    } else if (datum & baseline_left) { // vertical: baseline
      y -= (int)(_font_metrics.baseline * _text_style.size_y);
    }

    this->startWrite();
    std::int32_t padx = _padding_x;
    if ((_text_style.fore_rgb888 != _text_style.back_rgb888) && (padx > cwidth)) {
      this->setColor(_text_style.back_rgb888);
      if (datum & top_center) {
        auto halfcwidth = cwidth >> 1;
        auto halfpadx = (padx >> 1);
        this->writeFillRect(x - halfpadx, y, halfpadx - halfcwidth, cheight);
        halfcwidth = cwidth - halfcwidth;
        halfpadx = padx - halfpadx;
        this->writeFillRect(x + halfcwidth, y, halfpadx - halfcwidth, cheight);
      } else if (datum & top_right) {
        this->writeFillRect(x - padx, y, padx - cwidth, cheight);
      } else {
        this->writeFillRect(x + cwidth, y, padx - cwidth, cheight);
      }
    }

    if (datum & top_center) {           // Horizontal: middle
      x -= cwidth >> 1;
    } else if (datum & top_right) {     // Horizontal: right
      x -= cwidth;
    }

    y -= int(_font_metrics.y_offset * _text_style.size_y);

    _filled_x = 0;
    if (string && string[0]) {
      do {
        std::uint16_t uniCode = *string;
        if (_text_style.utf8) {
          do {
            uniCode = decodeUTF8(*string);
          } while (uniCode < 0x20 && *++string);
          if (uniCode < 0x20) break;
        }
//          sumX += (fpDrawChar)(this, x + sumX, y, uniCode, &_text_style, _font);
        sumX += _font->drawChar(this, x + sumX, y, uniCode, &_text_style);
      } while (*(++string));
    }
    this->endWrite();

    return sumX;
  }

  std::size_t LGFXBase::write(std::uint8_t utf8)
  {
    if (utf8 == '\r') return 1;
    if (utf8 == '\n') {
      _filled_x = (_textscroll) ? this->_sx : 0;
      _cursor_x = _filled_x;
      _cursor_y += _font_metrics.y_advance * _text_style.size_y;
    } else {
      std::uint16_t uniCode = utf8;
      if (_text_style.utf8) {
        uniCode = decodeUTF8(utf8);
        if (uniCode < 0x20) return 1;
      }
      //if (!(fpUpdateFontSize)(this, uniCode)) return 1;
      //if (!_font->updateFontMetric(&_font_metrics, uniCode)) return 1;
      _font->updateFontMetric(&_font_metrics, uniCode);

      std::int32_t xo = _font_metrics.x_offset  * _text_style.size_x;
      std::int32_t w  = std::max(xo + _font_metrics.width * _text_style.size_x, _font_metrics.x_advance * _text_style.size_x);
      if (_textscroll || _textwrap_x) {
        std::int32_t llimit = _textscroll ? this->_sx : this->_clip_l;
        std::int32_t rlimit = _textscroll ? this->_sx + this->_sw : (this->_clip_r + 1);
        if (_cursor_x + w > rlimit) {
          _filled_x = llimit;
          _cursor_x = llimit - std::min<std::int32_t>(0, xo);
          _cursor_y += _font_metrics.y_advance * _text_style.size_y;
        }
        if (_cursor_x < llimit - xo) _cursor_x = llimit - xo;
      }

      std::int32_t h  = _font_metrics.height * _text_style.size_y;

      std::int32_t ydiff = 0;
      if (_text_style.datum & middle_left) {          // vertical: middle
        ydiff -= h >> 1;
      } else if (_text_style.datum & bottom_left) {   // vertical: bottom
        ydiff -= h;
      } else if (_text_style.datum & baseline_left) { // vertical: baseline
        ydiff -= (int)(_font_metrics.baseline * _text_style.size_y);
      }
      std::int32_t y = _cursor_y + ydiff;

      if (_textscroll) {
        if (y < this->_sy) y = this->_sy;
        else {
          int yshift = (this->_sy + this->_sh) - (y + h);
          if (yshift < 0) {
            this->scroll(0, yshift);
            y += yshift;
          }
        }
      } else if (_textwrap_y) {
        if (y + h > (this->_clip_b + 1)) {
          y = this->_clip_t;
        } else
        if (y < this->_clip_t) y = this->_clip_t;
      }
      _cursor_y = y - ydiff;
      y -= int(_font_metrics.y_offset  * _text_style.size_y);
      //_cursor_x += (fpDrawChar)(this, _cursor_x, y, uniCode, &_text_style, _font);
      _cursor_x += _font->drawChar(this, _cursor_x, y, uniCode, &_text_style);
    }

    return 1;
  }

  std::size_t LGFXBase::printNumber(unsigned long n, std::uint8_t base)
  {
    std::size_t len = 8 * sizeof(long) + 1;
    char buf[len];
    return write(numberToStr(n, buf, len, base));
  }

  std::size_t LGFXBase::printFloat(double number, std::uint8_t digits)
  {
    std::size_t len = 14 + digits;
    char buf[len];
    return write(floatToStr(number, buf, len, digits));
  }

  std::size_t LGFXBase::vprintf(const char *format, va_list arg)
  {
    char loc_buf[64];
    char * temp = loc_buf;
    va_list copy;
    va_copy(copy, arg);
    std::size_t len = vsnprintf(temp, sizeof(loc_buf), format, copy);
    if (len >= sizeof(loc_buf)){
      temp = (char*) malloc(len+1);
      if (temp == nullptr) {
        return 0;
      }
      len = vsnprintf(temp, len+1, format, arg);
    }
    len = write((std::uint8_t*)temp, len);
    if (temp != loc_buf){
      free(temp);
    }
    return len;
  }

  #if !defined (ARDUINO)
  std::size_t LGFXBase::printf(const char * format, ...) 
  {
    va_list arg_ptr;
    va_start(arg_ptr, format);
    std::size_t len = vprintf(format, arg_ptr);
    va_end(arg_ptr);

    return len;
  }
  #endif

  void LGFXBase::setFont(const IFont* font)
  {
    if (_font == font) return;

    _runtime_font.reset();
    if (font == nullptr) font = &fonts::Font0;
    _font = font;
    //_decoderState = utf8_decode_state_t::utf8_state0;

    font->getDefaultMetric(&_font_metrics);

    //switch (font->getType()) {
    //default:
    //case IFont::font_type_t::ft_glcd: fpDrawChar = drawCharGLCD;  break;
    //case IFont::font_type_t::ft_bmp:  fpDrawChar = drawCharBMP;   break;
    //case IFont::font_type_t::ft_rle:  fpDrawChar = drawCharRLE;   break;
    //case IFont::font_type_t::ft_bdf:  fpDrawChar = drawCharBDF;   break;
    //case IFont::font_type_t::ft_gfx:  fpDrawChar = drawCharGFXFF; break;
    //}
  }

  /// load VLW font
  bool LGFXBase::loadFont(const std::uint8_t* array)
  {
    _font_data.set(array);
    return load_font(&_font_data);
  }

  bool LGFXBase::load_font(lgfx::DataWrapper* data)
  {
    this->unloadFont();
    bool result = false;

#ifdef LGFX_TTFFONT_HPP_
// TTF support.
    std::uint8_t buf[4];
    data->seek(0);
    data->read(buf, 4);
    data->seek(0);
    if ((buf[0] == 0 && buf[1] == 1 && buf[2] == 0 && buf[3] == 0)    // ttf
     || (buf[0] == 't' && buf[1] == 't' && buf[2] == 'c' && buf[3] == 'f'))  // ttc
    {
      this->_runtime_font.reset(new TTFfont());
    }
    else
#endif
    {
      this->_runtime_font.reset(new VLWfont());
    }

    if (this->_runtime_font->loadFont(data)) {
      result = true;
      this->_font = this->_runtime_font.get();
      this->_font->getDefaultMetric(&this->_font_metrics);
    } else {
      this->unloadFont();
    }
    return result;
  }

  void LGFXBase::unloadFont(void)
  {
    if (_runtime_font.get() != nullptr) { setFont(&fonts::Font0); }
  }

  void LGFXBase::showFont(std::uint32_t td)
  {
    auto font = (const VLWfont*)this->_font;
    if (!font->_fontLoaded) return;

    std::int16_t x = this->width();
    std::int16_t y = this->height();
    std::uint32_t timeDelay = 0;    // No delay before first page

    this->fillScreen(this->_text_style.back_rgb888);

    for (std::uint16_t i = 0; i < font->gCount; i++)
    {
      // Check if this will need a new screen
      if (x + font->gdX[i] + font->gWidth[i] >= this->width())  {
        x = - font->gdX[i];

        y += font->yAdvance;
        if (y + font->maxAscent + font->descent >= this->height()) {
          x = - font->gdX[i];
          y = 0;
          delay(timeDelay);
          timeDelay = td;
          this->fillScreen(this->_text_style.back_rgb888);
        }
      }

      this->drawChar(font->gUnicode[i], x, y);
      x += font->gxAdvance[i];
      //yield();
    }

    delay(timeDelay);
    this->fillScreen(this->_text_style.back_rgb888);
    //fontFile.close();
  }

  void LGFXBase::setAttribute(attribute_t attr_id, std::uint8_t param) {
    switch (attr_id) {
    case cp437_switch:
      _text_style.cp437 = param;
      break;
    case utf8_switch:
      _text_style.utf8  = param;
      _decoderState = utf8_decode_state_t::utf8_state0;
      break;
    case epd_mode_switch:
      _panel->setEpdMode((epd_mode_t)param);
      break;
    default: break;
    }
  }

  std::uint8_t LGFXBase::getAttribute(attribute_t attr_id) {
    switch (attr_id) {
      case cp437_switch: return _text_style.cp437;
      case utf8_switch: return _text_style.utf8;
      case epd_mode_switch: return _panel->getEpdMode();
      default: return 0;
    }
  }

//----------------------------------------------------------------------------

  void LGFXBase::qrcode(const char *string, std::int32_t x, std::int32_t y, std::int32_t w, std::uint8_t version) {
    if (w == -1) {
      w = std::min(width(), height()) * 9 / 10;
    }
    if (x == -1 || y == -1) {
      x = (width()  - w) >> 1;
      y = (height() - w) >> 1;
    }

    setColor(0xFFFFFFU);
    startWrite();
    writeFillRect(x, y, w, w);
    for (; version <= 40; ++version) {
      QRCode qrcode;
      std::uint8_t qrcodeData[lgfx_qrcode_getBufferSize(version)];
      if (0 != lgfx_qrcode_initText(&qrcode, qrcodeData, version, 0, string)) continue;
      std::int_fast16_t thickness = w / qrcode.size;
      if (!thickness) break;
      std::int_fast16_t lineLength = qrcode.size * thickness;
      std::int_fast16_t xOffset = x + ((w - lineLength) >> 1);
      std::int_fast16_t yOffset = y + ((w - lineLength) >> 1);
      setColor(0);
      y = 0;
      do {
        x = 0;
        do {
          if (lgfx_qrcode_getModule(&qrcode, x, y)) writeFillRect(x * thickness + xOffset, y * thickness + yOffset, thickness, thickness);
        } while (++x < qrcode.size);
      } while (++y < qrcode.size);
      break;
    }
    endWrite();
  }

//----------------------------------------------------------------------------

  bool LGFXBase::draw_bmp(DataWrapper* data, std::int32_t x, std::int32_t y, std::int32_t maxWidth, std::int32_t maxHeight, std::int32_t offX, std::int32_t offY, float scale_x, float scale_y, datum_t datum)
  {
    bitmap_header_t bmpdata;
    if (!bmpdata.load_bmp_header(data) || (bmpdata.biCompression > 3)) {
      return false;
    }

    std::uint32_t seekOffset = bmpdata.bfOffBits;
    uint_fast16_t bpp = bmpdata.biBitCount; // 24 bcBitCount 24=RGB24bit
    std::int32_t w = bmpdata.biWidth;
    std::int32_t h = abs(bmpdata.biHeight);  // bcHeight Image height (pixels)

    const auto cl = this->_clip_l;
    const auto cr = this->_clip_r + 1;
    const auto ct = this->_clip_t;
    const auto cb = this->_clip_b + 1;

    if (scale_y <= 0.0f || scale_x <= 0.0f)
    {
      float fit_width  = (maxWidth  > 0) ? maxWidth  : cr - cl;
      float fit_height = (maxHeight > 0) ? maxHeight : cb - ct;

      if (scale_x <= -1.0f) { scale_x = fit_width  / w; }
      if (scale_y <= -1.0f) { scale_y = fit_height / h; }
      if (scale_x <= 0.0f)
      {
        if (scale_y <= 0.0f)
        {
          scale_y = std::min<float>(fit_width / w, fit_height / h);
        }
        scale_x = scale_y;
      }
      if (scale_y <= 0.0f)
      {
        scale_y = scale_x;
      }
    }
    if (maxWidth  <= 0) maxWidth  = INT16_MAX;
    if (maxHeight <= 0) maxHeight = INT16_MAX;

    auto clip_x = x;
    auto clip_y = y;

    if (datum)
    {
      if (datum & (datum_t::top_center | datum_t::top_right))
      {
        float fw = maxWidth - (w * scale_x);
        if (datum & datum_t::top_center) { fw /= 2; }
        x += fw;
      }
      if (datum & (datum_t::middle_left | datum_t::bottom_left | datum_t::baseline_left))
      {
        float fh = maxHeight - (h * scale_y);
        if (datum & datum_t::middle_left) { fh /= 2; }
        y += fh;
      }
    }

    if (0 > clip_x - cl) { maxWidth += clip_x - cl; clip_x = cl; }
    if (maxWidth > (cr - clip_x)) maxWidth = (cr - clip_x);

    if (0 > clip_y - ct) { maxHeight += clip_y - ct; clip_y = ct; }
    if (maxHeight > (cb - clip_y)) maxHeight = (cb - clip_y);

    if (maxWidth > 0 && maxHeight > 0)
    {
      this->setClipRect(clip_x, clip_y, maxWidth, maxHeight);
      argb8888_t *palette = nullptr;
      if (bpp <= 8) {
        palette = (argb8888_t*)heap_alloc(sizeof(argb8888_t*) * (1 << bpp));
        data->seek(bmpdata.biSize + 14);
        data->read((std::uint8_t*)palette, (1 << bpp)*sizeof(argb8888_t)); // load palette
      }

      data->seek(seekOffset);

      auto dst_depth = this->_write_conv.depth;
      std::uint32_t buffersize = ((w * bpp + 31) >> 5) << 2;  // readline 4Byte align.
      std::uint8_t lineBuffer[buffersize + 4];
      pixelcopy_t p(lineBuffer, dst_depth, (color_depth_t)bpp, this->_palette_count, palette);
      p.no_convert = false;
      if (8 >= bpp && !this->_palette_count) {
        p.fp_copy = pixelcopy_t::get_fp_copy_palette_affine<argb8888_t>(dst_depth);
      } else {
        if (bpp == 16) {
          p.fp_copy = pixelcopy_t::get_fp_copy_rgb_affine<rgb565_t>(dst_depth);
        } else if (bpp == 24) {
          p.fp_copy = pixelcopy_t::get_fp_copy_rgb_affine<rgb888_t>(dst_depth);
        } else if (bpp == 32) {
          p.fp_copy = pixelcopy_t::get_fp_copy_rgb_affine<argb8888_t>(dst_depth);
        }
      }
      p.src_x32_add = (1u << FP_SCALE) / scale_x;

        //If the value of Height is positive, the image data is from bottom to top
        //If the value of Height is negative, the image data is from top to bottom.
      std::int32_t flow = (bmpdata.biHeight > 0) ? -1 : 1;
      if (bmpdata.biHeight > 0) y += ceilf(h * scale_y) - 1;

      x -= offX;
      y -= offY;

      std::int32_t y32 = (y << FP_SCALE);
      std::int32_t dst_y32_add = (1u << FP_SCALE) * scale_y;
      if (bmpdata.biHeight > 0) dst_y32_add = - dst_y32_add;

      this->startWrite(!data->hasParent());

      float affine[6] = { scale_x, 0.0f, (float)x, 0.0f, 1.0f, 0.0f };
      p.src_bitwidth = w;
      p.src_width = w;
      p.src_height = 1;

      do
      {
        data->preRead();
        if (bmpdata.biCompression == 1)
        {
          bmpdata.load_bmp_rle8(data, lineBuffer, w);
        } else
        if (bmpdata.biCompression == 2)
        {
          bmpdata.load_bmp_rle4(data, lineBuffer, w);
        }
        else
        {
          data->read(lineBuffer, buffersize);
        }
        data->postRead();
        y32 += dst_y32_add;
        std::int32_t next_y = y32 >> FP_SCALE;
        while (y != next_y)
        {
          p.src_x32 = 0;
          affine[5] = y;
          this->push_image_affine(affine, &p);
          y += flow;
        }
      } while (--h);

      if (palette != nullptr) heap_free(palette);
      this->_clip_l = cl;
      this->_clip_t = ct;
      this->_clip_r = cr-1;
      this->_clip_b = cb-1;
      this->endWrite();
    }
    return true;
  }


  struct draw_jpg_info_t
  {
    std::int32_t x;
    std::int32_t y;
    DataWrapper *data;
    LGFXBase *lgfx;
    pixelcopy_t *pc;
    float zoom_x;
    float zoom_y;
  };

  static std::uint32_t jpg_read_data(lgfxJdec  *decoder, std::uint8_t *buf, std::uint32_t len)
  {
    auto jpeg = (draw_jpg_info_t *)decoder->device;
    auto data = (DataWrapper*)jpeg->data;
    auto res = len;
    data->preRead();
    if (buf) {
      res = data->read(buf, len);
    } else {
      data->skip(len);
    }
    return res;
  }

  static std::uint32_t jpg_push_image(lgfxJdec *decoder, void *bitmap, JRECT *rect)
  {
    draw_jpg_info_t *jpeg = static_cast<draw_jpg_info_t*>(decoder->device);
    jpeg->pc->src_data = bitmap;
    auto data = static_cast<DataWrapper*>(jpeg->data);
    data->postRead();
    std::int32_t x = rect->left;
    std::int32_t y = rect->top;
    std::int32_t w = rect->right  - rect->left + 1;
    std::int32_t h = rect->bottom - rect->top + 1;
    jpeg->lgfx->pushImage( jpeg->x + x
                         , jpeg->y + y
                         , w
                         , h
                         , jpeg->pc
                         , false);
    return 1;
  }

  static std::uint32_t jpg_push_image_affine(lgfxJdec *decoder, void *bitmap, JRECT *rect)
  {
    draw_jpg_info_t *jpeg = static_cast<draw_jpg_info_t*>(decoder->device);
    jpeg->pc->src_data = bitmap;
    auto data = static_cast<DataWrapper*>(jpeg->data);
    data->postRead();

    std::int32_t x = rect->left;
    std::int32_t y = rect->top;
    std::int32_t w = rect->right  - rect->left + 1;
    std::int32_t h = rect->bottom - rect->top + 1;
    float affine[6] =
    { jpeg->zoom_x, 0.0f , x * jpeg->zoom_x + jpeg->x
    , 0.0f , jpeg->zoom_y, y * jpeg->zoom_y + jpeg->y
    };
    jpeg->lgfx->pushImageAffine( affine, w, h, (bgr888_t*)jpeg->pc->src_data );
    return 1;
  }

  bool LGFXBase::draw_jpg(DataWrapper* data, std::int32_t x, std::int32_t y, std::int32_t maxWidth, std::int32_t maxHeight, std::int32_t offX, std::int32_t offY, float scale_x, float scale_y, datum_t datum)
  {
    draw_jpg_info_t jpeg;
    pixelcopy_t pc(nullptr, this->getColorDepth(), bgr888_t::depth, this->hasPalette());
    jpeg.pc = &pc;
    jpeg.lgfx = this;
    jpeg.data = data;
    jpeg.x = x - offX;
    jpeg.y = y - offY;

    //TJpgD jpegdec;
    lgfxJdec jpegdec;

    static constexpr std::uint16_t sz_pool = 3100;
    std::uint8_t *pool = (std::uint8_t*)heap_alloc_dma(sz_pool);
    if (!pool)
    {
      return false;
    }

    auto jres = lgfx_jd_prepare(&jpegdec, jpg_read_data, pool, sz_pool, &jpeg);

    if (jres != JDR_OK)
    {
      heap_free(pool);
      return false;
    }

    const auto cl = this->_clip_l;
    const auto cr = this->_clip_r + 1;
    const auto ct = this->_clip_t;
    const auto cb = this->_clip_b + 1;

    if (scale_y <= 0.0f || scale_x <= 0.0f)
    {
      float fit_width  = (maxWidth  > 0) ? maxWidth  : cr - cl;
      float fit_height = (maxHeight > 0) ? maxHeight : cb - ct;

      if (scale_x <= -1.0f) { scale_x = fit_width  / jpegdec.width; }
      if (scale_y <= -1.0f) { scale_y = fit_height / jpegdec.height; }
      if (scale_x <= 0.0f)
      {
        if (scale_y <= 0.0f)
        {
          scale_y = std::min<float>(fit_width / jpegdec.width, fit_height / jpegdec.height);
        }
        scale_x = scale_y;
      }
      if (scale_y <= 0.0f)
      {
        scale_y = scale_x;
      }
    }
    if (maxWidth  <= 0) maxWidth  = INT16_MAX;
    if (maxHeight <= 0) maxHeight = INT16_MAX;

    if (datum)
    {
      if (datum & (datum_t::top_center | datum_t::top_right))
      {
        float w = maxWidth - (jpegdec.width * scale_x);
        if (datum & datum_t::top_center) { w /= 2; }
        jpeg.x += w;
      }
      if (datum & (datum_t::middle_left | datum_t::bottom_left | datum_t::baseline_left))
      {
        float h = maxHeight - (jpegdec.height * scale_y);
        if (datum & datum_t::middle_left) { h /= 2; }
        jpeg.y += h;
      }
    }

    jpeg_div::jpeg_div_t div = jpeg_div::jpeg_div_t::JPEG_DIV_NONE;
    float scale_max = std::max(scale_x, scale_y);
    if (scale_max <= 0.5f)
    {
      if (     scale_max <= 0.125f) { div = jpeg_div::jpeg_div_t::JPEG_DIV_8; }
      else if (scale_max <= 0.25f)  { div = jpeg_div::jpeg_div_t::JPEG_DIV_4; }
      else                          { div = jpeg_div::jpeg_div_t::JPEG_DIV_2; }

      scale_x *= 1 << div;
      scale_y *= 1 << div;
    }

    jpeg.zoom_x = scale_x;
    jpeg.zoom_y = scale_y;

    if (0 > x - cl) { maxWidth += x - cl; x = cl; }
    if (maxWidth > (cr - x)) maxWidth = (cr - x);

    if (0 > y - ct) { maxHeight += y - ct; y = ct; }
    if (maxHeight > (cb - y)) maxHeight = (cb - y);

    if (maxWidth > 0 && maxHeight > 0)
    {
      this->setClipRect(x, y, maxWidth, maxHeight);
      this->startWrite(!data->hasParent());

      jres = lgfx_jd_decomp(&jpegdec, jpeg.zoom_x == 1.0f && jpeg.zoom_y == 1.0f ? jpg_push_image : jpg_push_image_affine, div);

      this->_clip_l = cl;
      this->_clip_t = ct;
      this->_clip_r = cr-1;
      this->_clip_b = cb-1;
      this->endWrite();
    }
    heap_free(pool);

    if (jres != JDR_OK) {
//ESP_LOGE("LGFX","jpeg decomp error:%x", jres);
      return false;
    }
    return true;
  }


  struct png_file_decoder_t
  {
    std::int32_t x;
    std::int32_t y;
    std::int32_t offX;
    std::int32_t offY;
    std::int32_t maxWidth;
    std::int32_t maxHeight;
    float zoom_x;
    float zoom_y;
    datum_t datum;
    bgr888_t* lineBuffer;
    pixelcopy_t *pc;
    LGFXBase *gfx;
    std::uint32_t last_pos;
    std::uint32_t last_x;
    std::int32_t scale_y0;
    std::int32_t scale_y1;
  };

  static bool png_ypos_update(png_file_decoder_t *p, std::uint32_t y)
  {
    p->last_pos = y;
    p->scale_y0 = ceilf( y      * p->zoom_y) - p->offY;
    if (p->scale_y0 < 0) p->scale_y0 = 0;
    p->scale_y1 = ceilf((y + 1) * p->zoom_y) - p->offY;
    if (p->scale_y1 > p->maxHeight) p->scale_y1 = p->maxHeight;
    return (p->scale_y0 < p->scale_y1);
  }

  static void png_post_line(png_file_decoder_t *p)
  {
    std::int32_t h = p->scale_y1 - p->scale_y0;
    if (0 < h)
      p->gfx->pushImage(p->x, p->y + p->scale_y0, p->maxWidth, h, p->pc, true);
  }

  static void png_prepare_line(png_file_decoder_t *p, std::uint32_t y)
  {
    if (png_ypos_update(p, y))      // read next line
      p->gfx->readRectRGB(p->x, p->y + p->scale_y0, p->maxWidth, p->scale_y1 - p->scale_y0, p->lineBuffer);
  }

  static void png_done_callback(pngle_t *pngle)
  {
    auto p = (png_file_decoder_t *)lgfx_pngle_get_user_data(pngle);
    png_post_line(p);
  }

  static void png_draw_normal_callback(pngle_t *pngle, std::uint32_t x, std::uint32_t y, std::uint8_t rgba[4])
  {
    auto p = (png_file_decoder_t*)lgfx_pngle_get_user_data(pngle);

    std::int32_t l = x - p->offX;
    if (l < 0 || l >= p->maxWidth) return;
    x = p->x + l;

    if (x != p->last_pos) {
      std::int32_t t = y - p->offY;
      if (t < 0 || t >= p->maxHeight) return;
      p->gfx->setAddrWindow(x, p->y + t, p->maxWidth, 1);
    }
    p->last_pos = x + 1;
    p->gfx->writeColor(color888(rgba[0], rgba[1], rgba[2]), 1);
  }

  static void png_draw_normal_scale_callback(pngle_t *pngle, std::uint32_t x, std::uint32_t y, std::uint8_t rgba[4])
  {
    auto p = (png_file_decoder_t*)lgfx_pngle_get_user_data(pngle);

    if (y != p->last_pos) {
      png_ypos_update(p, y);
    }

    std::int32_t t = p->scale_y0;
    std::int32_t h = p->scale_y1 - t;
    if (h <= 0) return;

    std::int32_t l = ceilf( x      * p->zoom_x) - p->offX;
    if (l < 0) l = 0;
    std::int32_t r = ceilf((x + 1) * p->zoom_x) - p->offX;
    if (r > p->maxWidth) r = p->maxWidth;
    if (l >= r) return;

    p->gfx->setColor(color888(rgba[0], rgba[1], rgba[2]));
    p->gfx->writeFillRectPreclipped(p->x + l, p->y + t, r - l, h);
  }

  static void png_draw_alpha_callback(pngle_t *pngle, std::uint32_t x, std::uint32_t y, std::uint8_t rgba[4])
  {
    auto p = (png_file_decoder_t*)lgfx_pngle_get_user_data(pngle);
    if (y != p->last_pos) {
      png_post_line(p);
      png_prepare_line(p, y);
    }

    if (p->scale_y0 >= p->scale_y1) return;

    std::int32_t l = std::max<std::int32_t>(( x      ) - p->offX, 0);
    std::int32_t r = std::min<std::int32_t>(((x + 1) ) - p->offX, p->maxWidth);
    if (l >= r) return;

    if (rgba[3] == 255) {
      memcpy(&p->lineBuffer[l], rgba, 3);
    } else {
      auto data = &p->lineBuffer[l];
      uint_fast8_t inv = 256 - rgba[3];
      uint_fast8_t alpha = rgba[3] + 1;
      data->r = (rgba[0] * alpha + data->r * inv) >> 8;
      data->g = (rgba[1] * alpha + data->g * inv) >> 8;
      data->b = (rgba[2] * alpha + data->b * inv) >> 8;
    }
  }

  static void png_draw_alpha_scale_callback(pngle_t *pngle, std::uint32_t x, std::uint32_t y, std::uint8_t rgba[4])
  {
    auto p = (png_file_decoder_t*)lgfx_pngle_get_user_data(pngle);
    if (y != p->last_pos) {
      png_post_line(p);
      png_prepare_line(p, y);
    }

    std::int32_t b = p->scale_y1 - p->scale_y0;
    if (b <= 0) return;

    std::int32_t l = ceilf( x      * p->zoom_x) - p->offX;
    if (l < 0) l = 0;
    std::int32_t r = ceilf((x + 1) * p->zoom_x) - p->offX;
    if (r > p->maxWidth) r = p->maxWidth;
    if (l >= r) return;

    if (rgba[3] == 255) {
      std::int32_t i = l;
      do {
        for (std::int32_t j = 0; j < b; ++j) {
          auto data = &p->lineBuffer[i + j * p->maxWidth];
          memcpy(data, rgba, 3);
        }
      } while (++i < r);
    } else {
      uint_fast8_t inv = 256 - rgba[3];
      uint_fast8_t alpha = rgba[3] + 1;
      std::int32_t i = l;
      do {
        for (std::int32_t j = 0; j < b; ++j) {
          auto data = &p->lineBuffer[i + j * p->maxWidth];
          data->r = (rgba[0] * alpha + data->r * inv) >> 8;
          data->g = (rgba[1] * alpha + data->g * inv) >> 8;
          data->b = (rgba[2] * alpha + data->b * inv) >> 8;
        }
      } while (++i < r);
    }
  }

  static void png_init_callback(pngle_t *pngle, std::uint32_t w, std::uint32_t h, uint_fast8_t hasTransparent)
  {
    auto p = (png_file_decoder_t*)lgfx_pngle_get_user_data(pngle);
    auto me = p->gfx;

    std::int32_t cw, ch, cl, ct;
    me->getClipRect(&cl, &ct, &cw, &ch);

    if (p->zoom_y <= 0.0f || p->zoom_x <= 0.0f)
    {
      float fit_width  = (p->maxWidth  > 0) ? p->maxWidth  : cw;
      float fit_height = (p->maxHeight > 0) ? p->maxHeight : ch;

      if (p->zoom_x <= -1.0f) { p->zoom_x = fit_width  / w; }
      if (p->zoom_y <= -1.0f) { p->zoom_y = fit_height / h; }
      if (p->zoom_x <= 0.0f)
      {
        if (p->zoom_y <= 0.0f)
        {
          p->zoom_y = std::min<float>(fit_width / w, fit_height / h);
        }
        p->zoom_x = p->zoom_y;
      }
      if (p->zoom_y <= 0.0f)
      {
        p->zoom_y = p->zoom_x;
      }
    }
    if (p->maxWidth  <= 0) p->maxWidth  = INT16_MAX;
    if (p->maxHeight <= 0) p->maxHeight = INT16_MAX;

    w = ceilf(w * p->zoom_x);
    h = ceilf(h * p->zoom_y);

    if (p->datum)
    {
      if (p->datum & (datum_t::top_center | datum_t::top_right))
      {
        float fw = p->maxWidth - (std::int32_t)w;
        if (p->datum & datum_t::top_center) { fw /= 2; }
        p->offX -= fw;
      }
      if (p->datum & (datum_t::middle_left | datum_t::bottom_left | datum_t::baseline_left))
      {
        float fh = p->maxHeight - (std::int32_t)h;
        if (p->datum & datum_t::middle_left) { fh /= 2; }
        p->offY -= fh;
      }
    }

    const std::int32_t cr = cw + cl;
    const std::int32_t cb = ch + ct;

    if (0 > p->x - cl) { p->maxWidth += p->x - cl; p->offX -= p->x - cl; p->x = cl; }
    if (0 > p->offX) { p->x -= p->offX; p->maxWidth  += p->offX; p->offX = 0; }
    if (p->maxWidth > (cr - p->x)) p->maxWidth = (cr - p->x);

    std::int32_t ww = w - abs(p->offX);
    if (p->maxWidth > ww) p->maxWidth = ww;
    if (p->maxWidth < 0) return;

    if (0 > p->y - ct) { p->maxHeight += p->y - ct; p->offY -= p->y - ct; p->y = ct; }
    if (0 > p->offY) { p->y -= p->offY; p->maxHeight += p->offY; p->offY = 0; }
    if (p->maxHeight > (cb - p->y)) p->maxHeight = (cb - p->y);

    std::int32_t hh = h - abs(p->offY);
    if (p->maxHeight > hh) p->maxHeight = hh;
    if (p->maxHeight < 0) return;

    if (hasTransparent)
    { // need pixel read ?
      p->lineBuffer = (bgr888_t*)heap_alloc_dma(sizeof(bgr888_t) * p->maxWidth * ceilf(p->zoom_x));
      p->pc->src_data = p->lineBuffer;
      png_prepare_line(p, 0);
      lgfx_pngle_set_done_callback(pngle, png_done_callback);

      if (p->zoom_x == 1.0f && p->zoom_y == 1.0f)
      {
        lgfx_pngle_set_draw_callback(pngle, png_draw_alpha_callback);
      }
      else
      {
        lgfx_pngle_set_draw_callback(pngle, png_draw_alpha_scale_callback);
      }
    } else {
      if (p->zoom_x == 1.0f && p->zoom_y == 1.0f)
      {
        p->last_pos = ~0;
        lgfx_pngle_set_draw_callback(pngle, png_draw_normal_callback);
      }
      else
      {
        png_ypos_update(p, 0);
        lgfx_pngle_set_draw_callback(pngle, png_draw_normal_scale_callback);
      }
      return;
    }
  }

  bool LGFXBase::draw_png(DataWrapper* data, std::int32_t x, std::int32_t y, std::int32_t maxWidth, std::int32_t maxHeight, std::int32_t offX, std::int32_t offY, float scale_x, float scale_y, datum_t datum)
  {
    png_file_decoder_t png;
    png.x = x;
    png.y = y;
    png.offX = offX;
    png.offY = offY;
    png.maxWidth  = maxWidth ;
    png.maxHeight = maxHeight;
    png.zoom_x = scale_x;
    png.zoom_y = scale_y;
    png.datum = datum;
    png.gfx = this;
    png.lineBuffer = nullptr;

    pixelcopy_t pc(nullptr, this->getColorDepth(), bgr888_t::depth, this->_palette_count);
    png.pc = &pc;

    pngle_t *pngle = lgfx_pngle_new();

    lgfx_pngle_set_user_data(pngle, &png);

    lgfx_pngle_set_init_callback(pngle, png_init_callback);

    // Feed data to pngle
    std::uint8_t buf[512];
    int remain = 0;
    int len;
    bool res = true;

    this->startWrite(!data->hasParent());
    while (0 < (len = data->read(buf + remain, sizeof(buf) - remain))) {
      data->postRead();

      int fed = lgfx_pngle_feed(pngle, buf, remain + len);

      if (fed < 0) {
//ESP_LOGE("LGFX", "[pngle error] %s", lgfx_pngle_error(pngle));
        res = false;
        break;
      }

      remain = remain + len - fed;
      if (remain > 0) memmove(buf, buf + fed, remain);
      data->preRead();
    }
    this->endWrite();
    if (png.lineBuffer) {
      this->waitDMA();
      heap_free(png.lineBuffer);
    }
    lgfx_pngle_destroy(pngle);
    return res;
  }

  struct png_encoder_t
  {
    LGFXBase* gfx;
    std::int32_t x;
    std::int32_t y;
  };

  static uint8_t *png_encoder_get_row( std::uint8_t *pImage, int flip, int w, int h, int y, int, void *target )
  {
    auto enc = static_cast<png_encoder_t*>(target);
    uint32_t ypos = (flip ? (h - 1 - y) : y);
    enc->gfx->readRectRGB( enc->x, enc->y + ypos, w, 1, pImage );
    return pImage;
  }

  void* LGFXBase::createPng(std::size_t* datalen, std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h)
  {
    if (_adjust_abs(x, w)||_adjust_abs(y, h)) return nullptr;
    if (x < 0) { w += x; x = 0; }
    if (w > width() - x)  w = width()  - x;
    if (w < 1) return nullptr;
    if (y < 0) { h += y; y = 0; }
    if (h > height() - y) h = height() - y;
    if (h < 1) return nullptr;

    void* rgbBuffer = heap_alloc_dma(w * 3);

    png_encoder_t enc = { this, x, y };

    auto res = tdefl_write_image_to_png_file_in_memory_ex_with_cb(rgbBuffer, w, h, 3, datalen, 6, 0, (tdefl_get_png_row_func)png_encoder_get_row, &enc);

    heap_free(rgbBuffer);

    return res;
  }

//----------------------------------------------------------------------------

  void LGFXBase::prepareTmpTransaction(DataWrapper* data)
  {
    if (data->need_transaction && isBusShared())
    {
      data->parent = this;
      data->fp_pre_read  = tmpEndTransaction;
      data->fp_post_read = tmpBeginTransaction;
    }
  }

//----------------------------------------------------------------------------

  LGFX_Device::LGFX_Device(void)
  {
    setPanel(nullptr);
  }

  void LGFX_Device::initBus(void)
  {
    panel()->initBus();
  };

  void LGFX_Device::releaseBus(void)
  {
    panel()->releaseBus();
  };

  void LGFX_Device::setPanel(Panel_Device* panel)
  {
    static Panel_NULL nullobj;
    _panel = (nullptr == panel)
           ? &nullobj
           : panel;
  }

  bool LGFX_Device::init_impl(bool use_reset, bool use_clear)
  {
    if (_panel)
    {
      if (isEPD())
      {
        setBaseColor(TFT_WHITE);
        setTextColor(TFT_BLACK, TFT_WHITE);
      }
      if (getPanel()->init(use_reset))
      {
        startWrite();
        invertDisplay(_panel->getInvert());
        setColorDepth(_panel->getWriteDepth());
        setRotation(  _panel->getRotation());
        if (use_clear)
        {
          clear();
        }
        setPivot(width()>>1, height()>>1);
        endWrite();
        setBrightness(_brightness);
        getPanel()->initTouch();
        return true;
      }
    }
    return false;
  }

//----------------------------------------------------------------------------

    void LGFX_Device::draw_calibrate_point(std::int32_t x, std::int32_t y, std::int32_t r, std::uint32_t fg_rawcolor, std::uint32_t bg_rawcolor)
    {
      setRawColor(bg_rawcolor);
      fillRect(x - r, y - r, r * 2 + 1, r * 2 + 1);
      if (fg_rawcolor == bg_rawcolor) return;
      setClipRect(x - r, y - r, r * 2 + 1, r * 2 + 1);
      setRawColor(fg_rawcolor);
      startWrite();
      auto w = std::max<std::int32_t>(1, r >> 3);
      fillRect(x - w, y - r, w * 2 + 1, r * 2 + 1);
      fillRect(x - r, y - w, r * 2 + 1, w * 2 + 1);
      for (std::int32_t i = - r; i <= r; ++i) {
        drawFastHLine(x + i - w, y + i, w * 2 + 1);
        drawFastHLine(x - i - w, y + i, w * 2 + 1);
      }
      display();
      endWrite();
      clearClipRect();
    }

    void LGFX_Device::calibrate_touch(std::uint16_t *parameters, std::uint32_t fg_rawcolor, std::uint32_t bg_rawcolor, std::uint8_t size)
    {
      if (nullptr == touch()) return;
      auto rot = getRotation();

      std::uint_fast8_t panel_offsetrot = panel()->config().offset_rotation;
      std::uint_fast8_t touch_offsetrot = touch()->config().offset_rotation;

      // 回転オフセットをキャンセルしてタッチデバイスのデフォルトの向きに合わせる
      setRotation(( (touch_offsetrot ^ panel_offsetrot) & 4)
                 |(-(touch_offsetrot + panel_offsetrot) & 3));

      std::uint16_t orig[8];
      for (int i = 0; i < 4; ++i) {
        std::int32_t px = (width() -  1) * ((i>>1) & 1);
        std::int32_t py = (height() - 1) * ( i     & 1);
        draw_calibrate_point( px, py, size, fg_rawcolor, bg_rawcolor);
        delay(512);
        std::int32_t x_touch = 0, y_touch = 0;
        static constexpr int _RAWERR = 20;
        std::int32_t x_tmp, y_tmp, x_tmp2, y_tmp2;
        for (int j = 0; j < 8; ++j) {
          do {
            do { delay(1); } while (!getTouchRaw(&x_tmp,&y_tmp));
            delay(2); // Small delay to the next sample
          } while (!getTouchRaw(&x_tmp2,&y_tmp2)
                 || (abs(x_tmp - x_tmp2) > _RAWERR)
                 || (abs(y_tmp - y_tmp2) > _RAWERR));

          x_touch += x_tmp;
          x_touch += x_tmp2;
          y_touch += y_tmp;
          y_touch += y_tmp2;
        }
        orig[i*2  ] = x_touch >> 4;
        orig[i*2+1] = y_touch >> 4;
        draw_calibrate_point( px, py, size, bg_rawcolor, bg_rawcolor);
        while (getTouchRaw());
      }
      if (nullptr != parameters) {
        memcpy(parameters, orig, sizeof(std::uint16_t) * 8);
      }
      panel()->setCalibrate(orig);
      setRotation(rot);
    }

//----------------------------------------------------------------------------
 }
}
