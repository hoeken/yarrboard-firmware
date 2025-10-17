/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "adchelper.h"
#include "config.h"
#include "debug.h"
#include "logging.h"
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

unsigned long lastFrameMillis = 0;

void setup()
{
  // startup our serial
  Serial.begin(115200);
  Serial.setTimeout(50);

  /* need to zero out the ticklist array before starting */
  for (int i = 0; i < YB_FPS_SAMPLES; i++)
    ticklist[i] = 0;

  if (!LittleFS.begin(true)) {
    Serial.println("ERROR: Unable to mount LittleFS");
  }
  Serial.printf("LittleFS Storage: %d / %d\n", LittleFS.usedBytes(), LittleFS.totalBytes());

  Serial.println("Yarrboard");
  Serial.print("Hardware Version: ");
  Serial.println(YB_HARDWARE_VERSION);
  Serial.print("Firmware Version: ");
  Serial.println(YB_FIRMWARE_VERSION);

  Serial.printf("Firmware build: %s (%s)\n", GIT_HASH, BUILD_TIME);

  // get our prefs early on.
  prefs_setup();
  Serial.println("Prefs ok");

  debug_setup();
  Serial.println("Debug ok");

  // audio visual notifications
  rgb_setup();

#ifdef YB_HAS_PIEZO
  piezo_setup();
#endif

  network_setup();
  Serial.println("Network ok");

  // ntp_setup();
  // Serial.println("NTP ok");

  server_setup();
  Serial.println("Server ok");

  protocol_setup();
  Serial.println("Protocol ok");

  ota_setup();
  Serial.println("OTA ok");

#ifdef YB_HAS_ADC_CHANNELS
  adc_channels_setup();
  Serial.println("ADC channels ok");
#endif

#ifdef YB_HAS_PWM_CHANNELS
  pwm_channels_setup();
  Serial.println("PWM channels ok");
#endif

#ifdef YB_HAS_RELAY_CHANNELS
  relay_channels_setup();
  Serial.println("Relay channels ok");
#endif

#ifdef YB_HAS_SERVO_CHANNELS
  servo_channels_setup();
  Serial.println("Servo channels ok");
#endif

#ifdef YB_HAS_FANS
  fans_setup();
  Serial.println("Fans ok");
#endif

#ifdef YB_HAS_BUS_VOLTAGE
  bus_voltage_setup();
  Serial.println("Bus voltage ok");
#endif

#ifdef YB_IS_BRINEOMATIC
  brineomatic_setup();
#endif

  // we need to do this last so that all our channels, etc are fully configured.
  mqtt_setup();
  Serial.println("MQTT ok");

  rgb_set_status_color(0, 255, 0);
}

void loop()
{
#ifdef YB_HAS_ADC_CHANNELS
  adc_channels_loop();
#endif

#ifdef YB_HAS_PWM_CHANNELS
  pwm_channels_loop();
#endif

#ifdef YB_HAS_RELAY_CHANNELS
  relay_channels_loop();
#endif

#ifdef YB_HAS_SERVO_CHANNELS
  servo_channels_loop();
#endif

#ifdef YB_HAS_FANS
  fans_loop();
#endif

#ifdef YB_HAS_BUS_VOLTAGE
  bus_voltage_loop();
#endif

#ifdef YB_IS_BRINEOMATIC
  brineomatic_loop();
#endif

  network_loop();
  server_loop();
  protocol_loop();
  mqtt_loop();
  ota_loop();

  if (app_enable_mfd)
    navico_loop();

  // calculate our framerate
  framerate = calculateFramerate(millis() - lastFrameMillis);
  lastFrameMillis = millis();
}