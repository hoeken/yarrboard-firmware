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

#include "index.html.gz.h"
#include "logo.png.gz.h"

void setup()
{
  yba.registerController(navico);

#ifdef YB_HAS_ADC_CHANNELS
  yba.registerController(adc);
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