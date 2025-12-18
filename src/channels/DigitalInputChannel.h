/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_DIGITAL_INPUT_CHANNEL_H
#define YARR_DIGITAL_INPUT_CHANNEL_H

#include "config.h"
#include "prefs.h"
#include "protocol.h"
#include <Arduino.h>
#include <channels/BaseChannel.h>

class DigitalInputChannel : public BaseChannel
{
  public:
    typedef enum {
      DIRECT,
      INVERTING,
      TOGGLE_RISING,
      TOGGLE_FALLING,
      TOGGLE_FADE
    } SwitchMode;

    bool sendFastUpdate = false;

    void setup();
    void update();

    void init(uint8_t id) override;
    bool loadConfig(JsonVariantConst config, char* error, size_t len) override;
    void generateConfig(JsonVariant config) override;
    void generateUpdate(JsonVariant update) override;

    void haGenerateDiscovery(JsonVariant doc, const char* uuid);
    void haPublishState();

    void setState(bool state);
    bool getState();

    static const char* getModeName(SwitchMode mode);
    static SwitchMode getMode(const char* mode);

  private:
    char ha_topic_state[128];

    byte pin = 0;
    SwitchMode mode;
    bool defaultState;

    unsigned int stateChangeCount = 0;

    bool raw = false;
    bool lastRaw = false;
    bool originalRaw = false;
    bool state = false;
};

void digital_input_channels_setup();
void digital_input_channels_loop();

extern etl::array<DigitalInputChannel, YB_DIGITAL_INPUT_CHANNEL_COUNT> digital_input_channels;

#endif /* !YARR_DIGITAL_INPUT_CHANNEL_H */