// Compare with docs/api/display.md#orientation-and-dimensions. Keep the device still while it runs.
// LCD example: the same F and logical corner markers are redrawn for values 0-7.
#include <M5GFX.h>

M5GFX display;

static constexpr uint32_t viewDurationMs = 3000;
static uint32_t lastViewMs;
static uint8_t rotation;
static bool ready;

static void drawView(uint8_t r)
{
  display.waitDisplay();
  display.setRotation(r);
  const int w = display.width();
  const int h = display.height();
  const int shortest = (w < h) ? w : h;
  const int u = shortest / 9;
  const int x = (w - 3 * u) / 2;
  const int y = (h - 5 * u) / 2;
  const int marker = (u > 3) ? u / 2 : 2;

  display.startWrite();
  display.fillScreen(TFT_BLACK);

  // Logical corners: top-left, top-right, bottom-left, bottom-right.
  display.fillRect(0, 0, marker, marker, display.color888(245, 158, 11));
  display.fillRect(w - marker, 0, marker, marker, display.color888(34, 197, 94));
  display.fillRect(0, h - marker, marker, marker, display.color888(56, 189, 248));
  display.fillRect(w - marker, h - marker, marker, marker, display.color888(244, 114, 182));

  // A path-equivalent F: 3 units wide, 5 high, with 1-unit strokes.
  display.fillRect(x, y, u, 5 * u, TFT_WHITE);
  display.fillRect(x, y, 3 * u, u, TFT_WHITE);
  display.fillRect(x, y + 2 * u, 2 * u, u, TFT_WHITE);

  // Arrows near the logical top-left corner indicate +X and +Y.
  const int a = marker + 4;
  const int length = 2 * u;
  display.drawLine(a, a, a + length, a, TFT_WHITE);
  display.drawLine(a + length, a, a + length - 3, a - 3, TFT_WHITE);
  display.drawLine(a + length, a, a + length - 3, a + 3, TFT_WHITE);
  display.drawLine(a, a, a, a + length, TFT_WHITE);
  display.drawLine(a, a + length, a - 3, a + length - 3, TFT_WHITE);
  display.drawLine(a, a + length, a + 3, a + length - 3, TFT_WHITE);
  display.setTextSize(1);
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  display.setCursor(a + length + 4, a - 3);
  display.print("X");
  display.setCursor(a - 2, a + length + 4);
  display.print("Y");
  display.setCursor(marker + 4, h - 12);
  display.printf("r=%u  %dx%d", static_cast<unsigned>(display.getRotation()), w, h);
  display.endWrite();
  display.display();

  Serial.printf("rotation=%u width=%d height=%d\n",
                static_cast<unsigned>(display.getRotation()), w, h);
}

void setup()
{
  Serial.begin(115200);
  if (!display.init())
  {
    Serial.println("Display initialization failed.");
    return;
  }
  Serial.printf("Startup rotation=%u width=%d height=%d\n",
                static_cast<unsigned>(display.getRotation()),
                static_cast<int>(display.width()), static_cast<int>(display.height()));
  if (display.isEPD())
  {
    Serial.println("This timed rotation example is intended for LCD displays.");
    return;
  }
  ready = true;
  drawView(rotation);
  lastViewMs = millis();
}

void loop()
{
  if (ready && static_cast<uint32_t>(millis() - lastViewMs) >= viewDurationMs)
  {
    rotation = (rotation + 1) & 7;
    drawView(rotation);
    lastViewMs = millis();
  }
  delay(10);
}
