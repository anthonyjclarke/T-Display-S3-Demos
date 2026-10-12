# Changelog

## [0.1.0] 13-10-2026

### Added

- Initial PlatformIO project for LilyGO T-Display S3 (TFT_eSPI 8-bit parallel, PSRAM frame sprite)
- Demos: System Info (with fill-rate benchmark), Plasma, Fire, 3D Shapes, WiFi Scanner
- Button handling: BOOT next/previous demo, KEY action / brightness cycle
- Optional attract mode via `AUTO_CYCLE_MS`

### Fixed

- Phantom BOOT press on every reset: buttons now arm only after being seen released
