/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_IS_BRINEOMATIC

  #include "brineomatic.h"
  #include "debug.h"
  #include "etl/deque.h"
  #include "piezo.h"
  #include "relay_channel.h"
  #include "servo_channel.h"
  #include "stepper_channel.h"
  #include "validate.h"
  #include <ADS1X15.h>
  #include <Arduino.h>
  #include <DallasTemperature.h>
  #include <GravityTDS.h>
  #include <OneWire.h>

Brineomatic wm;

static volatile uint16_t product_flowmeter_pulse_counter = 0;
uint32_t lastProductFlowmeterCheckMicros = 0;
float productFlowmeterPulsesPerLiter = YB_PRODUCT_FLOWMETER_DEFAULT_PPL;

void IRAM_ATTR product_flowmeter_interrupt()
{
  product_flowmeter_pulse_counter = product_flowmeter_pulse_counter + 1;
}

static volatile uint16_t brine_flowmeter_pulse_counter = 0;
uint32_t lastBrineFlowmeterCheckMicros = 0;
float brineFlowmeterPulsesPerLiter = YB_BRINE_FLOWMETER_DEFAULT_PPL;

void IRAM_ATTR brine_flowmeter_interrupt()
{
  brine_flowmeter_pulse_counter = brine_flowmeter_pulse_counter + 1;
}

ADS1115 brineomatic_adc(YB_ADS1115_ADDRESS);
byte current_ads1115_channel = 0;

GravityTDS gravityTds;

void brineomatic_setup()
{
  wm.init();

  // do our init for our product flowmeter
  product_flowmeter_pulse_counter = 0;
  lastProductFlowmeterCheckMicros = 0;
  #ifdef YB_PRODUCT_FLOWMETER_PIN
  pinMode(YB_PRODUCT_FLOWMETER_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(YB_PRODUCT_FLOWMETER_PIN), product_flowmeter_interrupt, FALLING);
  #endif

  // do our init for our brine flowmeter
  brine_flowmeter_pulse_counter = 0;
  lastBrineFlowmeterCheckMicros = 0;
  #ifdef YB_BRINE_FLOWMETER_PIN
  pinMode(YB_BRINE_FLOWMETER_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(YB_BRINE_FLOWMETER_PIN), brine_flowmeter_interrupt, FALLING);
  #endif

  Wire.begin(YB_I2C_SDA_PIN, YB_I2C_SCL_PIN);
  Wire.setClock(YB_I2C_SPEED);
  brineomatic_adc.begin();
  if (brineomatic_adc.isConnected())
    YBP.println("ADS1115 OK");
  else
    YBP.println("ADS1115 Not Found");

  brineomatic_adc.setMode(1);     // SINGLE SHOT MODE
  brineomatic_adc.setGain(1);     // ±4.096V
  brineomatic_adc.setDataRate(3); // 64 samples per second.

  current_ads1115_channel = 0;
  brineomatic_adc.requestADC(current_ads1115_channel);

  gravityTds.setAref(YB_ADS1115_VREF); // reference voltage on ADC
  gravityTds.setAdcRange(15);          // 16 bit ADC, but its differential, so lose 1 bit.
  gravityTds.begin();                  // initialization

  // Create a FreeRTOS task for the state machine
  xTaskCreatePinnedToCore(
    brineomatic_state_machine, // Task function
    "brineomatic_sm",          // Name of the task
    4096,                      // Stack size
    NULL,                      // Task input parameters
    2,                         // Priority of the task
    NULL,                      // Task handle
    1                          // Core where the task should run
  );
}

uint32_t lastOutput;

