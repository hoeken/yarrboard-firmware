/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_ADC_CHANNELS

  #include "adc_channel.h"

// the main star of the event
etl::array<ADCChannel, YB_ADC_CHANNEL_COUNT> adc_channels;

  #ifdef YB_ADC_DRIVER_ADS1115
byte ads1_channel = 0;
byte ads2_channel = 0;
ADS1115 _adcVoltageADS1115_1(YB_CHANNEL_VOLTAGE_I2C_ADDRESS_1);
ADS1115 _adcVoltageADS1115_2(YB_CHANNEL_VOLTAGE_I2C_ADDRESS_2);
  #elif defined YB_ADC_DRIVER_MCP3564
MCP3564 _adcAnalogMCP3564(YB_ADC_CS, &SPI, YB_ADC_MOSI, YB_ADC_MISO, YB_ADC_SCK);
  #elif YB_ADC_DRIVER_MCP3208
MCP3208 _adcAnalogMCP3208;
  #endif

unsigned long previousHAUpdateMillis = 0;

void adc_channels_setup()
{
  #ifdef YB_ADC_DRIVER_ADS1115
    #if defined(YB_I2C_SDA_PIN) && defined(YB_I2C_SCL_PIN)
  Wire.begin(YB_I2C_SDA_PIN, YB_I2C_SCL_PIN);
      // Wire.setClock(400000); // 400 kHz I2C to cut overhead
    #else
  Wire.begin(); // fallback to defaults
    #endif
  _adcVoltageADS1115_1.begin();
  if (_adcVoltageADS1115_1.isConnected())
    Serial.println("Voltage ADS115 #1 OK");
  else
    Serial.println("Voltage ADS115 #1 Not Found");

  // BASIC CONFIG
  _adcVoltageADS1115_1.setMode(ADS1X15_MODE_SINGLE);
  _adcVoltageADS1115_1.setGain(1);
  _adcVoltageADS1115_1.setDataRate(5);
  _adcVoltageADS1115_1.requestADC(0); // trigger first read

  _adcVoltageADS1115_2.begin();
  if (_adcVoltageADS1115_2.isConnected())
    Serial.println("Voltage ADS115 #2 OK");
  else
    Serial.println("Voltage ADS115 #2 Not Found");

  // BASIC CONFIG
  _adcVoltageADS1115_2.setMode(ADS1X15_MODE_SINGLE);
  _adcVoltageADS1115_2.setGain(1);
  _adcVoltageADS1115_2.setDataRate(5);
  _adcVoltageADS1115_2.requestADC(0); // trigger first read

  #elif defined(YB_ADC_DRIVER_MCP3564)

  _adcAnalogMCP3564.singleEndedMode();
  _adcAnalogMCP3564.setConversionMode(MCP3x6x::conv_mode::ONESHOT_STANDBY);
  _adcAnalogMCP3564.setAveraging(MCP3x6x::osr::OSR_1024);

  #elif YB_ADC_DRIVER_MCP3208
  _adcAnalogMCP3208.begin(YB_ADC_CS);
    //_adcAnalogMCP3208.begin(YB_ADC_CS, 23, 19, 18);
  #endif

  // setup our channels
  for (auto& ch : adc_channels)
    ch.setup();
}

void adc_channels_loop()
{
  #ifdef YB_ADC_DRIVER_ADS1115

  if (!_adcVoltageADS1115_1.isBusy()) {
    adc_channels[ads1_channel].adcHelper->getReading();

    // next channel
    ads1_channel = (ads1_channel + 1) & 0x03; // 0..3
    adc_channels[ads1_channel].adcHelper->requestReading(ads1_channel);
  }

  if (!_adcVoltageADS1115_2.isBusy()) {
    adc_channels[ads2_channel + 4].adcHelper->getReading();

    // next channel
    ads2_channel = (ads2_channel + 1) & 0x03; // 0..3
    adc_channels[ads2_channel + 4].adcHelper->requestReading(ads2_channel);
  }

  #else
  // maintenance on our channels.
  for (auto& ch : adc_channels)
    ch.update();
  #endif
}

