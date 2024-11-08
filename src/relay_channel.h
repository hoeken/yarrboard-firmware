/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_RELAY_CHANNEL_H
#define YARR_RELAY_CHANNEL_H

#include "config.h"
#include "prefs.h"
#include "protocol.h"
#include <Arduino.h>

class RelayChannel
{
  protected:
    byte _pins[YB_RELAY_CHANNEL_COUNT] = YB_RELAY_CHANNEL_PINS;

  public:
    /**
     * @brief Potential status that channel might be in.
     */
    enum class Status {
      ON,
      OFF
    };

    byte id = 0;

    Status status = Status::OFF;
    bool outputState = false;
    char name[YB_CHANNEL_NAME_LENGTH];
    char type[30];
    char defaultState[10];
    char source[YB_HOSTNAME_LENGTH];

    unsigned int stateChangeCount = 0;

    bool isEnabled = false;

    void setup();
    void setupDefaultState();
    void updateOutput(bool check_status = false);

    void setState(const char* state);
    void setState(bool newState);

    const char* getStatus();
};

extern RelayChannel relay_channels[YB_RELAY_CHANNEL_COUNT];

void relay_channels_setup();
void relay_channels_loop();
bool isValidRelayChannel(byte cid);

#endif /* !YARR_RELAY_CHANNEL_H */