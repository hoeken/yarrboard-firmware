/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_RELAY_CHANNELS

  #include "debug.h"
  #include "relay_channel.h"

// the main star of the event
etl::array<RelayChannel, YB_RELAY_CHANNEL_COUNT> relay_channels;
byte _relay_pins[YB_RELAY_CHANNEL_COUNT] = YB_RELAY_CHANNEL_PINS;

  #ifdef YB_RELAY_DRIVER_TCA9554
TCA9554 TCA(YB_RELAY_DRIVER_TCA9554_ADDRESS);
  #endif

void relay_channels_setup()
{
  #ifdef YB_RELAY_DRIVER_TCA9554
  Wire.begin();
  Wire.setPins(YB_I2C_SDA_PIN, YB_I2C_SCL_PIN);
  Wire.setClock(YB_I2C_SPEED);
  TCA.begin(INPUT);

  YBP.printf("Is Connected: %d\n", TCA.isConnected());
  YBP.printf("Type: 0x%02x\n", TCA.getType());
  YBP.printf("Address: 0x%02x\n", TCA.getAddress());
  #endif

  // intitialize our channel
  for (auto& ch : relay_channels) {
    ch.setup();
    ch.setupDefaultState();
  }
}

void relay_channels_loop()
{
}

void RelayChannel::setup()
{
  // init!
  _pin = _relay_pins[id - 1];

  #ifdef YB_RELAY_DRIVER_TCA9554
  TCA.pinMode1(_pin, OUTPUT);
  TCA.write1(_pin, LOW);
  #else
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  #endif
}

void RelayChannel::setupDefaultState()
{
  // update our pin
  this->outputState = this->defaultState;
  this->updateOutput();
}

void RelayChannel::updateOutput()
{
  #ifdef YB_RELAY_DRIVER_TCA9554
  TCA.write1(_pin, outputState);
  #else
  digitalWrite(_pin, outputState);
  #endif
}

void RelayChannel::setState(const char* state)
{
  if (!strcmp(state, "ON"))
    this->setState(true);
  else if (!strcmp(state, "OFF"))
    this->setState(false);
}

void RelayChannel::setState(bool newState)
{
  if (newState != outputState) {
    // save our output state
    this->outputState = newState;

    // keep track of how many toggles
    this->stateChangeCount++;
  }

  // change our output pin to reflect
  this->updateOutput();
}

bool RelayChannel::getState()
{
  return this->outputState;
}

const char* RelayChannel::getStatus()
{
  if (this->outputState)
    return "ON";
  else
    return "OFF";
}

void RelayChannel::init(uint8_t id)
{
  BaseChannel::init(id);
  this->channel_type = "relay";

  snprintf(this->name, sizeof(this->name), "Relay Channel %d", id);
}

bool RelayChannel::loadConfig(JsonVariantConst config, char* error, size_t len)
{
  // make our parent do the work.
  if (!BaseChannel::loadConfig(config, error, len))
    return false;

  strlcpy(this->type, config["type"] | "other", sizeof(this->type));
  this->defaultState = config["defaultState"];

  return true;
}

void RelayChannel::generateConfig(JsonVariant config)
{
  BaseChannel::generateConfig(config);

  config["type"] = this->type;
  config["enabled"] = this->isEnabled;
  config["defaultState"] = this->defaultState;
}

void RelayChannel::generateUpdate(JsonVariant config)
{
  BaseChannel::generateUpdate(config);

  config["state"] = this->getStatus();
  config["source"] = this->source;
}

void RelayChannel::haGenerateDiscovery(JsonVariant doc)
{
  BaseChannel::haGenerateDiscovery(doc);

  // generate our topics
  sprintf(ha_topic_cmd_state, "yarrboard/%s/pwm/%s/ha_set", ha_key, this->key);
  sprintf(ha_topic_state_state, "yarrboard/%s/pwm/%s/ha_state", ha_key, this->key);

  // our callbacks to the command topics
  mqtt_on_topic(ha_topic_cmd_state, 0, relay_handle_ha_command);

  // configuration object for the individual channel
  JsonObject obj = doc[ha_uuid].to<JsonObject>();
  obj["platform"] = "light";
  obj["name"] = this->name;
  obj["unique_id"] = ha_uuid;
  obj["command_topic"] = ha_topic_cmd_state;
  obj["state_topic"] = ha_topic_state_state;
  obj["payload_on"] = "ON";
  obj["payload_off"] = "OFF";

  // availability is an array of objects
  JsonArray availability = obj["availability"].to<JsonArray>();
  JsonObject avail = availability.add<JsonObject>();
  avail["topic"] = ha_topic_avail;
}

void RelayChannel::haPublishState()
{
  if (this->getStatus())
    mqtt_publish(ha_topic_state_state, "ON", false);
  else
    mqtt_publish(ha_topic_state_state, "OFF", false);
}

void relay_handle_ha_command(const char* topic, const char* payload, int retain, int qos, bool dup)
{
  for (auto& ch : relay_channels) {
    ch.haHandleCommand(topic, payload);
  }
}

void RelayChannel::haHandleCommand(const char* topic, const char* payload)
{
  if (!strcmp(ha_topic_cmd_state, topic)) {
    if (!strcmp(payload, "ON"))
      this->setState(true);
    else
      this->setState(false);
  }
}

#endif