void ADCChannel::setup()
{
  #ifdef YB_ADC_DRIVER_ADS1115
  if (this->id <= 4)
    this->adcHelper = new ADS1115Helper(YB_ADC_VREF, this->id - 1, &_adcVoltageADS1115_1);
  else
    this->adcHelper = new ADS1115Helper(YB_ADC_VREF, this->id - 5, &_adcVoltageADS1115_2);
  #elif YB_ADC_DRIVER_MCP3208
  this->adcHelper = new MCP3208Helper(3.3, this->id - 1, &_adcAnalogMCP3208);
  #endif
}

void ADCChannel::update()
{
  this->adcHelper->getReading();
}

unsigned int ADCChannel::getReading()
{
  return this->adcHelper->getAverageReading();
}

float ADCChannel::getVoltage()
{
  return this->adcHelper->getAverageVoltage();
}

void ADCChannel::resetAverage()
{
  this->adcHelper->resetAverage();
}

float ADCChannel::getTypeValue()
{
  float value = 0.0;

  if (!strcmp(this->type, "raw"))
    value = this->getVoltage();
  else if (!strcmp(this->type, "digital_switch")) {
    if (this->getVoltage() >= YB_ADC_VCC * 0.7)
      value = 1.0;
    else if (this->getVoltage() <= YB_ADC_VCC * 0.3)
      value = 0.0;
    else
      value = this->lastValue;
  } else if (!strcmp(this->type, "thermistor")) {
    // what pullup?
    float r_pullup = 10000.0;
    float r_beta = 3950.0;
    float r_thermistor = 10000.0;

    // 2. Calculate thermistor resistance
    // Voltage divider: Vadc = Vcc * (R_ntc / (R_ntc + R_PULLUP))
    float r_ntc = (this->getVoltage() * r_pullup) / (YB_ADC_VREF - this->getVoltage());

    // 3. Apply Beta equation to compute temperature in Kelvin
    float inv_T = (1.0 / 298.15) + (1.0 / r_beta) * log(r_ntc / r_thermistor);
    float tempK = 1.0 / inv_T;

    // 4. Convert to Celsius
    float tempC = tempK - 273.15;
    value = tempC;
  } else if (!strcmp(this->type, "4-20ma")) {
    value = (this->getVoltage() / YB_SENDIT_420MA_R1) * 1000;
  } else if (!strcmp(this->type, "high_volt_divider"))
    value = this->getVoltage() * (YB_SENDIT_HIGH_DIVIDER_R1 + YB_SENDIT_HIGH_DIVIDER_R2) / YB_SENDIT_HIGH_DIVIDER_R2;
  else if (!strcmp(this->type, "low_volt_divider"))
    value = this->getVoltage() * (YB_SENDIT_LOW_DIVIDER_R1 + YB_SENDIT_LOW_DIVIDER_R2) / YB_SENDIT_LOW_DIVIDER_R2;
  else if (!strcmp(this->type, "ten_k_pullup")) {
    float r1 = 10000.0;
    if (this->getVoltage() < 0)
      value = -1;
    else if (this->getVoltage() >= YB_ADC_VCC * 0.999)
      value = -2;
    else
      value = (r1 * this->getVoltage()) / (YB_ADC_VCC - this->getVoltage());
  } else
    value = -1;

  this->lastValue = value;

  return value;
}

const char* ADCChannel::getTypeUnits()
{
  if (!strcmp(this->type, "raw"))
    return "v";
  else if (!strcmp(this->type, "digital_switch"))
    return "bool";
  else if (!strcmp(this->type, "thermistor"))
    return "C";
  else if (!strcmp(this->type, "4-20ma"))
    return "mA";
  else if (!strcmp(this->type, "high_volt_divider"))
    return "v";
  else if (!strcmp(this->type, "low_volt_divider"))
    return "v";
  else if (!strcmp(this->type, "ten_k_pullup"))
    return "Ω";
  else
    return "";
}

