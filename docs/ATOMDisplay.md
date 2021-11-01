# ATOM Display library


![M5Stack ATOM Display](https://static-cdn.m5stack.com/resource/docs/static/assets/img/product_pics/atom_base/atom_display/atom_display_01.webp "ATOM Display")


### Notes.
- The HDMI output resolution is fixed at 1280x720 60Hz.  
- If the image does not appear, reinsert the USB cable and wait a few seconds.  
- Audio output function is not included.  
- When inserting or removing the USB cable, please hold the ATOM itself. Repeatedly inserting and removing the USB cable while holding the docking base side may cause stress on the pin headers and result in poor contact.  

### 注意事項  
- HDMIの出力解像度は 1280x720 60Hz 固定です。  
- 音声出力機能は含まれません。  
- 画像が表示されない場合はUSBケーブルを挿し直し、数秒待機してください。  
- USBケーブルを挿抜する際はATOM本体を保持してください。ドッキングベース側を保持した状態でUSB挿抜を行うと、ピンヘッダに応力が掛かり接触不良を引き起こす可能性があります。  


Sample Code:
```
#include <M5AtomDisplay.h>

// Create an instance of M5AtomDisplay
// Set the resolution in the argument (width, height).  default : 1280, 720.
// M5AtomDisplay のインスタンスを用意
// 引数で解像度(幅,高さ)を設定できます。 省略時は 1280, 720

M5AtomDisplay display ( 640, 360 );

// ※ By lowering the resolution, you can enlarge the image.
//    If the resolution after enlargement is less than 1280x720
//    , there will be a gap around the periphery.
// ※ 解像度を下げることで、拡大表示することができます。
//    拡大後の解像度が1280x720に満たない場合は外周に隙間が生じます。
// width scale:
// ~ 1280 = x1
// ~ 640  = x2
// ~ 320  = x4
// ~ 256  = x5
// ~ 160  = x8

// height scale:
// ~ 720 = x1
// ~ 360 = x2
// ~ 240 = x3
// ~ 180 = x4
// ~ 144 = x5
// ~ 120 = x6
// ~ 102 = x7
// ~  90 = x8


void setup(void)
{
// initialize
// 初期化
  display.init();

// Set the display orientation
// ディスプレイの向きを設定
// 0 = normal / 1 = 90 deg / 2 = 180 deg / 3 = 270 deg / 4~7 = upside down
  display.setRotation(0);

// Set the color depth ( bpp )
// 色深度を設定
  display.setColorDepth(24);  // 24bit per pixel color setting
//display.setColorDepth(16);  // 16bit per pixel color setting ( default )
//display.setColorDepth( 8);  //  8bit per pixel color setting

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
  display.startWrite();

  static constexpr const char hello_str[] = "Hello ATOM Display !";
  display.setFont(&fonts::Orbitron_Light_32);
  for (int i = -display.textWidth(hello_str); i < display.width(); ++i)
  {
    display.drawString(hello_str, i, (display.height() - display.fontHeight()) >> 1);
  }

  display.endWrite();
}
```


#### Is PSRAM required ?  can I use it with ATOM Lite or ATOM Matrix?  
- PSRAM is NOT required. it will work with ATOM Lite and ATOM Matrix. The label on the product that says "FOR ATOM PSRAM ONLY" is incorrect.  
- In fact, in the early stages of development, we assumed that we would build a 1280x720 frame buffer in the ESP32's PSRAM and output the images. As the development progressed, the frame buffer was placed on the FPGA side, and in the final product, there was no need to have a frame buffer on the ESP32 side.  

#### What is the difference between ATOM Lite and ATOM PSRAM?  
- ATOM Lite has ESP32 PICO-D4,  ATOM PSRAM has ESP32 PICO-V3-02.  
- Some of the pins on the back are different. The ATOM PSRAM has a G5 pin where the ATOM Lite had a G23 pin, but the G23 pin is gone.  



#### PSRAMは必須ですか？ATOM LiteやATOM Matrixで使えますか？  
- PSRAMは必須ではありません。ATOM LiteやATOM Matrixでも動作します。製品のラベルに記載されている "FOR ATOM PSRAM ONLY" という表記は誤りです。  
- 実は開発初期段階では、ESP32のPSRAMに1280x720のフレームバッファを構築して画像を出力する想定でした。開発が進むにつれ、フレームバッファはFPGA側に持つようになり、最終製品ではにESP32側でフレームバッファを持つ必要がなくなりました。  

#### ATOM LiteとATOM PSRAMの違いは？  
- ATOM Liteには ESP32 PICO D4が搭載されています。ATOM PSRAMには ESP32 PICO V3-02 が搭載されています。 
- 背面のピンが一部違います。ATOM Liteの G23 が配置されていた箇所には、 ATOM PSRAM では G5 が配置されています。G23は無くなっているため、注意が必要です。  

