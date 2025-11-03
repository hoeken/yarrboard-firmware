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
#include "networking.h"
#include "protocol.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>

extern String arduino_version;

// storage for more permanent stuff.
extern Preferences preferences;
extern bool is_first_boot;

bool prefs_setup();

void initializeChannels();

void generateFullConfigJSON(JsonVariant output);
void generateBoardConfigJSON(JsonVariant output);
void generateAppConfigJSON(JsonVariant output);
void generateNetworkConfigJSON(JsonVariant output);

bool saveConfig(char* error, size_t len);

bool loadConfigFromFile(const char* file, char* error, size_t len);
bool loadConfigFromJSON(JsonVariantConst config, char* error, size_t len);
bool loadNetworkConfigFromJSON(JsonVariantConst config, char* error, size_t len);
bool loadAppConfigFromJSON(JsonVariantConst config, char* error, size_t len);
bool loadBoardConfigFromJSON(JsonVariantConst config, char* error, size_t len);

// this needs to be defined in the header due to how templates work
template <typename Channel, size_t N>
void initChannels(etl::array<Channel, N>& channels)
{
  for (byte i = 0; i < N; i++)
    channels[i].init(i + 1); // human indexed
}

// this needs to be defined in the header due to how templates work
template <typename Channel, size_t N>
bool loadChannelsConfigFromJSON(const char* channel_key,
  etl::array<Channel, N>& channels,
  JsonVariantConst config,
  char* error,
  size_t len)
{
  // did we get a config entry?
  if (config[channel_key]) {
    // populate our exact channel count
    for (byte i = 0; i < N; i++) {
      channels[i].init(i + 1); // load default values per channel.  channels are 1 indexed for humans.
    }

    // now iterate over our initialized channels
    for (auto& ch : channels) {
      bool found = false;

      // loop over our json config to see if we find a match
      for (JsonVariantConst ch_config : config[channel_key].as<JsonArrayConst>()) {

        // channels are one indexed for humans
        if (ch_config["id"] == ch.id) {

          // did we get a non-empty key?
          const char* val = ch_config["key"].as<const char*>();
          if (val && *val) {
            for (auto& test_ch : channels) {
              // did we find any with a different id?
              if (!strcmp(val, test_ch.key) && ch.id != test_ch.id) {
                snprintf(error, len, "%s channel #%d - duplicate key: %d/%s", channel_key, ch.id, test_ch.id, val);
                return false;
              }
            }
          }

          // okay, attempt to load our config.
          if (ch.loadConfig(ch_config, error, len))
            found = true;
          else
            return false;
        }
      }

      if (!found) {
        snprintf(error, len, "Missing 'board.%s' #%d config", channel_key, ch.id);
        return false;
      }
    }
  } else {
    snprintf(error, len, "Missing 'board.%s' config", channel_key);
    return false;
  }

  return true;
}

#endif /* !YARR_PREFS_H */