// Linear interpolation (clamped at the ends).
float ADCChannel::interpolateValue(float xv)
{
  // Precondition: this->calibrationTable.size() >= 1 and sorted by x
  if (this->calibrationTable.empty())
    return 0.0f;
  if (xv <= this->calibrationTable.front().voltage)
    return this->calibrationTable.front().calibrated;
  if (xv >= this->calibrationTable.back().voltage)
    return this->calibrationTable.back().calibrated;

  auto it = etl::lower_bound(
    this->calibrationTable.begin(),
    this->calibrationTable.end(),
    xv,
    [](const CalibrationPoint& p, float v) { return p.voltage < v; });

  if (it != this->calibrationTable.end() && it->voltage == xv)
    return it->calibrated; // exact hit

  // Neighboring points: [lo, hi]
  auto hi = it;
  auto lo = hi - 1;

  const float dx = hi->voltage - lo->voltage;
  if (dx == 0.0f)
    return lo->calibrated; // guard against duplicate x

  const float t = (xv - lo->voltage) / dx;
  return lo->calibrated + t * (hi->calibrated - lo->calibrated);
}

void ADCChannel::init(uint8_t id)
{
  BaseChannel::init(id);

  snprintf(this->name, sizeof(this->name), "ADC Channel %d", id);
}

bool ADCChannel::loadConfig(JsonVariantConst config, char* error, size_t err_size)
{
  // make our parent do the work.
  if (!BaseChannel::loadConfig(config, error, err_size))
    return false;

  const char* value;

  value = config["type"].as<const char*>();
  snprintf(this->type, sizeof(this->type), "%s", (value && *value) ? value : "raw");

  this->displayDecimals = 2;
  if (config["displayDecimals"].is<unsigned int>()) {
    this->displayDecimals = config["displayDecimals"];
    this->displayDecimals = max(0, (int)this->displayDecimals);
  }

  this->useCalibrationTable = config["useCalibrationTable"];

  value = config["calibratedUnits"].as<const char*>();
  snprintf(this->calibratedUnits, sizeof(this->calibratedUnits), "%s", (value && *value) ? value : "");

  // todo: we need a return here.
  if (config["calibrationTable"])
    this->parseCalibrationTableJson(config["calibrationTable"]);

  return true;
}

void ADCChannel::generateConfig(JsonVariant config)
{
  BaseChannel::generateConfig(config);

  config["type"] = this->type;
  config["displayDecimals"] = this->displayDecimals;
  config["units"] = this->getTypeUnits();
  config["useCalibrationTable"] = this->useCalibrationTable;
  config["calibratedUnits"] = this->calibratedUnits;

  JsonArray table = config["calibrationTable"].to<JsonArray>();
  for (auto& cp : this->calibrationTable) {
    JsonArray row = table.add<JsonArray>();
    row.add(cp.voltage);
    row.add(cp.calibrated);
  }
}

void ADCChannel::generateUpdate(JsonVariant config)
{
  BaseChannel::generateUpdate(config);

  // gotta convert from float to boolean here.
  if (!strcmp(this->type, "digital_switch")) {
    if (this->getTypeValue() == 1.0)
      config["value"] = true;
    else
      config["value"] = false;
    // or just the float.
  } else {
    config["value"] = this->getTypeValue();

    // do we interpolate it?
    if (this->useCalibrationTable)
      config["calibrated_value"] = this->interpolateValue(this->getTypeValue());
  }

  config["adc_voltage"] = this->getVoltage();
  config["adc_raw"] = this->getReading();
}

