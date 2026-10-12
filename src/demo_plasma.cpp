// Plasma — classic sine-table plasma rendered per-pixel into the PSRAM frame
// buffer, then pushed over the parallel bus every frame.

#include <Arduino.h>

#include "debug.h"
#include "demos.h"

static uint8_t   sinLut[256];
static uint16_t  palette[256];  // pre byte-swapped for direct buffer writes
static uint8_t*  radial = nullptr;  // per-pixel distance-from-centre term
static uint8_t   paletteIdx = 0;
static uint16_t  t = 0;
static FpsCounter fpsCounter;

constexpr uint8_t PALETTE_COUNT = 4;

static void buildPalette() {
  for (int i = 0; i < 256; i++) {
    uint16_t c;
    switch (paletteIdx) {
      case 0:  c = hsv565(i, 255, 255); break;                          // rainbow
      case 1:  c = rgb565(sinLut[i] >> 2, sinLut[(i + 64) & 255] >> 1, // ocean
                          sinLut[(i + 32) & 255]); break;
      case 2:  c = rgb565(sinLut[i], sinLut[(i + 85) & 255] >> 3, 0);  break; // lava
      default: c = rgb565(sinLut[i], sinLut[(i + 85) & 255],             // pastel
                          sinLut[(i + 170) & 255]); break;
    }
    palette[i] = swap565(c);
  }
}

static void enter() {
  for (int i = 0; i < 256; i++) sinLut[i] = 128 + 127 * sinf(i * TWO_PI / 256);
  buildPalette();
  fpsCounter.reset();

  if (radial == nullptr) {
    radial = (uint8_t*)ps_malloc(SCREEN_W * SCREEN_H);
    if (radial == nullptr) {
      DBG_ERROR("Plasma: radial table alloc failed");
      return;
    }
    for (int y = 0; y < SCREEN_H; y++) {
      for (int x = 0; x < SCREEN_W; x++) {
        float dx = x - SCREEN_W / 2, dy = y - SCREEN_H / 2;
        radial[y * SCREEN_W + x] = (uint8_t)(sqrtf(dx * dx + dy * dy) * 2.0f);
      }
    }
  }
}

static void update() {
  if (radial == nullptr) return;
  uint16_t* buf = (uint16_t*)frame.getPointer();
  t++;
  uint8_t t1 = t, t2 = t * 2, t3 = t * 3;

  for (int y = 0; y < SCREEN_H; y++) {
    uint8_t sy = sinLut[(uint8_t)(y * 2 + t2)];
    const uint8_t* rad = radial + y * SCREEN_W;
    uint16_t* row = buf + y * SCREEN_W;
    for (int x = 0; x < SCREEN_W; x++) {
      uint16_t v = sinLut[(uint8_t)(x + t1)] + sy
                 + sinLut[(uint8_t)((x + y) / 2 + t3)]
                 + sinLut[(uint8_t)(rad[x] - t2)];
      row[x] = palette[(uint8_t)((v >> 2) + t1)];
    }
  }

  fpsCounter.tick();
  drawFpsBadge(fpsCounter.fps);
  frame.pushSprite(0, 0);
}

static void action() {
  paletteIdx = (paletteIdx + 1) % PALETTE_COUNT;
  buildPalette();
}

static void exitDemo() {
  free(radial);
  radial = nullptr;
}

const Demo DEMO_PLASMA = {"Plasma", "change palette", enter, update, action, exitDemo};
