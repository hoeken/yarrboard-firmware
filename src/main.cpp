/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

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
  YBP.addPrinter(Serial);

  // startup log logs to a string for getting later
  YBP.addPrinter(startupLogger);

  if (!LittleFS.begin(true)) {
    YBP.println("ERROR: Unable to mount LittleFS");
  }
  YBP.printf("LittleFS Storage: %d / %d\n", LittleFS.usedBytes(), LittleFS.totalBytes());

  YBP.println("Yarrboard");
  YBP.print("Hardware Version: ");
  YBP.println(YB_HARDWARE_VERSION);
  YBP.print("Firmware Version: ");
  YBP.println(YB_FIRMWARE_VERSION);

  YBP.printf("Firmware build: %s (%s)\n", GIT_HASH, BUILD_TIME);

  // get our prefs early on.
  prefs_setup();

  debug_setup();
  YBP.println("Debug ok");

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