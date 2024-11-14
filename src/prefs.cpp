/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "prefs.h"

Preferences preferences;

void prefs_setup()
{
  if (preferences.begin("yarrboard", false))
    Serial.println("Prefs OK");
  else
    Serial.println("Opening Preferences failed.");

  Serial.printf("There are: %u entries available in the 'yarrboard' prefs table.\n", preferences.freeEntries());
}