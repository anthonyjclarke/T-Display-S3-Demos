// System info — chip, memory, battery, temperature, plus a display
// fill-rate benchmark that shows off the 8-bit parallel bus.

#include <Arduino.h>
#include <esp_chip_info.h>

#include "config.h"
#include "debug.h"
#include "demos.h"

static uint32_t lastRefresh = 0;
static uint32_t lastBatterySample = 0;
static float    batteryVolts = 0;
static float    benchFps = 0;  // 0 = not run yet

static float readBatteryVolts() {
  uint32_t mv = 0;
  for (uint8_t i = 0; i < 8; i++) mv += analogReadMilliVolts(PIN_BAT_VOLT);
  return (mv / 8) * 2 / 1000.0f;  // 1:2 divider on the board
}

static void row(int16_t x, int16_t y, const char* label, const String& value, uint16_t colour) {
  frame.setTextDatum(TL_DATUM);
  frame.setTextColor(TFT_DARKGREY, TFT_BLACK);
  frame.drawString(label, x, y);
  frame.setTextColor(colour, TFT_BLACK);
  frame.drawString(value, x + 62, y);
}

static void enter() {
  lastRefresh = 0;
  lastBatterySample = 0;
}

static void update() {
  uint32_t now = millis();
  if (lastBatterySample == 0 || now - lastBatterySample >= BATTERY_SAMPLE_MS) {
    lastBatterySample = now;
    batteryVolts = readBatteryVolts();
  }
  if (lastRefresh != 0 && now - lastRefresh < SYSINFO_REFRESH_MS) return;
  lastRefresh = now;

  esp_chip_info_t chip;
  esp_chip_info(&chip);
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  uint32_t up = now / 1000;
  char upStr[16];
  snprintf(upStr, sizeof(upStr), "%02lu:%02lu:%02lu",
           (unsigned long)(up / 3600), (unsigned long)(up / 60 % 60), (unsigned long)(up % 60));

  // Above ~4.3 V the reading is the USB/charger rail, not the cell
  bool onUsb = batteryVolts > 4.3f;
  String batt = String(batteryVolts, 2) + " V" + (onUsb ? "  (USB)" : "");
  uint16_t battColour = onUsb ? TFT_CYAN : batteryVolts > 3.7f ? TFT_GREEN
                                         : batteryVolts > 3.4f ? TFT_YELLOW : TFT_RED;

  frame.fillSprite(TFT_BLACK);
  frame.fillRect(0, 0, SCREEN_W, 22, rgb565(0, 40, 90));
  frame.setTextDatum(ML_DATUM);
  frame.setTextColor(TFT_WHITE, rgb565(0, 40, 90));
  frame.drawString(String(ESP.getChipModel()) + " rev " + ESP.getChipRevision(), 6, 11);
  frame.setTextDatum(MR_DATUM);
  frame.drawString(upStr, SCREEN_W - 6, 11);

  const int16_t c1 = 6, c2 = 166, y0 = 30, dy = 19;
  row(c1, y0 + dy * 0, "CPU",   String(chip.cores) + " x " + ESP.getCpuFreqMHz() + " MHz", TFT_WHITE);
  row(c1, y0 + dy * 1, "Flash", String(ESP.getFlashChipSize() >> 20) + " MB", TFT_WHITE);
  row(c1, y0 + dy * 2, "PSRAM", String(ESP.getPsramSize() >> 20) + " MB", TFT_WHITE);
  row(c1, y0 + dy * 3, "Temp",  String(temperatureRead(), 1) + " C", TFT_ORANGE);
  row(c1, y0 + dy * 4, "Batt",  batt, battColour);

  row(c2, y0 + dy * 0, "Heap",  String(ESP.getFreeHeap() / 1024) + " KB", TFT_GREEN);
  row(c2, y0 + dy * 1, "Min",   String(ESP.getMinFreeHeap() / 1024) + " KB", TFT_GREEN);
  row(c2, y0 + dy * 2, "PS free", String(ESP.getFreePsram() / 1024) + " KB", TFT_GREEN);
  row(c2, y0 + dy * 3, "SDK",   ESP.getSdkVersion(), TFT_WHITE);
  row(c2, y0 + dy * 4, "Sketch", String(ESP.getSketchSize() / 1024) + " KB", TFT_WHITE);

  row(c1, y0 + dy * 5, "MAC", macStr, TFT_LIGHTGREY);
  row(c1, y0 + dy * 6, "Fill",
      benchFps > 0 ? String(benchFps, 1) + " full-screen fps" : String("press KEY to benchmark"),
      benchFps > 0 ? TFT_MAGENTA : TFT_DARKGREY);

  frame.pushSprite(0, 0);
}

// Blocks for ~half a second — only runs on an explicit button press.
static void action() {
  const uint16_t colours[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_BLACK};
  constexpr uint8_t ROUNDS = 40;
  uint32_t start = micros();
  for (uint8_t i = 0; i < ROUNDS; i++) tft.fillScreen(colours[i & 3]);
  uint32_t elapsed = micros() - start;
  benchFps = ROUNDS * 1e6f / elapsed;
  DBG_INFO("Fill benchmark: %.1f fps (%.1f MB/s)", benchFps,
           benchFps * SCREEN_W * SCREEN_H * 2 / 1e6f);
  lastRefresh = 0;  // redraw immediately
}

static void exitDemo() {}

const Demo DEMO_SYSINFO = {"System Info", "run fill benchmark", enter, update, action, exitDemo};
