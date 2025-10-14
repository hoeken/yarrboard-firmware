/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_MQTT_H
#define YARR_MQTT_H

#include "config.h"
#include "network.h"
#include "protocol.h"
#include <ArduinoJson.h>
#include <PsychicMqttClient.h>

extern PsychicMqttClient mqttClient;

void mqtt_setup();
void mqtt_loop();

void onMqttConnect(bool sessionPresent);

void mqtt_publish(const char* topic, const char* payload);
void mqtt_traverse_json(JsonVariant node, const char* topic_prefix);
void mqtt_ha_discovery();

void mqtt_receive_message(const char* topic, const char* payload, int retain, int qos, bool dup);

#endif /* !YARR_MQTT_H */