/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "mqqt.h"

PsychicMqttClient mqttClient;
const char* mqqt_server = "mqtt://boatpi";

void mqqt_setup()
{
  mqttClient.setServer(mqqt_server);
  mqttClient.onTopic("your/topic", 0, mqqt_handle_command);
}

void mqqt_loop()
{
  mqttClient.publish("your/topic", 0, 0, "Hello World!");
}

void mqqt_handle_command(const char* topic, const char* payload, int retain, int qos, bool dup)
{
  Serial.printf("Received Topic: %s\r\n", topic);
  Serial.printf("Received Payload: %s\r\n", payload);
}