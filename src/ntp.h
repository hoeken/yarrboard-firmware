/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_NTP_H
#define YARR_NTP_H

#include "esp_sntp.h"
#include "time.h"
#include <Arduino.h>

void ntp_setup();
void ntp_loop();

void timeAvailable(struct timeval* t);
void printLocalTime();

#endif /* !YARR_NTP_H */