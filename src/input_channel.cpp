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
    //Serial.printf("%d ", millis() - lastInputCheckMillis);
    
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

  //load up our default state
  bool nextState = digitalRead(this->_pins[this->id]);
  if (this->mode == DIRECT)
    this->state = nextState;
  else if (this->mode == INVERTING)
    this->state = !nextState;

  //save our current value
  this->originalRaw = nextState;

  if (this->state)
    rgb_channels[this->id].setRGB(0, 1.0, 0);
  else
    rgb_channels[this->id].setRGB(0, 0, 0);
}

void InputChannel::update()
{
  //load it up!
  this->raw = digitalRead(this->_pins[this->id]);
  bool nextState = this->state;

  //okay, did we actually change?
  if (this->raw != this->originalRaw)
  {
    if (this->raw)
      Serial.print(1);
    else
      Serial.print(0);

    //and debounce.  needs steadystate
    if (this->raw == this->lastRaw)
    {
      //direct is easy
      if (this->mode == DIRECT)
        nextState = this->raw;
      //inverting is easy too
      else if (this->mode == INVERTING)
        nextState = !this->raw;
      //just toggle it on rising
      else if (this->raw && this->mode == TOGGLE_RISING)
        nextState = !this->state;
      //just toggle it on falling
      else if (!this->raw && this->mode == TOGGLE_FALLING)
        nextState = !this->state;

      //our new state
      this->originalRaw = this->raw;
    }
  }

  //save our raw statuses
  this->lastRaw = this->raw;

  //update our state variable
  this->setState(nextState);
}

void InputChannel::setState(bool state)
{
  if (this->state != state)
  {
    this->state = state;
    this->stateChangeCount++;
    this->sendFastUpdate = true;

    if (this->state)
      rgb_channels[this->id].setRGB(0, 1.0, 0);
    else
      rgb_channels[this->id].setRGB(0, 0, 0);
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