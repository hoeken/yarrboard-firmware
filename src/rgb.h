/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_RGB_H
#define YARR_RGB_H

#include "config.h"

#ifdef YB_HAS_STATUS_WS2818
  #include <Adafruit_NeoPixel.h>
#endif

void rgb_setup();
void rgb_loop();

void rgb_set_status_color(uint8_t r, uint8_t g, uint8_t b);
void rgb_set_pixel_color(uint8_t c, uint8_t r, uint8_t g, uint8_t b);

#endif /* !YARR_RGB_H */