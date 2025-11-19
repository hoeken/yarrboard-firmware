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
  #include "relay_channel.h"
  #include "servo_channel.h"
  #include "stepper_channel.h"
  #include <ADS1X15.h>
  #include <Arduino.h>
  #include <DallasTemperature.h>
  #include <GravityTDS.h>
  #include <OneWire.h>

etl::deque<float, YB_BOM_DATA_SIZE> motor_temperature_data;
etl::deque<float, YB_BOM_DATA_SIZE> water_temperature_data;
etl::deque<float, YB_BOM_DATA_SIZE> filter_pressure_data;
etl::deque<float, YB_BOM_DATA_SIZE> membrane_pressure_data;
etl::deque<float, YB_BOM_DATA_SIZE> product_salinity_data;
etl::deque<float, YB_BOM_DATA_SIZE> brine_salinity_data;
etl::deque<float, YB_BOM_DATA_SIZE> product_flowrate_data;
etl::deque<float, YB_BOM_DATA_SIZE> brine_flowrate_data;
etl::deque<float, YB_BOM_DATA_SIZE> tank_level_data;

uint32_t lastDataStore = 0;

Brineomatic wm;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(YB_DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);
DeviceAddress motorThermometer;

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

  // DS18B20 Sensor
  ds18b20.begin();
  YBP.print("Found ");
  YBP.print(ds18b20.getDeviceCount(), DEC);
  YBP.println(" DS18B20 devices.");

  // // report parasite power requirements
  // YBP.print("Parasite power is: ");
  // if (ds18b20.isParasitePowerMode())
  //   YBP.println("ON");
  // else
  //   YBP.println("OFF");

  // lookup our address
  if (!ds18b20.getAddress(motorThermometer, 0))
    YBP.println("Unable to find address for DS18B20");

  ds18b20.setResolution(motorThermometer, 9);
  ds18b20.setWaitForConversion(false);
  ds18b20.requestTemperatures();

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

  // temporary hardcoding.
  wm.flushValve = &relay_channels[1];
  wm.flushValve->setName("Flush Valve");
  wm.flushValve->setKey("flush_valve");
  wm.flushValve->defaultState = false;
  strncpy(wm.flushValve->type, "solenoid", sizeof(wm.flushValve->type));

  wm.coolingFan = &relay_channels[2];
  wm.coolingFan->setName("Cooling Fan");
  wm.coolingFan->setKey("cooling_fan");
  wm.coolingFan->defaultState = false;
  strncpy(wm.coolingFan->type, "fan", sizeof(wm.coolingFan->type));

  wm.highPressurePump = &relay_channels[3];
  wm.highPressurePump->setName("High Pressure Pump");
  wm.highPressurePump->setKey("hp_pump");
  wm.highPressurePump->defaultState = false;
  strncpy(wm.highPressurePump->type, "water_pump", sizeof(wm.highPressurePump->type));

  // disabled until we implement config xml
  // wm.boostPump = &relay_channels[3];
  // wm.boostPump->setName("Boost Pump");
  // wm.boostPump->setKey("boost_pump");
  // wm.boostPump->defaultState = false;
  // strncpy(wm.boostPump->type, "water_pump", sizeof(wm.boostPump->type));

  wm.diverterValve = &servo_channels[0];
  wm.diverterValve->setName("Diverter Valve");
  wm.diverterValve->setKey("diverter_valve");

  wm.highPressureValveServo = &servo_channels[1];
  wm.highPressureValveServo->setName("High Pressure Valve");
  wm.highPressureValveServo->setKey("hp_valve");

  wm.highPressureValveStepper = &stepper_channels[0];
  wm.highPressureValveStepper->setName("High Pressure Valve");
  wm.highPressureValveStepper->setKey("hp_valve");

  wm.highPressureValveMode = Brineomatic::HighPressureValveControlMode::STEPPER;

  // Create a FreeRTOS task for the state machine
  xTaskCreatePinnedToCore(
    brineomatic_state_machine, // Task function
    "brineomatic_sm",          // Name of the task
    4096,                      // Stack size
    NULL,                      // Task input parameters
    1,                         // Priority of the task
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
  measure_temperature();

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

  if (millis() - lastDataStore > YB_BOM_DATA_INTERVAL) {
    lastDataStore = millis();

    if (motor_temperature_data.full())
      motor_temperature_data.pop_front();
    motor_temperature_data.push_back(wm.getMotorTemperature());

    if (water_temperature_data.full())
      water_temperature_data.pop_front();
    water_temperature_data.push_back(wm.getWaterTemperature());

    if (filter_pressure_data.full())
      filter_pressure_data.pop_front();
    filter_pressure_data.push_back(wm.getFilterPressure());

    if (membrane_pressure_data.full())
      membrane_pressure_data.pop_front();
    membrane_pressure_data.push_back(wm.getMembranePressure());

    if (product_salinity_data.full())
      product_salinity_data.pop_front();
    product_salinity_data.push_back(wm.getProductSalinity());

    if (brine_salinity_data.full())
      brine_salinity_data.pop_front();
    brine_salinity_data.push_back(wm.getBrineSalinity());

    if (product_flowrate_data.full())
      product_flowrate_data.pop_front();
    product_flowrate_data.push_back(wm.getProductFlowrate());

    if (brine_flowrate_data.full())
      brine_flowrate_data.pop_front();
    brine_flowrate_data.push_back(wm.getBrineFlowrate());

    if (tank_level_data.full())
      tank_level_data.pop_front();
    tank_level_data.push_back(wm.getTankLevel());
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

void measure_temperature()
{
  if (ds18b20.isConversionComplete()) {
    float tempC = ds18b20.getTempC(motorThermometer);

    if (tempC == DEVICE_DISCONNECTED_C) {
      wm.setMotorTemperature(-999);
    }
    wm.setMotorTemperature(tempC);

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
  if (amperage < 1.0) {
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

  if (amperage < 1.0) {
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

Brineomatic::Brineomatic()
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

  autoFlushEnabled = true;

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
  membranePressureTarget = -1;

  currentStatus = Status::STARTUP;
  runResult = Result::STARTUP;
  flushResult = Result::STARTUP;
  pickleResult = Result::STARTUP;
  depickleResult = Result::STARTUP;

  // HSR-1425CR
  //  highPressureValveOpenMax = 125;
  //  highPressureValveOpenMin = 90;
  //  highPressureValveCloseMin = 90;
  //  highPressureValveCloseMax = 55;

  // HSR-2645CR
  highPressureValveOpenMax = 125;
  highPressureValveOpenMin = 92;
  highPressureValveCloseMin = 92;
  highPressureValveCloseMax = 55;

  highPressureValveMaintainOpenMax = 100;
  highPressureValveMaintainOpenMin = 92;
  highPressureValveMaintainCloseMin = 92;
  highPressureValveMaintainCloseMax = 80;

  // PID settings - Ramp Up
  KpRamp = 2.2;
  KiRamp = 0;
  KdRamp = 0.55;

  // PID Settings - Maintain
  KpMaintain = 1.50;
  KiMaintain = 0.02;
  KdMaintain = 0;

  // PID controller
  membranePressurePID = QuickPID(&currentMembranePressure, &membranePressurePIDOutput, &membranePressureTarget);
  membranePressurePID.SetMode(QuickPID::Control::automatic);
  membranePressurePID.SetAntiWindupMode(QuickPID::iAwMode::iAwClamp);
  membranePressurePID.SetTunings(KpRamp, KiRamp, KdRamp);
  membranePressurePID.SetControllerDirection(QuickPID::Action::direct);
  membranePressurePID.SetOutputLimits(YB_BOM_PID_OUTPUT_MIN, YB_BOM_PID_OUTPUT_MAX);
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

  // positive target, initialize our PID.
  if (pressure >= 0) {
    membranePressurePID.Initialize();
    membranePressurePID.Reset();

    // header for debugging.
    YBP.println("Membrane Pressure Target,Current Membrane Pressure,Pterm,Iterm,Kterm,Output Sum, PID Output, Servo Angle");
  }
  // negative target, turn off the servo.
  else {
    if (highPressureValveMode == HighPressureValveControlMode::SERVO) {
      YBP.println("target <= 0, turn off the servo.");
      highPressureValveServo->write(highPressureValveCloseMin); // neutral / stop
      highPressureValveServo->disable();
    } else if (highPressureValveMode == HighPressureValveControlMode::STEPPER) {
      YBP.println("target <= 0, disable our stepper");
      highPressureValveStepper->gotoAngle(0, 40);
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
  if (currentStatus == Status::IDLE || currentStatus == Status::PICKLED) {
    desiredFlushDuration = 0;
    desiredFlushVolume = 0;
    currentStatus = Status::FLUSHING;
  }
}

void Brineomatic::flushDuration(uint32_t duration)
{
  if (currentStatus == Status::IDLE || currentStatus == Status::PICKLED) {
    desiredFlushDuration = duration;
    desiredFlushVolume = 0;
    currentStatus = Status::FLUSHING;
  }
}

void Brineomatic::flushVolume(float volume)
{
  if (currentStatus == Status::IDLE || currentStatus == Status::PICKLED) {
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
  if (membranePressureTarget > 0) {
    setMembranePressureTarget(0);
    uint32_t membranePressureStart = millis();
    while (getMembranePressure() > 65) {
      vTaskDelay(pdMS_TO_TICKS(100));

      if (millis() - membranePressureStart > membranePressureTimeout) {
        isFailure = true;
        break;
      }
    }

    // turns our high pressure valve controller off
    setMembranePressureTarget(-1);
  }

  disableHighPressurePump();

  if (highPressureValveMode == HighPressureValveControlMode::STEPPER)
    highPressureValveStepper->home();

  disableBoostPump();
  closeFlushValve();
  disableCoolingFan();

  return isFailure;
}

bool Brineomatic::hasBoostPump()
{
  return boostPump != nullptr;
}

bool Brineomatic::isBoostPumpOn()
{
  return boostPumpOnState;
}

void Brineomatic::enableBoostPump()
{
  if (hasBoostPump()) {
    boostPumpOnState = true;
    boostPump->setState(true);
  } else
    boostPumpOnState = false;
}

void Brineomatic::disableBoostPump()
{
  if (hasBoostPump())
    boostPump->setState(false);
  boostPumpOnState = false;
}

bool Brineomatic::hasHighPressurePump()
{
  return highPressurePump != nullptr;
}

bool Brineomatic::isHighPressurePumpOn()
{
  return highPressurePumpOnState;
}

void Brineomatic::enableHighPressurePump()
{
  if (hasHighPressurePump()) {
    highPressurePump->setState(true);
    highPressurePumpOnState = true;
  } else
    highPressurePumpOnState = false;
}

void Brineomatic::disableHighPressurePump()
{
  if (hasHighPressurePump())
    highPressurePump->setState(false);
  highPressurePumpOnState = false;
}

bool Brineomatic::hasDiverterValve()
{
  return flushValve != nullptr;
}

bool Brineomatic::isDiverterValveOpen()
{
  return diverterValveOpenState;
}

void Brineomatic::openDiverterValve()
{
  if (hasDiverterValve()) {
    diverterValveOpenState = true;
    diverterValve->write(diverterValveOpenAngle);
    // vTaskDelay(pdMS_TO_TICKS(1000));
    // diverterValve->disable();
  } else
    diverterValveOpenState = false;
}

void Brineomatic::closeDiverterValve()
{
  if (hasDiverterValve()) {
    diverterValve->write(diverterValveClosedAngle);
    // vTaskDelay(pdMS_TO_TICKS(1000));
    // diverterValve->disable();
  }
  diverterValveOpenState = false;
}

bool Brineomatic::hasFlushValve()
{
  return flushValve != nullptr;
}

bool Brineomatic::isFlushValveOpen()
{
  return flushValveOpenState;
}

void Brineomatic::openFlushValve()
{
  if (hasFlushValve()) {
    flushValve->setState(true);
    flushValveOpenState = true;
  } else
    flushValveOpenState = false;
}

void Brineomatic::closeFlushValve()
{
  if (hasFlushValve())
    flushValve->setState(false);
  flushValveOpenState = false;
}

bool Brineomatic::hasCoolingFan()
{
  return coolingFan != nullptr;
}

bool Brineomatic::isCoolingFanOn()
{
  return coolingFanOnState;
}

void Brineomatic::enableCoolingFan()
{
  if (hasCoolingFan()) {
    coolingFan->setState(true);
    coolingFanOnState = true;
  } else
    coolingFanOnState = false;
}

void Brineomatic::disableCoolingFan()
{
  if (hasCoolingFan())
    coolingFan->setState(false);
  coolingFanOnState = false;
}

void Brineomatic::manageCoolingFan()
{
  if (currentStatus != Status::MANUAL) {
    if (hasCoolingFan()) {
      if (getMotorTemperature() >= coolingFanOnTemperature)
        enableCoolingFan();
      else if (getMotorTemperature() <= coolingFanOffTemperature)
        disableCoolingFan();
    }
  }
}

float Brineomatic::getFilterPressure()
{
  // ignore residual pressure in idle mode.
  if (currentStatus == Status::IDLE && currentFilterPressure <= 2)
    return 0;
  else
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
  return productFlowrateMinimum;
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
  return motorTemperatureMaximum;
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
  return productSalinityMaximum;
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
  if (currentStatus == Status::IDLE && autoFlushEnabled)
    return flushInterval - (millis() - lastAutoFlushTime);

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
    }
    // if we have tank capacity and a flowrate, we can estimate.
    else if (getTankCapacity() > 0 && getProductFlowrate() > 0) {
      float remainingVolume = getTankCapacity() * (1.0 - getTankLevel());
      uint32_t remainingMillis = (remainingVolume / getProductFlowrate()) * 3600 * 1000;
      return remainingMillis;
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
    float remainingVolume = desiredFlushVolume - getFlushVolume();
    uint32_t remainingMillis = (remainingVolume / getBrineFlowrate()) * 3600 * 1000;
    return remainingMillis;
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
  return highPressureValveMode != HighPressureValveControlMode::NONE;
}

void Brineomatic::manageHighPressureValve()
{
  float angle;

  if (currentStatus != Status::IDLE) {
    if (hasHighPressureValve()) {
      if (membranePressureTarget >= 0) {
        // only use Ki for tuning once we are close to our target.
        if (abs(membranePressureTarget - currentMembranePressure) / membranePressureTarget > 0.05)
          membranePressurePID.SetTunings(KpRamp, KiRamp, KdRamp);
        else
          membranePressurePID.SetTunings(KpMaintain, KpMaintain, KdMaintain);

        // run our PID calculations
        if (membranePressurePID.Compute()) {
          // different max values for the ramp
          if (abs(membranePressureTarget - currentMembranePressure) / membranePressureTarget > 0.05)
            angle = map(membranePressurePIDOutput, YB_BOM_PID_OUTPUT_MIN, YB_BOM_PID_OUTPUT_MAX, highPressureValveOpenMax, highPressureValveCloseMax);
          // smaller max values for maintain.
          else
            angle = map(membranePressurePIDOutput, YB_BOM_PID_OUTPUT_MIN, YB_BOM_PID_OUTPUT_MAX, highPressureValveMaintainOpenMax, highPressureValveMaintainCloseMax);

          // if we're close, just disable so its not constantly drawing current.
          if (highPressureValveMode == HighPressureValveControlMode::SERVO) {

            if (abs(membranePressureTarget - currentMembranePressure) / membranePressureTarget > 0.01)
              highPressureValveServo->write(angle);
            else {
              highPressureValveServo->write(highPressureValveCloseMin); // neutral / stop
              highPressureValveServo->disable();
              membranePressurePID.Reset(); // keep our pid from winding up.
            }
          } else if (highPressureValveMode == HighPressureValveControlMode::STEPPER) {
            if (membranePressureTarget > 0)
              highPressureValveStepper->gotoAngle(1650, 10);
            else
              highPressureValveStepper->gotoAngle(0, 10);
          }

          // YBP.printf("HP PID | current: %.0f / target: %.0f | p: % .3f / i: % .3f / d: % .3f / sum: % .3f | output: %.0f / angle: %.0f\n", round(currentMembranePressure), round(membranePressureTarget), membranePressurePID.GetPterm(), membranePressurePID.GetIterm(), membranePressurePID.GetDterm(), membranePressurePID.GetOutputSum(), membranePressurePIDOutput, angle);
        }
      }
    }
  }
}

void Brineomatic::runStateMachine()
{
  switch (currentStatus) {

    //
    // STARTUP
    //
    case Status::STARTUP:
      initializeHardware();

      if (isPickled)
        currentStatus = Status::PICKLED;
      else {
        if (autoFlushEnabled)
          lastAutoFlushTime = millis();
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
      // YBP.println("State: IDLE");
      if (autoFlushEnabled && millis() - lastAutoFlushTime > flushInterval) {
        currentStatus = Status::FLUSHING;
        desiredFlushDuration = defaultFlushDuration;
      }
      break;

    //
    // RUNNING
    //
    case Status::RUNNING: {
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
        // YBP.println("Boost Pump Started");
        enableBoostPump();
        while (getFilterPressure() < getFilterPressureMinimum()) {
          if (checkStopFlag(runResult))
            return;

          if (millis() - boostPumpStart > filterPressureTimeout) {
            currentStatus = Status::STOPPING;
            runResult = Result::ERR_FILTER_PRESSURE_TIMEOUT;
            return;
          }

          vTaskDelay(pdMS_TO_TICKS(100));
        }
        // YBP.println("Boost Pump OK");
      }

      YBP.println("High Pressure Pump Started");
      enableHighPressurePump();
      setMembranePressureTarget(defaultMembranePressureTarget);

      if (waitForMembranePressure())
        return;

      YBP.println("High Pressure Pump OK");

      if (waitForProductFlowrate())
        return;

      YBP.println("Flowrate OK");

      if (waitForProductSalinity())
        return;

      YBP.println("Salinity OK");

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

        if (checkTotalFlowrateLow(runTotalFlowrateMinimum))
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

        if (millis() - productionStart > productionTimeout) {
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

        // save our total runtime occasionally (every 5 minutes)
        if (millis() - lastRuntimeUpdate > 60000 * 5) {
          totalRuntime += (millis() - lastRuntimeUpdate) / 1000;
          preferences.putULong("bomTotRuntime", totalRuntime); // store as seconds
          lastRuntimeUpdate = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(100));
      }

      // save our total volume produced
      preferences.putFloat("bomTotVolume", totalVolume);

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
      resetErrorTimers();

      if (initializeHardware())
        currentStatus = Status::IDLE;
      else {
        currentStatus = Status::FLUSHING;
        desiredFlushDuration = defaultFlushDuration;
      }

      break;
    }

    //
    // FLUSHING
    //
    case Status::FLUSHING: {
      resetErrorTimers();

      flushStart = millis();
      currentFlushVolume = 0;

      DUMP(desiredFlushDuration);
      DUMP(desiredFlushVolume);

      if (initializeHardware()) {
        currentStatus = Status::IDLE;
        return;
      }

      // start up our hardware
      openFlushValve();
      if (flushUseHighPressureMotor)
        enableHighPressurePump();

      // check our sensors while we flush
      while (true) {

        if (checkFlushFilterPressureLow())
          break;

        if (checkFlushFlowrateLow())
          break;

        if (flushUseHighPressureMotor && checkMotorTemperature(flushResult))
          break;

        if (checkStopFlag(flushResult))
          break;

        // are we going for time?
        if (desiredFlushDuration > 0 && getFlushElapsed() > desiredFlushDuration) {
          flushResult = Result::SUCCESS_TIME;
          DUMP("DURATION");
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
          if (getBrineSalinity() < flushSalinityFinished) {
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

      if (autoFlushEnabled)
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
      brineFlowrateLowStart = 0;

      if (initializeHardware()) {
        currentStatus = Status::IDLE;
        return;
      }

      enableHighPressurePump();

      while (getPickleElapsed() < pickleDuration) {
        if (stopFlag)
          break;

        if (checkBrineFlowrateLow(pickleTotalFlowrateMinimum, pickleResult)) {
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
      brineFlowrateLowStart = 0;

      if (initializeHardware()) {
        currentStatus = Status::IDLE;
        return;
      }

      enableHighPressurePump();
      while (getDepickleElapsed() < depickleDuration) {
        if (stopFlag)
          break;

        if (checkBrineFlowrateLow(pickleTotalFlowrateMinimum, depickleResult)) {
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
  return checkTimedError(
    getMembranePressure() > highPressureMaximum,
    membranePressureHighStart,
    membranePressureHighTimeout,
    Result::ERR_MEMBRANE_PRESSURE_HIGH,
    runResult);
}

bool Brineomatic::checkMembranePressureLow()
{
  return checkTimedError(
    getMembranePressure() < highPressureMinimum,
    membranePressureLowStart,
    membranePressureLowTimeout,
    Result::ERR_MEMBRANE_PRESSURE_LOW,
    runResult);
}

bool Brineomatic::checkFilterPressureHigh()
{
  return checkTimedError(
    getFilterPressure() > lowPressureMaximum,
    filterPressureHighStart,
    filterPressureHighTimeout,
    Result::ERR_FILTER_PRESSURE_HIGH,
    runResult);
}

bool Brineomatic::checkFilterPressureLow()
{
  return checkTimedError(
    getFilterPressure() < lowPressureMinimum,
    filterPressureLowStart,
    filterPressureLowTimeout,
    Result::ERR_FILTER_PRESSURE_LOW,
    runResult);
}

bool Brineomatic::checkProductFlowrateLow()
{
  return checkTimedError(
    getProductFlowrate() < getProductFlowrateMinimum(),
    productFlowrateLowStart,
    productFlowrateLowTimeout,
    Result::ERR_PRODUCT_FLOWRATE_LOW,
    runResult);
}

bool Brineomatic::checkProductFlowrateHigh()
{
  return checkTimedError(
    getProductFlowrate() > productFlowrateMaximum,
    productFlowrateHighStart,
    productFlowrateHighTimeout,
    Result::ERR_PRODUCT_FLOWRATE_HIGH,
    runResult);
}

bool Brineomatic::checkBrineFlowrateLow(float flowrate, Result& result)
{
  return checkTimedError(
    getBrineFlowrate() < flowrate,
    brineFlowrateLowStart,
    brineFlowrateLowTimeout,
    Result::ERR_BRINE_FLOWRATE_LOW,
    result);
}

bool Brineomatic::checkFlushFilterPressureLow()
{
  return checkTimedError(
    getFilterPressure() < flushFilterPressureMinimum,
    flushFilterPressureLowStart,
    flushFilterPressureLowTimeout,
    Result::ERR_FLUSH_FILTER_PRESSURE_LOW,
    flushResult);
}

bool Brineomatic::checkFlushFlowrateLow()
{
  return checkTimedError(
    getTotalFlowrate() < flushFlowrateMinimum,
    flushFlowrateLowStart,
    flushFlowrateLowTimeout,
    Result::ERR_FLUSH_FLOWRATE_LOW,
    flushResult);
}

bool Brineomatic::checkTotalFlowrateLow(float flowrate)
{
  return checkTimedError(
    getTotalFlowrate() < flowrate,
    totalFlowrateLowStart,
    totalFlowrateLowTimeout,
    Result::ERR_TOTAL_FLOWRATE_LOW,
    runResult);
}

bool Brineomatic::checkDiverterValveClosed()
{
  return checkTimedError(
    getTotalFlowrate() > getBrineFlowrate() + getProductFlowrate(),
    diverterValveOpenStart,
    diverterValveOpenTimeout,
    Result::ERR_DIVERTER_VALVE_OPEN,
    runResult);
}

bool Brineomatic::checkProductSalinityHigh()
{
  return checkTimedError(
    getProductSalinity() > getProductSalinityMaximum(),
    productSalinityHighStart,
    productSalinityHighTimeout,
    Result::ERR_PRODUCT_SALINITY_HIGH,
    runResult);
}

bool Brineomatic::checkMotorTemperature(Result& result)
{
  return checkTimedError(
    getMotorTemperature() > getMotorTemperatureMaximum(),
    motorTemperatureStart,
    motorTemperatureTimeout,
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

bool Brineomatic::waitForMembranePressure()
{
  uint32_t highPressurePumpStart = millis();
  while (getMembranePressure() < getMembranePressureMinimum()) {

    // let the spice flow
    if (checkTotalFlowrateLow(runTotalFlowrateMinimum))
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

  return false;
}

bool Brineomatic::waitForProductFlowrate()
{
  int flowReady = 0;
  uint32_t flowCheckStart = millis();

  while (flowReady < 10) {
    if (checkMembranePressureHigh())
      return false;
    if (checkStopFlag(runResult))
      return false;

    if (getProductFlowrate() > getProductFlowrateMinimum() && getProductFlowrate() < productFlowrateMaximum)
      flowReady++;
    else
      flowReady = 0;

    if (millis() - flowCheckStart > productFlowRateTimeout) {
      currentStatus = Status::STOPPING;
      runResult = Result::ERR_PRODUCT_FLOWRATE_TIMEOUT;
      return true;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }

  return false;
}

bool Brineomatic::waitForProductSalinity()
{
  int salinityReady = 0;
  uint32_t salinityCheckStart = millis();
  while (salinityReady < 10) {
    if (checkMembranePressureHigh())
      return false;
    if (checkStopFlag(runResult))
      return false;

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

  return false;
}

bool Brineomatic::waitForFlushValveOff()
{
  uint32_t start = millis();
  while (getFilterPressure() > 2 || getBrineFlowrate() > 0) {
    if (millis() - start > flushValveOffTimeout) {
      currentStatus = Status::IDLE;
      flushResult = Result::ERR_FLUSH_VALVE_ON;
      return true;
    }

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

void Brineomatic::updateMQTT()
{
  JsonDocument output;
  this->generateUpdateJSON(output);
  output.remove("brineomatic");

  mqtt_traverse_json(output, "watermaker");
}

#endif