// Requires M5Unified and M5GFX. Keep the device still when comparing views.
// Reference: docs/api/display.md#orientation-and-dimensions
#include <M5Unified.h>

static uint8_t rotation = 0;
static bool ready = false;

static void drawView(uint8_t value)
{
  auto& display = M5.Display;
  display.waitDisplay();
  display.setRotation(value);
  rotation = display.getRotation();

  const int w = display.width();
  const int h = display.height();
  const int shortest = w < h ? w : h;
  const bool round = M5.getBoard() == m5::board_t::board_M5Dial
                  || M5.getBoard() == m5::board_t::board_M5StopWatch;
  const bool compact = w < 180 || h < 180;
  const bool tiny = shortest < 80;
  const int divisor = tiny ? 12 : (round ? 13 : (compact ? 11 : 9));
  const int u = shortest / divisor > 0 ? shortest / divisor : 1;
  const int x = (w - 3 * u) / 2;
  const int y = (h - 5 * u - (tiny ? (w < 60 ? 20 : 10) : 0)) / 2;
  const int minMarker = tiny ? 4 : 8;
  const int marker = shortest / 10 > minMarker ? shortest / 10 : minMarker;
  const int inset = round ? shortest / 6 : 0;
  const int ax = round ? shortest / 7 : marker + (tiny ? 2 : 3);
  const int ay = round ? h * 37 / 100 : ax;
  const int length = round || tiny ? u : 2 * u;

  display.startWrite();
  display.fillScreen(TFT_WHITE);

  // On round screens, inset the corner markers into the circular visible area.
  display.fillRect(inset, inset, marker, marker, TFT_RED);
  display.fillRect(w - inset - marker, inset, marker, marker, TFT_GREEN);
  display.fillRect(inset, h - inset - marker, marker, marker, TFT_BLUE);
  display.fillRect(w - inset - marker, h - inset - marker, marker, marker, TFT_BLACK);

  // The same asymmetric F used in the eight-view diagram.
  display.fillRect(x, y, u, 5 * u, TFT_BLACK);
  display.fillRect(x, y, 3 * u, u, TFT_BLACK);
  display.fillRect(x, y + 2 * u, 2 * u, u, TFT_BLACK);

  display.drawLine(ax, ay, ax + length, ay, TFT_BLACK);
  display.drawLine(ax + length, ay, ax + length - 2, ay - 2, TFT_BLACK);
  display.drawLine(ax + length, ay, ax + length - 2, ay + 2, TFT_BLACK);
  display.drawLine(ax, ay, ax, ay + length, TFT_BLACK);
  display.drawLine(ax, ay + length, ax - 2, ay + length - 2, TFT_BLACK);
  display.drawLine(ax, ay + length, ax + 2, ay + length - 2, TFT_BLACK);
  if (tiny) display.setFont(&fonts::Font0);
  else display.setFont(compact ? &fonts::Font2 : &fonts::Font4);
  display.setTextSize(1);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  display.setTextWrap(false);
  auto drawBold = [&](const char* text, int32_t textX, int32_t textY)
  {
    display.drawString(text, textX, textY);
    display.drawString(text, textX + 1, textY);
  };
  drawBold("X", ax + length + 3, ay - 2);
  drawBold("Y", ax - 2, ay + length + 3);

  // Keep the value near the center line so it remains visible on circular screens.
  char label[32];
  snprintf(label, sizeof(label), "R=%u", static_cast<unsigned>(rotation));
  char dimensions[32];
  snprintf(dimensions, sizeof(dimensions), "%dx%d", w, h);
  const int gap = tiny ? 2 : (compact ? 4 : 8);
  const int totalWidth = display.textWidth(label) + gap + display.textWidth(dimensions);
  const int footerY = tiny ? h - (w < 60 ? 20 : 10) : y + 5 * u + (compact ? 4 : 10);
  if (round)
  {
    drawBold(label, (w - display.textWidth(label)) / 2, footerY);
    drawBold(dimensions, (w - display.textWidth(dimensions)) / 2,
             footerY + display.fontHeight() + 1);
  }
  else if (h < 80 && !tiny)
  {
    drawBold(label, (w - display.textWidth(label)) / 2, footerY);
  }
  else if (totalWidth <= w - 4)
  {
    const int startX = (w - totalWidth) / 2;
    drawBold(label, startX, footerY);
    drawBold(dimensions, startX + display.textWidth(label) + gap, footerY);
  }
  else
  {
    drawBold(label, (w - display.textWidth(label)) / 2, footerY);
    drawBold(dimensions, (w - display.textWidth(dimensions)) / 2,
             footerY + (tiny ? display.fontHeight() + 1 : (compact ? 17 : 30)));
  }
  display.endWrite();
  display.display();

  Serial.printf("requested=%u rotation=%u width=%d height=%d\n",
                static_cast<unsigned>(value), static_cast<unsigned>(rotation), w, h);
}

void setup()
{
  M5.begin();
  Serial.begin(115200);
  Serial.printf("Startup after M5.begin(): rotation=%u width=%d height=%d\n",
                static_cast<unsigned>(M5.Display.getRotation()),
                static_cast<int>(M5.Display.width()), static_cast<int>(M5.Display.height()));
  if (M5.Display.width() <= 0 || M5.Display.height() <= 0)
  {
    Serial.println("No display available.");
    return;
  }
  Serial.printf("Input: touch or Button A advances rotation (0..7); Serial: n for next, or 0..7 to select. Touch=%s\n",
                M5.Touch.isEnabled() ? "enabled" : "unavailable");
  ready = true;
  drawView(0);
}

void loop()
{
  M5.update();
  if (ready)
  {
    int next = -1;
    if (M5.BtnA.wasPressed()) next = (rotation + 1) & 7;
    if (M5.Touch.getCount() > 0 && M5.Touch.getDetail().wasPressed()) next = (rotation + 1) & 7;
    if (Serial.available())
    {
      const int key = Serial.read();
      if (key >= '0' && key <= '7') next = key - '0';
      else if (key == 'n' || key == 'N') next = (rotation + 1) & 7;
    }
    if (next >= 0) drawView(static_cast<uint8_t>(next));
  }
  delay(1);
}
