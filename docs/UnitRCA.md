# Unit RCA library


![Unit RCA](https://static-cdn.m5stack.com/resource/docs/products/unit/RCA/img-9420bb3d-22b8-4f80-b7fe-e708088f1e51.webp "Unit RCA")

### Notes.
- Video signals can be output using the ESP32's built-in DAC output.  
- If you wish to use UnitRCA for audio output, please use other libraries such as M5Unified::Speaker.  
- Only GPIO 25 or 26 can be used for the ESP32's built-in DAC output.  
- ESP32-S3 does not have a DAC output and cannot be used.  
- Display colors are limited to 8-bit (RGB332) format.  
- The output resolution can be set freely, but it uses memory in proportion to the area.  

### 注意事項  
- ESP32の内蔵DAC出力を使用してビデオ信号を出力できます。  
- UnitRCAを使用して音声出力を希望される場合はM5Unified::Speaker等の他のライブラリをご利用ください。  
- ESP32の内蔵DAC出力は GPIO 25または26のみが使用できます。  
- ESP32-S3ではDAC出力がないため使用できません。  
- 表示色は 8bit(RGB332) 形式に限定されます。  
- 出力解像度は自由に設定できますが、面積に比例してメモリを使用します。  


Sample Code:
```
#include <M5UnitRCA.h>

// Resolution, signal type, and output pin number can be specified in the constructor or init function.
// If the argument is omitted, the resolution is set to 216x144, PAL format, and GPIO 26 pins.
// If the resolution is too high, it may not work due to insufficient memory.

// 解像度、信号の種類、出力ピン番号はコンストラクタまたはinit関数で指定できます。
// 引数の省略時は 解像度 216x144 , PAL形式 , GPIO26ピン が設定されます。
// 解像度が高すぎる場合はメモリ不足のため動作しない可能性があります。

M5UnitRCA display ( 216                            // logical_width
                  , 144                            // logical_height
                  , 256                            // output_width
                  , 160                            // output_height
                  , M5UnitRCA::signal_type_t::PAL  // signal_type
                  , 26                             // GPIO pin
                  );

// If output_width and output_height are set larger than logical_width and logical_height, there will be margins at the edge of the screen.
// signal_type can be selected from NTSC / NTSC_J / PAL / PAL_M / PAL_N.
// The maximum output resolution depends on the signal_type.
// 720 x 480 (NTSC,NTSC_J)
// 864 x 576 (PAL,PAL_M)
// 720 x 576 (PAL_N)

// output_width,output_height を logical_width,logical_height より大きく設定すると、画面端に余白ができます。
// signal_typeは NTSC / NTSC_J / PAL / PAL_M / PAL_N から選択できます。
// 出力できる最大解像度はsignal_typeによって異なります。


void setup(void)
{
// For models with a protection resistor on the GPIO (M5Stack Core2), the voltage of the output signal drops.
// This function sets the amplification of the output signal.
// GPIOに保護抵抗がついた機種 (M5Stack Core2) の場合、出力信号の電圧が低下します。
// こちらの関数で出力信号の増幅を設定します。
// display.setOutputBoost(true);
//
// For more fine-tuning, use this function.
// より細かく調整したい場合はこちらの関数を使用します。
// display.setOutputLevel(200);  // default=128


// If you want to change the GPIO pin number of the output destination, specify it with this function.
// 出力先のGPIOピン番号を変更したい場合、この関数で指定します。
// display.setOutputPin(25);  // Only GPIO 25 or 26 can be used.


// To change the output signal type, specify it with this function.
// 出力信号の種類を変更したい場合、この関数で指定します。
// display.setSignalType(M5UnitRCA::signal_type_t::NTSC); // NTSC / NTSC_J / PAL / PAL_M / PAL_N


// If you wish to allocate PSRAM for buffer memory, specify it with this function.
// ※ When PSRAM is used, video may be disturbed under high load.
// バッファメモリにPSRAMを割当てたい場合、この関数で指定します。
// ※ PSRAMを使用した場合は高負荷時に映像が乱れる場合があります。
// display.setPsram(M5UnitRCA::use_psram_t::psram_use);  // psram_no_use / psram_half_use / psram_use


// initialize
// 初期化
  display.init();

// Set the display orientation
// ディスプレイの向きを設定
// 0 = normal / 1 = 90 deg / 2 = 180 deg / 3 = 270 deg / 4~7 = upside down
  display.setRotation(0);

  display.startWrite();
  for (int y = 0; y < display.height(); ++y)
  {
    for (int x = 0; x < display.width(); ++x)
    {
      display.writePixel(x, y, display.color888(x, x+y, y));
    }
  }
  display.endWrite();

  for (int i = 0; i < 16; ++i)
  {
    int x = rand() % display.width();
    int y = rand() % display.height();
    display.drawCircle(x, y, 16, rand());
  }
}

void loop(void)
{
  static constexpr const char hello_str[] = "Hello Unit RCA !";
  display.setFont(&fonts::Orbitron_Light_32);
  int i_end = -display.textWidth(hello_str);
  for (int i = display.width(); i > i_end; --i)
  {
    display.drawString(hello_str, i, (display.height() - display.fontHeight()) >> 1);
    delay(5);
  }
}
```


