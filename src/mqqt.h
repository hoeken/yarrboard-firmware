/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_MQQT_H
#define YARR_MQQT_H

#include "config.h"
#include "network.h"
#include <PsychicMqttClient.h>

extern PsychicMqttClient mqttClient;

void mqqt_setup();
void mqqt_loop();

void mqqt_publish(const char* topic, const char* payload);
void mqqt_receive_message(const char* topic, const char* payload, int retain, int qos, bool dup);

#endif /* !YARR_MQQT_H */