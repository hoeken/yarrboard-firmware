/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_INPUT_CHANNELS

#include "input_channel.h"

//the main star of the event
InputChannel input_channels[YB_INPUT_CHANNEL_COUNT];

unsigned long lastInputCheckMillis = 0;

void input_channels_setup()
{
  //intitialize our channel
  for (short i = 0; i < YB_INPUT_CHANNEL_COUNT; i++)
  {
    input_channels[i].id = i;
    input_channels[i].setup();
  }
}

void input_channels_loop()
{
  bool doSendFastUpdate = false;

  if (millis() - lastInputCheckMillis >= YB_INPUT_DEBOUNCE_RATE_MS)
  {
    Serial.printf("%d ", millis() - lastInputCheckMillis);
    
    //maintenance on our channels.
    for (byte id = 0; id < YB_INPUT_CHANNEL_COUNT; id++)
    {
      input_channels[id].update();

      if (input_channels[id].sendFastUpdate)
        doSendFastUpdate = true;
    }

    //let the client know immediately.
    if (doSendFastUpdate)
      sendFastUpdate();

    lastInputCheckMillis = millis();
  }
}

bool isValidInputChannel(byte cid)
{
  if (cid < 0 || cid >= YB_INPUT_CHANNEL_COUNT)
    return false;
  else
    return true;
}

void InputChannel::setup()
{
  char prefIndex[YB_PREF_KEY_LENGTH];

  //lookup our name
  sprintf(prefIndex, "iptName%d", this->id);
  if (preferences.isKey(prefIndex))
    strlcpy(this->name, preferences.getString(prefIndex).c_str(), sizeof(this->name));
  else
    sprintf(this->name, "Switch #%d", this->id);

  //enabled or no
  sprintf(prefIndex, "iptEnabled%d", this->id);
  if (preferences.isKey(prefIndex))
    this->isEnabled = preferences.getBool(prefIndex);
  else
    this->isEnabled = true;

  //input mode handling
  sprintf(prefIndex, "iptMode%d", this->id);
  if (preferences.isKey(prefIndex))
    this->mode = (SwitchMode)preferences.getUChar(prefIndex);
  else
    this->mode = DIRECT;

  //setup our pin
  pinMode(this->_pins[this->id], INPUT);
}

void InputChannel::update()
{
  //load it up!
  this->raw = digitalRead(this->_pins[this->id]);
  bool nextState = this->state;

  //first test is debounce.  needs steadystate
  if (this->raw == this->lastRaw)
  {
    //okay, did we actually change?
    if (this->raw != this->originalRaw)
    {
      //direct is easy
      if (this->mode == DIRECT)
        nextState = this->mode;
      //inverting is easy too
      else if (this->mode == INVERTING)
        nextState = !this->mode;
      //just toggle it on rising
      else if (this->raw && this->mode == TOGGLE_RISING)
        nextState = !this->state;
      //just toggle it on falling
      else if (!this->raw && this->mode == TOGGLE_FALLING)
        nextState = !this->state;
    }
  }

  //save our raw statuses
  this->originalRaw = this->lastRaw;
  this->lastRaw == this->raw;

  //did we actually change?
  if (nextState != this->state)
  {
    this->state = nextState;
    this->stateChangeCount++;
    this->sendFastUpdate = true;
  }
}

String InputChannel::getModeName(SwitchMode mode)
{
  if (mode == DIRECT)
    return "DIRECT";
  else if (mode == INVERTING)
    return "INVERTING";
  else if (mode == TOGGLE_RISING)
    return "TOGGLE_RISING";
  else if (mode == TOGGLE_FALLING)
    return "TOGGLE_FALLING";
  else if (mode == TOGGLE_FADE)
    return "TOGGLE_FADE";
  else
    return "";
}

SwitchMode InputChannel::getMode(String mode)
{
  if (mode.equals("DIRECT"))
    return DIRECT;
  else if (mode.equals("INVERTING"))
    return INVERTING;
  else if (mode.equals("TOGGLE_RISING"))
    return TOGGLE_RISING;
  else if (mode.equals("TOGGLE_FALLING"))
    return TOGGLE_FALLING;
  else if (mode.equals("TOGGLE_FADE"))
    return TOGGLE_FADE;
  else
    return DIRECT;
}

#endif