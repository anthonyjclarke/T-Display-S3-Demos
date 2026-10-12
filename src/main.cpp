// LilyGO T-Display S3 — capability demos
//
// BOOT (GPIO0)  short: next demo        long: previous demo
// KEY  (GPIO14) short: demo action      long: cycle backlight brightness

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "config.h"
#include "debug.h"
#include "demos.h"
#include "fonts.h"

uint8_t debugLevel = DEBUG_LEVEL;

TFT_eSPI    tft;
TFT_eSprite frame(&tft);

static const Demo* const DEMOS[] = {
  &DEMO_SYSINFO, &DEMO_PLASMA, &DEMO_FIRE, &DEMO_SHAPES, &DEMO_WIFISCAN,
};
constexpr uint8_t DEMO_COUNT = sizeof(DEMOS) / sizeof(DEMOS[0]);
constexpr uint32_t TITLE_CARD_MS = 1200;

static uint8_t  currentDemo   = 0;
static uint32_t titleCardUntil = 0;
static uint8_t  brightnessIdx = BRIGHTNESS_DEFAULT_IDX;
static uint32_t lastSwitch = 0;

// ---------- Shared helpers (declared in demos.h) ----------

uint16_t hsv565(uint8_t h, uint8_t s, uint8_t v) {
  if (s == 0) return rgb565(v, v, v);
  uint8_t region = h / 43;
  uint8_t rem = (h - region * 43) * 6;
  uint8_t p = (v * (255 - s)) >> 8;
  uint8_t q = (v * (255 - ((s * rem) >> 8))) >> 8;
  uint8_t t = (v * (255 - ((s * (255 - rem)) >> 8))) >> 8;
  switch (region) {
    case 0:  return rgb565(v, t, p);
    case 1:  return rgb565(q, v, p);
    case 2:  return rgb565(p, v, t);
    case 3:  return rgb565(p, q, v);
    case 4:  return rgb565(t, p, v);
    default: return rgb565(v, p, q);
  }
}

void drawFpsBadge(uint16_t fps) {
  char buf[12];
  snprintf(buf, sizeof(buf), "%u fps", fps);
  frame.fillRoundRect(4, 4, 62, 20, 4, TFT_BLACK);
  frame.setTextColor(TFT_WHITE, TFT_BLACK);
  frame.setTextDatum(ML_DATUM);
  frame.drawString(buf, 10, 15);
}

// ---------- Buttons ----------

struct Button {
  uint8_t  pin;
  bool     down;          // debounced state
  bool     rawLast;
  uint32_t rawChangedAt;
  uint32_t pressedAt;
  bool     longFired;
};

enum class Press { None, Short, Long };

// Start as "held, long already fired" so a button must be seen released before
// it can report. GPIO0 is a strapping pin and reads LOW briefly after reset,
// which otherwise registers as a phantom BOOT press on every boot.
static Button btnBoot = {PIN_BUTTON_1, true, true, 0, 0, true};
static Button btnKey  = {PIN_BUTTON_2, true, true, 0, 0, true};

// Short press fires on release; long press fires once while still held.
static Press pollButton(Button& b) {
  uint32_t now = millis();
  bool raw = digitalRead(b.pin) == LOW;
  if (raw != b.rawLast) {
    b.rawLast = raw;
    b.rawChangedAt = now;
  }
  if (now - b.rawChangedAt < BTN_DEBOUNCE_MS) return Press::None;

  if (raw && !b.down) {
    b.down = true;
    b.pressedAt = now;
    b.longFired = false;
  } else if (!raw && b.down) {
    b.down = false;
    if (!b.longFired) return Press::Short;
  } else if (raw && b.down && !b.longFired && now - b.pressedAt >= BTN_LONG_PRESS_MS) {
    b.longFired = true;
    return Press::Long;
  }
  return Press::None;
}

// ---------- Display ----------

static void setBrightness(uint8_t level) {
  ledcWrite(BL_LEDC_CHANNEL, level);
  DBG_INFO("Brightness %u/255", level);
}

