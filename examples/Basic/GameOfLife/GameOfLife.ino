//The Game of Life, also known simply as Life, is a cellular automaton
//devised by the British mathematician John Horton Conway in 1970.
// https://en.wikipedia.org/wiki/Conway's_Game_of_Life

#include <Arduino.h>

#include <M5GFX.h>
M5GFX display;

//#include <M5UnitOLED.h>
//M5UnitOLED display; // default setting
//M5UnitOLED display ( 21, 22, 400000 ); // SDA, SCL, FREQ

//#include <M5UnitLCD.h>
//M5UnitLCD display;  // default setting
//M5UnitLCD display  ( 21, 22, 400000 ); // SDA, SCL, FREQ

M5Canvas canvas[2];

void setup(void)
{
  display.begin();
  display.setEpdMode(epd_mode_t::epd_fastest);

  if (display.width() < display.height())
  {
    display.setRotation(display.getRotation() ^ 1);
    display.setPivot(display.width() /2 -0.5, display.height() /2 - 0.5);
  }

  for (int i = 0; i < 2; i++)
  {
    canvas[i].setColorDepth(8);
    canvas[i].createSprite(std::min(192, display.width()>>1), std::min(160, display.height()>>1));
    canvas[i].createPalette();
    canvas[i].setPaletteColor(1, TFT_WHITE);
    canvas[i].setPivot(canvas[i].width() /2 -0.5, canvas[i].height() /2 - 0.5);
  }
  canvas[0].setTextColor(1);
  canvas[0].setTextDatum(textdatum_t::bottom_center);
  canvas[0].drawString("Conway's", canvas[0].width() >> 1, canvas[0].height() >> 1);
  canvas[0].setTextDatum(textdatum_t::top_center);
  canvas[0].drawString("Game of Life", canvas[0].width() >> 1, canvas[0].height() >> 1);
  canvas[0].pushRotateZoom(&display, 0,  (float)display.width() / canvas[0].width(), (float)display.height() / canvas[0].height());
  delay(1000);
}

void loop(void)
{
  bool flip = false;
  int width = canvas[flip].width();
  int height = canvas[flip].height();

  int y = 1;
  do
  {
    int x = 1;
    do
    {
      if (random(6) == 0) { canvas[flip].drawPixel(x, y, 1); }
    } while (++x < width - 1);
  } while (++y < height - 1);

  int diff;
  do
  {
    flip = !flip;
    diff = 0;

    auto old_buf = (uint8_t*)canvas[!flip].getBuffer();
    auto new_buf = (uint8_t*)canvas[ flip].getBuffer();
    int width  = canvas[flip].width();
    int height = canvas[flip].height();
    int py;
    int y  = height - 1;
    int ny = 0;
    do
    {
      py = y;
      y = ny;
      if (++ny == height) ny = 0;

      int px;
      int x  = width - 1;
      int nx = 0;
      do
      {
        px = x;
        x = nx;
        if (++nx == width) nx = 0;

        int neighbors = old_buf[px + py * width]
                      + old_buf[ x + py * width]
                      + old_buf[nx + py * width]
                      + old_buf[px +  y * width]
                      + old_buf[nx +  y * width]
                      + old_buf[px + ny * width]
                      + old_buf[ x + ny * width]
                      + old_buf[nx + ny * width];
        int idx = x + y * width;
        bool flg = (neighbors == 3) || (neighbors == 2 && old_buf[idx]);
        if (flg != new_buf[idx])
        {
          new_buf[idx] = flg;
          ++diff;
        }
      } while (nx);
    } while (ny);

    canvas[flip].pushRotateZoom(&display, 0,  (float)display.width() / width, (float)display.height() / height);
  } while (diff);
}