// ---- The loader you can call with a JSON string ----
bool ADCChannel::parseCalibrationTableJson(JsonVariantConst tv)
{
  // Clear any existing table
  this->calibrationTable.clear();

  // Accept either "table" (preferred compact array) or "points" (object list)
  bool foundAny = false;

  // // 1) Preferred: "table": [ [v, y], ... ]
  // JsonVariantConst tv = root["table"]; // may be unbound (missing key)

  if (tv.is<JsonVariantConst>()) {
    // Key exists: validate it's an array
    if (!tv.is<JsonArrayConst>()) {
      Serial.println(F("\"table\" must be an array"));
      return false;
    }

    JsonArrayConst arr = tv.as<JsonArrayConst>();

    for (JsonVariantConst row : arr) {
      if (!row.is<JsonArrayConst>()) {
        Serial.println(F("Each table entry must be an array [v, y]"));
        return false;
      }

      JsonArrayConst pair = row.as<JsonArrayConst>();
      if (pair.size() != 2) {
        Serial.println(F("Each table entry must have exactly 2 elements [v, y]"));
        return false;
      }

      float v = pair[0].as<float>();
      float y = pair[1].as<float>();

      if (!this->addCalibrationValue({v, y}))
        return false;
    }

    foundAny = true;
  }

  if (!foundAny) {
    Serial.println("No calibration array found (expected \"table\" or \"points\")");
    return false;
  }

  if (this->calibrationTable.empty()) {
    Serial.println("Calibration array is empty");
    return false;
  }

  // Sort & dedupe on voltage
  this->_sortAndDedupeCalibrationTable();

  return true;
}

// ---- Sort & dedupe by voltage (keep last occurrence) ----
void ADCChannel::_sortAndDedupeCalibrationTable()
{
  if (calibrationTable.size() <= 1)
    return;
  etl::sort(calibrationTable.begin(), calibrationTable.end(), [](const CalibrationPoint& a, const CalibrationPoint& b) {
    return a.voltage < b.voltage;
  });
  size_t w = 1;
  for (size_t r = 1; r < calibrationTable.size(); ++r) {
    if (calibrationTable[r].voltage != calibrationTable[w - 1].voltage) {
      calibrationTable[w++] = calibrationTable[r];
    } else {
      // replace previous with the later point
      calibrationTable[w - 1] = calibrationTable[r];
    }
  }
  calibrationTable.resize(w);
}

bool ADCChannel::addCalibrationValue(CalibrationPoint cp)
{
  // Use std::isfinite(v) if <cmath> is available; otherwise isfinite(v) with <math.h>
  if (!std::isfinite(cp.voltage) || !std::isfinite(cp.calibrated)) {
    Serial.println(F("Non-finite number in table"));
    return false;
  }

  if (this->calibrationTable.full()) {
    Serial.println(F("Calibration table capacity exceeded"));
    return false;
  }

  this->calibrationTable.push_back({cp.voltage, cp.calibrated});
  return true;
}

void ADCChannel::haGenerateDiscovery(JsonVariant doc)
{
  // generate our id / topics
  sprintf(ha_uuid, "%s_adc_%s", uuid, this->key);
  sprintf(ha_topic_value, "yarrboard/%s/adc/%s/value", local_hostname, this->key);
  sprintf(ha_topic_avail, "yarrboard/%s/adc/%s/ha_availability", local_hostname, this->key);

  this->haGenerateSensorDiscovery(doc);
}

void ADCChannel::haGenerateSensorDiscovery(JsonVariant doc)
{
  JsonObject obj = doc[ha_uuid].to<JsonObject>();
  obj["p"] = "sensor";
  obj["name"] = this->name;
  obj["unique_id"] = ha_uuid;
  obj["state_topic"] = ha_topic_value;
  obj["unit_of_measurement"] = this->getTypeUnits();
  // obj["device_class"] = "voltage";
  obj["state_class"] = "measurement";
  obj["entity_category"] = "diagnostic";
  obj["suggested_display_precision"] = this->displayDecimals;

  // availability is an array of objects
  JsonArray availability = obj["availability"].to<JsonArray>();
  JsonObject avail = availability.add<JsonObject>();
  avail["topic"] = ha_topic_avail;
}

void ADCChannel::haPublishAvailable()
{
  mqtt_publish(ha_topic_avail, "online", false);
}

#endif