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

#ifdef YB_HAS_PIEZO
  #include "controllers/BuzzerController.h"
BuzzerController buzzer(yba);
#endif

#ifdef YB_HAS_STATUS_RGB
  #include "controllers/RGBController.h"
RGBController<WS2812B, YB_STATUS_RGB_PIN, YB_STATUS_RGB_ORDER> rgb(yba, YB_STATUS_RGB_COUNT);
#endif

#ifdef YB_IS_FROTHFET
  #include "controllers/BusVoltageController.h"
  #include "controllers/FanController.h"
  #include "controllers/PWMController.h"
  #include "gulp/logo-frothfet.png.gz.h"

FanController fans(yba);
BusVoltageController bus_voltage(yba);
PWMController pwm(yba);
#elifdef YB_IS_BRINEOMATIC
  #include "Brineomatic.h"
  #include "controllers/BrineomaticController.h"
  #include "controllers/RelayController.h"
  #include "controllers/ServoController.h"
  #include "controllers/StepperController.h"
  #include "gulp/logo-brineomatic.png.gz.h"

RelayController relays(yba);
ServoController servos(yba);
StepperController steppers(yba);
BrineomaticController bom(yba, relays, servos, steppers);
#elifdef YB_IS_SENDIT
  #include "controllers/ADCController.h"
  #include "gulp/logo-sendit.png.gz.h"
ADCController adc(yba);
#endif

#include "gulp/index.html.gz.h"

void setup()
{
  yba.registerController(navico);

#ifdef YB_HAS_ADC_CHANNELS
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

#ifdef YB_IS_FROTHFET
  bus_voltage.address = YB_BUS_VOLTAGE_ADDRESS;
  bus_voltage.r1 = YB_BUS_VOLTAGE_R1;
  bus_voltage.r2 = YB_BUS_VOLTAGE_R2;
  yba.registerController(bus_voltage);

  pwm.busVoltage = &bus_voltage;
  pwm.rgb = (RGBControllerInterface*)yba.getController("rgb");
  yba.registerController(pwm);

  fans.pwm = &pwm;
  yba.registerController(fans);

  yba.http.logo_length = logo_frothfet_png_gz_len;
  yba.http.logo_sha = logo_frothfet_png_gz_sha;
  yba.http.logo_data = logo_frothfet_png_gz;

#elifdef YB_IS_BRINEOMATIC
  yba.registerController(relays);
  yba.registerController(servos);
  yba.registerController(steppers);
  yba.registerController(bom);

  yba.http.logo_length = logo_brineomatic_png_gz_len;
  yba.http.logo_sha = logo_brineomatic_png_gz_sha;
  yba.http.logo_data = logo_brineomatic_png_gz;
#elifdef YB_IS_SENDIT
  yba.registerController(adc);

  yba.http.logo_length = logo_sendit_png_gz_len;
  yba.http.logo_sha = logo_sendit_png_gz_sha;
  yba.http.logo_data = logo_sendit_png_gz;
#endif

  yba.board_name = YB_BOARD_NAME;
  yba.default_hostname = YB_DEFAULT_HOSTNAME;
  yba.firmware_version = YB_FIRMWARE_VERSION;
  yba.hardware_version = YB_HARDWARE_VERSION;
  yba.manufacturer = YB_MANUFACTURER;
  yba.hardware_url = YB_HARDWARE_URL;
  yba.project_name = YB_PROJECT_NAME;
  yba.project_url = YB_PROJECT_URL;
  yba.ota.firmware_manifest_url = "https://raw.githubusercontent.com/hoeken/yarrboard-firmware/main/firmware.json";

  yba.http.index_length = index_html_gz_len;
  yba.http.index_sha = index_html_gz_sha;
  yba.http.index_data = index_html_gz;

  yba.setup();
}

void loop()
{
  yba.loop();
}