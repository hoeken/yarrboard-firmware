/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <YarrboardFramework.h>
#include <controllers/NavicoController.h>

#include "controllers/ADCController.h"

YarrboardApp yba;
NavicoController navico(yba);

ADCController adc(yba);

void setup()
{
  yba.registerController(navico);
  yba.registerController(adc);
  yba.setup();
}

void loop()
{
  yba.loop();
}