# T-Display-S3-Demos

A demo reel for the **LilyGO T-Display S3**: ESP32-S3R8, 16 MB flash, 8 MB OPI PSRAM and a 1.9″ ST7789 170×320 IPS panel on an 8-bit parallel bus. Each demo exercises a different part of the board.

---

## Hardware References

| Resource                 | Link                                                   |
|:-------------------------|:-------------------------------------------------------|
| Product page             | <https://lilygo.cc/products/t-display-s3>              |
| LilyGO repo (pins, docs) | <https://github.com/Xinyuan-LilyGO/T-Display-S3>       |

The LilyGO repo holds the schematic, datasheets, `pin_config.h` and the factory examples that this project's pin and backlight settings were taken from.

---

## Demos

| #  | Demo         | Shows off                                  | KEY action          |
|:---|:-------------|:-------------------------------------------|:--------------------|
| 1  | System Info  | Chip, memory, temp, battery, MAC, uptime   | Fill-rate benchmark |
| 2  | Plasma       | Per-pixel maths into a PSRAM frame buffer  | Change palette      |
| 3  | Fire         | Doom PSX fire, half-res heat map scaled 2× | Douse / relight     |
| 4  | 3D Shapes    | Anti-aliased wireframes over a starfield   | Next shape          |
| 5  | WiFi Scanner | Async scan, strongest 7 with RSSI bars     | Rescan now          |

Measured on hardware: Plasma ~28 fps, Fire ~31 fps, 3D Shapes ~25 fps.

---

## Controls

| Button        | Short press  | Long press (600 ms) |
|:--------------|:-------------|:--------------------|
| BOOT (GPIO0)  | Next demo    | Previous demo       |
| KEY (GPIO14)  | Demo action  | Cycle brightness    |

A title card shows the demo name and what KEY does each time a demo starts. Press KEY to skip it.

---

## Build & Flash

```bash
pio run -t upload
```

The board uses native USB CDC (`/dev/cu.usbmodem*`). If an upload fails with "multiple access on port", close any open serial monitor first.

Set `AUTO_CYCLE_MS` in `include/config.h` to a non-zero value for an unattended attract mode that rotates through the demos.

---

## Adding a Demo

Create `src/demo_<name>.cpp` that defines a `const Demo DEMO_<NAME>` with `enter` / `update` / `action` / `exit` callbacks, declare it in `src/demos.h`, and add it to the `DEMOS[]` table in `src/main.cpp`. Draw into the shared `frame` sprite and finish each frame with `frame.pushSprite(0, 0)`.

---

## Ideas for More Demos

- Touch paint (touch variant only: CST816 on I2C 18/17)
- BLE scanner
- NTP clock using ezTime
- Mandelbrot zoom using both cores
- Faster display path via `esp_lcd` i80 + DMA (LovyanGFX) for comparison
