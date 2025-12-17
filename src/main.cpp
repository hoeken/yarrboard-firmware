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

YarrboardApp yba;
NavicoController navico(yba);

#ifdef YB_HAS_ADC_CHANNELS
  #include "controllers/ADCController.h"
ADCController adc(yba);
#endif

#ifdef YB_HAS_PIEZO
  #include "controllers/BuzzerController.h"
BuzzerController buzzer(yba);
#endif

#ifdef YB_HAS_STATUS_RGB
  #include "controllers/RGBController.h"
RGBController<WS2812B, YB_STATUS_RGB_PIN, YB_STATUS_RGB_ORDER> rgb(yba, YB_STATUS_RGB_COUNT);
#endif

#ifdef YB_HAS_BUS_VOLTAGE
  #include "controllers/BusVoltageController.h"
BusVoltageController bus_voltage(yba);
#endif

#ifdef YB_HAS_PWM_CHANNELS
  #include "controllers/PWMController.h"
PWMController pwm(yba);
#endif

#ifdef YB_HAS_FANS
  #include "controllers/FanController.h"
FanController fans(yba);
#endif

#include "index.html.gz.h"
#include "logo.png.gz.h"

void setup()
{
  yba.registerController(navico);

#ifdef YB_HAS_ADC_CHANNELS
  yba.registerController(adc);
#endif

#ifdef YB_HAS_PIEZO
  buzzer.buzzerPin = YB_PIEZO_PIN;
  #ifdef YB_PIEZO_PASSIVE
  buzzer.isActive = false;
  #else
  buzzer.isActive = true;
  #endif
  yba.registerController(buzzer);
#endif

#ifdef YB_HAS_STATUS_RGB
  yba.registerController(rgb);
#endif

#ifdef YB_HAS_BUS_VOLTAGE
  bus_voltage.address = YB_BUS_VOLTAGE_ADDRESS;
  bus_voltage.r1 = YB_BUS_VOLTAGE_R1;
  bus_voltage.r2 = YB_BUS_VOLTAGE_R2;
  yba.registerController(bus_voltage);
#endif

#ifdef YB_HAS_PWM_CHANNELS
  pwm.busVoltage = &bus_voltage;
  pwm.mqtt = (MQTTController*)yba.getController("mqtt");
  pwm.rgb = (RGBControllerInterface*)yba.getController("rgb");
  yba.registerController(pwm);
#endif

#ifdef YB_HAS_FANS
  fans.pwm = &pwm;
  yba.registerController(fans);
#endif

  yba.board_name = YB_BOARD_NAME;
  yba.default_hostname = YB_DEFAULT_HOSTNAME;
  yba.firmware_version = YB_FIRMWARE_VERSION;
  yba.hardware_version = YB_HARDWARE_VERSION;
  yba.manufacturer = YB_MANUFACTURER;

  yba.http.index_length = index_html_gz_len;
  yba.http.index_sha = index_html_gz_sha;
  yba.http.index_data = index_html_gz;

  yba.http.logo_length = logo_gz_len;
  yba.http.logo_sha = logo_gz_sha;
  yba.http.logo_data = logo_gz;

  yba.setup();
}

void loop()
{
  yba.loop();
}