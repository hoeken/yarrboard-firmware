/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_DEBUG_H
#define YARR_DEBUG_H

#include <Arduino.h>
#include <LittleFS.h>
#include <esp_core_dump.h>
#include <esp_partition.h>
#include <esp_system.h>

extern bool has_coredump;

void debug_setup();

String getResetReason();
bool checkCoreDump();
String readCoreDump();
bool deleteCoreDump();
void crash_me_hard();

#endif /* !YARR_PREFS_H */