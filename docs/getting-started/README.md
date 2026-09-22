# Getting started

## Choose your display

Check the [supported device list](../../README.md) and the
[device guides](../devices/README.md). Built-in screens and external display
accessories can require different display classes and connection settings.

## Prepare your project

M5GFX supports Arduino for ESP32 and ESP-IDF. Add this library to your project
and select the board configuration for your device. For Arduino used as an
ESP-IDF component, see the [project notes](../../README.md#notes).

## Run an example

1. Open a complete example from the [example index](../examples/README.md).
2. Check its display declaration and any device-specific configuration.
3. Build and upload it using your board's development environment.
4. Initialize the display before drawing. Read `width()` and `height()` after
   selecting the orientation so that your layout fits the current surface.

Start with [graphics primitives](../../examples/Basic/TFT_graphicstest_PDQ)
to explore drawing, or [image data](../../examples/Basic/drawImageData)
to display an image. Keep an example's accompanying asset files with its sketch.

## Continue

- [Basic concepts](../concepts/README.md)
- [API reference](../api/README.md)
- [Usage guides](../guides/README.md)

[Documentation home](../README.md)
