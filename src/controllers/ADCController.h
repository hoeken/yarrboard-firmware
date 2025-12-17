/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_ADC_CONTROLLER_H
#define YARR_ADC_CONTROLLER_H

#include "channels/ADCChannel.h"
#include "controllers/ChannelController.h"

class YarrboardApp;

class ADCController : public ChannelController<ADCChannel, YB_ADC_CHANNEL_COUNT>
{
  public:
    ADCController(YarrboardApp& app);

    bool setup() override;
    void loop() override;

    void handleConfigADC(JsonVariantConst input, JsonVariant output);

  private:
    ADS1115 _adcVoltageADS1115_1;
    ADS1115 _adcVoltageADS1115_2;
    ADS1115Helper* adcHelper1;
    ADS1115Helper* adcHelper2;

    unsigned long previousHAUpdateMillis = 0;
};

#endif /* !YARR_OTA_H */