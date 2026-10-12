// WiFi Scanner — non-blocking async scan, strongest networks listed with
// RSSI bars. No credentials needed; the radio is switched off on exit.

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "debug.h"
#include "demos.h"

constexpr uint8_t MAX_ROWS = 7;
constexpr int16_t ROW_H = 20;
constexpr int16_t LIST_Y = 26;

static bool     scanning = false;
static uint32_t lastScanDone = 0;
static uint32_t lastSpinner = 0;
static uint8_t  spinnerStep = 0;
static int16_t  networkCount = -1;  // -1 = no results yet

struct Net {
  String  ssid;
  int32_t rssi;
  uint8_t channel;
  bool    open;
};
static Net nets[MAX_ROWS];
static uint8_t netRows = 0;

static void startScan() {
  WiFi.scanDelete();
  WiFi.scanNetworks(true);  // async — poll scanComplete()
  scanning = true;
  DBG_INFO("WiFi scan started");
}

static uint16_t rssiColour(int32_t rssi) {
  if (rssi > -60) return TFT_GREEN;
  if (rssi > -75) return TFT_YELLOW;
  return TFT_RED;
}

static void drawHeader() {
  frame.fillRect(0, 0, SCREEN_W, 22, rgb565(0, 60, 40));
  frame.setTextColor(TFT_WHITE, rgb565(0, 60, 40));
  frame.setTextDatum(ML_DATUM);
  String title = networkCount < 0 ? String("WiFi Scan") : String("WiFi Scan  -  ") + networkCount + " found";
  frame.drawString(title, 6, 11);
  if (scanning) {
    static const char* const SPIN[] = {"|", "/", "-", "\\"};
    frame.setTextDatum(MR_DATUM);
    frame.drawString(String("scanning ") + SPIN[spinnerStep & 3], SCREEN_W - 6, 11);
  }
}

static void drawAll() {
  frame.fillSprite(TFT_BLACK);
  drawHeader();

  if (networkCount == 0) {
    frame.setTextDatum(MC_DATUM);
    frame.setTextColor(TFT_DARKGREY, TFT_BLACK);
    frame.drawString("No networks found", SCREEN_W / 2, SCREEN_H / 2);
  }

  for (uint8_t i = 0; i < netRows; i++) {
    const Net& n = nets[i];
    int16_t y = LIST_Y + i * ROW_H;
    int16_t mid = y + ROW_H / 2;

    frame.fillCircle(8, mid, 3, n.open ? TFT_RED : TFT_DARKGREEN);  // red = open network
    String ssid = n.ssid.length() ? n.ssid : String("(hidden)");
    if (ssid.length() > 20) ssid = ssid.substring(0, 19) + "~";
    frame.setTextDatum(ML_DATUM);
    frame.setTextColor(TFT_WHITE, TFT_BLACK);
    frame.drawString(ssid, 18, mid);

    frame.setTextColor(TFT_DARKGREY, TFT_BLACK);
    frame.setTextDatum(MR_DATUM);
    frame.drawString(String("ch") + n.channel, 196, mid);

    // Bar: -100 dBm = empty, -30 dBm = full
    int16_t barW = constrain(map(n.rssi, -100, -30, 0, 70), 2, 70);
    frame.drawRoundRect(202, mid - 5, 72, 10, 3, TFT_DARKGREY);
    frame.fillRoundRect(203, mid - 4, barW, 8, 2, rssiColour(n.rssi));

    frame.setTextColor(rssiColour(n.rssi), TFT_BLACK);
    frame.drawString(String(n.rssi), SCREEN_W - 4, mid);
  }
  frame.pushSprite(0, 0);
}

static void collectResults(int16_t found) {
  networkCount = found;
  netRows = 0;
  // Pick the strongest MAX_ROWS networks (results are not guaranteed sorted)
  bool used[64] = {false};
  int16_t limit = min<int16_t>(found, 64);
  while (netRows < MAX_ROWS && netRows < limit) {
    int16_t best = -1;
    for (int16_t i = 0; i < limit; i++) {
      if (!used[i] && (best < 0 || WiFi.RSSI(i) > WiFi.RSSI(best))) best = i;
    }
    used[best] = true;
    nets[netRows++] = {WiFi.SSID(best), WiFi.RSSI(best), (uint8_t)WiFi.channel(best),
                       WiFi.encryptionType(best) == WIFI_AUTH_OPEN};
  }
  WiFi.scanDelete();
  DBG_INFO("WiFi scan done: %d networks", found);
}

static void enter() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  networkCount = -1;
  netRows = 0;
  startScan();
  drawAll();
}

static void update() {
  uint32_t now = millis();

  if (scanning) {
    int16_t result = WiFi.scanComplete();
    if (result >= 0 || result == WIFI_SCAN_FAILED) {
      scanning = false;
      lastScanDone = now;
      if (result >= 0) {
        collectResults(result);
      } else {
        DBG_WARN("WiFi scan failed");
      }
      drawAll();
    } else if (now - lastSpinner >= 150) {
      lastSpinner = now;
      spinnerStep++;
      drawAll();
    }
  } else if (now - lastScanDone >= WIFI_RESCAN_MS) {
    startScan();
  }
}

static void action() {
  if (!scanning) startScan();
}

static void exitDemo() {
  WiFi.scanDelete();
  WiFi.mode(WIFI_OFF);
  scanning = false;
}

const Demo DEMO_WIFISCAN = {"WiFi Scanner", "rescan now", enter, update, action, exitDemo};
