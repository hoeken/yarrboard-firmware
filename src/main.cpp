/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "adchelper.h"
#include "config.h"
#include "debug.h"
#include "navico.h"
#include "network.h"
#include "ota.h"
#include "prefs.h"
#include "server.h"
#include "utility.h"
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

#ifdef YB_HAS_INPUT_CHANNELS
  #include "input_channel.h"
#endif

#ifdef YB_HAS_ADC_CHANNELS
  #include "adc_channel.h"
#endif

#ifdef YB_HAS_RGB_CHANNELS
  #include "rgb_channel.h"
#endif

#ifdef YB_HAS_FANS
  #include "fans.h"
#endif

#ifdef YB_HAS_BUS_VOLTAGE
  #include "bus_voltage.h"
#endif

#ifdef YB_HAS_STATUS_WS2818
  #include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel status_led(1, YB_STATUS_WS2818_PIN, NEO_RGB + NEO_KHZ800);
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

  Serial.println("Yarrboard");
  Serial.print("Hardware Version: ");
  Serial.println(YB_HARDWARE_VERSION);
  Serial.print("Firmware Version: ");
  Serial.println(YB_FIRMWARE_VERSION);

  debug_setup();
  Serial.println("Debug ok");

#ifdef YB_HAS_STATUS_WS2818
  status_led.begin();
  status_led.setPixelColor(0, status_led.Color(0, 0, 255));
  status_led.setBrightness(50);
  status_led.show();
#endif

  // ntp_setup();
  prefs_setup();
  Serial.println("Prefs ok");

  network_setup();
  Serial.println("Network ok");

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

#ifdef YB_HAS_RGB_CHANNELS
  rgb_channels_setup();
  Serial.println("RGB channels ok");
#endif

// should be after RGB for default state to show up
#ifdef YB_HAS_INPUT_CHANNELS
  input_channels_setup();
  Serial.println("Input channels ok");
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

#ifdef YB_HAS_STATUS_WS2818
  status_led.setPixelColor(0, status_led.Color(0, 255, 0));
  status_led.show();
#endif
}

void loop()
{
// should be before RGB for fastest response
#ifdef YB_HAS_INPUT_CHANNELS
  input_channels_loop();
#endif

#ifdef YB_HAS_ADC_CHANNELS
  adc_channels_loop();
#endif

#ifdef YB_HAS_RGB_CHANNELS
  rgb_channels_loop();
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
  ota_loop();

  if (app_enable_mfd)
    navico_loop();

  // calculate our framerate
  framerate = calculateFramerate(millis() - lastFrameMillis);
  lastFrameMillis = millis();
}