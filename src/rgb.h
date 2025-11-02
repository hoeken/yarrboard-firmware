/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_RGB_H
#define YARR_RGB_H

#include "FastLED.h"
#include "config.h"

void rgb_setup();
void rgb_loop();

void rgb_set_status_color(uint8_t r, uint8_t g, uint8_t b);
void rgb_set_status_color(const CRGB& color);
void rgb_set_pixel_color(uint8_t c, uint8_t r, uint8_t g, uint8_t b);
void rgb_set_pixel_color(uint8_t c, const CRGB& color);

#endif /* !YARR_RGB_H */