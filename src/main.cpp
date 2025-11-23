/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "Arduino.h"
#include "config.h"
#include "debug.h"
#include "piezo.h"
#include "prefs.h"
#include "rgb.h"
#include "setup.h"
#include <LittleFS.h>

void setup()
{
  debug_setup();

  // more memory!!!
  // if (!psramInit())
  //   YBP.println("PSRAM init failed!");
  // else
  //   YBP.printf("PSRAM size: %d bytes\n", ESP.getPsramSize());

  // get our prefs early on.
  prefs_setup();

// audio visual notifications
#ifdef YB_HAS_STATUS_RGB
  rgb_setup();
#endif
#ifdef YB_HAS_PIEZO
  piezo_setup();
#endif

  // get an IP address
  if (is_first_boot)
    improv_setup();
  else
    full_setup();
}

void loop()
{
  if (is_first_boot)
    improv_loop();
  else
    full_loop();
}