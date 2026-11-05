# Transactions and transfers

Group related drawing operations with a matching startWrite/endWrite pair. DMA availability and buffer requirements depend on the backend. DMA completion and display refresh completion are separate concerns.

## Drawing transactions

`startWrite()`, `endWrite()`.

## Pixel transfers

`pushPixels()`, `pushImage()`, `pushPixelsDMA()`, `pushImageDMA()`.

## Synchronization

`waitDMA()`, `waitDisplay()`.

## Learn more

See [display control](display.md), [Sprites](sprites.md) and [DisplayRotation](../../examples/Basic/DisplayRotation) for a grouped drawing example.

[Public declarations](../../src/lgfx/v1/LGFXBase.hpp)

[API index](README.md) · [Documentation home](../README.md)
