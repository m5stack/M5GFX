# Button-controlled display rotation

Requires **M5Unified** and **M5GFX**. Build against the M5GFX version you want to test.

1. Upload the sketch and hold the device in a fixed position. It starts at rotation 0.
2. Press **A** once to advance: 0 → 1 → 2 → 3 → 4 → 5 → 6 → 7 → 0.
3. Compare F, the X/Y arrows and the corner colors with the
   [eight-view reference](../../../docs/api/display.md#orientation-and-dimensions).
4. Record the device model, library version and observed direction at each value.

The value appears below F. Dimensions also appear when the screen is large enough.
Serial output at **115200 baud** always reports the requested value, returned value
and dimensions. Send `n` for the next value, or `0`–`7` to select a value directly.
Use serial input on devices without an available `M5.BtnA` mapping.

The display is redrawn only when a value is selected. For e-paper, wait for each
refresh before pressing again; supported rotation and mirror behavior depends on
the panel. Circular displays clip logical corner markers outside the visible circle.

Startup values are logged after **M5Unified initialization**. They do not represent
M5GFX-only initialization. This sketch changes drawing orientation; it does not
change the driver mounting offset or standardize the device's direction.
