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
float temperatureReading = 0.0;

byte flowmeter_pin = YB_FLOWMETER_PIN;
static volatile int pulse_counter = 0;
unsigned long lastFlowmeterCheckMillis = 0;
float flowmeterPulsesPerLiter = YB_FLOWMETER_DEFAULT_PPL;
float flowrateReading = 0.0;

void IRAM_ATTR flowmeter_interrupt()
{
  pulse_counter++;
}

ADS1115 brineomatic_adc(YB_ADS1115_ADDRESS);
GravityTDS gravityTds;
float water_temperature = 25;
float tdsReading = 0;

float lowPressureReading = 0.0;
float highPressureReading = 0.0;

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
  flowrateReading = 0.0;

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
}

void brineomatic_loop()
{
  measure_flowmeter();
  measure_temperature();
  measure_tds();
  measure_lp_sensor();
  measure_hp_sensor();
}

void measure_flowmeter()
{
  if (millis() - lastFlowmeterCheckMillis >= 1000) {
    // detach interrupt while calculating rpm
    detachInterrupt(digitalPinToInterrupt(flowmeter_pin));

    // calculate flowrate
    flowrateReading = pulse_counter / flowmeterPulsesPerLiter;

    // reset counter
    pulse_counter = 0;

    // store milliseconds when tacho was measured the last time
    lastFlowmeterCheckMillis = millis();

    // attach interrupt again
    attachInterrupt(digitalPinToInterrupt(flowmeter_pin), flowmeter_interrupt, FALLING);
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

  temperatureReading = tempC;
}

void measure_tds()
{
  // temperature = readTemperature();  //add your temperature sensor and read it
  // gravityTds.setTemperature(temperature); // set the temperature and execute temperature compensation

  int16_t reading = brineomatic_adc.readADC(1);
  if (brineomatic_adc.getError() == ADS1X15_OK) {
    gravityTds.update(reading);            // sample and calculate
    tdsReading = gravityTds.getTdsValue(); // then get the value
  }
}

void measure_lp_sensor()
{
  int16_t reading = brineomatic_adc.readADC(2);
  if (brineomatic_adc.getError() == ADS1X15_OK) {
    float voltage = brineomatic_adc.toVoltage(reading);

    if (voltage < 0.4) {
      Serial.println("No LP Sensor Detected");
      lowPressureReading = -1;
      return;
    }

    float amperage = (voltage / YB_420_RESISTOR) * 1000;
    float lowPressureReading = map_generic(amperage, 4.0, 20.0, 0.0, YB_LP_SENSOR_MAX);
  }
}

void measure_hp_sensor()
{
  int16_t reading = brineomatic_adc.readADC(3);
  if (brineomatic_adc.getError() == ADS1X15_OK) {
    float voltage = brineomatic_adc.toVoltage(reading);

    if (voltage < 0.4) {
      Serial.println("No HP Sensor Detected");
      highPressureReading = -1;
      return;
    }

    float amperage = (voltage / YB_420_RESISTOR) * 1000;
    float highPressureReading = map_generic(amperage, 4.0, 20.0, 0.0, YB_HP_SENSOR_MAX);
  }
}

#endif