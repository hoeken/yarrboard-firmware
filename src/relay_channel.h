/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_RELAY_CHANNEL_H
#define YARR_RELAY_CHANNEL_H

#include "base_channel.h"
#include "config.h"
#include "etl/array.h"
#include "mqtt.h"
#include "prefs.h"
#include "protocol.h"
#include <Arduino.h>

#ifdef YB_RELAY_DRIVER_TCA9554
  #include "TCA9554.h"
#endif

class RelayChannel : public BaseChannel
{
  protected:
    byte _pin;
    char ha_topic_cmd_state[128];
    char ha_topic_state_state[128];

    bool outputState = false;

  public:
    bool defaultState = false;
    char type[30] = "other";
    char source[YB_HOSTNAME_LENGTH];

    unsigned int stateChangeCount = 0;

    void init(uint8_t id) override;
    bool loadConfig(JsonVariantConst config, char* error, size_t err_size) override;
    void generateConfig(JsonVariant config) override;
    void generateUpdate(JsonVariant config) override;

    void haGenerateDiscovery(JsonVariant doc);
    void haPublishState();
    void haHandleCommand(const char* topic, const char* payload);

    void setup();
    void setupDefaultState();
    void updateOutput();

    void setState(const char* state);
    void setState(bool newState);
    bool getState();

    const char* getStatus();
};

extern etl::array<RelayChannel, YB_RELAY_CHANNEL_COUNT> relay_channels;

void relay_channels_setup();
void relay_channels_loop();
void relay_handle_ha_command(const char* topic, const char* payload, int retain, int qos, bool dup);

#endif /* !YARR_RELAY_CHANNEL_H */