static void drawTitleCard() {
  const Demo* d = DEMOS[currentDemo];
  char counter[8];
  snprintf(counter, sizeof(counter), "%u / %u", currentDemo + 1, DEMO_COUNT);

  frame.fillSprite(TFT_BLACK);
  for (int16_t x = 0; x < SCREEN_W; x++) {
    frame.drawFastVLine(x, 0, 4, hsv565(x * 255 / SCREEN_W, 255, 255));
  }
  frame.setTextDatum(TR_DATUM);
  frame.setTextColor(TFT_DARKGREY, TFT_BLACK);
  frame.drawString(counter, SCREEN_W - 8, 12);

  frame.unloadFont();
  frame.loadFont(NotoSansBold36);
  frame.setTextDatum(MC_DATUM);
  frame.setTextColor(TFT_WHITE, TFT_BLACK);
  frame.drawString(d->name, SCREEN_W / 2, 72);
  frame.unloadFont();
  frame.loadFont(NotoSansBold15);

  frame.setTextColor(TFT_CYAN, TFT_BLACK);
  frame.drawString(String("KEY: ") + d->hint, SCREEN_W / 2, 118);
  frame.setTextColor(TFT_DARKGREY, TFT_BLACK);
  frame.drawString("BOOT: next   |   hold KEY: brightness", SCREEN_W / 2, 150);
  frame.pushSprite(0, 0);
}

static void switchDemo(int8_t step) {
  DEMOS[currentDemo]->exit();
  currentDemo = (currentDemo + DEMO_COUNT + step) % DEMO_COUNT;
  DBG_INFO("Demo -> %s", DEMOS[currentDemo]->name);
  DEMOS[currentDemo]->enter();
  drawTitleCard();
  titleCardUntil = millis() + TITLE_CARD_MS;
  lastSwitch = millis();
}

// ---------- Init ----------

static void initPower() {
  // GPIO15 enables the LCD/peripheral rail — without it the screen is dark on battery
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);
}

static bool initDisplay() {
  tft.init();
  tft.setRotation(SCREEN_ROTATION);
  tft.fillScreen(TFT_BLACK);

  // Take the backlight over from TFT_eSPI's plain digitalWrite
  ledcSetup(BL_LEDC_CHANNEL, BL_LEDC_FREQ, BL_LEDC_BITS);
  ledcAttachPin(TFT_BL, BL_LEDC_CHANNEL);
  setBrightness(BRIGHTNESS_LEVELS[brightnessIdx]);

  frame.setColorDepth(16);
  if (frame.createSprite(SCREEN_W, SCREEN_H) == nullptr) {
    DBG_ERROR("Frame sprite allocation failed (PSRAM missing?)");
    return false;
  }
  frame.loadFont(NotoSansBold15);
  return true;
}

void setup() {
  Serial.begin(115200);
  initPower();
  DBG_INFO("T-Display S3 demos starting");
  DBG_INFO("PSRAM: %u bytes", ESP.getPsramSize());

  if (!initDisplay()) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("Sprite alloc failed", 10, 10, 2);
    while (true) yield();  // nothing useful can run without the frame buffer
  }

  DEMOS[currentDemo]->enter();
  drawTitleCard();
  titleCardUntil = millis() + TITLE_CARD_MS;

  if (DEBUG_LEVEL >= 3) {
    DBG_INFO("Setup done — free heap %u, free PSRAM %u", ESP.getFreeHeap(), ESP.getFreePsram());
  }
}

void loop() {
  Press boot = pollButton(btnBoot);
  Press key  = pollButton(btnKey);

  if (boot == Press::Short) switchDemo(+1);
  if (boot == Press::Long)  switchDemo(-1);
  if (AUTO_CYCLE_MS > 0 && millis() - lastSwitch >= AUTO_CYCLE_MS) switchDemo(+1);

  if (key == Press::Long) {
    brightnessIdx = (brightnessIdx + 1) % sizeof(BRIGHTNESS_LEVELS);
    setBrightness(BRIGHTNESS_LEVELS[brightnessIdx]);
  }

  // Hold the title card for a moment; any KEY press dismisses it
  if ((int32_t)(millis() - titleCardUntil) < 0) {
    if (key == Press::Short) titleCardUntil = millis();
    return;
  }

  if (key == Press::Short) DEMOS[currentDemo]->action();
  DEMOS[currentDemo]->update();
}
