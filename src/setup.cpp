#include "setup.h"
#include "IntervalTimer.h"
#include "adchelper.h"
#include "config.h"
#include "debug.h"
#include "mqtt.h"
#include "navico.h"
#include "networking.h"
#include "ota.h"
#include "prefs.h"
#include "rgb.h"
#include "utility.h"
#include "yb_server.h"

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

void full_setup()
{
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

  // we're done with startup log, switch to network print
  YBP.removePrinter(startupLogger);
  YBP.addPrinter(networkLogger);

  lastLoopMicros = micros();
}

void full_loop()
{
  // start our interval timer
  it.start();

#ifdef YB_HAS_STATUS_RGB
  rgb_loop();
  it.time("rgb_loop");
#endif

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
  // it.print(5000);

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