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
#include "etl/array.h"
#include <cstring> // for strncpy

class BaseChannel
{
  public:
    BaseChannel();

    byte id = 0;
    bool isEnabled = true;
    char name[YB_CHANNEL_NAME_LENGTH];
    char key[YB_CHANNEL_KEY_LENGTH];

    void setup();
    bool isValidId(unsigned int testId);
    bool isValidKey(const char* testKey);

    virtual void init(uint8_t id);
    virtual bool loadConfig(JsonVariantConst config, char* error, size_t err_size);
    virtual void generateConfig(JsonVariant config);
    virtual void generateUpdate(JsonVariant config);
};

template <typename Channel, size_t N>
Channel* getChannelById(uint8_t id, etl::array<Channel, N>& channels)
{
  static_assert(std::is_base_of<BaseChannel, Channel>::value,
    "Channel must derive from BaseChannel");

  for (auto& ch : channels) {
    if (ch.id == id)
      return &ch;
  }
  return nullptr;
}

template <typename Channel, size_t N>
Channel* getChannelByKey(const char* key, etl::array<Channel, N>& channels)
{
  static_assert(std::is_base_of<BaseChannel, Channel>::value,
    "Channel must derive from BaseChannel");

  for (auto& ch : channels) {
    if (!strcmp(key, ch.key))
      return &ch;
  }
  return nullptr;
}

#endif /* !YARR_BASE_CHANNEL_H */