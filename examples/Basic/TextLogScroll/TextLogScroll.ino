
#include <M5GFX.h>

M5GFX display;
M5Canvas canvas(&display);

static constexpr char text0[] = "hello world";
static constexpr char text1[] = "this";
static constexpr char text2[] = "is";
static constexpr char text3[] = "text";
static constexpr char text4[] = "log";
static constexpr char text5[] = "vertical";
static constexpr char text6[] = "scroll";
static constexpr char text7[] = "sample";
static constexpr const char* text[] = { text0, text1, text2, text3, text4, text5, text6, text7 };

void setup(void)
{
  display.begin();

  if (display.isEPD())
  {
    display.setEpdMode(epd_mode_t::epd_fastest);
    display.invertDisplay(true);
    display.clear(TFT_BLACK);
  }
  if (display.width() < display.height())
  {
    display.setRotation(display.getRotation() ^ 1);
  }

  canvas.setColorDepth(1); // mono color
  canvas.createSprite(display.width(), display.height());
  canvas.setTextSize((float)canvas.width() / 160);
  canvas.setTextScroll(true);
}

void loop(void)
{
  static int count = 0;

  canvas.printf("%04d:%s\r\n", count, text[count & 7]);
  canvas.pushSprite(0, 0);
  ++count;
}
