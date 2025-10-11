/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "rgb.h"

#ifdef YB_HAS_STATUS_WS2818
Adafruit_NeoPixel status_led(YB_STATUS_WS2818_COUNT, YB_STATUS_WS2818_PIN, NEO_RGB + NEO_KHZ800);
#endif

void rgb_setup()
{
#ifdef YB_HAS_STATUS_WS2818
  status_led.begin();
  status_led.setPixelColor(0, status_led.Color(0, 0, 255));
  status_led.setBrightness(50);
  status_led.show();
#endif
}

void rgb_loop()
{
}

void rgb_set_status_color(uint8_t r, uint8_t g, uint8_t b)
{
#ifdef YB_HAS_STATUS_WS2818
  status_led.setPixelColor(0, r, g, b);
  status_led.show();
#endif
}

void rgb_set_pixel_color(uint8_t c, uint8_t r, uint8_t g, uint8_t b)
{
#ifdef YB_HAS_STATUS_WS2818
  // out of bounds?
  if (c >= YB_STATUS_WS2818_COUNT)
    return;

  status_led.setPixelColor(c, r, g, b);
  status_led.show();
#endif
}