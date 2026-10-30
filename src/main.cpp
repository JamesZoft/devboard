#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "image.h"

#define TFT_SCLK 3
#define TFT_MOSI 4
#define TFT_CS   2
#define TFT_DC   0
#define TFT_RST  5

#define BTN_PREV 8    // IO8
#define BTN_NEXT 10   // IO10

#define BL_PIN -1     // set to the backlight GPIO if the schematic shows one

constexpr uint32_t IDLE_MS = 60000;   // time before deep sleep

Adafruit_ST7735 tft(&SPI, TFT_CS, TFT_DC, TFT_RST);

int current = 0;
uint32_t lastActivity = 0;

void show(int i) {
  tft.drawRGBBitmap(0, 0, images[i], IMG_W, IMG_H);
}

// Returns true once per press (active-low button, 40 ms debounce)
bool pressed(uint8_t pin, bool &last, uint32_t &changedAt) {
  bool now = digitalRead(pin) == LOW;
  if (now != last && millis() - changedAt > 40) {
    last = now;
    changedAt = millis();
    return now;
  }
  return false;
}

void goToSleep() {
  tft.enableDisplay(false);
  tft.enableSleep(true);                 // panel sleep-in command
  if (BL_PIN >= 0) digitalWrite(BL_PIN, LOW);

  // Park the display lines in a safe state so a floating pin can't wake or reset the panel
  digitalWrite(TFT_RST, HIGH);
  digitalWrite(TFT_CS, HIGH);
  gpio_hold_en((gpio_num_t)TFT_RST);
  gpio_hold_en((gpio_num_t)TFT_CS);
  if (BL_PIN >= 0 && BL_PIN <= 5) gpio_hold_en((gpio_num_t)BL_PIN);
  gpio_deep_sleep_hold_en();

  esp_deep_sleep_start();                // no wake source: sleeps until RESET
}

void setup() {
  // Release any pin holds left over from a previous deep sleep
  gpio_deep_sleep_hold_dis();
  gpio_hold_dis((gpio_num_t)TFT_RST);
  gpio_hold_dis((gpio_num_t)TFT_CS);

  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  if (BL_PIN >= 0) { pinMode(BL_PIN, OUTPUT); digitalWrite(BL_PIN, HIGH); }

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.initR(INITR_144GREENTAB);
  show(current);
  lastActivity = millis();
}

void loop() {
  static bool prevDown = false, nextDown = false;
  static uint32_t prevAt = 0, nextAt = 0;

  if (pressed(BTN_NEXT, nextDown, nextAt)) {
    current = (current + 1) % IMG_COUNT;
    show(current);
    lastActivity = millis();
  }
  if (pressed(BTN_PREV, prevDown, prevAt)) {
    current = (current + IMG_COUNT - 1) % IMG_COUNT;
    show(current);
    lastActivity = millis();
  }

  if (millis() - lastActivity > IDLE_MS) goToSleep();
}