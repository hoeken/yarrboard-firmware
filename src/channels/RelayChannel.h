/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_RELAY_CHANNEL_H
#define YARR_RELAY_CHANNEL_H

#include "config.h"
#include "etl/array.h"
#include <Arduino.h>
#include <channels/BaseChannel.h>

class MQTTController;

class RelayChannel : public BaseChannel
{
  protected:
    byte _pin;
    bool outputState = false;

  public:
    bool defaultState = false;
    char type[30] = "other";
    char source[YB_HOSTNAME_LENGTH];
    bool inverted = false;

    unsigned int stateChangeCount = 0;

    void init(uint8_t id) override;
    bool loadConfig(JsonVariantConst config, char* error, size_t err_size) override;
    void generateConfig(JsonVariant config) override;
    void generateUpdate(JsonVariant config) override;

    void setup(byte pin);
    void setupDefaultState();
    void updateOutput();

    void setState(const char* state);
    void setState(bool newState);
    bool getState();

    const char* getStatus();
};

#endif /* !YARR_RELAY_CHANNEL_H */