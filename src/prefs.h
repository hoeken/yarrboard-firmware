/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_PREFS_H
#define YARR_PREFS_H

#include "config.h"
#include "etl/array.h"
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

// this needs to be defined in the header due to how templates work
template <typename Channel, size_t N>
bool loadChannelsConfigFromJSON(const char* channel_key,
  etl::array<Channel, N>& channels,
  JsonVariantConst config,
  char* error,
  size_t error_len)
{
  DUMP(channel_key);
  DUMP(N);

  if (config[channel_key]) {
    for (byte i = 1; i <= N; i++) {
      bool found = false;
      for (JsonVariantConst ch_config : config[channel_key].as<JsonArrayConst>()) {
        if (ch_config["id"] == i) {
          if (channels[i - 1].loadConfigFromJSON(ch_config, error))
            found = true;
          else
            return false;
        }
      }

      if (!found) {
        snprintf(error, error_len, "Missing 'board.%s' #%d config", channel_key, i);
        return false;
      }
    }
  } else {
    snprintf(error, error_len, "Missing 'board.%s' config", channel_key);
    return false;
  }

  return true;
}

#endif /* !YARR_PREFS_H */