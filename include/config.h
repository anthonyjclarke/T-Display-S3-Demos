#pragma once

#include <Arduino.h>

// ---------- Board pins (not covered by TFT_eSPI build flags) ----------
constexpr uint8_t PIN_POWER_ON = 15;  // must be HIGH or the LCD is dark on battery
constexpr uint8_t PIN_BUTTON_1 = 0;   // BOOT button — next demo
constexpr uint8_t PIN_BUTTON_2 = 14;  // KEY button — demo action / long-press brightness
constexpr uint8_t PIN_BAT_VOLT = 4;   // 1:2 divider to battery
constexpr uint8_t PIN_I2C_SDA  = 18;  // touch variant only (CST816 @ 0x15)
constexpr uint8_t PIN_I2C_SCL  = 17;

// ---------- Display ----------
constexpr uint8_t  SCREEN_ROTATION   = 1;     // landscape, buttons on the right
constexpr uint8_t  BL_LEDC_CHANNEL   = 0;
constexpr uint32_t BL_LEDC_FREQ      = 2000;  // matches LilyGO examples
constexpr uint8_t  BL_LEDC_BITS      = 8;
constexpr uint8_t  BRIGHTNESS_LEVELS[] = {40, 90, 160, 255};
constexpr uint8_t  BRIGHTNESS_DEFAULT_IDX = 2;

// ---------- Buttons ----------
constexpr uint32_t BTN_DEBOUNCE_MS   = 30;
constexpr uint32_t BTN_LONG_PRESS_MS = 600;

// ---------- Timing ----------
constexpr uint32_t AUTO_CYCLE_MS      = 0;  // attract mode: advance demo every N ms (0 = off)
constexpr uint32_t SYSINFO_REFRESH_MS = 1000;
constexpr uint32_t WIFI_RESCAN_MS     = 15000;
constexpr uint32_t BATTERY_SAMPLE_MS  = 2000;
