/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_PREFS_H
#define YARR_PREFS_H

#include "config.h"
#include "network.h"
#include "protocol.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>

// storage for more permanent stuff.
extern Preferences preferences;

bool prefs_setup();

bool loadConfigFromFile(const char* file, char* error);
bool loadConfigFromJSON(JsonVariantConst config, char* error);
bool loadNetworkConfigFromJSON(JsonVariantConst config, char* error);
bool loadAppConfigFromJSON(JsonVariantConst config, char* error);
bool loadBoardConfigFromJSON(JsonVariantConst config, char* error);

#endif /* !YARR_PREFS_H */