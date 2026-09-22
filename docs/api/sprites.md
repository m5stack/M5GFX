# Sprites

Use M5Canvas or LGFX_Sprite for a drawing surface in memory. Select its dimensions and color depth, check allocation success, draw on it, then send it to the target. Release buffers when no longer needed.

## Allocation

`setColorDepth()`, `createSprite()`, `deleteSprite()`.

## Output

`pushSprite()`, `pushRotateZoom()`.

## Learn more

See [SpinTile](../../examples/Basic/SpinTile) and [GameOfLife](../../examples/Basic/GameOfLife). Drawing and text operations are shared with the [drawing API](drawing.md).

[Public declarations](../../src/lgfx/v1/LGFX_Sprite.hpp)

[API index](README.md) · [Documentation home](../README.md)
