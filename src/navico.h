/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_NAVICO_H
#define YARR_NAVICO_H

#include <Arduino.h>
#include "protocol.h"
#include "server.h"

void navico_setup();
void navico_loop();

#endif /* !YARR_NAVICO_H */