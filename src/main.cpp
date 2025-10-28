/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "IntervalTimer.h"
#include "adchelper.h"
#include "config.h"
#include "debug.h"
#include "mqtt.h"
#include "navico.h"
#include "networking.h"
#include "ota.h"
#include "piezo.h"
#include "prefs.h"
#include "rgb.h"
#include "utility.h"
#include "yb_server.h"
#include <LittleFS.h>

#ifdef YB_HAS_PWM_CHANNELS
  #include "pwm_channel.h"
#endif

#ifdef YB_HAS_RELAY_CHANNELS
  #include "relay_channel.h"
#endif

#ifdef YB_HAS_SERVO_CHANNELS
  #include "servo_channel.h"
#endif

#ifdef YB_HAS_STEPPER_CHANNELS
  #include "stepper_channel.h"
#endif

#ifdef YB_HAS_ADC_CHANNELS
  #include "adc_channel.h"
#endif

#ifdef YB_HAS_FANS
  #include "fans.h"
#endif

#ifdef YB_HAS_BUS_VOLTAGE
  #include "bus_voltage.h"
#endif

#ifdef YB_IS_BRINEOMATIC
  #include "brineomatic.h"
#endif

// various timer things.
IntervalTimer it;
RollingAverage loopSpeed(100, 1000);
RollingAverage framerateAvg(10, 10000);
unsigned long lastLoopMicros = 0;
unsigned long lastLoopMillis = 0;

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
  YBP.println("Prefs ok");

  debug_setup();
  YBP.println("Debug ok");

  // audio visual notifications
  rgb_setup();

#ifdef YB_HAS_PIEZO
  piezo_setup();
#endif

  network_setup();
  YBP.println("Network ok");

  // ntp_setup();
  // YBP.println("NTP ok");

  server_setup();
  YBP.println("Server ok");

  protocol_setup();
  YBP.println("Protocol ok");

  ota_setup();
  YBP.println("OTA ok");

#ifdef YB_HAS_BUS_VOLTAGE
  bus_voltage_setup();
  YBP.println("Bus voltage ok");
#endif

#ifdef YB_HAS_ADC_CHANNELS
  adc_channels_setup();
  YBP.println("ADC channels ok");
#endif

#ifdef YB_HAS_PWM_CHANNELS
  pwm_channels_setup();
  YBP.println("PWM channels ok");
#endif

#ifdef YB_HAS_RELAY_CHANNELS
  relay_channels_setup();
  YBP.println("Relay channels ok");
#endif

#ifdef YB_HAS_SERVO_CHANNELS
  servo_channels_setup();
  YBP.println("Servo channels ok");
#endif

#ifdef YB_HAS_STEPPER_CHANNELS
  stepper_channels_setup();
  YBP.println("Stepper channels ok");
#endif

#ifdef YB_HAS_FANS
  fans_setup();
  YBP.println("Fans ok");
#endif

#ifdef YB_IS_BRINEOMATIC
  brineomatic_setup();
#endif

  // we need to do this last so that all our channels, etc are fully configured.
  mqtt_setup();
  YBP.println("MQTT ok");

  rgb_set_status_color(0, 255, 0);

  // we're done with startup log, switch to network print
  YBP.removePrinter(startupLogger);
  YBP.addPrinter(networkLogger);

  lastLoopMicros = micros();
}

void loop()
{
  // start our interval timer
  it.start();

#ifdef YB_HAS_ADC_CHANNELS
  adc_channels_loop();
  it.time("adc_loop");
#endif

#ifdef YB_HAS_PWM_CHANNELS
  pwm_channels_loop();
  it.time("pwm_loop");
#endif

#ifdef YB_HAS_RELAY_CHANNELS
  relay_channels_loop();
  it.time("relay_loop");
#endif

#ifdef YB_HAS_SERVO_CHANNELS
  servo_channels_loop();
  it.time("servo_loop");
#endif

#ifdef YB_HAS_STEPPER_CHANNELS
  stepper_channels_loop();
  it.time("stepper_loop");
#endif

#ifdef YB_HAS_FANS
  fans_loop();
  it.time("fans_loop");
#endif

#ifdef YB_HAS_BUS_VOLTAGE
  bus_voltage_loop();
  it.time("bus_voltage_loop");
#endif

#ifdef YB_IS_BRINEOMATIC
  brineomatic_loop();
  it.time("brineomatic_loop");
#endif

  network_loop();
  it.time("network_loop");

  server_loop();
  it.time("server_loop");

  protocol_loop();
  it.time("protocol_loop");

  mqtt_loop();
  it.time("mqtt_loop");

  ota_loop();
  it.time("ota_loop");

  if (app_enable_mfd) {
    navico_loop();
    it.time("navico_loop");
  }

  // our debug.
  it.print(5000);

  // calculate our framerate
  unsigned long loopDelta = micros() - lastLoopMicros;
  lastLoopMicros = micros();
  loopSpeed.add(loopDelta);
  unsigned long framerate_now = 1000000 / loopSpeed.average();

  // smooth out our frequency
  if (millis() - lastLoopMillis > 1000) {
    framerateAvg.add(framerate_now);
    framerate = framerateAvg.average();
    lastLoopMillis = millis();
  }
}