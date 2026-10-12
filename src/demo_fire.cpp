// Fire — the Doom PSX fire algorithm on a half-resolution heat map,
// scaled 2x into the frame buffer.

#include <Arduino.h>

#include "debug.h"
#include "demos.h"

constexpr int16_t FW = SCREEN_W / 2;
constexpr int16_t FH = SCREEN_H / 2;
constexpr uint8_t HEAT_MAX = 36;

static uint8_t*  heat = nullptr;
static uint16_t  palette[HEAT_MAX + 1];  // pre byte-swapped
static bool      burning = true;
static uint32_t  rng = 0x12345678;
static FpsCounter fpsCounter;

// Original 37-entry Doom fire palette (RGB888)
static const uint8_t DOOM_RGB[HEAT_MAX + 1][3] PROGMEM = {
  {0x07,0x07,0x07},{0x1F,0x07,0x07},{0x2F,0x0F,0x07},{0x47,0x0F,0x07},{0x57,0x17,0x07},
  {0x67,0x1F,0x07},{0x77,0x1F,0x07},{0x8F,0x27,0x07},{0x9F,0x2F,0x07},{0xAF,0x3F,0x07},
  {0xBF,0x47,0x07},{0xC7,0x47,0x07},{0xDF,0x4F,0x07},{0xDF,0x57,0x07},{0xDF,0x57,0x07},
  {0xD7,0x5F,0x07},{0xD7,0x5F,0x07},{0xD7,0x67,0x0F},{0xCF,0x6F,0x0F},{0xCF,0x77,0x0F},
  {0xCF,0x7F,0x0F},{0xCF,0x87,0x17},{0xC7,0x87,0x17},{0xC7,0x8F,0x17},{0xC7,0x97,0x1F},
  {0xBF,0x9F,0x1F},{0xBF,0x9F,0x1F},{0xBF,0xA7,0x27},{0xBF,0xA7,0x27},{0xBF,0xAF,0x2F},
  {0xB7,0xAF,0x2F},{0xB7,0xB7,0x2F},{0xB7,0xB7,0x37},{0xCF,0xCF,0x6F},{0xDF,0xDF,0x9F},
  {0xEF,0xEF,0xC7},{0xFF,0xFF,0xFF},
};

static inline uint32_t xorshift() {
  rng ^= rng << 13;
  rng ^= rng >> 17;
  rng ^= rng << 5;
  return rng;
}

static void setFuel(uint8_t level) {
  memset(heat + (FH - 1) * FW, level, FW);
}

static void enter() {
  for (int i = 0; i <= HEAT_MAX; i++) {
    palette[i] = swap565(rgb565(pgm_read_byte(&DOOM_RGB[i][0]),
                                pgm_read_byte(&DOOM_RGB[i][1]),
                                pgm_read_byte(&DOOM_RGB[i][2])));
  }
  if (heat == nullptr) heat = (uint8_t*)ps_malloc(FW * FH);
  if (heat == nullptr) {
    DBG_ERROR("Fire: heat map alloc failed");
    return;
  }
  memset(heat, 0, FW * FH);
  burning = true;
  fpsCounter.reset();
  setFuel(HEAT_MAX);
}

static void update() {
  if (heat == nullptr) return;

  // Propagate heat upward with random sideways drift and decay
  for (int x = 0; x < FW; x++) {
    for (int y = 1; y < FH; y++) {
      int src = y * FW + x;
      uint8_t h = heat[src];
      if (h == 0) {
        heat[src - FW] = 0;
      } else {
        uint32_t r = xorshift() & 3;
        int dst = src - FW - (int)r + 1;
        if (dst < 0) dst = 0;
        heat[dst] = h - (r & 1);
      }
    }
  }

  // Scale 2x into the frame buffer
  uint16_t* buf = (uint16_t*)frame.getPointer();
  for (int y = 0; y < FH; y++) {
    const uint8_t* hrow = heat + y * FW;
    uint16_t* r0 = buf + (y * 2) * SCREEN_W;
    uint16_t* r1 = r0 + SCREEN_W;
    for (int x = 0; x < FW; x++) {
      uint16_t c = palette[hrow[x]];
      r0[x * 2] = r0[x * 2 + 1] = c;
      r1[x * 2] = r1[x * 2 + 1] = c;
    }
  }

  fpsCounter.tick();
  drawFpsBadge(fpsCounter.fps);
  frame.pushSprite(0, 0);
}

static void action() {
  burning = !burning;
  if (heat != nullptr) setFuel(burning ? HEAT_MAX : 0);
}

static void exitDemo() {
  free(heat);
  heat = nullptr;
}

const Demo DEMO_FIRE = {"Fire", "douse / relight", enter, update, action, exitDemo};
