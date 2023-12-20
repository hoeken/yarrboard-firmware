/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_INPUT_CHANNEL_H
#define YARR_INPUT_CHANNEL_H

#include <Arduino.h>
#include "config.h"
#include "prefs.h"
#include "protocol.h"

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

  public:
    byte id = 0;
    char name[YB_CHANNEL_NAME_LENGTH];
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
    void setState(bool state);
    const char * getState();

    static String getModeName(SwitchMode mode);
    static SwitchMode getMode(String mode);
};

extern InputChannel input_channels[YB_INPUT_CHANNEL_COUNT];

void input_channels_setup();
void input_channels_loop();
bool isValidInputChannel(byte cid);

#endif /* !YARR_INPUT_CHANNEL_H */