# Basic concepts

## Coordinates and orientation

Drawing uses the current surface's coordinates. X increases to the right and Y
downward, starting at the top-left corner. Use `width()` and `height()` for its
current dimensions. Rotation can change these dimensions.

See [display control](../api/display.md) for rotation, clipping and scrolling,
and [device orientation](../device-orientation.md) for physical reference views.

## Colors and pixels

Colors and image buffers are separate concerns. A drawing call can accept a
color while an image buffer also has a pixel format and, for some formats, a
palette. Choose the matching image overload for your data.

See [drawing](../api/drawing.md) and [images](../api/images.md).

## Display and Sprite

A display is the target device. A Sprite is an image buffer in memory on which
you can draw before sending the result to a display. Its dimensions and memory
requirements are chosen by the application.

See [Sprites](../api/sprites.md).

## Drawing and refreshing

Drawing operations and physical display updates are related but distinct.
E-paper and buffered displays can have different update behavior from LCDs.
Use the device guide alongside [display control](../api/display.md) and
[transfers](../api/transfers.md).

## Device capabilities

Touch, backlight control and refresh modes depend on the connected hardware.
Use the relevant [device guide](../devices/README.md) when choosing these features.

[Documentation home](../README.md)
