/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "mqqt.h"

PsychicMqttClient mqttClient;
const char* mqqt_server = "mqtt://192.168.2.42";

unsigned long previousMQQTMillis = 0;

void mqqt_setup()
{
  mqttClient.setServer(mqqt_server);

  // look for json messages on this path...
  char mqqt_path[128];
  sprintf(mqqt_path, "yarrboard/%s/message", local_hostname);
  mqttClient.onTopic(mqqt_path, 0, mqqt_receive_message);

  mqttClient.connect();
}

void mqqt_loop()
{
  unsigned int messageDelta = millis() - previousMQQTMillis;
  if (messageDelta >= 1000) {

    char mqqt_message[128];
    sprintf(mqqt_message, "millis: %d", previousMQQTMillis);
    mqqt_publish("test", mqqt_message);

    previousMQQTMillis = millis();
  }
}

void mqqt_publish(const char* topic, const char* payload)
{
  // prepare our path
  char mqqt_path[256];
  sprintf(mqqt_path, "yarrboard/%s/%s", local_hostname, topic);

  // send it off!
  mqttClient.publish(mqqt_path, 0, 0, payload);
}

void mqqt_receive_message(const char* topic, const char* payload, int retain, int qos, bool dup)
{
  Serial.printf("Received Topic: %s\r\n", topic);
  Serial.printf("Received Payload: %s\r\n", payload);
}