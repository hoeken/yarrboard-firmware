/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "base_channel.h"

BaseChannel::BaseChannel()
{
}

void BaseChannel::init(uint8_t id)
{
  this->id = id;
  this->isEnabled = true;
  snprintf(this->name, sizeof(this->name), "Channel %d", id);
  snprintf(this->key, sizeof(this->key), "%d", id);

  DUMP(this->name);
}

void BaseChannel::setup()
{
}

bool BaseChannel::isValidId(unsigned int testId)
{
  return (this->id == testId);
}

bool BaseChannel::isValidKey(const char* testKey)
{
  return !strcmp(this->key, testKey);
}

bool BaseChannel::loadConfig(JsonVariantConst config, char* error, size_t err_size)
{
  TRACE();

  // we need a valid object
  if (!config.is<JsonObjectConst>()) {
    strlcpy(error, "No JsonObject passed to loadConfig()", err_size);
    return false;
  }

  // we need a valid id
  if (config["id"])
    this->id = (int)config["id"];
  else {
    strlcpy(error, "'id' is a required parameter for channel config.", err_size);
    return false;
  }

  // enabled.  missing defaults to true
  this->isEnabled = config["enabled"] | true;

  // every channel has a name
  snprintf(this->name, sizeof(this->name), "Channel %d", this->id);
  if (config["name"]) {
    if (strlen(config["name"]) >= sizeof(this->name)) {
      snprintf(error, err_size, "Channel name max length %d characters", sizeof(this->name) - 1);
      return false;
    }
    strlcpy(this->name, config["name"], sizeof(this->name));
  }

  DUMP(this->key);
  // optional key - must not be an empty string.
  snprintf(this->key, sizeof(this->key), "%d", this->id);
  DUMP(this->key);
  const char* val = config["key"].as<const char*>();
  if (val && *val && strlen(val)) {
    if (strlen(config["key"]) >= sizeof(this->key)) {
      snprintf(error, err_size, "Channel key max length %d characters", sizeof(this->key) - 1);
      return false;
    }

    // gotta be alphanumeric plus dash or underscore
    for (const char* p = val; *p; p++) {
      if (!(isalnum((unsigned char)*p) || *p == '-' || *p == '_')) {
        snprintf(error, err_size, "Channel key contains invalid character: '%c'", *p);
        return false;
      }
    }

    // okay save it to our object
    strlcpy(this->key, val, sizeof(this->key));
  } else {
    Serial.println("empty key");
  }

  DUMP(this->key);

  return true;
}

void BaseChannel::generateConfig(JsonVariant config)
{
  config["id"] = this->id;
  config["name"] = this->name;
  config["key"] = this->key;
  config["enabled"] = this->isEnabled;
}

void BaseChannel::generateUpdate(JsonVariant config)
{
  config["id"] = this->id;
}