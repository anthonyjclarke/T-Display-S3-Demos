// 3D Shapes — anti-aliased wireframe solids spinning over a warp starfield.

#include <Arduino.h>

#include "demos.h"

struct Vec3 { float x, y, z; };

constexpr uint8_t MAX_VERTS = 12;
constexpr uint8_t MAX_EDGES = 30;
constexpr uint8_t STAR_COUNT = 160;
constexpr float   SHAPE_SCALE = 72.0f;
constexpr float   CAMERA_DIST = 3.5f;

static Vec3    verts[MAX_VERTS];
static uint8_t edges[MAX_EDGES][2];
static uint8_t vertCount = 0, edgeCount = 0;
static uint8_t shapeIdx = 0;
static const char* const SHAPE_NAMES[] = {"Cube", "Octahedron", "Icosahedron"};
constexpr uint8_t SHAPE_COUNT = 3;

struct Star { float x, y, z; };
static Star stars[STAR_COUNT];

static float    ax = 0, ay = 0, az = 0;
static uint8_t  hue = 0;
static uint32_t lastFrameUs = 0;
static FpsCounter fpsCounter;

// Builds the edge list by joining every vertex pair at the shortest distance,
// then normalises the solid to unit radius.
static void finishShape() {
  float best = 1e9f;
  for (uint8_t i = 0; i < vertCount; i++)
    for (uint8_t j = i + 1; j < vertCount; j++) {
      float dx = verts[i].x - verts[j].x, dy = verts[i].y - verts[j].y, dz = verts[i].z - verts[j].z;
      best = min(best, dx * dx + dy * dy + dz * dz);
    }
  edgeCount = 0;
  for (uint8_t i = 0; i < vertCount; i++)
    for (uint8_t j = i + 1; j < vertCount && edgeCount < MAX_EDGES; j++) {
      float dx = verts[i].x - verts[j].x, dy = verts[i].y - verts[j].y, dz = verts[i].z - verts[j].z;
      if (fabsf(dx * dx + dy * dy + dz * dz - best) < 0.01f) {
        edges[edgeCount][0] = i;
        edges[edgeCount][1] = j;
        edgeCount++;
      }
    }
  for (uint8_t i = 0; i < vertCount; i++) {
    float len = sqrtf(verts[i].x * verts[i].x + verts[i].y * verts[i].y + verts[i].z * verts[i].z);
    verts[i] = {verts[i].x / len, verts[i].y / len, verts[i].z / len};
  }
}

static void buildShape() {
  vertCount = 0;
  if (shapeIdx == 0) {
    for (int i = 0; i < 8; i++)
      verts[vertCount++] = {(i & 1) ? 1.0f : -1.0f, (i & 2) ? 1.0f : -1.0f, (i & 4) ? 1.0f : -1.0f};
  } else if (shapeIdx == 1) {
    for (int s = -1; s <= 1; s += 2) {
      verts[vertCount++] = {(float)s, 0, 0};
      verts[vertCount++] = {0, (float)s, 0};
      verts[vertCount++] = {0, 0, (float)s};
    }
  } else {
    const float phi = 1.618034f;
    for (int a = -1; a <= 1; a += 2)
      for (int b = -1; b <= 1; b += 2) {
        verts[vertCount++] = {0, (float)a, b * phi};
        verts[vertCount++] = {(float)a, b * phi, 0};
        verts[vertCount++] = {b * phi, 0, (float)a};
      }
  }
  finishShape();
}

static void resetStar(Star& s, bool anyDepth) {
  s.x = (int32_t)(esp_random() % 640) - 320;
  s.y = (int32_t)(esp_random() % 340) - 170;
  s.z = anyDepth ? 1 + esp_random() % 255 : 256;
}

static void enter() {
  buildShape();
  for (auto& s : stars) resetStar(s, true);
  lastFrameUs = micros();
  fpsCounter.reset();
}

static void update() {
  uint32_t nowUs = micros();
  float dt = (nowUs - lastFrameUs) / 1e6f;
  lastFrameUs = nowUs;

  frame.fillSprite(TFT_BLACK);

  // Starfield
  const int16_t cx = SCREEN_W / 2, cy = SCREEN_H / 2;
  for (auto& s : stars) {
    s.z -= 120.0f * dt;
    if (s.z < 1) resetStar(s, false);
    int16_t sx = cx + s.x * 64 / s.z;
    int16_t sy = cy + s.y * 64 / s.z;
    if (sx < 0 || sx >= SCREEN_W || sy < 0 || sy >= SCREEN_H) {
      resetStar(s, false);
      continue;
    }
    uint8_t b = 255 - (uint8_t)min(255.0f, s.z);
    uint16_t c = rgb565(b, b, b);
    if (s.z < 64) frame.fillRect(sx, sy, 2, 2, c);
    else frame.drawPixel(sx, sy, c);
  }

  // Rotate + project
  ax += 0.9f * dt;
  ay += 1.3f * dt;
  az += 0.5f * dt;
  float sxA = sinf(ax), cxA = cosf(ax), syA = sinf(ay), cyA = cosf(ay), szA = sinf(az), czA = cosf(az);
  int16_t px[MAX_VERTS], py[MAX_VERTS];
  float   pz[MAX_VERTS];
  for (uint8_t i = 0; i < vertCount; i++) {
    Vec3 v = verts[i];
    float y1 = v.y * cxA - v.z * sxA, z1 = v.y * sxA + v.z * cxA;
    float x2 = v.x * cyA + z1 * syA,  z2 = -v.x * syA + z1 * cyA;
    float x3 = x2 * czA - y1 * szA,   y3 = x2 * szA + y1 * czA;
    float persp = CAMERA_DIST / (CAMERA_DIST + z2);
    px[i] = cx + x3 * SHAPE_SCALE * persp;
    py[i] = cy + y3 * SHAPE_SCALE * persp;
    pz[i] = z2;
  }

  // Edges: hue shifts around the solid, nearer edges drawn thicker
  hue += 1;
  for (uint8_t e = 0; e < edgeCount; e++) {
    uint8_t a = edges[e][0], b = edges[e][1];
    float depth = (pz[a] + pz[b]) * 0.5f;  // -1 (near) .. +1 (far)
    float width = 3.5f - depth * 1.5f;
    uint8_t v = 255 - (uint8_t)((depth + 1) * 60);
    frame.drawWideLine(px[a], py[a], px[b], py[b], width, hsv565(hue + e * 6, 220, v), TFT_BLACK);
  }

  frame.setTextDatum(BR_DATUM);
  frame.setTextColor(TFT_DARKGREY, TFT_BLACK);
  frame.drawString(SHAPE_NAMES[shapeIdx], SCREEN_W - 6, SCREEN_H - 4);

  fpsCounter.tick();
  drawFpsBadge(fpsCounter.fps);
  frame.pushSprite(0, 0);
}

static void action() {
  shapeIdx = (shapeIdx + 1) % SHAPE_COUNT;
  buildShape();
}

static void exitDemo() {}

const Demo DEMO_SHAPES = {"3D Shapes", "next shape", enter, update, action, exitDemo};
