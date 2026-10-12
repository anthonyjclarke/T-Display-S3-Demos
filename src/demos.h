#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "debug.h"

// Shared display objects (defined in main.cpp).
// `frame` is a full-screen 320x170 16-bit sprite held in PSRAM. Demos draw
// into it and call frame.pushSprite(0, 0). NotoSansBold15 is permanently
// loaded into `frame`.
extern TFT_eSPI    tft;
extern TFT_eSprite frame;

constexpr int16_t SCREEN_W = 320;
constexpr int16_t SCREEN_H = 170;

// Each demo is a set of callbacks driven by main.cpp.
//   enter()  — allocate / reset state (no drawing needed)
//   update() — called every loop() while active; must not block
//   action() — KEY button short press
//   exit()   — free anything allocated in enter()
struct Demo {
  const char* name;
  const char* hint;  // what the KEY button does
  void (*enter)();
  void (*update)();
  void (*action)();
  void (*exit)();
};

extern const Demo DEMO_SYSINFO;
extern const Demo DEMO_PLASMA;
extern const Demo DEMO_FIRE;
extern const Demo DEMO_SHAPES;
extern const Demo DEMO_WIFISCAN;

// ---------- Helpers ----------

inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Sprite buffers store each pixel byte-swapped; use this when writing
// directly into frame.getPointer().
inline uint16_t swap565(uint16_t c) { return (c >> 8) | (c << 8); }

// h, s, v all 0–255
uint16_t hsv565(uint8_t h, uint8_t s, uint8_t v);

// Frames-per-second counter; reset() in enter(), tick() once per frame, read fps.
struct FpsCounter {
  uint32_t windowStart = 0;
  uint16_t frames = 0;
  uint16_t fps = 0;
  void reset() {
    windowStart = millis();
    frames = 0;
    fps = 0;
  }
  void tick() {
    frames++;
    uint32_t now = millis();
    if (now - windowStart >= 1000) {
      fps = frames;
      frames = 0;
      DBG_VERBOSE("fps %u", fps);
      windowStart = now;
    }
  }
};

// Small "NN fps" badge in the top-left corner of `frame`.
void drawFpsBadge(uint16_t fps);