void brineomatic_loop()
{
  if (millis() - lastOutput > 2000) {
    // for debug stuff here.
    lastOutput = millis();
  }

  measure_product_flowmeter();
  measure_brine_flowmeter();
  wm.measureMotorTemperature();

  if (brineomatic_adc.isReady()) {
    int16_t value = brineomatic_adc.getValue();

    if (brineomatic_adc.getError() == ADS1X15_OK) {
      if (current_ads1115_channel == 0)
        measure_brine_salinity(value);
      else if (current_ads1115_channel == 1)
        measure_product_salinity(value);
      else if (current_ads1115_channel == 2)
        measure_filter_pressure(value);
      else if (current_ads1115_channel == 3)
        measure_membrane_pressure(value);
    } else
      YBP.println("ADC Error.");

    // update to our channel index
    current_ads1115_channel++;
    if (current_ads1115_channel == 4)
      current_ads1115_channel = 0;

    // request new conversion
    brineomatic_adc.requestADC(current_ads1115_channel);
  }

  wm.manageHighPressureValve();
  wm.manageCoolingFan();
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

void measure_product_flowmeter()
{
  // check roughly every second
  uint32_t elapsed = micros() - lastProductFlowmeterCheckMicros;
  if (elapsed >= 1000000) {
    // calculate flowrate
    float elapsed_seconds = elapsed / 1000000;

    // Calculate liters per second
    float liters_per_second = (product_flowmeter_pulse_counter / productFlowmeterPulsesPerLiter) / elapsed_seconds;

    // Convert to liters per hour
    float flowrate = liters_per_second * 3600;

    // update our volume
    if ((wm.hasDiverterValve() && !wm.isDiverterValveOpen()) || !wm.hasDiverterValve()) {
      wm.currentVolume += product_flowmeter_pulse_counter / productFlowmeterPulsesPerLiter;
      wm.totalVolume += product_flowmeter_pulse_counter / productFlowmeterPulsesPerLiter;
    }

    // reset counter
    product_flowmeter_pulse_counter = 0;

    // store microseconds when tacho was measured the last time
    lastProductFlowmeterCheckMicros = micros();

    // update our model
    wm.setProductFlowrate(flowrate);
  }
}

void measure_brine_flowmeter()
{
  // check roughly every second
  uint32_t elapsed = micros() - lastBrineFlowmeterCheckMicros;
  if (elapsed >= 1000000) {
    // calculate flowrate
    float elapsed_seconds = elapsed / 1000000;

    // Calculate liters per second
    float liters_per_second = (brine_flowmeter_pulse_counter / brineFlowmeterPulsesPerLiter) / elapsed_seconds;

    // Convert to liters per hour
    float flowrate = liters_per_second * 3600;

    // update our volume
    if (wm.isFlushValveOpen())
      wm.currentFlushVolume += brine_flowmeter_pulse_counter / brineFlowmeterPulsesPerLiter;

    // reset counter
    brine_flowmeter_pulse_counter = 0;

    // store microseconds when tacho was measured the last time
    lastBrineFlowmeterCheckMicros = micros();

    // update our model
    wm.setBrineFlowrate(flowrate);
  }
}

void Brineomatic::measureMotorTemperature()
{
  if (ds18b20.isConversionComplete()) {
    float tempC = ds18b20.getTempC(motorThermometer);

    if (tempC == DEVICE_DISCONNECTED_C)
      setMotorTemperature(-999);
    setMotorTemperature(tempC);

    ds18b20.requestTemperatures();
  }
}

void measure_product_salinity(int16_t reading)
{
  gravityTds.setTemperature(wm.getWaterTemperature()); // set the temperature and execute temperature compensation
  gravityTds.update(reading);                          // sample and calculate
  float tdsReading = gravityTds.getTdsValue();         // then get the value
  wm.setProductSalinity(tdsReading);
}

void measure_brine_salinity(int16_t reading)
{
  gravityTds.setTemperature(wm.getWaterTemperature()); // set the temperature and execute temperature compensation
  gravityTds.update(reading);                          // sample and calculate
  float tdsReading = gravityTds.getTdsValue();         // then get the value
  wm.setBrineSalinity(tdsReading);
}

void measure_filter_pressure(int16_t reading)
{
  float voltage = brineomatic_adc.toVoltage(reading);
  float amperage = (voltage / YB_420_RESISTOR) * 1000;

  // we wan
  if (amperage < 3.5) {
    // YBP.println("No LP Sensor Detected");
    wm.setFilterPressure(-999);
    return;
  }

  if (amperage < 4.0)
    amperage = 4.0;

  float lowPressureReading = map_generic(amperage, 4.0, 20.0, 0.0, YB_LP_SENSOR_MAX);
  wm.setFilterPressure(lowPressureReading);
}

void measure_membrane_pressure(int16_t reading)
{
  float voltage = brineomatic_adc.toVoltage(reading);
  float amperage = (voltage / YB_420_RESISTOR) * 1000;

  if (amperage < 3.5) {
    // YBP.println("No HP Sensor Detected");
    wm.setMembranePressure(-999);
    return;
  }

  if (amperage < 4.0)
    amperage = 4.0;

  float highPressureReading = map_generic(amperage, 4.0, 20.0, 0.0, YB_HP_SENSOR_MAX);

  // if (highPressureReading > ge)
  //   YBP.printf("high pressure: %.3f\n", highPressureReading);

  wm.setMembranePressure(highPressureReading);
}

Brineomatic::Brineomatic() : oneWire(YB_DS18B20_PIN), // constructor arg for OneWire
                             ds18b20(&oneWire)        // DallasTemperature needs pointer to OneWire
{
}

void Brineomatic::init()
{
  // enabled or no
  if (preferences.isKey("bomPickled"))
    isPickled = preferences.getBool("bomPickled");
  else
    isPickled = false;

  if (preferences.isKey("bomTotVolume"))
    totalVolume = preferences.getFloat("bomTotVolume");
  else
    totalVolume = 0.0;

  if (preferences.isKey("bomTotRuntime"))
    totalRuntime = preferences.getULong("bomTotRuntime");
  else
    totalRuntime = 0;

  if (preferences.isKey("bomTotCycles"))
    totalCycles = preferences.getUInt("bomTotCycles");
  else
    totalCycles = 0;

  boostPumpOnState = false;
  highPressurePumpOnState = false;
  diverterValveOpenState = true;
  flushValveOpenState = false;
  coolingFanOnState = false;

  currentTankLevel = -1;
  currentWaterTemperature = 25.0;
  currentMotorTemperature = 0.0;
  currentProductFlowrate = 0.0;
  currentBrineFlowrate = 0.0;
  currentVolume = 0.0;
  currentFlushVolume = 0.0;
  currentProductSalinity = 0.0;
  currentBrineSalinity = 0.0;
  currentFilterPressure = 0.0;
  currentMembranePressure = 0.0;
  currentMembranePressureTarget = -1;

  currentStatus = Status::STARTUP;
  runResult = Result::STARTUP;
  flushResult = Result::STARTUP;
  pickleResult = Result::STARTUP;
  depickleResult = Result::STARTUP;

  // PID settings - Ramp Up
  // KpRamp = 2.2;
  // KiRamp = 0;
  // KdRamp = 0.55;

  // PID Settings - Maintain
  // KpMaintain = 1.50;
  // KiMaintain = 0.02;
  // KdMaintain = 0;

  // PID controller
  // membranePressurePID = QuickPID(&currentMembranePressure, &membranePressurePIDOutput, &currentMembranePressureTarget);
  // membranePressurePID.SetMode(QuickPID::Control::automatic);
  // membranePressurePID.SetAntiWindupMode(QuickPID::iAwMode::iAwClamp);
  // membranePressurePID.SetTunings(KpRamp, KiRamp, KdRamp);
  // membranePressurePID.SetControllerDirection(QuickPID::Action::direct);
  // membranePressurePID.SetOutputLimits(YB_BOM_PID_OUTPUT_MIN, YB_BOM_PID_OUTPUT_MAX);

  this->initChannels();

  // DS18B20 Sensor
  ds18b20.begin();
  YBP.print("Found ");
  YBP.print(ds18b20.getDeviceCount(), DEC);
  YBP.println(" DS18B20 devices.");

  // lookup our address
  if (!ds18b20.getAddress(motorThermometer, 0))
    YBP.println("Unable to find address for DS18B20");

  ds18b20.setResolution(motorThermometer, 9);
  ds18b20.setWaitForConversion(false);
  ds18b20.requestTemperatures();

  if (YB_HAS_MODBUS) {
    YB_MODBUS_SERIAL.setPins(YB_MODBUS_RX, YB_MODBUS_TX);
    YB_MODBUS_SERIAL.begin(YB_MODBUS_SPEED);
  }
}

void Brineomatic::initChannels()
{
  for (auto& ch : relay_channels) {
    ch.init(ch.id);
    ch.isEnabled = false;
    ch.defaultState = false;
  }

  for (auto& ch : servo_channels) {
    ch.init(ch.id);
    ch.isEnabled = false;
  }

  for (auto& ch : stepper_channels) {
    ch.init(ch.id);
    ch.isEnabled = false;
  }

  if (boostPumpControl.equals("RELAY")) {
    boostPump = getChannelById(boostPumpRelayId, relay_channels);
    boostPump->isEnabled = true;
    boostPump->setName("Boost Pump");
    boostPump->setKey("boost_pump");
    strncpy(boostPump->type, "water_pump", sizeof(boostPump->type));
  }

  if (flushValveControl.equals("RELAY")) {
    flushValve = getChannelById(flushValveRelayId, relay_channels);
    flushValve->isEnabled = true;
    flushValve->setName("Flush Valve");
    flushValve->setKey("flush_valve");
    strncpy(flushValve->type, "solenoid", sizeof(flushValve->type));
  }

  if (coolingFanControl.equals("RELAY")) {
    coolingFan = getChannelById(coolingFanRelayId, relay_channels);
    coolingFan->isEnabled = true;
    coolingFan->setName("Cooling Fan");
    coolingFan->setKey("cooling_fan");
    strncpy(coolingFan->type, "fan", sizeof(coolingFan->type));
  }

  if (highPressurePumpControl.equals("RELAY")) {
    highPressurePump = getChannelById(highPressureRelayId, relay_channels);
    highPressurePump->isEnabled = true;
    highPressurePump->setName("High Pressure Pump");
    highPressurePump->setKey("hp_pump");
    strncpy(highPressurePump->type, "water_pump", sizeof(highPressurePump->type));
  }

  if (diverterValveControl.equals("SERVO")) {
    diverterValve = getChannelById(diverterValveServoId, servo_channels);
    diverterValve->isEnabled = true;
    diverterValve->setName("Diverter Valve");
    diverterValve->setKey("diverter_valve");
  }

  if (highPressureValveControl.equals("STEPPER")) {
    highPressureValveStepper = getChannelById(highPressureValveStepperId, stepper_channels);
    highPressureValveStepper->isEnabled = true;
    highPressureValveStepper->setName("High Pressure Valve");
    highPressureValveStepper->setKey("hp_valve");
    highPressureValveStepper->setStepsPerDegree(
      (360.0 / highPressureValveStepperStepAngle) *
      YB_STEPPER_MICROSTEPS *
      highPressureValveStepperGearRatio);
  }
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
  currentMembranePressureTarget = pressure;

  // we got a real pressure
  if (pressure >= 0) {
    if (highPressureValveControl.equals("STEPPER")) {
      // static angle mode for now.
      if (pressure > 0) {
        highPressureValveStepper->gotoAngle(
          highPressureValveStepperCloseAngle,
          highPressureValveStepperCloseSpeed);
      } else {
        highPressureValveStepper->gotoAngle(
          highPressureValveStepperOpenAngle,
          highPressureValveStepperOpenSpeed);
      }
    }

    // if (highPressureValveControl.equals("SERVO")) {
    //   membranePressurePID.Initialize();
    //   membranePressurePID.Reset();

    //   // header for debugging.
    //   YBP.println("Membrane Pressure Target,Current Membrane Pressure,Pterm,Iterm,Kterm,Output Sum, PID Output, Servo Angle");
    // }
  }
  // negative target, we're done
  else {
    if (highPressureValveControl.equals("STEPPER")) {
      YBP.println("target <= 0, disable our stepper");
      highPressureValveStepper->gotoAngle(highPressureValveStepperOpenAngle, highPressureValveStepperOpenSpeed);
      highPressureValveStepper->waitUntilStopped();
      highPressureValveStepper->disable();
    }
  }
}

void Brineomatic::setProductFlowrate(float flowrate)
{
  currentProductFlowrate = flowrate;
}

void Brineomatic::setBrineFlowrate(float flowrate)
{
  currentBrineFlowrate = flowrate;
}

void Brineomatic::setMotorTemperature(float temp)
{
  currentMotorTemperature = temp;
}

void Brineomatic::setProductSalinity(float salinity)
{
  currentProductSalinity = salinity;
}

void Brineomatic::setBrineSalinity(float salinity)
{
  currentBrineSalinity = salinity;
}

void Brineomatic::idle()
{
  if (currentStatus == Status::MANUAL)
    currentStatus = Status::IDLE;
}

void Brineomatic::manual()
{
  if (currentStatus == Status::IDLE)
    currentStatus = Status::MANUAL;
}

void Brineomatic::start()
{
  if (currentStatus == Status::IDLE) {
    desiredRuntime = 0;
    desiredVolume = 0;
    currentStatus = Status::RUNNING;
  }
}

void Brineomatic::startDuration(uint32_t duration)
{
  if (currentStatus == Status::IDLE) {
    desiredRuntime = duration;
    desiredVolume = 0;
    currentStatus = Status::RUNNING;
  }
}

void Brineomatic::startVolume(float volume)
{
  if (currentStatus == Status::IDLE) {
    desiredRuntime = 0;
    desiredVolume = volume;
    currentStatus = Status::RUNNING;
  }
}

void Brineomatic::flush()
{
  if (currentStatus == Status::IDLE || currentStatus == Status::PICKLED || currentStatus == Status::STOPPING) {
    desiredFlushDuration = 0;
    desiredFlushVolume = 0;
    currentStatus = Status::FLUSHING;
  }
}

void Brineomatic::flushDuration(uint32_t duration)
{
  if (currentStatus == Status::IDLE || currentStatus == Status::PICKLED || currentStatus == Status::STOPPING) {
    desiredFlushDuration = duration;
    desiredFlushVolume = 0;
    currentStatus = Status::FLUSHING;
  }
}

void Brineomatic::flushVolume(float volume)
{
  if (currentStatus == Status::IDLE || currentStatus == Status::PICKLED || currentStatus == Status::STOPPING) {
    desiredFlushDuration = 0;
    desiredFlushVolume = volume;
    currentStatus = Status::FLUSHING;
  }
}

void Brineomatic::pickle(uint32_t duration)
{
  if (currentStatus == Status::IDLE) {
    pickleDuration = duration;
    currentStatus = Status::PICKLING;
  }
}

void Brineomatic::depickle(uint32_t duration)
{
  if (currentStatus == Status::PICKLED) {
    depickleDuration = duration;
    currentStatus = Status::DEPICKLING;
  }
}

void Brineomatic::stop()
{
  if (currentStatus == Status::RUNNING || currentStatus == Status::FLUSHING || currentStatus == Status::PICKLING || currentStatus == Status::DEPICKLING) {
    stopFlag = true;
  }
}

bool Brineomatic::initializeHardware()
{
  bool isFailure = false;

  openDiverterValve();

  // actively running, zero out our pressure
  if (currentMembranePressureTarget > 0) {
    setMembranePressureTarget(0);

    if (hasMembranePressureSensor) {
      uint32_t membranePressureStart = millis();
      YBP.println("Waiting for zero pressure.");
      while (getMembranePressure() > 65) {
        if (INTERVAL(250))
          YBP.print(".");

        if (millis() - membranePressureStart > membranePressureTimeout) {
          YBP.println("Membrane pressure timeout.");
          isFailure = true;
          break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
      }
      YBP.println("\nMembrane Pressure off");
    }

    // turns our high pressure valve controller off
    setMembranePressureTarget(-1);
  }

  disableHighPressurePump();

  if (highPressureValveControl.equals("STEPPER")) {
    if (highPressureValveStepper->home(highPressureValveStepperOpenSpeed))
      YBP.println("Stepper Homing OK");
    else
      YBP.println("Stepper Homing failed.");
  }

  disableBoostPump();
  closeFlushValve();
  disableCoolingFan();

  if (isFailure)
    YBP.println("Hardware Init Failed");
  else
    YBP.println("Hardware Init OK");

  return isFailure;
}

bool Brineomatic::autoflushEnabled()
{
  if (!hasFlushValve())
    return false;

  return !autoflushMode.equals("NONE");
}

bool Brineomatic::hasBoostPump()
{
  return !boostPumpControl.equals("NONE");
}

bool Brineomatic::isBoostPumpOn()
{
  return boostPumpOnState;
}

void Brineomatic::enableBoostPump()
{
  if (hasBoostPump()) {
    YBP.println("Boost Pump ON");
    if (boostPumpControl.equals("RELAY"))
      boostPump->setState(true);
  }
  boostPumpOnState = true;
}

void Brineomatic::disableBoostPump()
{
  if (hasBoostPump()) {
    YBP.println("Boost Pump OFF");
    if (boostPumpControl.equals("RELAY"))
      boostPump->setState(false);
  }
  boostPumpOnState = false;
}

bool Brineomatic::hasHighPressurePump()
{
  return !highPressurePumpControl.equals("NONE");
}

bool Brineomatic::isHighPressurePumpOn()
{
  return highPressurePumpOnState;
}

void Brineomatic::enableHighPressurePump()
{
  if (hasHighPressurePump()) {
    YBP.println("High Pressure Pump ON");
    if (highPressurePumpControl.equals("RELAY"))
      highPressurePump->setState(true);
    else if (highPressurePumpControl.equals("MODBUS"))
      modbusEnableHighPressurePump();
  }
  highPressurePumpOnState = true;
}

void Brineomatic::disableHighPressurePump()
{
  if (hasHighPressurePump()) {
    YBP.println("High Pressure Pump OFF");
    if (highPressurePumpControl.equals("RELAY"))
      highPressurePump->setState(false);
    else if (highPressurePumpControl.equals("MODBUS"))
      modbusDisableHighPressurePump();
  }
  highPressurePumpOnState = false;
}

void Brineomatic::modbusEnableHighPressurePump()
{
  if (highPressurePumpModbusDevice.equals("GD20")) {
    YBP.println("GD20 Pump Enable");
  }
}

void Brineomatic::modbusDisableHighPressurePump()
{
  if (highPressurePumpModbusDevice.equals("GD20")) {
    YBP.println("GD20 Pump Disable");
  }
}

bool Brineomatic::hasDiverterValve()
{
  return !diverterValveControl.equals("NONE");
}

bool Brineomatic::isDiverterValveOpen()
{
  return diverterValveOpenState;
}

void Brineomatic::openDiverterValve()
{
  if (hasDiverterValve()) {
    YBP.println("Diverter Valve Open");
    if (diverterValveControl.equals("SERVO"))
      diverterValve->write(diverterValveOpenAngle);
  }
  diverterValveOpenState = true;
}

void Brineomatic::closeDiverterValve()
{
  if (hasDiverterValve()) {
    YBP.println("Diverter Valve Closed");
    if (diverterValveControl.equals("SERVO"))
      diverterValve->write(diverterValveCloseAngle);
  }
  diverterValveOpenState = false;
}

bool Brineomatic::hasFlushValve()
{
  return !flushValveControl.equals("NONE");
}

bool Brineomatic::isFlushValveOpen()
{
  return flushValveOpenState;
}

void Brineomatic::openFlushValve()
{
  if (hasFlushValve()) {
    YBP.println("Flush Valve Open");
    if (flushValveControl.equals("RELAY"))
      flushValve->setState(true);
  }
  flushValveOpenState = true;
}

void Brineomatic::closeFlushValve()
{
  if (hasFlushValve()) {
    YBP.println("Flush Valve Closed");
    if (flushValveControl.equals("RELAY"))
      flushValve->setState(false);
  }
  flushValveOpenState = false;
}

bool Brineomatic::hasCoolingFan()
{
  return !coolingFanControl.equals("NONE");
}

bool Brineomatic::isCoolingFanOn()
{
  return coolingFanOnState;
}

void Brineomatic::enableCoolingFan()
{
  if (hasCoolingFan()) {
    // YBP.println("Cooling Fan ON");
    if (coolingFanControl.equals("RELAY"))
      coolingFan->setState(true);
  }
  coolingFanOnState = true;
}

void Brineomatic::disableCoolingFan()
{
  if (hasCoolingFan()) {
    // YBP.println("Cooling Fan OFF");
    if (coolingFanControl.equals("RELAY"))
      coolingFan->setState(false);
  }
  coolingFanOnState = false;
}

void Brineomatic::manageCoolingFan()
{
  if (currentStatus != Status::MANUAL) {
    if (hasCoolingFan() && hasMotorTemperatureSensor) {
      if (getMotorTemperature() >= coolingFanOnTemperature)
        enableCoolingFan();
      else if (getMotorTemperature() <= coolingFanOffTemperature)
        disableCoolingFan();
    }
  }
}

float Brineomatic::getFilterPressure()
{
  return currentFilterPressure;
}

float Brineomatic::getFilterPressureMinimum()
{
  return filterPressureLowThreshold;
}

float Brineomatic::getMembranePressure()
{
  return currentMembranePressure;
}

float Brineomatic::getMembranePressureMinimum()
{
  return membranePressureLowThreshold;
}

float Brineomatic::getProductFlowrate()
{
  return currentProductFlowrate;
}

float Brineomatic::getBrineFlowrate()
{
  return currentBrineFlowrate;
}

float Brineomatic::getProductFlowrateMinimum()
{
  return productFlowrateLowThreshold;
}

float Brineomatic::getTotalFlowrate()
{
  if (isDiverterValveOpen())
    return getBrineFlowrate();
  else
    return getProductFlowrate() + getBrineFlowrate();
}

float Brineomatic::getVolume()
{
  return currentVolume;
}

float Brineomatic::getFlushVolume()
{
  return currentFlushVolume;
}

float Brineomatic::getTotalVolume()
{
  return totalVolume;
}

uint32_t Brineomatic::getTotalRuntime()
{
  return totalRuntime;
}

uint32_t Brineomatic::getTotalCycles()
{
  return totalCycles;
}

float Brineomatic::getWaterTemperature()
{
  return currentWaterTemperature;
}

void Brineomatic::setWaterTemperature(float temp)
{
  currentWaterTemperature = temp;
}

void Brineomatic::setTankLevel(float level)
{
  currentTankLevel = level;
}

float Brineomatic::getMotorTemperature()
{
  return currentMotorTemperature;
}

float Brineomatic::getMotorTemperatureMaximum()
{
  return motorTemperatureHighThreshold;
}

float Brineomatic::getProductSalinity()
{
  if (currentStatus == Status::IDLE)
    return 0;
  else
    return currentProductSalinity;
}

float Brineomatic::getBrineSalinity()
{
  if (currentStatus == Status::IDLE)
    return 0;
  else
    return currentBrineSalinity;
}

float Brineomatic::getProductSalinityMaximum()
{
  return productSalinityHighThreshold;
}

float Brineomatic::getTankLevel()
{
  return currentTankLevel;
}

float Brineomatic::getTankCapacity()
{
  return tankCapacity;
}

const char* Brineomatic::getStatus()
{
  if (currentStatus == Status::STARTUP)
    return "STARTUP";
  else if (currentStatus == Status::MANUAL)
    return "MANUAL";
  else if (currentStatus == Status::IDLE)
    return "IDLE";
  else if (currentStatus == Status::RUNNING)
    return "RUNNING";
  else if (currentStatus == Status::STOPPING)
    return "STOPPING";
  else if (currentStatus == Status::FLUSHING)
    return "FLUSHING";
  else if (currentStatus == Status::PICKLING)
    return "PICKLING";
  else if (currentStatus == Status::DEPICKLING)
    return "DEPICKLING";
  else if (currentStatus == Status::PICKLED)
    return "PICKLED";
  else
    return "UNKNOWN";
}

Brineomatic::Result Brineomatic::getRunResult()
{
  return runResult;
}

Brineomatic::Result Brineomatic::getFlushResult()
{
  return flushResult;
}

Brineomatic::Result Brineomatic::getPickleResult()
{
  return pickleResult;
}

Brineomatic::Result Brineomatic::getDepickleResult()
{
  return depickleResult;
}

const char* Brineomatic::resultToString(Result result)
{
  switch (result) {
  #define X(name)      \
    case Result::name: \
      return #name;
    BOM_RESULT_LIST
  #undef X
    default:
      return "UNKNOWN";
  }
}

uint32_t Brineomatic::getNextFlushCountdown()
{
  if (currentStatus == Status::IDLE && autoflushEnabled())
    return autoflushInterval - (millis() - lastAutoFlushTime);

  return 0;
}

uint32_t Brineomatic::getRuntimeElapsed()
{
  if (currentStatus == Status::RUNNING)
    runtimeElapsed = millis() - runtimeStart;

  return runtimeElapsed;
}

uint32_t Brineomatic::getFinishCountdown()
{
  if (currentStatus == Status::RUNNING) {
    // are we on a timer?
    if (desiredRuntime > 0) {
      int32_t countdown = desiredRuntime - (millis() - runtimeStart);
      if (countdown > 0)
        return countdown;
    } else if (desiredVolume > 0) {
      float flowrate = getProductFlowrate();
      if (flowrate > 0) {
        float remainingVolume = desiredVolume - currentVolume;
        uint32_t remainingMillis = (remainingVolume / flowrate) * 3600 * 1000;
        return remainingMillis;
      }
    }
    // if we have tank capacity and a flowrate, we can estimate.
    else if (getTankCapacity() > 0 && getProductFlowrate() > 0) {
      float remainingVolume = getTankCapacity() * (1.0 - getTankLevel());
      float flowrate = getProductFlowrate();
      if (flowrate > 0) {
        uint32_t remainingMillis = (remainingVolume / flowrate) * (3600 * 1000);
        return remainingMillis;
      }
    }
  }

  return 0;
}

uint32_t Brineomatic::getFlushElapsed()
{
  if (currentStatus == Status::FLUSHING)
    return millis() - flushStart;

  return 0;
}

uint32_t Brineomatic::getFlushCountdown()
{
  if (currentStatus != Status::FLUSHING)
    return 0;

  if (desiredFlushDuration) {
    int32_t countdown = desiredFlushDuration - (millis() - flushStart);
    if (countdown > 0)
      return countdown;
  } else if (desiredFlushVolume) {
    float flowrate = getBrineFlowrate();
    if (flowrate > 0) {
      float remainingVolume = desiredFlushVolume - getFlushVolume();
      uint32_t remainingMillis = (remainingVolume / flowrate) * 3600 * 1000;
      return remainingMillis;
    }
  } else {
    int32_t countdown = flushTimeout - (millis() - flushStart);
    if (countdown > 0)
      return countdown;
  }

  return 0;
}

uint32_t Brineomatic::getPickleElapsed()
{
  if (currentStatus == Status::PICKLING)
    return millis() - pickleStart;

  return 0;
}

uint32_t Brineomatic::getPickleCountdown()
{
  if (currentStatus == Status::PICKLING) {
    int32_t countdown = pickleDuration - (millis() - pickleStart);
    if (countdown > 0)
      return countdown;
  }

  return 0;
}

uint32_t Brineomatic::getDepickleElapsed()
{
  if (currentStatus == Status::DEPICKLING)
    return millis() - depickleStart;

  return 0;
}

uint32_t Brineomatic::getDepickleCountdown()
{
  if (currentStatus == Status::DEPICKLING) {
    int32_t countdown = depickleDuration - (millis() - depickleStart);
    if (countdown > 0)
      return countdown;
  }

  return 0;
}

bool Brineomatic::hasHighPressureValve()
{
  return !highPressureValveControl.equals("NONE");
}

void Brineomatic::manageHighPressureValve()
{
  //
  // TODO: putting all of this on hold until its time to re-implement PID
  //

  // float angle;

  // if (currentStatus != Status::IDLE) {
  //   if (hasHighPressureValve()) {
  //     if (currentMembranePressureTarget >= 0) {
  //       // only use Ki for tuning once we are close to our target.
  //       if (abs(currentMembranePressureTarget - currentMembranePressure) / currentMembranePressureTarget > 0.05)
  //         membranePressurePID.SetTunings(KpRamp, KiRamp, KdRamp);
  //       else
  //         membranePressurePID.SetTunings(KpMaintain, KpMaintain, KdMaintain);

  //       // run our PID calculations
  //       if (membranePressurePID.Compute()) {
  //         // different max values for the ramp
  //         if (abs(currentMembranePressureTarget - currentMembranePressure) / currentMembranePressureTarget > 0.05)
  //           angle = map(membranePressurePIDOutput, YB_BOM_PID_OUTPUT_MIN, YB_BOM_PID_OUTPUT_MAX, highPressureValveOpenMax, highPressureValveCloseMax);
  //         // smaller max values for maintain.
  //         else
  //           angle = map(membranePressurePIDOutput, YB_BOM_PID_OUTPUT_MIN, YB_BOM_PID_OUTPUT_MAX, highPressureValveMaintainOpenMax, highPressureValveMaintainCloseMax);

  //         // YBP.printf("HP PID | current: %.0f / target: %.0f | p: % .3f / i: % .3f / d: % .3f / sum: % .3f | output: %.0f / angle: %.0f\n", round(currentMembranePressure), round(currentMembranePressureTarget), membranePressurePID.GetPterm(), membranePressurePID.GetIterm(), membranePressurePID.GetDterm(), membranePressurePID.GetOutputSum(), membranePressurePIDOutput, angle);
  //       }
  //     }
  //   }
  // }
}

void Brineomatic::runStateMachine()
{
  switch (currentStatus) {

    //
    // STARTUP
    //
    case Status::STARTUP:
      YBP.println("STARTUP");
      initializeHardware();

      if (isPickled)
        currentStatus = Status::PICKLED;
      else {
        if (autoflushEnabled()) {
          lastAutoFlushTime = millis();
        }
        currentStatus = Status::IDLE;
      }
      break;

    //
    // PICKLED
    //
    case Status::PICKLED:
      break;

    //
    // MANUAL
    //
    case Status::MANUAL:
      break;

    //
    // IDLE
    //
    case Status::IDLE:
      if (autoflushEnabled() && millis() - lastAutoFlushTime > autoflushInterval) {
        if (autoflushMode.equals("TIME"))
          flushDuration(autoflushDuration);
        else if (autoflushMode.equals("VOLUME"))
          flushVolume(autoflushVolume);
        else if (autoflushMode.equals("SALINITY"))
          flush();
      }
      break;

    //
    // RUNNING
    //
    case Status::RUNNING: {
      YBP.println("RUNNING");

      resetErrorTimers();
      runtimeStart = millis();
      uint32_t lastRuntimeUpdate = runtimeStart;

      currentVolume = 0;
      currentFlushVolume = 0;

      if (initializeHardware()) {
        currentStatus = Status::IDLE;
        return;
      }

      uint32_t boostPumpStart = millis();
      if (hasBoostPump()) {
        YBP.println("Boost Pump Started");
        enableBoostPump();
        if (hasFilterPressureSensor && enableFilterPressureLowCheck) {
          while (getFilterPressure() < getFilterPressureMinimum()) {
            if (checkStopFlag(runResult))
              return;

            if (checkFilterPressureLow())
              return;

            vTaskDelay(pdMS_TO_TICKS(100));
          }
        }
        YBP.println("Boost Pump OK");
      }

      enableHighPressurePump();
      setMembranePressureTarget(membranePressureTarget);

      if (waitForMembranePressure()) {
        YBP.println("Membrane Pressure Error");
        return;
      }

      if (waitForProductFlowrate()) {
        YBP.println("Product Flowrate Error");
        return;
      }

      if (waitForProductSalinity()) {
        YBP.println("Product Salinity Error");
        return;
      }

      closeDiverterValve();

      uint32_t productionStart = millis();
      while (true) {
        if (checkDiverterValveClosed())
          return;

        if (checkFilterPressureLow())
          return;

        if (checkFilterPressureHigh())
          return;

        if (checkMembranePressureLow())
          return;

        if (checkMembranePressureHigh())
          return;

        if (checkRunTotalFlowrateLow())
          return;

        if (checkProductFlowrateLow())
          return;

        if (checkProductFlowrateHigh())
          return;

        if (checkProductSalinityHigh())
          return;

        if (checkMotorTemperature(runResult))
          return;

        if (checkStopFlag(runResult))
          return;

        if (millis() - productionStart > productionRuntimeTimeout) {
          currentStatus = Status::STOPPING;
          runResult = Result::ERR_PRODUCTION_TIMEOUT;
          return;
        }

        // are we going for time?
        if (desiredRuntime > 0 && getRuntimeElapsed() >= desiredRuntime) {
          runResult = Result::SUCCESS_TIME;
          break;
        }

        // are we going for volume?
        if (desiredVolume > 0 && getVolume() >= desiredVolume) {
          runResult = Result::SUCCESS_VOLUME;
          break;
        }

        // tank level means we're finished successfully
        if (checkTankLevel()) {
          runResult = Result::SUCCESS_TANK_LEVEL;
          break;
        }

        // save our total runtime occasionally
        if (millis() - lastRuntimeUpdate > 15 * 60 * 1000) {
          totalRuntime += (millis() - lastRuntimeUpdate) / 1000; // store as seconds
          preferences.putULong("bomTotRuntime", totalRuntime);
          lastRuntimeUpdate = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(100));
      }

      // save our total volume produced
      preferences.putFloat("bomTotVolume", totalVolume);

      // save our runtime too.
      totalRuntime += (millis() - lastRuntimeUpdate) / 1000; // store as seconds
      preferences.putULong("bomTotRuntime", totalRuntime);

      // save our total number of cycles
      totalCycles++;
      preferences.putUInt("bomTotCycles", totalCycles);

      // next step... turn it off!
      currentStatus = Status::STOPPING;

      break;
    }

    //
    // STOPPING
    //
    case Status::STOPPING: {
      YBP.println("STOPPING");
      YBP.printf("Run Status: %s\n", resultToString(runResult));

      resetErrorTimers();

      if (initializeHardware()) {
        currentStatus = Status::IDLE;
        return;
      } else {
        if (runResult == Result::SUCCESS_TIME || runResult == Result::SUCCESS_VOLUME || runResult == Result::SUCCESS_VOLUME)
          playMelodyByName(successMelody.c_str());
        else
          playMelodyByName(errorMelody.c_str());

        if (autoflushMode.equals("TIME"))
          flushDuration(autoflushDuration);
        else if (autoflushMode.equals("VOLUME"))
          flushVolume(autoflushVolume);
        else if (autoflushMode.equals("SALINITY"))
          flush();
        else if (autoflushMode.equals("NONE"))
          currentStatus = Status::IDLE;
        else
          currentStatus = Status::IDLE;
      }

      break;
    }

    //
    // FLUSHING
    //
    case Status::FLUSHING: {
      YBP.println("FLUSHING");

      if (!hasFlushValve()) {
        currentStatus = Status::IDLE;
        return;
      }

      resetErrorTimers();

      flushStart = millis();
      currentFlushVolume = 0;

      if (initializeHardware()) {
        currentStatus = Status::IDLE;
        return;
      }

      // start up our hardware
      openFlushValve();
      if (autoflushUseHighPressureMotor)
        enableHighPressurePump();

      // check our sensors while we flush
      while (true) {

        if (checkFlushFilterPressureLow())
          break;

        if (checkFlushFlowrateLow())
          break;

        if (hasHighPressurePump() && autoflushUseHighPressureMotor && checkMotorTemperature(flushResult))
          break;

        if (checkStopFlag(flushResult))
          break;

        // are we going for time?
        if (desiredFlushDuration > 0 && getFlushElapsed() > desiredFlushDuration) {
          flushResult = Result::SUCCESS_TIME;
          // DUMP("DURATION");
          break;
        }

        // are we going for volume?
        if (desiredFlushVolume > 0 && getFlushVolume() >= desiredFlushVolume) {
          flushResult = Result::SUCCESS_VOLUME;
          DUMP("VOLUME");
          break;
        }

        // how about salinity? (auto)
        if (desiredFlushDuration == 0 && desiredFlushVolume == 0) {
          if (getBrineSalinity() < autoflushSalinity) {
            DUMP("SALINITY");
            flushResult = Result::SUCCESS_SALINITY;
            break;
          }
        }

        // did we hit our flush timeout?
        if (getFlushElapsed() > flushTimeout) {
          flushResult = Result::ERR_FLUSH_TIMEOUT;
          break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
      }

      if (autoflushEnabled())
        lastAutoFlushTime = millis();

      // keep track over restarts.
      preferences.putBool("bomPickled", false);

      initializeHardware();
      waitForFlushValveOff();

      currentStatus = Status::IDLE;

      break;
    }

    //
    // PICKLING
    //
    case Status::PICKLING:
      resetErrorTimers();

      pickleStart = millis();

      if (initializeHardware()) {
        currentStatus = Status::IDLE;
        return;
      }

      enableHighPressurePump();

      while (getPickleElapsed() < pickleDuration) {
        if (stopFlag)
          break;

        if (checkPickleTotalFlowrateLow(pickleResult)) {
          currentStatus = Status::IDLE;
          initializeHardware();
          return;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
      }

      if (initializeHardware()) {
        currentStatus = Status::IDLE;
        return;
      }

      currentStatus = Status::PICKLED;

      if (stopFlag)
        pickleResult = Result::USER_STOP;
      else
        pickleResult = Result::SUCCESS;

      // keep track over restarts.
      preferences.putBool("bomPickled", true);

      break;

    //
    // DEPICKLING
    //
    case Status::DEPICKLING:
      resetErrorTimers();

      depickleStart = millis();

      if (initializeHardware()) {
        currentStatus = Status::IDLE;
        return;
      }

      enableHighPressurePump();
      while (getDepickleElapsed() < depickleDuration) {
        if (stopFlag)
          break;

        if (checkPickleTotalFlowrateLow(depickleResult)) {
          currentStatus = Status::IDLE;
          initializeHardware();
          return;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
      }

      if (initializeHardware()) {
        currentStatus = Status::IDLE;
        return;
      }

      currentStatus = Status::IDLE;

      if (stopFlag)
        depickleResult = Result::USER_STOP;
      else
        depickleResult = Result::SUCCESS;

      // keep track over restarts.
      preferences.putBool("bomPickled", false);

      break;
  }
}

void Brineomatic::resetErrorTimers()
{
  stopFlag = false;
  membranePressureHighStart = 0;
  membranePressureLowStart = 0;
  filterPressureHighStart = 0;
  filterPressureLowStart = 0;
  productFlowrateLowStart = 0;
  productFlowrateHighStart = 0;
  brineFlowrateLowStart = 0;
  totalFlowrateLowStart = 0;
  flushFilterPressureLowStart = 0;
  flushFlowrateLowStart = 0;
  diverterValveOpenStart = 0;
  productSalinityHighStart = 0;
  motorTemperatureStart = 0;
}

bool Brineomatic::checkStopFlag(Result& result)
{
  if (stopFlag) {
    currentStatus = Status::STOPPING;
    result = Result::USER_STOP;
    return true;
  }

  return false;
}

bool Brineomatic::checkMembranePressureHigh()
{
  if (!hasMembranePressureSensor)
    return false;

  if (!enableMembranePressureHighCheck)
    return false;

  return checkTimedError(
    getMembranePressure() > membranePressureHighThreshold,
    membranePressureHighStart,
    membranePressureHighDelay,
    Result::ERR_MEMBRANE_PRESSURE_HIGH,
    runResult);
}

bool Brineomatic::checkMembranePressureLow()
{
  if (!hasMembranePressureSensor)
    return false;

  if (!enableMembranePressureLowCheck)
    return false;

  return checkTimedError(
    getMembranePressure() < membranePressureLowThreshold,
    membranePressureLowStart,
    membranePressureLowDelay,
    Result::ERR_MEMBRANE_PRESSURE_LOW,
    runResult);
}

bool Brineomatic::checkFilterPressureHigh()
{
  if (!hasFilterPressureSensor)
    return false;

  if (!enableFilterPressureHighCheck)
    return false;

  return checkTimedError(
    getFilterPressure() > filterPressureHighThreshold,
    filterPressureHighStart,
    filterPressureHighDelay,
    Result::ERR_FILTER_PRESSURE_HIGH,
    runResult);
}

bool Brineomatic::checkFilterPressureLow()
{
  if (!hasFilterPressureSensor)
    return false;

  if (!enableFilterPressureLowCheck)
    return false;

  return checkTimedError(
    getFilterPressure() < filterPressureLowThreshold,
    filterPressureLowStart,
    filterPressureLowDelay,
    Result::ERR_FILTER_PRESSURE_LOW,
    runResult);
}

bool Brineomatic::checkProductFlowrateLow()
{
  if (!hasProductFlowSensor)
    return false;

  if (!enableProductFlowrateLowCheck)
    return false;

  return checkTimedError(
    getProductFlowrate() < getProductFlowrateMinimum(),
    productFlowrateLowStart,
    productFlowrateLowDelay,
    Result::ERR_PRODUCT_FLOWRATE_LOW,
    runResult);
}

bool Brineomatic::checkProductFlowrateHigh()
{
  if (!hasProductFlowSensor)
    return false;

  if (!enableProductFlowrateHighCheck)
    return false;

  return checkTimedError(
    getProductFlowrate() > productFlowrateHighThreshold,
    productFlowrateHighStart,
    productFlowrateHighDelay,
    Result::ERR_PRODUCT_FLOWRATE_HIGH,
    runResult);
}

bool Brineomatic::checkPickleTotalFlowrateLow(Result& result)
{
  if (!hasBrineFlowSensor)
    return false;

  if (!enablePickleTotalFlowrateLowCheck)
    return false;

  return checkTimedError(
    getBrineFlowrate() < pickleTotalFlowrateLowThreshold,
    brineFlowrateLowStart,
    pickleTotalFlowrateLowDelay,
    Result::ERR_BRINE_FLOWRATE_LOW,
    result);
}

bool Brineomatic::checkFlushFilterPressureLow()
{
  if (!hasFilterPressureSensor)
    return false;

  if (!enableFlushFilterPressureLowCheck)
    return false;

  return checkTimedError(
    getFilterPressure() < flushFilterPressureLowThreshold,
    flushFilterPressureLowStart,
    flushFilterPressureLowDelay,
    Result::ERR_FLUSH_FILTER_PRESSURE_LOW,
    flushResult);
}

bool Brineomatic::checkFlushFlowrateLow()
{
  if (!hasBrineFlowSensor)
    return false;

  if (!enableFlushFlowrateLowCheck)
    return false;

  return checkTimedError(
    getBrineFlowrate() < flushFlowrateLowThreshold,
    flushFlowrateLowStart,
    flushFlowrateLowDelay,
    Result::ERR_FLUSH_FLOWRATE_LOW,
    flushResult);
}

bool Brineomatic::checkRunTotalFlowrateLow()
{
  if (!hasBrineFlowSensor)
    return false;

  if (!enableRunTotalFlowrateLowCheck)
    return false;

  return checkTimedError(
    getTotalFlowrate() < runTotalFlowrateLowThreshold,
    totalFlowrateLowStart,
    runTotalFlowrateLowDelay,
    Result::ERR_TOTAL_FLOWRATE_LOW,
    runResult);
}

bool Brineomatic::checkDiverterValveClosed()
{
  if (!hasProductFlowSensor)
    return false;

  if (!hasBrineFlowSensor)
    return false;

  if (!enableDiverterValveClosedCheck)
    return false;

  return checkTimedError(
    getTotalFlowrate() > getBrineFlowrate() + getProductFlowrate(),
    diverterValveOpenStart,
    diverterValveClosedDelay,
    Result::ERR_DIVERTER_VALVE_OPEN,
    runResult);
}

bool Brineomatic::checkProductSalinityHigh()
{
  if (!hasProductTDSSensor)
    return false;

  if (!enableProductSalinityHighCheck)
    return false;

  return checkTimedError(
    getProductSalinity() > getProductSalinityMaximum(),
    productSalinityHighStart,
    productSalinityHighDelay,
    Result::ERR_PRODUCT_SALINITY_HIGH,
    runResult);
}

bool Brineomatic::checkMotorTemperature(Result& result)
{
  if (!hasMotorTemperatureSensor)
    return false;

  if (!enableMotorTemperatureCheck)
    return false;

  return checkTimedError(
    getMotorTemperature() > getMotorTemperatureMaximum(),
    motorTemperatureStart,
    motorTemperatureHighDelay,
    Result::ERR_MOTOR_TEMPERATURE_HIGH,
    result);
}

bool Brineomatic::checkTimedError(bool condition,
  uint32_t& startTime,
  uint32_t timeout,
  Result errorResult,
  Result& result)
{
  if (condition) {
    if (startTime != 0) {
      if (millis() - startTime > timeout) {
        currentStatus = Status::STOPPING;
        result = errorResult;
        return true;
      }
    } else {
      startTime = millis();
    }
  } else {
    startTime = 0;
  }

  return false;
}

// return true on error
// return false on success
bool Brineomatic::waitForMembranePressure()
{
  // skip this if we dont have the sensor
  if (!hasMembranePressureSensor)
    return false;

  YBP.println("Wait for Membrane Pressure");

  uint32_t highPressurePumpStart = millis();
  while (getMembranePressure() < getMembranePressureMinimum()) {

    // let the spice flow
    if (checkRunTotalFlowrateLow())
      return true;

    // check this here in case our PID goes crazy
    if (checkMembranePressureHigh())
      return true;

    if (checkStopFlag(runResult))
      return true;

    if (millis() - highPressurePumpStart > membranePressureTimeout) {
      currentStatus = Status::STOPPING;
      runResult = Result::ERR_MEMBRANE_PRESSURE_TIMEOUT;
      return true;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }

  YBP.println("High Pressure Pump OK");

  return false;
}

bool Brineomatic::waitForProductFlowrate()
{
  if (!hasProductFlowSensor)
    return false;

  YBP.println("Wait for Product Flowrate");

  int flowReady = 0;
  uint32_t flowCheckStart = millis();

  while (flowReady < 10) {
    if (checkMembranePressureHigh())
      return true;
    if (checkStopFlag(runResult))
      return true;

    if (getProductFlowrate() > getProductFlowrateMinimum() && getProductFlowrate() < productFlowrateHighThreshold)
      flowReady++;
    else
      flowReady = 0;

    if (millis() - flowCheckStart > productFlowrateTimeout) {
      currentStatus = Status::STOPPING;
      runResult = Result::ERR_PRODUCT_FLOWRATE_TIMEOUT;
      return true;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }

  YBP.println("Flowrate OK");

  return false;
}

bool Brineomatic::waitForProductSalinity()
{
  if (!hasProductTDSSensor)
    return false;

  YBP.println("Wait for Product Salinity");

  int salinityReady = 0;
  uint32_t salinityCheckStart = millis();
  while (salinityReady < 10) {
    if (checkMembranePressureHigh())
      return true;
    if (checkStopFlag(runResult))
      return true;

    if (getProductSalinity() < getProductSalinityMaximum())
      salinityReady++;
    else
      salinityReady = 0;

    if (millis() - salinityCheckStart > productSalinityTimeout) {
      currentStatus = Status::STOPPING;
      runResult = Result::ERR_PRODUCT_SALINITY_TIMEOUT;
      return true;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }

  YBP.println("Salinity OK");

  return false;
}

bool Brineomatic::waitForFlushValveOff()
{
  if (!enableFlushValveOffCheck)
    return false;

  if (!hasFilterPressureSensor && !hasBrineFlowSensor)
    return false;

  YBP.println("Wait for Flush Valve Off");

  uint32_t start = millis();

  bool done = false;
  while (!done) {
    if (millis() - start > flushValveOffDelay) {
      currentStatus = Status::IDLE;
      flushResult = Result::ERR_FLUSH_VALVE_ON;
      return true;
    }

    done = true;

    if (hasFilterPressureSensor)
      if (getFilterPressure() > 2)
        done = false;

    if (hasBrineFlowSensor)
      if (getBrineFlowrate() > 0)
        done = false;

    vTaskDelay(pdMS_TO_TICKS(100));
  }

  return false;
}

bool Brineomatic::checkTankLevel()
{
  return currentTankLevel >= tankLevelFull;
}

void Brineomatic::generateUpdateJSON(JsonVariant output)
{
  output["brineomatic"] = true;
  output["status"] = getStatus();
  output["run_result"] = resultToString(getRunResult());
  output["flush_result"] = resultToString(getFlushResult());
  output["pickle_result"] = resultToString(getPickleResult());
  output["depickle_result"] = resultToString(getDepickleResult());
  output["motor_temperature"] = getMotorTemperature();
  output["water_temperature"] = getWaterTemperature();
  output["product_flowrate"] = getProductFlowrate();
  output["brine_flowrate"] = getBrineFlowrate();
  output["total_flowrate"] = getTotalFlowrate();
  output["volume"] = getVolume();
  output["flush_volume"] = getFlushVolume();
  output["product_salinity"] = getProductSalinity();
  output["brine_salinity"] = getBrineSalinity();
  output["filter_pressure"] = getFilterPressure();
  output["membrane_pressure"] = getMembranePressure();
  output["tank_level"] = getTankLevel();

  if (hasBoostPump())
    output["boost_pump_on"] = isBoostPumpOn();
  if (hasHighPressurePump())
    output["high_pressure_pump_on"] = isHighPressurePumpOn();
  if (hasDiverterValve())
    output["diverter_valve_open"] = isDiverterValveOpen();
  if (hasFlushValve())
    output["flush_valve_open"] = isFlushValveOpen();
  if (hasCoolingFan())
    output["cooling_fan_on"] = isCoolingFanOn();

  output["next_flush_countdown"] = getNextFlushCountdown();
  output["runtime_elapsed"] = getRuntimeElapsed();
  output["finish_countdown"] = getFinishCountdown();

  if (!strcmp(getStatus(), "FLUSHING")) {
    output["flush_elapsed"] = getFlushElapsed();
    output["flush_countdown"] = getFlushCountdown();
  }

  if (!strcmp(getStatus(), "PICKLING")) {
    output["pickle_elapsed"] = getPickleElapsed();
    output["pickle_countdown"] = getPickleCountdown();
  }

  if (!strcmp(getStatus(), "DEPICKLING")) {
    output["depickle_elapsed"] = getDepickleElapsed();
    output["depickle_countdown"] = getDepickleCountdown();
  }
}

void Brineomatic::generateConfigJSON(JsonVariant output)
{
  JsonObject bom = output["brineomatic"].to<JsonObject>();
  bom["has_boost_pump"] = this->hasBoostPump();
  bom["has_high_pressure_pump"] = this->hasHighPressurePump();
  bom["has_diverter_valve"] = this->hasDiverterValve();
  bom["has_flush_valve"] = this->hasFlushValve();
  bom["has_cooling_fan"] = this->hasCoolingFan();

  bom["autoflush_mode"] = this->autoflushMode;
  bom["autoflush_salinity"] = this->autoflushSalinity;
  bom["autoflush_duration"] = this->autoflushDuration;
  bom["autoflush_volume"] = this->autoflushVolume;
  bom["autoflush_interval"] = this->autoflushInterval;
  bom["autoflush_use_high_pressure_motor"] = this->autoflushUseHighPressureMotor;

  bom["flush_timeout"] = this->flushTimeout;
  bom["membrane_pressure_timeout"] = this->membranePressureTimeout;
  bom["product_flowrate_timeout"] = this->productFlowrateTimeout;
  bom["product_salinity_timeout"] = this->productSalinityTimeout;
  bom["production_runtime_timeout"] = this->productionRuntimeTimeout;

  bom["tank_capacity"] = this->tankCapacity;
  bom["temperature_units"] = this->temperatureUnits;
  bom["pressure_units"] = this->pressureUnits;
  bom["volume_units"] = this->volumeUnits;
  bom["flowrate_units"] = this->flowrateUnits;
  bom["success_melody"] = this->successMelody;
  bom["error_melody"] = this->errorMelody;

  bom["boost_pump_control"] = this->boostPumpControl;
  bom["boost_pump_relay_id"] = this->boostPumpRelayId;

  bom["high_pressure_pump_control"] = this->highPressurePumpControl;
  bom["high_pressure_relay_id"] = this->highPressureRelayId;
  bom["high_pressure_modbus_device"] = this->highPressurePumpModbusDevice;

  bom["high_pressure_valve_control"] = this->highPressureValveControl;
  bom["membrane_pressure_target"] = this->membranePressureTarget;
  bom["high_pressure_valve_stepper_id"] = this->highPressureValveStepperId;
  bom["high_pressure_stepper_step_angle"] = this->highPressureValveStepperStepAngle;
  bom["high_pressure_stepper_gear_ratio"] = this->highPressureValveStepperGearRatio;
  bom["high_pressure_stepper_close_angle"] = this->highPressureValveStepperCloseAngle;
  bom["high_pressure_stepper_close_speed"] = this->highPressureValveStepperCloseSpeed;
  bom["high_pressure_stepper_open_angle"] = this->highPressureValveStepperOpenAngle;
  bom["high_pressure_stepper_open_speed"] = this->highPressureValveStepperOpenSpeed;

  bom["diverter_valve_control"] = this->diverterValveControl;
  bom["diverter_valve_servo_id"] = this->diverterValveServoId;
  bom["diverter_valve_open_angle"] = this->diverterValveOpenAngle;
  bom["diverter_valve_close_angle"] = this->diverterValveCloseAngle;

  bom["flush_valve_control"] = this->flushValveControl;
  bom["flush_valve_relay_id"] = this->flushValveRelayId;

  bom["cooling_fan_control"] = this->coolingFanControl;
  bom["cooling_fan_relay_id"] = this->coolingFanRelayId;
  bom["cooling_fan_on_temperature"] = this->coolingFanOnTemperature;
  bom["cooling_fan_off_temperature"] = this->coolingFanOffTemperature;

  bom["has_membrane_pressure_sensor"] = this->hasMembranePressureSensor;
  bom["membrane_pressure_sensor_min"] = this->membranePressureSensorMin;
  bom["membrane_pressure_sensor_max"] = this->membranePressureSensorMax;

  bom["has_filter_pressure_sensor"] = this->hasFilterPressureSensor;
  bom["filter_pressure_sensor_min"] = this->filterPressureSensorMin;
  bom["filter_pressure_sensor_max"] = this->filterPressureSensorMax;

  bom["has_product_tds_sensor"] = this->hasProductTDSSensor;
  bom["has_brine_tds_sensor"] = this->hasBrineTDSSensor;

  bom["has_product_flow_sensor"] = this->hasProductFlowSensor;
  bom["product_flowmeter_ppl"] = this->productFlowmeterPPL;

  bom["has_brine_flow_sensor"] = this->hasBrineFlowSensor;
  bom["brine_flowmeter_ppl"] = this->brineFlowmeterPPL;

  bom["has_motor_temperature_sensor"] = this->hasMotorTemperatureSensor;

  bom["enable_membrane_pressure_high_check"] = this->enableMembranePressureHighCheck;
  bom["membrane_pressure_high_threshold"] = this->membranePressureHighThreshold;
  bom["membrane_pressure_high_delay"] = this->membranePressureHighDelay;

  bom["enable_membrane_pressure_low_check"] = this->enableMembranePressureLowCheck;
  bom["membrane_pressure_low_threshold"] = this->membranePressureLowThreshold;
  bom["membrane_pressure_low_delay"] = this->membranePressureLowDelay;

  bom["enable_filter_pressure_high_check"] = this->enableFilterPressureHighCheck;
  bom["filter_pressure_high_threshold"] = this->filterPressureHighThreshold;
  bom["filter_pressure_high_delay"] = this->filterPressureHighDelay;

  bom["enable_filter_pressure_low_check"] = this->enableFilterPressureLowCheck;
  bom["filter_pressure_low_threshold"] = this->filterPressureLowThreshold;
  bom["filter_pressure_low_delay"] = this->filterPressureLowDelay;

  bom["enable_product_flowrate_high_check"] = this->enableProductFlowrateHighCheck;
  bom["product_flowrate_high_threshold"] = this->productFlowrateHighThreshold;
  bom["product_flowrate_high_delay"] = this->productFlowrateHighDelay;

  bom["enable_product_flowrate_low_check"] = this->enableProductFlowrateLowCheck;
  bom["product_flowrate_low_threshold"] = this->productFlowrateLowThreshold;
  bom["product_flowrate_low_delay"] = this->productFlowrateLowDelay;

  bom["enable_run_total_flowrate_low_check"] = this->enableRunTotalFlowrateLowCheck;
  bom["run_total_flowrate_low_threshold"] = this->runTotalFlowrateLowThreshold;
  bom["run_total_flowrate_low_delay"] = this->runTotalFlowrateLowDelay;

  bom["enable_pickle_total_flowrate_low_check"] = this->enablePickleTotalFlowrateLowCheck;
  bom["pickle_total_flowrate_low_threshold"] = this->pickleTotalFlowrateLowThreshold;
  bom["pickle_total_flowrate_low_delay"] = this->pickleTotalFlowrateLowDelay;

  bom["enable_diverter_valve_closed_check"] = this->enableDiverterValveClosedCheck;
  bom["diverter_valve_closed_delay"] = this->diverterValveClosedDelay;

  bom["enable_product_salinity_high_check"] = this->enableProductSalinityHighCheck;
  bom["product_salinity_high_threshold"] = this->productSalinityHighThreshold;
  bom["product_salinity_high_delay"] = this->productSalinityHighDelay;

  bom["enable_motor_temperature_check"] = this->enableMotorTemperatureCheck;
  bom["motor_temperature_high_threshold"] = this->motorTemperatureHighThreshold;
  bom["motor_temperature_high_delay"] = this->motorTemperatureHighDelay;

  bom["enable_flush_flowrate_low_check"] = this->enableFlushFlowrateLowCheck;
  bom["flush_flowrate_low_threshold"] = this->flushFlowrateLowThreshold;
  bom["flush_flowrate_low_delay"] = this->flushFlowrateLowDelay;

  bom["enable_flush_filter_pressure_low_check"] = this->enableFlushFilterPressureLowCheck;
  bom["flush_filter_pressure_low_threshold"] = this->flushFilterPressureLowThreshold;
  bom["flush_filter_pressure_low_delay"] = this->flushFilterPressureLowDelay;

  bom["enable_flush_valve_off_check"] = this->enableFlushValveOffCheck;
  bom["enable_flush_valve_off_threshold"] = this->flushValveOffThreshold;
  bom["enable_flush_valve_off_delay"] = this->flushValveOffDelay;
}

bool Brineomatic::validateConfigJSON(JsonVariantConst config, char* error, size_t err_size)
{
  if (!validateGeneralConfigJSON(config, error, err_size))
    return false;
  if (!validateHardwareConfigJSON(config, error, err_size))
    return false;
  if (!validateSafeguardsConfigJSON(config, error, err_size))
    return false;

  return true;
}

bool Brineomatic::validateGeneralConfigJSON(JsonVariantConst config, char* error, size_t err_size)
{
  // autoflush_mode (required, string inclusion)
  if (!checkPresence(config, "autoflush_mode", error, err_size))
    return false;
  if (!checkInclusion(config, "autoflush_mode", Brineomatic::AUTOFLUSH_MODES, error, err_size))
    return false;

  // autoflush_salinity (integer >= 0)
  if (config["autoflush_salinity"]) {
    if (!checkIsInteger(config, "autoflush_salinity", error, err_size))
      return false;
    if (!checkNumGT(config, "autoflush_salinity", 0, error, err_size))
      return false;
  }

  // autoflush_duration (number >= 0)
  if (config["autoflush_duration"]) {
    if (!checkIsNumber(config, "autoflush_duration", error, err_size))
      return false;
    if (!checkNumGT(config, "autoflush_duration", 0, error, err_size))
      return false;
  }

  if (config["autoflush_volume"]) {
    if (!checkIsNumber(config, "autoflush_volume", error, err_size))
      return false;
    if (!checkNumGT(config, "autoflush_volume", 0, error, err_size))
      return false;
  }

  if (config["autoflush_interval"]) {
    if (!checkIsNumber(config, "autoflush_interval", error, err_size))
      return false;
    if (!checkNumGT(config, "autoflush_interval", 0, error, err_size))
      return false;
  }

  if (config["autoflush_use_high_pressure_motor"])
    if (!checkIsBool(config, "autoflush_use_high_pressure_motor", error, err_size))
      return false;

  // tank_capacity (required, number > 0)
  if (!checkPresence(config, "tank_capacity", error, err_size))
    return false;
  if (!checkIsNumber(config, "tank_capacity", error, err_size))
    return false;
  if (!checkNumGT(config, "tank_capacity", 0, error, err_size))
    return false;

  // enum-like fields
  if (!checkPresence(config, "temperature_units", error, err_size))
    return false;
  if (!checkInclusion(config, "temperature_units", Brineomatic::TEMPERATURE_UNITS, error, err_size))
    return false;

  if (!checkPresence(config, "pressure_units", error, err_size))
    return false;
  if (!checkInclusion(config, "pressure_units", Brineomatic::PRESSURE_UNITS, error, err_size))
    return false;

  if (!checkPresence(config, "volume_units", error, err_size))
    return false;
  if (!checkInclusion(config, "volume_units", Brineomatic::VOLUME_UNITS, error, err_size))
    return false;

  if (!checkPresence(config, "flowrate_units", error, err_size))
    return false;
  if (!checkInclusion(config, "flowrate_units", Brineomatic::FLOWRATE_UNITS, error, err_size))
    return false;

  // success_melody + error_melody (presence only)
  if (!checkPresence(config, "success_melody", error, err_size))
    return false;
  if (!checkPresence(config, "error_melody", error, err_size))
    return false;

  return true;
}

bool Brineomatic::validateHardwareConfigJSON(JsonVariantConst config, char* error, size_t err_size)
{
  // ---------------------------------------------------------
  // Control mode enums
  // ---------------------------------------------------------

  String control;

  if (!checkPresence(config, "boost_pump_control", error, err_size))
    return false;
  if (!checkInclusion(config, "boost_pump_control", BOOST_PUMP_CONTROLS, error, err_size))
    return false;

  if (config["boost_pump_relay_id"]) {
    if (!checkIsInteger(config, "boost_pump_relay_id", error, err_size))
      return false;
    if (!checkIntGE(config, "boost_pump_relay_id", 0, error, err_size))
      return false;
  }

  control = config["boost_pump_control"].as<String>();
  if (control.equals("RELAY")) {
    auto* ch = getChannelById(config["boost_pump_relay_id"], relay_channels);
    if (!ch)
      return fail("Boost Pump relay id invalid.", error, err_size);
  }

  if (!checkPresence(config, "high_pressure_pump_control", error, err_size))
    return false;
  if (!checkInclusion(config, "high_pressure_pump_control", HIGH_PRESSURE_PUMP_CONTROLS, error, err_size))
    return false;

  if (config["high_pressure_relay_id"]) {
    if (!checkIsInteger(config, "high_pressure_relay_id", error, err_size))
      return false;
    if (!checkIntGE(config, "high_pressure_relay_id", 0, error, err_size))
      return false;
  }

  control = config["high_pressure_pump_control"].as<String>();
  if (control.equals("RELAY")) {
    auto* ch = getChannelById(config["high_pressure_relay_id"], relay_channels);
    if (!ch)
      return fail("High Pressure Pump relay id invalid.", error, err_size);
  }

  if (config["high_pressure_modbus_device"]) {
    if (!checkInclusion(config, "high_pressure_modbus_device", HIGH_PRESSURE_PUMP_MODBUS_DEVICES, error, err_size))
      return false;
  }

  if (!checkPresence(config, "high_pressure_valve_control", error, err_size))
    return false;
  if (!checkInclusion(config, "high_pressure_valve_control", HIGH_PRESSURE_VALVE_CONTROLS, error, err_size))
    return false;

  if (config["membrane_pressure_target"]) {
    if (!checkIsNumber(config, "membrane_pressure_target", error, err_size))
      return false;
    if (!checkNumGT(config, "membrane_pressure_target", 0.0f, error, err_size))
      return false;
  }

  if (config["high_pressure_valve_stepper_id"]) {
    if (!checkIsInteger(config, "high_pressure_valve_stepper_id", error, err_size))
      return false;
    if (!checkIntGE(config, "high_pressure_valve_stepper_id", 0, error, err_size))
      return false;
  }

  control = config["high_pressure_valve_control"].as<String>();
  if (control.equals("STEPPER")) {
    auto* ch = getChannelById(config["high_pressure_valve_stepper_id"], stepper_channels);
    if (!ch)
      return fail("High Pressure Valve stepper id invalid.", error, err_size);
  }

  // Stepper angles and speeds
  auto checkAngle = [&](const char* key, float minv, float maxv) -> bool {
    if (!checkIsNumber(config, key, error, err_size))
      return false;
    float v = config[key].as<float>();
    if (v < minv) {
      snprintf(error, err_size, "Field '%s' must be >= %.2f", key, minv);
      return false;
    }
    if (v > maxv) {
      snprintf(error, err_size, "Field '%s' must be <= %.2f", key, maxv);
      return false;
    }
    return true;
  };

  if (config["high_pressure_stepper_step_angle"]) {
    if (!checkAngle("high_pressure_stepper_step_angle", 0.0001f, 90.0f))
      return false;
  }

  if (config["high_pressure_stepper_gear_ratio"]) {
    if (!checkNumGT(config, "high_pressure_stepper_gear_ratio", 0.0f, error, err_size))
      return false;
  }

  if (config["high_pressure_stepper_close_angle"]) {
    if (!checkAngle("high_pressure_stepper_close_angle", 0.0f, 5000.0f))
      return false;
  }

  if (config["high_pressure_stepper_close_speed"]) {
    if (!checkAngle("high_pressure_stepper_close_speed", 0.0001f, 200.0f))
      return false;
  }

  if (config["high_pressure_stepper_open_angle"]) {
    if (!checkAngle("high_pressure_stepper_open_angle", 0.0f, 5000.0f))
      return false;
  }

  if (config["high_pressure_stepper_open_speed"]) {
    if (!checkAngle("high_pressure_stepper_open_speed", 0.0001f, 200.0f))
      return false;
  }

  // ---------------------------------------------------------
  // Diverter valve
  // ---------------------------------------------------------

  if (!checkPresence(config, "diverter_valve_control", error, err_size))
    return false;
  if (!checkInclusion(config, "diverter_valve_control", DIVERTER_VALVE_CONTROLS, error, err_size))
    return false;

  if (config["diverter_valve_servo_id"]) {
    if (!checkIsInteger(config, "diverter_valve_servo_id", error, err_size))
      return false;
    if (!checkIntGE(config, "diverter_valve_servo_id", 0, error, err_size))
      return false;
  }

  control = config["diverter_valve_control"].as<String>();
  if (control.equals("SERVO")) {
    auto* ch = getChannelById(config["diverter_valve_servo_id"], relay_channels);
    if (!ch)
      return fail("Diverter Valve servo id invalid.", error, err_size);
  }

  if (config["diverter_valve_open_angle"]) {
    if (!checkAngle("diverter_valve_open_angle", 0.0f, 180.0f))
      return false;
  }

  if (config["diverter_valve_close_angle"]) {
    if (!checkAngle("diverter_valve_close_angle", 0.0f, 180.0f))
      return false;
  }

  // ---------------------------------------------------------
  // Flush valve
  // ---------------------------------------------------------

  if (!checkPresence(config, "flush_valve_control", error, err_size))
    return false;
  if (!checkInclusion(config, "flush_valve_control", FLUSH_VALVE_CONTROLS, error, err_size))
    return false;

  if (config["flush_valve_relay_id"]) {
    if (!checkIsInteger(config, "flush_valve_relay_id", error, err_size))
      return false;
    if (!checkIntGE(config, "flush_valve_relay_id", 0, error, err_size))
      return false;
  }

  control = config["flush_valve_control"].as<String>();
  if (control.equals("RELAY")) {
    auto* ch = getChannelById(config["flush_valve_relay_id"], relay_channels);
    if (!ch)
      return fail("Flush Valve relay id invalid.", error, err_size);
  }

  // ---------------------------------------------------------
  // Cooling fan
  // ---------------------------------------------------------

  if (!checkPresence(config, "cooling_fan_control", error, err_size))
    return false;
  if (!checkInclusion(config, "cooling_fan_control", COOLING_FAN_CONTROLS, error, err_size))
    return false;

  if (config["cooling_fan_relay_id"]) {
    if (!checkIsInteger(config, "cooling_fan_relay_id", error, err_size))
      return false;
    if (!checkIntGE(config, "cooling_fan_relay_id", 0, error, err_size))
      return false;
  }

  control = config["cooling_fan_control"].as<String>();
  if (control.equals("RELAY")) {
    auto* ch = getChannelById(config["cooling_fan_relay_id"], relay_channels);
    if (!ch)
      return fail("Cooling Fan relay id invalid.", error, err_size);
  }

  if (config["cooling_fan_on_temperature"]) {
    if (!checkAngle("cooling_fan_on_temperature", 0.0f, 100.0f))
      return false;
  }

  if (config["cooling_fan_off_temperature"]) {
    if (!checkAngle("cooling_fan_off_temperature", 0.0f, 100.0f))
      return false;
  }

  // ---------------------------------------------------------
  // Sensor booleans
  // ---------------------------------------------------------

  if (config["has_membrane_pressure_sensor"])
    if (!checkIsBool(config, "has_membrane_pressure_sensor", error, err_size))
      return false;

  if (config["membrane_pressure_sensor_min"]) {
    if (!checkNumGE(config, "membrane_pressure_sensor_min", 0.0f, error, err_size))
      return false;
  }

  if (config["membrane_pressure_sensor_max"]) {
    if (!checkNumGT(config, "membrane_pressure_sensor_max", 0.0f, error, err_size))
      return false;
  }

  if (config["has_filter_pressure_sensor"])
    if (!checkIsBool(config, "has_filter_pressure_sensor", error, err_size))
      return false;

  if (config["filter_pressure_sensor_min"]) {
    if (!checkNumGE(config, "filter_pressure_sensor_min", 0.0f, error, err_size))
      return false;
  }

  if (config["filter_pressure_sensor_max"]) {
    if (!checkNumGT(config, "filter_pressure_sensor_max", 0.0f, error, err_size))
      return false;
  }

  if (config["has_product_tds_sensor"])
    if (!checkIsBool(config, "has_product_tds_sensor", error, err_size))
      return false;

  if (config["has_brine_tds_sensor"])
    if (!checkIsBool(config, "has_brine_tds_sensor", error, err_size))
      return false;

  if (config["has_product_flow_sensor"])
    if (!checkIsBool(config, "has_product_flow_sensor", error, err_size))
      return false;

  if (config["product_flowmeter_ppl"]) {
    if (!checkNumGT(config, "product_flowmeter_ppl", 0.0f, error, err_size))
      return false;
  }

  if (config["has_brine_flow_sensor"])
    if (!checkIsBool(config, "has_brine_flow_sensor", error, err_size))
      return false;

  if (config["brine_flowmeter_ppl"]) {
    if (!checkNumGT(config, "brine_flowmeter_ppl", 0.0f, error, err_size))
      return false;
  }

  if (config["has_motor_temperature_sensor"])
    if (!checkIsBool(config, "has_motor_temperature_sensor", error, err_size))
      return false;

  return true;
}

bool Brineomatic::validateSafeguardsConfigJSON(JsonVariantConst config, char* error, size_t err_size)
{
  // ---------------------------------------------------------
  // Basic timeout fields (number > 0)
  // ---------------------------------------------------------

  if (config["flush_timeout"]) {
    if (!checkIsNumber(config, "flush_timeout", error, err_size))
      return false;
    if (!checkNumGT(config, "flush_timeout", 0.0f, error, err_size))
      return false;
  }

  if (config["membrane_pressure_timeout"]) {
    if (!checkIsNumber(config, "membrane_pressure_timeout", error, err_size))
      return false;
    if (!checkNumGT(config, "membrane_pressure_timeout", 0.0f, error, err_size))
      return false;
  }

  if (config["product_flowrate_timeout"]) {
    if (!checkIsNumber(config, "product_flowrate_timeout", error, err_size))
      return false;
    if (!checkNumGT(config, "product_flowrate_timeout", 0.0f, error, err_size))
      return false;
  }

  if (config["product_salinity_timeout"]) {
    if (!checkIsNumber(config, "product_salinity_timeout", error, err_size))
      return false;
    if (!checkNumGT(config, "product_salinity_timeout", 0.0f, error, err_size))
      return false;
  }

  if (config["production_runtime_timeout"]) {
    if (!checkIsNumber(config, "production_runtime_timeout", error, err_size))
      return false;
    if (!checkNumGT(config, "production_runtime_timeout", 0.0f, error, err_size))
      return false;
  }

  // ---------------------------------------------------------
  // Repeated patterns: boolean enable + threshold/delay
  // ---------------------------------------------------------

  // enable_membrane_pressure_high_check
  if (config["enable_membrane_pressure_high_check"]) {
    if (!checkIsBool(config, "enable_membrane_pressure_high_check", error, err_size))
      return false;
  }
  if (config["membrane_pressure_high_threshold"]) {
    if (!checkIsNumber(config, "membrane_pressure_high_threshold", error, err_size))
      return false;
    if (!checkNumGT(config, "membrane_pressure_high_threshold", 0.0f, error, err_size))
      return false;
  }
  if (config["membrane_pressure_high_delay"]) {
    if (!checkIsNumber(config, "membrane_pressure_high_delay", error, err_size))
      return false;
    if (!checkNumGE(config, "membrane_pressure_high_delay", 0.0f, error, err_size))
      return false;
  }

  // enable_membrane_pressure_low_check
  if (config["enable_membrane_pressure_low_check"]) {
    if (!checkIsBool(config, "enable_membrane_pressure_low_check", error, err_size))
      return false;
  }
  if (config["membrane_pressure_low_threshold"]) {
    if (!checkIsNumber(config, "membrane_pressure_low_threshold", error, err_size))
      return false;
    if (!checkNumGT(config, "membrane_pressure_low_threshold", 0.0f, error, err_size))
      return false;
  }
  if (config["membrane_pressure_low_delay"]) {
    if (!checkIsNumber(config, "membrane_pressure_low_delay", error, err_size))
      return false;
    if (!checkNumGE(config, "membrane_pressure_low_delay", 0.0f, error, err_size))
      return false;
  }

  // enable_filter_pressure_high_check
  if (config["enable_filter_pressure_high_check"]) {
    if (!checkIsBool(config, "enable_filter_pressure_high_check", error, err_size))
      return false;
  }
  if (config["filter_pressure_high_threshold"]) {
    if (!checkIsNumber(config, "filter_pressure_high_threshold", error, err_size))
      return false;
    if (!checkNumGT(config, "filter_pressure_high_threshold", 0.0f, error, err_size))
      return false;
  }
  if (config["filter_pressure_high_delay"]) {
    if (!checkIsNumber(config, "filter_pressure_high_delay", error, err_size))
      return false;
    if (!checkNumGE(config, "filter_pressure_high_delay", 0.0f, error, err_size))
      return false;
  }

  // enable_filter_pressure_low_check
  if (config["enable_filter_pressure_low_check"]) {
    if (!checkIsBool(config, "enable_filter_pressure_low_check", error, err_size))
      return false;
  }
  if (config["filter_pressure_low_threshold"]) {
    if (!checkIsNumber(config, "filter_pressure_low_threshold", error, err_size))
      return false;
    if (!checkNumGT(config, "filter_pressure_low_threshold", 0.0f, error, err_size))
      return false;
  }
  if (config["filter_pressure_low_delay"]) {
    if (!checkIsNumber(config, "filter_pressure_low_delay", error, err_size))
      return false;
    if (!checkNumGE(config, "filter_pressure_low_delay", 0.0f, error, err_size))
      return false;
  }

  // enable_product_flowrate_high_check
  if (config["enable_product_flowrate_high_check"]) {
    if (!checkIsBool(config, "enable_product_flowrate_high_check", error, err_size))
      return false;
  }
  if (config["product_flowrate_high_threshold"]) {
    if (!checkIsNumber(config, "product_flowrate_high_threshold", error, err_size))
      return false;
    if (!checkNumGT(config, "product_flowrate_high_threshold", 0.0f, error, err_size))
      return false;
  }
  if (config["product_flowrate_high_delay"]) {
    if (!checkIsNumber(config, "product_flowrate_high_delay", error, err_size))
      return false;
    if (!checkNumGE(config, "product_flowrate_high_delay", 0.0f, error, err_size))
      return false;
  }

  // enable_product_flowrate_low_check
  if (config["enable_product_flowrate_low_check"]) {
    if (!checkIsBool(config, "enable_product_flowrate_low_check", error, err_size))
      return false;
  }
  if (config["product_flowrate_low_threshold"]) {
    if (!checkIsNumber(config, "product_flowrate_low_threshold", error, err_size))
      return false;
    if (!checkNumGT(config, "product_flowrate_low_threshold", 0.0f, error, err_size))
      return false;
  }
  if (config["product_flowrate_low_delay"]) {
    if (!checkIsNumber(config, "product_flowrate_low_delay", error, err_size))
      return false;
    if (!checkNumGE(config, "product_flowrate_low_delay", 0.0f, error, err_size))
      return false;
  }

  // enable_run_total_flowrate_low_check
  if (config["enable_run_total_flowrate_low_check"]) {
    if (!checkIsBool(config, "enable_run_total_flowrate_low_check", error, err_size))
      return false;
  }
  if (config["run_total_flowrate_low_threshold"]) {
    if (!checkIsNumber(config, "run_total_flowrate_low_threshold", error, err_size))
      return false;
    if (!checkNumGT(config, "run_total_flowrate_low_threshold", 0.0f, error, err_size))
      return false;
  }
  if (config["run_total_flowrate_low_delay"]) {
    if (!checkIsNumber(config, "run_total_flowrate_low_delay", error, err_size))
      return false;
    if (!checkNumGE(config, "run_total_flowrate_low_delay", 0.0f, error, err_size))
      return false;
  }

  // enable_pickle_total_flowrate_low_check
  if (config["enable_pickle_total_flowrate_low_check"]) {
    if (!checkIsBool(config, "enable_pickle_total_flowrate_low_check", error, err_size))
      return false;
  }
  if (config["pickle_total_flowrate_low_threshold"]) {
    if (!checkIsNumber(config, "pickle_total_flowrate_low_threshold", error, err_size))
      return false;
    if (!checkNumGT(config, "pickle_total_flowrate_low_threshold", 0.0f, error, err_size))
      return false;
  }
  if (config["pickle_total_flowrate_low_delay"]) {
    if (!checkIsNumber(config, "pickle_total_flowrate_low_delay", error, err_size))
      return false;
    if (!checkNumGE(config, "pickle_total_flowrate_low_delay", 0.0f, error, err_size))
      return false;
  }

  // enable_diverter_valve_closed_check
  if (config["enable_diverter_valve_closed_check"]) {
    if (!checkIsBool(config, "enable_diverter_valve_closed_check", error, err_size))
      return false;
  }
  if (config["diverter_valve_closed_delay"]) {
    if (!checkIsNumber(config, "diverter_valve_closed_delay", error, err_size))
      return false;
    if (!checkNumGE(config, "diverter_valve_closed_delay", 0.0f, error, err_size))
      return false;
  }

  // enable_product_salinity_high_check
  if (config["enable_product_salinity_high_check"]) {
    if (!checkIsBool(config, "enable_product_salinity_high_check", error, err_size))
      return false;
  }
  if (config["product_salinity_high_threshold"]) {
    if (!checkIsNumber(config, "product_salinity_high_threshold", error, err_size))
      return false;
    if (!checkNumGT(config, "product_salinity_high_threshold", 0.0f, error, err_size))
      return false;
  }
  if (config["product_salinity_high_delay"]) {
    if (!checkIsNumber(config, "product_salinity_high_delay", error, err_size))
      return false;
    if (!checkNumGE(config, "product_salinity_high_delay", 0.0f, error, err_size))
      return false;
  }

  // enable_motor_temperature_check
  if (config["enable_motor_temperature_check"]) {
    if (!checkIsBool(config, "enable_motor_temperature_check", error, err_size))
      return false;
  }
  if (config["motor_temperature_high_threshold"]) {
    if (!checkIsNumber(config, "motor_temperature_high_threshold", error, err_size))
      return false;
    if (!checkNumGT(config, "motor_temperature_high_threshold", 0.0f, error, err_size))
      return false;
  }
  if (config["motor_temperature_high_delay"]) {
    if (!checkIsNumber(config, "motor_temperature_high_delay", error, err_size))
      return false;
    if (!checkNumGE(config, "motor_temperature_high_delay", 0.0f, error, err_size))
      return false;
  }

  // enable_flush_flowrate_low_check
  if (config["enable_flush_flowrate_low_check"]) {
    if (!checkIsBool(config, "enable_flush_flowrate_low_check", error, err_size))
      return false;
  }
  if (config["flush_flowrate_low_threshold"]) {
    if (!checkIsNumber(config, "flush_flowrate_low_threshold", error, err_size))
      return false;
    if (!checkNumGT(config, "flush_flowrate_low_threshold", 0.0f, error, err_size))
      return false;
  }
  if (config["flush_flowrate_low_delay"]) {
    if (!checkIsNumber(config, "flush_flowrate_low_delay", error, err_size))
      return false;
    if (!checkNumGE(config, "flush_flowrate_low_delay", 0.0f, error, err_size))
      return false;
  }

  // enable_flush_filter_pressure_low_check
  if (config["enable_flush_filter_pressure_low_check"]) {
    if (!checkIsBool(config, "enable_flush_filter_pressure_low_check", error, err_size))
      return false;
  }
  if (config["flush_filter_pressure_low_threshold"]) {
    if (!checkIsNumber(config, "flush_filter_pressure_low_threshold", error, err_size))
      return false;
    if (!checkNumGT(config, "flush_filter_pressure_low_threshold", 0.0f, error, err_size))
      return false;
  }
  if (config["flush_filter_pressure_low_delay"]) {
    if (!checkIsNumber(config, "flush_filter_pressure_low_delay", error, err_size))
      return false;
    if (!checkNumGE(config, "flush_filter_pressure_low_delay", 0.0f, error, err_size))
      return false;
  }

  // enable_flush_valve_off_check
  if (config["enable_flush_valve_off_check"]) {
    if (!checkIsBool(config, "enable_flush_valve_off_check", error, err_size))
      return false;
  }
  if (config["enable_flush_valve_off_threshold"]) {
    if (!checkIsNumber(config, "enable_flush_valve_off_threshold", error, err_size))
      return false;
    if (!checkNumGT(config, "enable_flush_valve_off_threshold", 0.0f, error, err_size))
      return false;
  }
  if (config["enable_flush_valve_off_delay"]) {
    if (!checkIsNumber(config, "enable_flush_valve_off_delay", error, err_size))
      return false;
    if (!checkNumGE(config, "enable_flush_valve_off_delay", 0.0f, error, err_size))
      return false;
  }

  return true;
}

void Brineomatic::loadConfigJSON(JsonVariantConst config)
{
  this->loadGeneralConfigJSON(config);
  this->loadHardwareConfigJSON(config);
  this->loadSafeguardsConfigJSON(config);
}

void Brineomatic::loadGeneralConfigJSON(JsonVariantConst config)
{
  this->autoflushMode = config["autoflush_mode"] | YB_AUTOFLUSH_MODE;
  this->autoflushSalinity = config["autoflush_salinity"] | YB_AUTOFLUSH_SALINITY;
  this->autoflushDuration = config["autoflush_duration"] | YB_AUTOFLUSH_DURATION;
  this->autoflushVolume = config["autoflush_volume"] | YB_AUTOFLUSH_VOLUME;
  this->autoflushInterval = config["autoflush_interval"] | YB_AUTOFLUSH_INTERVAL;
  this->autoflushUseHighPressureMotor = config["autoflush_use_high_pressure_motor"] | YB_AUTOFLUSH_USE_HIGH_PRESSURE_MOTOR;
  this->tankCapacity = config["tank_capacity"] | YB_TANK_CAPACITY;
  this->temperatureUnits = config["temperature_units"] | YB_TEMPERATURE_UNITS;
  this->pressureUnits = config["pressure_units"] | YB_PRESSURE_UNITS;
  this->volumeUnits = config["volume_units"] | YB_VOLUME_UNITS;
  this->flowrateUnits = config["flowrate_units"] | YB_FLOWRATE_UNITS;
  this->successMelody = config["success_melody"] | YB_SUCCESS_MELODY;
  this->errorMelody = config["error_melody"] | YB_ERROR_MELODY;
}

void Brineomatic::loadHardwareConfigJSON(JsonVariantConst config)
{
  this->boostPumpControl = config["boost_pump_control"] | YB_BOOST_PUMP_CONTROL;
  this->boostPumpRelayId = config["boost_pump_relay_id"] | YB_BOOST_PUMP_RELAY_ID;

  this->highPressurePumpControl = config["high_pressure_pump_control"] | YB_HIGH_PRESSURE_PUMP_CONTROL;
  this->highPressureRelayId = config["high_pressure_relay_id"] | YB_HIGH_PRESSURE_RELAY_ID;
  this->highPressurePumpModbusDevice = config["high_pressure_modbus_device"] | YB_HIGH_PRESSURE_PUMP_MODBUS_DEVICE;

  this->highPressureValveControl = config["high_pressure_valve_control"] | YB_HIGH_PRESSURE_VALVE_CONTROL;
  this->membranePressureTarget = config["membrane_pressure_target"] | YB_MEMBRANE_PRESSURE_TARGET;
  this->highPressureValveStepperId = config["high_pressure_valve_stepper_id"] | YB_HIGH_PRESSURE_VALVE_STEPPER_ID;
  this->highPressureValveStepperStepAngle = config["high_pressure_stepper_step_angle"] | YB_HIGH_PRESSURE_VALVE_STEPPER_STEP_ANGLE;
  this->highPressureValveStepperGearRatio = config["high_pressure_stepper_gear_ratio"] | YB_HIGH_PRESSURE_VALVE_STEPPER_GEAR_RATIO;
  this->highPressureValveStepperCloseAngle = config["high_pressure_stepper_close_angle"] | YB_HIGH_PRESSURE_VALVE_STEPPER_CLOSE_ANGLE;
  this->highPressureValveStepperCloseSpeed = config["high_pressure_stepper_close_speed"] | YB_HIGH_PRESSURE_VALVE_STEPPER_CLOSE_SPEED;
  this->highPressureValveStepperOpenAngle = config["high_pressure_stepper_open_angle"] | YB_HIGH_PRESSURE_VALVE_STEPPER_OPEN_ANGLE;
  this->highPressureValveStepperOpenSpeed = config["high_pressure_stepper_open_speed"] | YB_HIGH_PRESSURE_VALVE_STEPPER_OPEN_SPEED;

  this->diverterValveControl = config["diverter_valve_control"] | YB_DIVERTER_VALVE_CONTROL;
  this->diverterValveServoId = config["diverter_valve_servo_id"] | YB_DIVERTER_VALVE_SERVO_ID;
  this->diverterValveOpenAngle = config["diverter_valve_open_angle"] | YB_DIVERTER_VALVE_OPEN_ANGLE;
  this->diverterValveCloseAngle = config["diverter_valve_close_angle"] | YB_DIVERTER_VALVE_CLOSE_ANGLE;

  this->flushValveControl = config["flush_valve_control"] | YB_FLUSH_VALVE_CONTROL;
  this->flushValveRelayId = config["flush_valve_relay_id"] | YB_FLUSH_VALVE_RELAY_ID;

  this->coolingFanControl = config["cooling_fan_control"] | YB_COOLING_FAN_CONTROL;
  this->coolingFanRelayId = config["cooling_fan_relay_id"] | YB_COOLING_FAN_RELAY_ID;
  this->coolingFanOnTemperature = config["cooling_fan_on_temperature"] | YB_COOLING_FAN_ON_TEMPERATURE;
  this->coolingFanOffTemperature = config["cooling_fan_off_temperature"] | YB_COOLING_FAN_OFF_TEMPERATURE;

  this->hasMembranePressureSensor = config["has_membrane_pressure_sensor"] | YB_HAS_MEMBRANE_PRESSURE_SENSOR;
  this->membranePressureSensorMin = config["membrane_pressure_sensor_min"] | YB_MEMBRANE_PRESSURE_SENSOR_MIN;
  this->membranePressureSensorMax = config["membrane_pressure_sensor_max"] | YB_MEMBRANE_PRESSURE_SENSOR_MAX;

  this->hasFilterPressureSensor = config["has_filter_pressure_sensor"] | YB_HAS_FILTER_PRESSURE_SENSOR;
  this->filterPressureSensorMin = config["filter_pressure_sensor_min"] | YB_FILTER_PRESSURE_SENSOR_MIN;
  this->filterPressureSensorMax = config["filter_pressure_sensor_max"] | YB_FILTER_PRESSURE_SENSOR_MAX;

  this->hasProductTDSSensor = config["has_product_tds_sensor"] | YB_HAS_PRODUCT_TDS_SENSOR;
  this->hasBrineTDSSensor = config["has_brine_tds_sensor"] | YB_HAS_BRINE_TDS_SENSOR;

  this->hasProductFlowSensor = config["has_product_flow_sensor"] | YB_HAS_PRODUCT_FLOW_SENSOR;
  this->productFlowmeterPPL = config["product_flowmeter_ppl"] | YB_PRODUCT_FLOWMETER_PPL;

  this->hasBrineFlowSensor = config["has_brine_flow_sensor"] | YB_HAS_BRINE_FLOW_SENSOR;
  this->brineFlowmeterPPL = config["brine_flowmeter_ppl"] | YB_BRINE_FLOWMETER_PPL;

  this->hasMotorTemperatureSensor = config["has_motor_temperature_sensor"] | YB_HAS_MOTOR_TEMPERATURE_SENSOR;
}

void Brineomatic::loadSafeguardsConfigJSON(JsonVariantConst config)
{
  this->flushTimeout = config["flush_timeout"] | YB_FLUSH_TIMEOUT;
  this->membranePressureTimeout = config["membrane_pressure_timeout"] | YB_MEMBRANE_PRESSURE_TIMEOUT;
  this->productFlowrateTimeout = config["product_flowrate_timeout"] | YB_PRODUCT_FLOWRATE_TIMEOUT;
  this->productSalinityTimeout = config["product_salinity_timeout"] | YB_PRODUCT_SALINITY_TIMEOUT;
  this->productionRuntimeTimeout = config["production_runtime_timeout"] | YB_PRODUCTION_RUNTIME_TIMEOUT;

  this->enableMembranePressureHighCheck = config["enable_membrane_pressure_high_check"] | YB_ENABLE_MEMBRANE_PRESSURE_HIGH_CHECK;
  this->membranePressureHighThreshold = config["membrane_pressure_high_threshold"] | YB_MEMBRANE_PRESSURE_HIGH_THRESHOLD;
  this->membranePressureHighDelay = config["membrane_pressure_high_delay"] | YB_MEMBRANE_PRESSURE_HIGH_DELAY;

  this->enableMembranePressureLowCheck = config["enable_membrane_pressure_low_check"] | YB_ENABLE_MEMBRANE_PRESSURE_LOW_CHECK;
  this->membranePressureLowThreshold = config["membrane_pressure_low_threshold"] | YB_MEMBRANE_PRESSURE_LOW_THRESHOLD;
  this->membranePressureLowDelay = config["membrane_pressure_low_delay"] | YB_MEMBRANE_PRESSURE_LOW_DELAY;

  this->enableFilterPressureHighCheck = config["enable_filter_pressure_high_check"] | YB_ENABLE_FILTER_PRESSURE_HIGH_CHECK;
  this->filterPressureHighThreshold = config["filter_pressure_high_threshold"] | YB_FILTER_PRESSURE_HIGH_THRESHOLD;
  this->filterPressureHighDelay = config["filter_pressure_high_delay"] | YB_FILTER_PRESSURE_HIGH_DELAY;

  this->enableFilterPressureLowCheck = config["enable_filter_pressure_low_check"] | YB_ENABLE_FILTER_PRESSURE_LOW_CHECK;
  this->filterPressureLowThreshold = config["filter_pressure_low_threshold"] | YB_FILTER_PRESSURE_LOW_THRESHOLD;
  this->filterPressureLowDelay = config["filter_pressure_low_delay"] | YB_FILTER_PRESSURE_LOW_DELAY;

  this->enableProductFlowrateHighCheck = config["enable_product_flowrate_high_check"] | YB_ENABLE_PRODUCT_FLOWRATE_HIGH_CHECK;
  this->productFlowrateHighThreshold = config["product_flowrate_high_threshold"] | YB_PRODUCT_FLOWRATE_HIGH_THRESHOLD;
  this->productFlowrateHighDelay = config["product_flowrate_high_delay"] | YB_PRODUCT_FLOWRATE_HIGH_DELAY;

  this->enableProductFlowrateLowCheck = config["enable_product_flowrate_low_check"] | YB_ENABLE_PRODUCT_FLOWRATE_LOW_CHECK;
  this->productFlowrateLowThreshold = config["product_flowrate_low_threshold"] | YB_PRODUCT_FLOWRATE_LOW_THRESHOLD;
  this->productFlowrateLowDelay = config["product_flowrate_low_delay"] | YB_PRODUCT_FLOWRATE_LOW_DELAY;

  this->enableRunTotalFlowrateLowCheck = config["enable_run_total_flowrate_low_check"] | YB_ENABLE_RUN_TOTAL_FLOWRATE_LOW_CHECK;
  this->runTotalFlowrateLowThreshold = config["run_total_flowrate_low_threshold"] | YB_RUN_TOTAL_FLOWRATE_LOW_THRESHOLD;
  this->runTotalFlowrateLowDelay = config["run_total_flowrate_low_delay"] | YB_RUN_TOTAL_FLOWRATE_LOW_DELAY;

  this->enablePickleTotalFlowrateLowCheck = config["enable_pickle_total_flowrate_low_check"] | YB_ENABLE_PICKLE_TOTAL_FLOWRATE_LOW_CHECK;
  this->pickleTotalFlowrateLowThreshold = config["pickle_total_flowrate_low_threshold"] | YB_PICKLE_TOTAL_FLOWRATE_LOW_THRESHOLD;
  this->pickleTotalFlowrateLowDelay = config["pickle_total_flowrate_low_delay"] | YB_PICKLE_TOTAL_FLOWRATE_LOW_DELAY;

  this->enableDiverterValveClosedCheck = config["enable_diverter_valve_closed_check"] | YB_ENABLE_DIVERTER_VALVE_CLOSED_CHECK;
  this->diverterValveClosedDelay = config["diverter_valve_closed_delay"] | YB_DIVERTER_VALVE_CLOSED_DELAY;

  this->enableProductSalinityHighCheck = config["enable_product_salinity_high_check"] | YB_ENABLE_PRODUCT_SALINITY_HIGH_CHECK;
  this->productSalinityHighThreshold = config["product_salinity_high_threshold"] | YB_PRODUCT_SALINITY_HIGH_THRESHOLD;
  this->productSalinityHighDelay = config["product_salinity_high_delay"] | YB_PRODUCT_SALINITY_HIGH_DELAY;

  this->enableMotorTemperatureCheck = config["enable_motor_temperature_check"] | YB_ENABLE_MOTOR_TEMPERATURE_CHECK;
  this->motorTemperatureHighThreshold = config["motor_temperature_high_threshold"] | YB_MOTOR_TEMPERATURE_HIGH_THRESHOLD;
  this->motorTemperatureHighDelay = config["motor_temperature_high_delay"] | YB_MOTOR_TEMPERATURE_HIGH_DELAY;

  this->enableFlushFlowrateLowCheck = config["enable_flush_flowrate_low_check"] | YB_ENABLE_FLUSH_FLOWRATE_LOW_CHECK;
  this->flushFlowrateLowThreshold = config["flush_flowrate_low_threshold"] | YB_FLUSH_FLOWRATE_LOW_THRESHOLD;
  this->flushFlowrateLowDelay = config["flush_flowrate_low_delay"] | YB_FLUSH_FLOWRATE_LOW_DELAY;

  this->enableFlushFilterPressureLowCheck = config["enable_flush_filter_pressure_low_check"] | YB_ENABLE_FLUSH_FILTER_PRESSURE_LOW_CHECK;
  this->flushFilterPressureLowThreshold = config["flush_filter_pressure_low_threshold"] | YB_FLUSH_FILTER_PRESSURE_LOW_THRESHOLD;
  this->flushFilterPressureLowDelay = config["flush_filter_pressure_low_delay"] | YB_FLUSH_FILTER_PRESSURE_LOW_DELAY;

  this->enableFlushValveOffCheck = config["enable_flush_valve_off_check"] | YB_ENABLE_FLUSH_VALVE_OFF_CHECK;
  this->flushValveOffThreshold = config["flush_valve_off_threshold"] | YB_FLUSH_VALVE_OFF_THRESHOLD;
  this->flushValveOffDelay = config["flush_valve_off_delay"] | YB_FLUSH_VALVE_OFF_DELAY;
}

void Brineomatic::updateMQTT()
{
  JsonDocument output;
  this->generateUpdateJSON(output);
  output.remove("brineomatic");

  mqtt_traverse_json(output, "watermaker");
}

#endif