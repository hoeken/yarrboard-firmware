/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "base_channel.h"

void BaseChannel::setup(byte newId, JsonVariant config)
{
  this->id = newId;
  this->parseConfigJSON(config);
}

bool BaseChannel::isValidId(unsigned int testId)
{
  return (this->id == testId);
}

bool BaseChannel::isValidKey(const char* testKey)
{
  return !strcmp(this->key, testKey);
}

bool BaseChannel::parseConfigJSON(JsonVariant config)
{
  // we need a valid object
  if (!config.is<JsonObject>())
    return false;
  JsonObject obj = config.as<JsonObject>();

  // parse the config id
  byte configId;
  if (obj["id"].is<unsigned long>()) {
    unsigned long v = obj["id"].as<unsigned long>();
    configId = static_cast<byte>(v & 0xFF);
  } else if (obj["id"].is<long>()) {
    long v = obj["id"].as<long>();
    if (v < 0)
      v = 0;
    if (v > 255)
      v = 255;
    configId = static_cast<byte>(v);
  } else
    return false;

  // config needs to match.
  if (!this->isValidId(configId))
    return false;

  // isEnabled (bool)
  if (obj["enabled"].is<bool>()) {
    this->isEnabled = obj["enabled"].as<bool>();
  } else {
    this->isEnabled = true;
  }

  // name (char[YB_CHANNEL_NAME_LENGTH])
  if (obj["name"].is<const char*>()) {
    const char* s = obj["name"].as<const char*>();
    std::strncpy(this->name, s, sizeof(this->name) - 1);
    this->name[sizeof(this->name) - 1] = '\0';
  } else {
    sprintf(this->name, "Channel %d", this->id);
  }

  // key (char[YB_CHANNEL_KEY_LENGTH])
  if (obj["key"].is<const char*>()) {
    const char* s = obj["key"].as<const char*>();
    std::strncpy(this->key, s, sizeof(this->key) - 1);
    this->key[sizeof(this->key) - 1] = '\0';
  } else {
    sprintf(this->name, "ch%d", this->id);
  }

  return true;
}

void BaseChannel::generateConfigJSON(JsonVariant config)
{
  config["id"] = this->id;
  config["name"] = this->name;
  config["key"] = this->key;
  config["enabled"] = this->isEnabled;
}