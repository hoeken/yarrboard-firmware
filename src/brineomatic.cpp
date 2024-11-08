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
  #include <ESP32Servo.h>
  #include <GravityTDS.h>
  #include <OneWire.h>

Brineomatic wm;
uint64_t desiredRuntime = 0;
float desiredVolume = 0;
uint64_t flushDuration = 0;
uint64_t pickleDuration = 0;
uint64_t nextFlushTime = 0;
uint64_t flushInterval = 5ULL * 24 * 60 * 60 * 1000000; // 5 day default, in microseconds

byte relay_pins[YB_RELAY_CHANNEL_COUNT] = YB_RELAY_CHANNEL_PINS;

Servo servos[YB_SERVO_CHANNEL_COUNT];
byte servo_pins[YB_SERVO_CHANNEL_COUNT] = YB_SERVO_CHANNEL_PINS;
int servo_min_us = 1000;
int servo_max_us = 2000;

byte motor_a_pins[YB_DC_MOTOR_CHANNEL_COUNT] = YB_DC_MOTOR_A_PINS;
byte motor_b_pins[YB_DC_MOTOR_CHANNEL_COUNT] = YB_DC_MOTOR_B_PINS;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(YB_DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);
DeviceAddress motorThermometer;

byte flowmeter_pin = YB_FLOWMETER_PIN;
static volatile int pulse_counter = 0;
unsigned long lastFlowmeterCheckMillis = 0;
float flowmeterPulsesPerLiter = YB_FLOWMETER_DEFAULT_PPL;

void IRAM_ATTR flowmeter_interrupt()
{
  pulse_counter++;
}

ADS1115 brineomatic_adc(YB_ADS1115_ADDRESS);
GravityTDS gravityTds;
float water_temperature = 25;

void brineomatic_setup()
{
  byte relay_pins[YB_RELAY_CHANNEL_COUNT] = YB_RELAY_CHANNEL_PINS;
  byte servo_pins[YB_SERVO_CHANNEL_COUNT] = YB_SERVO_CHANNEL_PINS;
  byte motor_a_pins[YB_DC_MOTOR_CHANNEL_COUNT] = YB_DC_MOTOR_A_PINS;
  byte motor_b_pins[YB_DC_MOTOR_CHANNEL_COUNT] = YB_DC_MOTOR_B_PINS;

  byte flowmeter_pin = YB_FLOWMETER_PIN;

  for (byte i = 0; i < YB_RELAY_CHANNEL_COUNT; i++) {
    pinMode(relay_pins[i], OUTPUT);
    digitalWrite(relay_pins[i], LOW);
  }

  for (byte i = 0; i < YB_SERVO_CHANNEL_COUNT; i++) {
    ESP32PWM::allocateTimer(i);
    servos[i].setPeriodHertz(50); // Standard 50hz servo
    servos[i].attach(servo_pins[i], servo_min_us, servo_max_us);
  }

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
  lastFlowmeterCheckMillis = 0;

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

  Wire.begin();
  brineomatic_adc.begin();
  if (brineomatic_adc.isConnected())
    Serial.println("ADS115 OK");
  else
    Serial.println("ADS115 Not Found");

  brineomatic_adc.setMode(1);     // SINGLE SHOT MODE
  brineomatic_adc.setGain(1);     // ±4.096V
  brineomatic_adc.setDataRate(7); // fastest

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
}

void brineomatic_loop()
{
  measure_flowmeter();
  measure_temperature();
  measure_salinity();
  measure_filter_pressure();
  measure_membrane_pressure();
}

Brineomatic::Status currentState = Brineomatic::Status::STARTUP;

