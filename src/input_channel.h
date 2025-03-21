/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_INPUT_CHANNEL_H
#define YARR_INPUT_CHANNEL_H

#include "config.h"
#include "prefs.h"
#include "protocol.h"
#include <Arduino.h>

typedef enum {
  DIRECT,
  INVERTING,
  TOGGLE_RISING,
  TOGGLE_FALLING,
  TOGGLE_FADE
} SwitchMode;

class InputChannel
{
    byte _pins[YB_INPUT_CHANNEL_COUNT] = YB_INPUT_CHANNEL_PINS;

  protected:
    void setState(bool state);

  public:
    byte id = 0;
    char name[YB_CHANNEL_NAME_LENGTH];
    char defaultState[10];
    SwitchMode mode;
    bool isEnabled = true;

    unsigned int stateChangeCount = 0;

    bool raw = false;
    bool lastRaw = false;
    bool originalRaw = false;
    bool state = false;
    char source[YB_HOSTNAME_LENGTH];

    bool sendFastUpdate = false;

    void setup();
    void update();

    void setState(const char* state);
    const char* getState();

    static String getModeName(SwitchMode mode);
    static SwitchMode getMode(String mode);
};

extern InputChannel input_channels[YB_INPUT_CHANNEL_COUNT];

void input_channels_setup();
void input_channels_loop();
bool isValidInputChannel(byte cid);

#endif /* !YARR_INPUT_CHANNEL_H */