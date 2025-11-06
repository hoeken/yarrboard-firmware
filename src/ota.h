/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_OTA_H
#define YARR_OTA_H

#include "utility.h"
#include "yb_server.h"
#include <ArduinoOTA.h>

#define DISABLE_ALL_LIBRARY_WARNINGS
#include <esp32FOTA.hpp>

extern esp32FOTA FOTA;
extern bool doOTAUpdate;
extern unsigned long ota_last_message;

void ota_setup();
void ota_loop();
void ota_end();

#endif /* !YARR_OTA_H */