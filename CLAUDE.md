# CLAUDE.md — T-Display-S3-Demos

## Hardware

LilyGO T-Display S3 (non-touch assumed): ESP32-S3R8, 16 MB flash, 8 MB OPI PSRAM, ST7789 170×320 on an **8-bit parallel** bus (not SPI). Native USB CDC only, no USB-UART bridge.

Reference: <https://github.com/Xinyuan-LilyGO/T-Display-S3> (schematic, `pin_config.h`, examples) · <https://lilygo.cc/products/t-display-s3>. A local clone lives at `../T-Display-S3`.

## Summary

v0.1.0. A button-driven demo reel. `main.cpp` owns the display, buttons and the `DEMOS[]` table; each `demo_*.cpp` exports a `Demo` struct of callbacks (see `demos.h`).

## Pins (non-display)

| Function  | GPIO | Note                                       |
|:----------|:-----|:-------------------------------------------|
| Power on  | 15   | Must be HIGH or the LCD is dark on battery |
| BOOT btn  | 0    | Strapping pin, reads LOW briefly at reset  |
| KEY btn   | 14   |                                            |
| Battery   | 4    | 1:2 divider; >4.3 V means USB powered      |
| Backlight | 38   | LEDC PWM ch 0 at 2 kHz (LilyGO's value)    |

## Decisions and Gotchas

- Platform pinned to `espressif32@6.9.0` (Arduino core 2.0.17). TFT_eSPI's S3 parallel driver is unreliable on core 3.x, so don't bump it.
- TFT_eSPI config is entirely in `build_flags` (mirrors Setup206). `CGRAM_OFFSET` and `TFT_INVERSION_ON` are required or the image is offset or has inverted colours.
- The `frame` sprite (320×170×16) lives in PSRAM; `-DBOARD_HAS_PSRAM` must stay in the build flags.
- Sprite buffers store pixels **byte-swapped**. Direct writes via `frame.getPointer()` must use `swap565()`.
- `NotoSansBold15` stays loaded in `frame` permanently; the title card temporarily swaps to 36 pt and restores 15.
- The font headers define non-`static` `const` arrays. Include them only in `fonts.cpp`; elsewhere use the externs in `fonts.h`.
- Smooth fonts only have basic Latin glyphs. Non-ASCII characters such as `·` or `°` don't render.
- Buttons start in the "held, long-fired" state so the GPIO0 LOW at reset doesn't count as a press.
- Upload fails with "multiple access on port" if any `pio device monitor` is open. 921600 baud was unreliable, so 460800 is used.
- Full-frame push is the bottleneck (~30 fps). TFT_eSPI drives the parallel bus without DMA.
