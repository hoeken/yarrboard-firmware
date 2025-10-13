/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_BASE_CHANNEL_H
#define YARR_BASE_CHANNEL_H

#include "ArduinoJson.h"
#include "config.h"
#include <cstring> // for strncpy

class BaseChannel
{
  public:
    byte id = 0;
    bool isEnabled = true;
    char name[YB_CHANNEL_NAME_LENGTH];
    char key[YB_CHANNEL_KEY_LENGTH];

    void setup(byte newId, JsonVariant config);
    bool isValidId(unsigned int testId);
    bool isValidKey(const char* testKey);

    void generateConfigJSON(JsonVariant config);
    bool parseConfigJSON(JsonVariant config);
};

#endif /* !YARR_BASE_CHANNEL_H */