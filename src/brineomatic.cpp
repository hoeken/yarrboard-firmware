/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_IS_BRINEOMATIC

  #include "brineomatic.h"
  #include <ADS1X15.h>
  #include <Arduino.h>
  #include <DallasTemperature.h>
  #include <GravityTDS.h>
  #include <OneWire.h>

Brineomatic wm;

byte motor_a_pins[YB_DC_MOTOR_CHANNEL_COUNT] = YB_DC_MOTOR_A_PINS;
byte motor_b_pins[YB_DC_MOTOR_CHANNEL_COUNT] = YB_DC_MOTOR_B_PINS;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(YB_DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);
DeviceAddress motorThermometer;

byte flowmeter_pin = YB_FLOWMETER_PIN;
static volatile uint16_t pulse_counter = 0;
uint64_t lastFlowmeterCheckMicros = 0;
float flowmeterPulsesPerLiter = YB_FLOWMETER_DEFAULT_PPL;

void IRAM_ATTR flowmeter_interrupt()
{
  pulse_counter++;
}

ADS1115 brineomatic_adc(YB_ADS1115_ADDRESS);
byte current_ads1115_channel = 0;

GravityTDS gravityTds;
float water_temperature = 25;

void brineomatic_setup()
{
  byte motor_a_pins[YB_DC_MOTOR_CHANNEL_COUNT] = YB_DC_MOTOR_A_PINS;
  byte motor_b_pins[YB_DC_MOTOR_CHANNEL_COUNT] = YB_DC_MOTOR_B_PINS;

  byte flowmeter_pin = YB_FLOWMETER_PIN;

  for (byte i = 0; i < YB_DC_MOTOR_CHANNEL_COUNT; i++) {
    pinMode(motor_a_pins[i], OUTPUT);
    pinMode(motor_b_pins[i], OUTPUT);
    digitalWrite(motor_a_pins[i], LOW);
    digitalWrite(motor_a_pins[i], LOW);
  }

  // do our init for our flowmeter
  pinMode(flowmeter_pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(flowmeter_pin), flowmeter_interrupt, FALLING);
  pulse_counter = 0;
  lastFlowmeterCheckMicros = 0;

  // DS18B20 Sensor
  ds18b20.begin();
  Serial.print("Found ");
  Serial.print(ds18b20.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (ds18b20.isParasitePowerMode())
    Serial.println("ON");
  else
    Serial.println("OFF");

  // lookup our address
  if (!ds18b20.getAddress(motorThermometer, 0))
    Serial.println("Unable to find address for DS18B20");

  ds18b20.setResolution(motorThermometer, 9);
  ds18b20.setWaitForConversion(false);
  ds18b20.requestTemperatures();

  Wire.begin();
  brineomatic_adc.begin();
  if (brineomatic_adc.isConnected())
    Serial.println("ADS1115 OK");
  else
    Serial.println("ADS1115 Not Found");

  brineomatic_adc.setMode(1);     // SINGLE SHOT MODE
  brineomatic_adc.setGain(1);     // ±4.096V
  brineomatic_adc.setDataRate(4); // 128sps

  current_ads1115_channel = 0;
  brineomatic_adc.requestADC(current_ads1115_channel);

  gravityTds.setAref(YB_ADS1115_VREF); // reference voltage on ADC
  gravityTds.setAdcRange(2 ^ 15);      // 16 bit ADC, but its differential
  gravityTds.begin();                  // initialization

  // Create a FreeRTOS task for the state machine
  xTaskCreatePinnedToCore(
    brineomatic_state_machine,   // Task function
    "brineomatic_state_machine", // Name of the task
    2048,                        // Stack size
    NULL,                        // Task input parameters
    1,                           // Priority of the task
    NULL,                        // Task handle
    1                            // Core where the task should run
  );

  // temporary hardcoding.
  wm.highPressurePump = &relay_channels[0];
  wm.flushValve = &relay_channels[1];
  wm.diverterValve = &servo_channels[0];
  wm.highPressureValve = &servo_channels[1];
}

void brineomatic_loop()
{
  measure_flowmeter();
  measure_temperature();

  if (brineomatic_adc.isReady()) {
    int16_t value = brineomatic_adc.getValue();

    if (brineomatic_adc.getError() == ADS1X15_OK) {
      if (current_ads1115_channel == 0)
        true; // thermistor channel - unused for now
      else if (current_ads1115_channel == 1)
        measure_salinity(value);
      else if (current_ads1115_channel == 2)
        measure_filter_pressure(value);
      else if (current_ads1115_channel == 3)
        measure_membrane_pressure(value);
    } else
      Serial.println("ADC Error.");

    // update to our channel index
    current_ads1115_channel++;
    if (current_ads1115_channel == 4)
      current_ads1115_channel = 0;

    // request new conversion
    brineomatic_adc.requestADC(current_ads1115_channel);
  }
}

// State machine task function
void brineomatic_state_machine(void* pvParameters)
{
  while (true) {
    wm.runStateMachine();

    // Add a delay to prevent task starvation
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void measure_flowmeter()
{
  if (esp_timer_get_time() - lastFlowmeterCheckMicros >= 1000000) {
    // detach interrupt while calculating rpm
    detachInterrupt(digitalPinToInterrupt(flowmeter_pin));

    // calculate flowrate
    float flowrate = pulse_counter / flowmeterPulsesPerLiter;

    // reset counter
    pulse_counter = 0;

    // store microseconds when tacho was measured the last time
    lastFlowmeterCheckMicros = esp_timer_get_time();

    // attach interrupt again
    attachInterrupt(digitalPinToInterrupt(flowmeter_pin), flowmeter_interrupt, FALLING);

    // update our model
    wm.setFlowrate(flowrate);
  }
}

void measure_temperature()
{
  if (ds18b20.isConversionComplete()) {
    float tempC = ds18b20.getTempC(motorThermometer);

    if (tempC == DEVICE_DISCONNECTED_C) {
      wm.setTemperature(-999);
    }
    wm.setTemperature(tempC);

    ds18b20.requestTemperatures();
  }
}

void measure_salinity(int16_t reading)
{
  gravityTds.setTemperature(water_temperature); // set the temperature and execute temperature compensation
  gravityTds.update(reading);                   // sample and calculate
  float tdsReading = gravityTds.getTdsValue();  // then get the value
  wm.setSalinity(tdsReading);
}

void measure_filter_pressure(int16_t reading)
{
  float voltage = brineomatic_adc.toVoltage(reading);

  if (voltage < 0.4) {
    Serial.println("No LP Sensor Detected");
    wm.setFilterPressure(-999);
    return;
  }

  float amperage = (voltage / YB_420_RESISTOR) * 1000;
  float lowPressureReading = map_generic(amperage, 4.0, 20.0, 0.0, YB_LP_SENSOR_MAX);
  wm.setFilterPressure(lowPressureReading);
}

void measure_membrane_pressure(int16_t reading)
{
  float voltage = brineomatic_adc.toVoltage(reading);

  if (voltage < 0.4) {
    Serial.println("No HP Sensor Detected");
    wm.setMembranePressure(-999);
    return;
  }

  float amperage = (voltage / YB_420_RESISTOR) * 1000;
  float highPressureReading = map_generic(amperage, 4.0, 20.0, 0.0, YB_HP_SENSOR_MAX);
  wm.setMembranePressure(highPressureReading);
}

Brineomatic::Brineomatic() : isPickled(false),
                             autoFlushEnabled(true),
                             diversionValveOpen(false),
                             highPressurePumpEnabled(false),
                             boostPumpEnabled(false),
                             flushValveOpen(false),
                             currentTemperature(0.0),
                             currentFlowrate(0.0),
                             currentSalinity(0.0),
                             currentFilterPressure(0.0),
                             currentMembranePressure(0.0),
                             membranePressureTarget(0),
                             currentStatus(Status::STARTUP)
{
}

void Brineomatic::setFilterPressure(float pressure)
{
  currentFilterPressure = pressure;
}

void Brineomatic::setMembranePressure(float pressure)
{
  currentMembranePressure = pressure;
}

void Brineomatic::setMembranePressureTarget(float pressure)
{
  membranePressureTarget = pressure;
}

void Brineomatic::setFlowrate(float flowrate)
{
  currentFlowrate = flowrate;
}

void Brineomatic::setTemperature(float temp)
{
  currentTemperature = temp;
}

void Brineomatic::setSalinity(float salinity)
{
  currentSalinity = salinity;
}

void Brineomatic::start()
{
  stopFlag = false;
  if (currentStatus == Status::IDLE) {
    desiredRuntime = 0;
    desiredVolume = 0;
    currentStatus = Status::RUNNING;
  }
}

void Brineomatic::startDuration(uint64_t duration)
{
  stopFlag = false;
  if (currentStatus == Status::IDLE) {
    desiredRuntime = duration;
    desiredVolume = 0;
    currentStatus = Status::RUNNING;
  }
}

void Brineomatic::startVolume(float volume)
{
  stopFlag = false;
  if (currentStatus == Status::IDLE) {
    desiredRuntime = 0;
    desiredVolume = volume;
    currentStatus = Status::RUNNING;
  }
}

void Brineomatic::flush(uint64_t duration)
{
  TRACE();
  DUMP(duration);

  stopFlag = false;
  if (currentStatus == Status::IDLE || currentStatus == Status::PICKLED) {
    flushDuration = duration;
    currentStatus = Status::FLUSHING;
  }
}

void Brineomatic::pickle(uint64_t duration)
{
  stopFlag = false;
  if (currentStatus == Status::IDLE) {
    pickleDuration = duration;
    currentStatus = Status::PICKLING;
  }
}

void Brineomatic::stop()
{
  if (currentStatus == Status::RUNNING || currentStatus == Status::FLUSHING || currentStatus == Status::PICKLING) {
    stopFlag = true;
  }
}

void Brineomatic::initializeHardware()
{
  setMembranePressureTarget(0);
  openDiverterValve();
  disableHighPressurePump();
  disableBoostPump();
  closeFlushValve();
}

bool Brineomatic::hasDiverterValve()
{
  return flushValve != nullptr;
}

void Brineomatic::openDiverterValve()
{
}

void Brineomatic::closeDiverterValve()
{
}

bool Brineomatic::hasFlushValve()
{
  return flushValve != nullptr;
}

void Brineomatic::openFlushValve()
{
  if (hasFlushValve())
    flushValve->setState(true);
}

void Brineomatic::closeFlushValve()
{
  if (hasFlushValve())
    flushValve->setState(false);
}

bool Brineomatic::hasHighPressurePump()
{
  return highPressurePump != nullptr;
}

void Brineomatic::enableHighPressurePump()
{
  if (hasHighPressurePump())
    highPressurePump->setState(true);
}

void Brineomatic::disableHighPressurePump()
{
  if (hasHighPressurePump())
    highPressurePump->setState(false);
}

bool Brineomatic::hasBoostPump()
{
  return boostPump != nullptr;
}

void Brineomatic::enableBoostPump()
{
  if (hasBoostPump())
    boostPump->setState(true);
}

void Brineomatic::disableBoostPump()
{
  if (hasBoostPump())
    boostPump->setState(false);
}

float Brineomatic::getFilterPressure()
{
  return currentFilterPressure;
}

float Brineomatic::getFilterPressureMinimum()
{
  return lowPressureMinimum;
}

float Brineomatic::getMembranePressure()
{
  return currentMembranePressure;
}

float Brineomatic::getMembranePressureMinimum()
{
  return highPressureMinimum;
}

float Brineomatic::getFlowrate()
{
  return currentFlowrate;
}

float Brineomatic::getFlowrateMinimum()
{
  return flowrateMinimum;
}

float Brineomatic::getTemperature()
{
  return currentTemperature;
}

float Brineomatic::getSalinity()
{
  return currentSalinity;
}

float Brineomatic::getSalinityMaximum()
{
  return salinityMaximum;
}

const char* Brineomatic::getStatus()
{
  if (currentStatus == Status::STARTUP)
    return "STARTUP";
  else if (currentStatus == Status::IDLE)
    return "IDLE";
  else if (currentStatus == Status::RUNNING)
    return "RUNNING";
  else if (currentStatus == Status::FLUSHING)
    return "FLUSHING";
  else if (currentStatus == Status::PICKLING)
    return "PICKLING";
  else if (currentStatus == Status::PICKLED)
    return "PICKLED";
  else
    return "UNKNOWN";
}

uint64_t Brineomatic::getNextFlushCountdown()
{

  if (currentStatus == Status::IDLE && autoFlushEnabled)
    return nextFlushTime - esp_timer_get_time();

  return 0;
}

uint64_t Brineomatic::getRuntimeElapsed()
{
  if (currentStatus == Status::RUNNING)
    return esp_timer_get_time() - runtimeStart;

  return 0;
}

uint64_t Brineomatic::getFinishCountdown()
{
  if (currentStatus == Status::RUNNING && desiredRuntime > 0)
    return (runtimeStart + desiredRuntime) - esp_timer_get_time();

  return 0;
}

uint64_t Brineomatic::getFlushElapsed()
{
  if (currentStatus == Status::FLUSHING)
    return esp_timer_get_time() - flushStart;

  return 0;
}

uint64_t Brineomatic::getFlushCountdown()
{
  if (currentStatus == Status::FLUSHING)
    return (flushStart + flushDuration) - esp_timer_get_time();

  return 0;
}

uint64_t Brineomatic::getPickleElapsed()
{
  if (currentStatus == Status::PICKLING)
    return esp_timer_get_time() - pickleStart;

  return 0;
}

uint64_t Brineomatic::getPickleCountdown()
{
  if (currentStatus == Status::PICKLING)
    return (pickleStart + pickleDuration) - esp_timer_get_time();

  return 0;
}

void Brineomatic::runStateMachine()
{
  switch (currentStatus) {

    //
    // STARTUP
    //
    case Status::STARTUP:
      Serial.println("State: STARTUP");
      initializeHardware();
      if (isPickled)
        currentStatus = Status::PICKLED;
      else {
        if (autoFlushEnabled)
          nextFlushTime = esp_timer_get_time() + flushInterval;
        currentStatus = Status::IDLE;
      }
      break;

    //
    // PICKLED
    //
    case Status::PICKLED:
      Serial.println("State: PICKLED");
      break;

    //
    // IDLE
    //
    case Status::IDLE:
      Serial.println("State: IDLE");
      if (autoFlushEnabled && esp_timer_get_time() > nextFlushTime)
        currentStatus = Status::FLUSHING;
      break;

    //
    // RUNNING
    //
    case Status::RUNNING: {
      Serial.println("State: RUNNING");

      runtimeStart = esp_timer_get_time();
      float volume = 0.0;

      initializeHardware();

      if (hasBoostPump()) {
        enableBoostPump();
        while (getFilterPressure() < getFilterPressureMinimum()) {
          if (stopFlag) {
            initializeHardware();
            currentStatus = Status::FLUSHING;
            return;
          }
          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }

      enableHighPressurePump();
      setMembranePressureTarget(750);
      while (getMembranePressure() < getMembranePressureMinimum()) {
        if (stopFlag) {
          initializeHardware();
          currentStatus = Status::FLUSHING;
          return;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
      }

      bool ready = false;
      while (!ready) {
        if (stopFlag) {
          initializeHardware();
          currentStatus = Status::FLUSHING;
          return;
        }

        if (getFlowrate() >= getFlowrateMinimum())
          ready = true;
        else if (getSalinity() <= getSalinityMaximum())
          ready = true;

        vTaskDelay(pdMS_TO_TICKS(100));
      }

      closeDiverterValve();

      while (esp_timer_get_time() - runtimeStart < desiredRuntime && volume < desiredVolume) {
        if (stopFlag) {
          initializeHardware();
          currentStatus = Status::FLUSHING;
          return;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
      }

      initializeHardware();
      currentStatus = Status::FLUSHING;

      break;
    }

    //
    // FLUSHING
    //
    case Status::FLUSHING: {
      Serial.println("State: FLUSHING");

      flushStart = esp_timer_get_time();

      initializeHardware();
      TRACE();

      DUMP(lowPressureMinimum);
      openFlushValve();
      while (getFilterPressure() < getFilterPressureMinimum()) {
        if (stopFlag) {
          initializeHardware();
          currentStatus = Status::IDLE;
          return;
        }

        DUMP(getFilterPressure());
        Serial.println("Waiting on filter pressure");
        vTaskDelay(pdMS_TO_TICKS(100));
      }
      TRACE();

      enableHighPressurePump();
      TRACE();

      while (esp_timer_get_time() - flushStart < flushDuration) {
        Serial.println("Flushing.");
        if (stopFlag) {
          initializeHardware();
          currentStatus = Status::IDLE;
          return;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
      }
      TRACE();

      Serial.println("Flush done.");

      initializeHardware();

      if (autoFlushEnabled)
        nextFlushTime = esp_timer_get_time() + flushInterval;

      currentStatus = Status::IDLE;
      break;
    }

    //
    // PICKLING
    //
    case Status::PICKLING:
      Serial.println("State: PICKLING");

      pickleStart = esp_timer_get_time();

      initializeHardware();

      enableHighPressurePump();
      while (esp_timer_get_time() - pickleStart < pickleDuration) {
        if (stopFlag) {
          initializeHardware();
          currentStatus = Status::PICKLED;
          return;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
      }

      initializeHardware();

      currentStatus = Status::PICKLED;

      break;
  }
}
#endif