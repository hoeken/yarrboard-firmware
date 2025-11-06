/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "rgb.h"

CRGB _leds[YB_STATUS_RGB_COUNT];

void rgb_setup()
{
  FastLED.addLeds<YB_STATUS_RGB_TYPE, YB_STATUS_RGB_PIN, YB_STATUS_RGB_ORDER>(_leds, YB_STATUS_RGB_COUNT);
  FastLED.setBrightness(32);
  FastLED.clear();
  rgb_set_status_color(CRGB::Blue);
  FastLED.show();
}

unsigned long lastRGBUpdateMillis = 0;

void rgb_loop()
{
  // 10hz refresh
  if (millis() - lastRGBUpdateMillis > 100) {
    FastLED.show();
    lastRGBUpdateMillis = millis();
  }
}

void rgb_set_status_color(uint8_t r, uint8_t g, uint8_t b)
{
  rgb_set_pixel_color(0, r, g, b);
}

void rgb_set_pixel_color(uint8_t c, uint8_t r, uint8_t g, uint8_t b)
{
  // out of bounds?
  if (c >= YB_STATUS_RGB_COUNT)
    return;

  _leds[c].setRGB(r, g, b);

  if (millis() - lastRGBUpdateMillis > 100) {
    FastLED.show();
    lastRGBUpdateMillis = millis();
  }
}

void rgb_set_status_color(const CRGB& color)
{
  rgb_set_pixel_color(0, color);
}

void rgb_set_pixel_color(uint8_t c, const CRGB& color)
{
  _leds[c] = color;

  if (millis() - lastRGBUpdateMillis > 100) {
    FastLED.show();
    lastRGBUpdateMillis = millis();
  }
}