// State machine task function
void brineomatic_state_machine(void* pvParameters)
{
  while (true) {
    switch (currentState) {

      //
      // STARTUP
      //
      case Brineomatic::Status::STARTUP:
        Serial.println("State: STARTUP");
        wm.setMembranePressureTarget(0);
        wm.setDiversion(false);
        wm.disableHighPressurePump();
        wm.disableBoostPump();
        if (wm.isPickled)
          currentState = Brineomatic::Status::PICKLED;
        else {
          if (wm.autoFlushEnabled)
            nextFlushTime = millis() + flushInterval;
          currentState = Brineomatic::Status::IDLE;
        }
        break;

      //
      // PICKLED
      //
      case Brineomatic::Status::PICKLED:
        Serial.println("State: PICKLED");
        break;

      //
      // IDLE
      //
      case Brineomatic::Status::IDLE:
        Serial.println("State: IDLE");
        if (wm.autoFlushEnabled && millis() > nextFlushTime)
          currentState = Brineomatic::Status::FLUSHING;
        break;

      //
      // RUNNING
      //
      case Brineomatic::Status::RUNNING: {
        Serial.println("State: RUNNING");

        unsigned long runtimeStart = millis();
        float volume = 0.0;

        wm.setMembranePressureTarget(0);
        wm.setDiversion(false);
        wm.disableHighPressurePump();
        wm.disableBoostPump();

        if (wm.hasBoostPump()) {
          wm.enableBoostPump();
          while (wm.getFilterPressure() < wm.getFilterPressureMinimum())
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        wm.enableHighPressurePump();
        wm.setMembranePressureTarget(750);
        while (wm.getMembranePressure() < wm.getMembranePressureMinimum())
          vTaskDelay(pdMS_TO_TICKS(100));

        bool ready = false;
        while (!ready) {
          if (wm.getFlowrate() >= wm.getFlowrateMinimum())
            ready = true;
          else if (wm.getSalinity() <= wm.getSalinityMaximum())
            ready = true;

          vTaskDelay(pdMS_TO_TICKS(100));
        }

        wm.setDiversion(true);

        if (millis() - runtimeStart > desiredRuntime || volume > desiredVolume) {
          wm.setDiversion(false);
          wm.setMembranePressureTarget(0);
          wm.disableHighPressurePump();
          wm.disableBoostPump();

          currentState = Brineomatic::Status::FLUSHING;
        }
        break;
      }

      //
      // FLUSHING
      //
      case Brineomatic::Status::FLUSHING: {
        Serial.println("State: FLUSHING");

        unsigned long flushStart = millis();

        wm.setDiversion(false);
        wm.setMembranePressureTarget(0);
        wm.disableHighPressurePump();
        wm.disableBoostPump();

        wm.openFlushValve();
        while (millis() - flushStart > flushDuration) {
          vTaskDelay(pdMS_TO_TICKS(100));
        }
        wm.closeFlushValve();

        if (wm.autoFlushEnabled)
          nextFlushTime = millis() + flushInterval;

        currentState = Brineomatic::Status::IDLE;
        break;
      }

      //
      // PICKLING
      //
      case Brineomatic::Status::PICKLING:
        Serial.println("State: PICKLING");

        unsigned long pickleStart = millis();

        wm.setDiversion(false);
        wm.setMembranePressureTarget(0);
        wm.disableHighPressurePump();
        wm.disableBoostPump();

        wm.enableBoostPump();
        wm.enableHighPressurePump();
        while (millis() - pickleStart > pickleDuration) {
          vTaskDelay(pdMS_TO_TICKS(100));
        }
        wm.disableHighPressurePump();
        wm.disableBoostPump();

        currentState = Brineomatic::Status::PICKLED;

        break;
    }

    // Add a delay to prevent task starvation
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void measure_flowmeter()
{
  if (millis() - lastFlowmeterCheckMillis >= 1000) {
    // detach interrupt while calculating rpm
    detachInterrupt(digitalPinToInterrupt(flowmeter_pin));

    // calculate flowrate
    float flowrate = pulse_counter / flowmeterPulsesPerLiter;

    // reset counter
    pulse_counter = 0;

    // store milliseconds when tacho was measured the last time
    lastFlowmeterCheckMillis = millis();

    // attach interrupt again
    attachInterrupt(digitalPinToInterrupt(flowmeter_pin), flowmeter_interrupt, FALLING);

    // update our model
    wm.setFlowrate(flowrate);
  }
}

void measure_temperature()
{
  // method 2 - faster
  float tempC = ds18b20.getTempC(motorThermometer);
  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: Could not read ds18B20 temperature data");
    return;
  }

  // update our model
  wm.setTemperature(tempC);
}

void measure_salinity()
{
  int16_t reading = brineomatic_adc.readADC(1);
  if (brineomatic_adc.getError() == ADS1X15_OK) {
    gravityTds.setTemperature(water_temperature); // set the temperature and execute temperature compensation
    gravityTds.update(reading);                   // sample and calculate
    float tdsReading = gravityTds.getTdsValue();  // then get the value
    wm.setSalinity(tdsReading);
  }
}

void measure_filter_pressure()
{
  int16_t reading = brineomatic_adc.readADC(2);
  if (brineomatic_adc.getError() == ADS1X15_OK) {
    float voltage = brineomatic_adc.toVoltage(reading);

    if (voltage < 0.4) {
      Serial.println("No LP Sensor Detected");
      wm.setFilterPressure(-1);
      return;
    }

    float amperage = (voltage / YB_420_RESISTOR) * 1000;
    float lowPressureReading = map_generic(amperage, 4.0, 20.0, 0.0, YB_LP_SENSOR_MAX);
    wm.setFilterPressure(lowPressureReading);
  }
}

void measure_membrane_pressure()
{
  int16_t reading = brineomatic_adc.readADC(3);
  if (brineomatic_adc.getError() == ADS1X15_OK) {
    float voltage = brineomatic_adc.toVoltage(reading);

    if (voltage < 0.4) {
      Serial.println("No HP Sensor Detected");
      wm.setMembranePressure(-1);
      return;
    }

    float amperage = (voltage / YB_420_RESISTOR) * 1000;
    float highPressureReading = map_generic(amperage, 4.0, 20.0, 0.0, YB_HP_SENSOR_MAX);
    wm.setMembranePressure(highPressureReading);
  }
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
                             membranePressureTarget(0)
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

void Brineomatic::setDiversion(bool value)
{
  diversionValveOpen = value;
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

void Brineomatic::disableHighPressurePump()
{
  highPressurePumpEnabled = false;
  Serial.println("High-pressure pump disabled");
}

void Brineomatic::disableBoostPump()
{
  boostPumpEnabled = false;
  Serial.println("Boost pump disabled");
}

void Brineomatic::enableHighPressurePump()
{
  highPressurePumpEnabled = true;
  Serial.println("High-pressure pump enabled");
}

void Brineomatic::enableBoostPump()
{
  boostPumpEnabled = true;
  Serial.println("Boost pump enabled");
}

bool Brineomatic::hasBoostPump()
{
  // Assume the system always has a boost pump
  return true;
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

void Brineomatic::openFlushValve()
{
  flushValveOpen = true;
  Serial.println("Flush valve opened");
}

void Brineomatic::closeFlushValve()
{
  flushValveOpen = false;
  Serial.println("Flush valve closed");
}

#endif