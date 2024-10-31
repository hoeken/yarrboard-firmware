/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_FANS_H
#define YARR_FANS_H

extern int fans_last_rpm[YB_FAN_COUNT];

void fans_setup();
void fans_loop();
void measure_fan_rpm(byte i);
void set_fans_state(bool state);

#endif /* !YARR_FANS_H */