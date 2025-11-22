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
  // startup our serial
  Serial.begin(115200);
  Serial.setTimeout(50);

  // more memory!!!
  // if (!psramInit())
  //   Serial.println("PSRAM init failed!");
  // else
  //   Serial.printf("PSRAM size: %d bytes\n", ESP.getPsramSize());

  // we need littlefs to store our coredump if any in debug_setup()
  if (!LittleFS.begin(true)) {
    YBP.println("ERROR: Unable to mount LittleFS");
  }
  YBP.printf("LittleFS Storage: %d / %d\n", LittleFS.usedBytes(), LittleFS.totalBytes());

  debug_setup();

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