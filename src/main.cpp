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
  #include "gulp/logo-frothfet.png.h"

FanController fans(yba);
BusVoltageController bus_voltage(yba);
PWMController pwm(yba);
#elifdef YB_IS_BRINEOMATIC
  #include "Brineomatic.h"
  #include "controllers/BrineomaticController.h"
  #include "controllers/RelayController.h"
  #include "controllers/ServoController.h"
  #include "controllers/StepperController.h"
  #include "gulp/logo-brineomatic.png.h"

RelayController relays(yba);
ServoController servos(yba);
StepperController steppers(yba);
BrineomaticController bom(yba, relays, servos, steppers);
#elifdef YB_IS_SENDIT
  #include "controllers/ADCController.h"
  #include "gulp/logo-sendit.png.h"
ADCController adc(yba);
#endif

#include "gulp/index.html.h"

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

  yba.http.registerGulpedFile(&logo_frothfet_png, "/logo.png");

#elifdef YB_IS_BRINEOMATIC
  yba.registerController(relays);
  yba.registerController(servos);
  yba.registerController(steppers);
  yba.registerController(bom);

  yba.http.registerGulpedFile(&logo_brineomatic_png, "/logo.png");

#elifdef YB_IS_SENDIT
  yba.registerController(adc);

  yba.http.registerGulpedFile(&logo_sendit_png, "/logo.png");
#endif

  yba.board_name = YB_BOARD_NAME;
  yba.default_hostname = YB_DEFAULT_HOSTNAME;
  yba.firmware_version = YB_FIRMWARE_VERSION;
  yba.hardware_version = YB_HARDWARE_VERSION;
  yba.manufacturer = YB_MANUFACTURER;
  yba.hardware_url = YB_HARDWARE_URL;
  yba.project_name = YB_PROJECT_NAME;
  yba.project_url = YB_PROJECT_URL;

  yba.ota.firmware_manifest_url = "https://https://hoeken.github.io/yarrboard-firmware/releases/ota_manifest.json";
  yba.ota.public_key = R"PUBLIC_KEY(
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAjsPaBVvAoSlNEdxLnKl5
71m+8nEbI6jTenIau884++X+tzjRM/4vctpkfM+b6yPEER6hLKLU5Sr/sVbNAu3s
Ih9UHsgbyzQ4r+NMzM8ohvPov1j5+NgzoIRPn9IQR40p/Mr3T31MXoeSh/WXw7yJ
BjVH2KhTD14e8Yc9CiEUvzYhFVjs8Doy1q2+jffiutcR8z+zGBSGHI3klTK8mNau
r9weglTUCObkUfbgrUWXOkDN50Q97OOv99+p8NPkcThZYbaqjbrOCO9vnMFB9Mxj
5yDruS9QF/qhJ5mC7AuHLhAGdkPu+3OXRDlIJN1j7y8SorvQj9F17B8wnhNBfDPN
QbJc4isLIIBGECfmamCONi5tt6fcZC/xZTxCiEURG+JVgUKjw+mIBrv+iVn9NKYK
UF8shPfl0CGKzOvsXBf91pqF5rHs6TpVw985u1VFbRrUL6nmsCELFxBz/+y83uhj
jsROITwP34vi7qMuHm8UzTnfxH0dSuI6PfWESIM8aq6bidBgUWlnoN/zQ/pwLVsz
0Gh5tAoFCyJ+FZiKS+2spkJ5mJBMY0Ti3dHinp6E2YNxY7IMV/4E9oK+MzvX1m5s
rgu4zp1Wfh2Q5QMX6bTrDCTn52KdyJ6z2WTnafaA08zeKOP+uVAPT0HLShF/ITEX
+Cd7GvvuZMs80QvqoXi+k8UCAwEAAQ==
-----END PUBLIC KEY-----
)PUBLIC_KEY";

  yba.http.registerGulpedFile(&index_html);

  yba.setup();
}

void loop()
{
  yba.loop();
}