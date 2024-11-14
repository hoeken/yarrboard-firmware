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

void brineomatic_setup()
{
  wm.init();

  byte motor_a_pins[YB_DC_MOTOR_CHANNEL_COUNT] = YB_DC_MOTOR_A_PINS;
  byte motor_b_pins[YB_DC_MOTOR_CHANNEL_COUNT] = YB_DC_MOTOR_B_PINS;

  for (byte i = 0; i < YB_DC_MOTOR_CHANNEL_COUNT; i++) {
    pinMode(motor_a_pins[i], OUTPUT);
    pinMode(motor_b_pins[i], OUTPUT);
    digitalWrite(motor_a_pins[i], LOW);
    digitalWrite(motor_a_pins[i], LOW);
  }

  // do our init for our flowmeter
  pinMode(YB_FLOWMETER_PIN, INPUT);
  pulse_counter = 0;
  lastFlowmeterCheckMicros = 0;
  attachInterrupt(digitalPinToInterrupt(YB_FLOWMETER_PIN), flowmeter_interrupt, FALLING);

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
  brineomatic_adc.setDataRate(4); // 128 samples per second.

  current_ads1115_channel = 0;
  brineomatic_adc.requestADC(current_ads1115_channel);

  gravityTds.setAref(YB_ADS1115_VREF); // reference voltage on ADC
  gravityTds.setAdcRange(15);          // 16 bit ADC, but its differential, so lose 1 bit.
  gravityTds.begin();                  // initialization

  char prefIndex[YB_PREF_KEY_LENGTH];

  // temporary hardcoding.
  wm.highPressurePump = &relay_channels[0];
  wm.flushValve = &relay_channels[1];
  wm.diverterValve = &servo_channels[0];
  wm.highPressureValve = &servo_channels[1];

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

  wm.manageHighPressureValve();
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
  // check roughly every second
  uint64_t elapsed = esp_timer_get_time() - lastFlowmeterCheckMicros;
  if (elapsed >= 1000000) {
    // calculate flowrate
    float elapsed_seconds = elapsed / 1e6;

    // Calculate liters per second
    float liters_per_second = (pulse_counter / flowmeterPulsesPerLiter) / elapsed_seconds;

    // Convert to liters per hour
    float flowrate = liters_per_second * 3600;

    // update our volume
    wm.currentVolume += pulse_counter / flowmeterPulsesPerLiter;
    wm.totalVolume += pulse_counter / flowmeterPulsesPerLiter;

    // reset counter
    // detachInterrupt(digitalPinToInterrupt(YB_FLOWMETER_PIN));
    pulse_counter = 0;
    // attachInterrupt(digitalPinToInterrupt(YB_FLOWMETER_PIN), flowmeter_interrupt, FALLING);

    // store microseconds when tacho was measured the last time
    lastFlowmeterCheckMicros = esp_timer_get_time();

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
  gravityTds.setTemperature(wm.getWaterTemperature()); // set the temperature and execute temperature compensation
  gravityTds.update(reading);                          // sample and calculate
  float tdsReading = gravityTds.getTdsValue();         // then get the value
  wm.setSalinity(tdsReading);
}

void measure_filter_pressure(int16_t reading)
{
  float voltage = brineomatic_adc.toVoltage(reading);

  if (voltage < 0.4) {
    // Serial.println("No LP Sensor Detected");
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
    // Serial.println("No HP Sensor Detected");
    wm.setMembranePressure(-999);
    return;
  }

  float amperage = (voltage / YB_420_RESISTOR) * 1000;
  float highPressureReading = map_generic(amperage, 4.0, 20.0, 0.0, YB_HP_SENSOR_MAX);

  if (highPressureReading > 800)
    sendDebug("high pressure: %.3f", highPressureReading);

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
    totalRuntime = preferences.getULong64("bomTotRuntime");
  else
    totalRuntime = 0.0;

  if (preferences.isKey("bomTotCycles"))
    totalCycles = preferences.getUInt("bomTotCycles");
  else
    totalCycles = 0;

  autoFlushEnabled = true;
  diversionValveOpen = false;
  highPressurePumpEnabled = false;
  boostPumpEnabled = false;
  flushValveOpen = false;

  currentTankLevel = -1;
  currentWaterTemperature = 25.0;
  currentTemperature = 0.0;
  currentFlowrate = 0.0;
  currentVolume = 0.0;
  currentSalinity = 0.0;
  currentFilterPressure = 0.0;
  currentMembranePressure = 0.0;
  membranePressureTarget = -1;

  currentStatus = Status::STARTUP;
  runResult = Result::STARTUP;
  flushResult = Result::STARTUP;
  pickleResult = Result::STARTUP;
  depickleResult = Result::STARTUP;

  Kp = 0.180;
  Ki = 0.012;
  Kd = 0.020;

  // HSR-1425CR
  //  highPressureValveOpenMin = 90;
  //  highPressureValveOpenMax = 125;
  //  highPressureValveCloseMin = 90;
  //  highPressureValveCloseMax = 55;

  // HSR-2645CR
  highPressureValveOpenMin = 92.5;
  highPressureValveOpenMax = 125;
  highPressureValveCloseMin = 92.5;
  highPressureValveCloseMax = 55;

  membranePressurePID = QuickPID(&currentMembranePressure, &membranePressurePIDOutput, &membranePressureTarget);
  membranePressurePID.SetTunings(Kp, Ki, Kd);
  membranePressurePID.SetMode(QuickPID::Control::automatic);
  membranePressurePID.SetOutputLimits(0, 255);
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

  if (pressure >= 0) {
    // we need forward and reverse to control the valve.
    if (membranePressureTarget > currentMembranePressure) {
      membranePressurePID.SetControllerDirection(QuickPID::Action::direct);
    } else {
      membranePressurePID.SetControllerDirection(QuickPID::Action::reverse);
    }

    membranePressurePID.Initialize();
  }
  // negative target, turn off the servo.
  else {
    sendDebug("negative target, turn off the servo.");
    highPressureValve->write(highPressureValveCloseMin);
    highPressureValve->disable();
  }
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

void Brineomatic::depickle(uint64_t duration)
{
  stopFlag = false;
  if (currentStatus == Status::PICKLED) {
    depickleDuration = duration;
    currentStatus = Status::DEPICKLING;
  }
}

void Brineomatic::stop()
{
  if (currentStatus == Status::RUNNING || currentStatus == Status::FLUSHING || currentStatus == Status::PICKLING) {
    stopFlag = true;
  }
}

bool Brineomatic::initializeHardware()
{
  openDiverterValve();

  // zero pressure and wait for it to drop

  uint64_t membranePressureStart = esp_timer_get_time();
  if (membranePressureTarget > 0) {
    setMembranePressureTarget(0);
    while (getMembranePressure() > 50) {
      vTaskDelay(pdMS_TO_TICKS(100));

      if (esp_timer_get_time() - membranePressureStart > membranePressureTimeout)
        return true;
    }
    setMembranePressureTarget(-1);
  }

  disableHighPressurePump();
  disableBoostPump();
  closeFlushValve();

  return false;
}

bool Brineomatic::hasDiverterValve()
{
  return flushValve != nullptr;
}

void Brineomatic::openDiverterValve()
{
  diverterValve->write(diverterValveOpenAngle);
  vTaskDelay(pdMS_TO_TICKS(1000));
  diverterValve->disable();
}

void Brineomatic::closeDiverterValve()
{
  diverterValve->write(diverterValveClosedAngle);
  vTaskDelay(pdMS_TO_TICKS(1000));
  diverterValve->disable();
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

float Brineomatic::getVolume()
{
  return currentVolume;
}

float Brineomatic::getTotalVolume()
{
  return totalVolume;
}

uint64_t Brineomatic::getTotalRuntime()
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

float Brineomatic::getTankLevel()
{
  return currentTankLevel;
}

const char* Brineomatic::getStatus()
{
  if (currentStatus == Status::STARTUP)
    return "STARTUP";
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
    case Result::STARTUP:
      return "STARTUP";
    case Result::SUCCESS:
      return "SUCCESS";
    case Result::USER_STOP:
      return "USER_STOP";
    case Result::ERR_FLUSH_VALVE_TIMEOUT:
      return "ERR_FLUSH_VALVE_TIMEOUT";
    case Result::ERR_FILTER_PRESSURE_TIMEOUT:
      return "ERR_FILTER_PRESSURE_TIMEOUT";
    case Result::ERR_FILTER_PRESSURE_LOW:
      return "ERR_FILTER_PRESSURE_LOW";
    case Result::ERR_FILTER_PRESSURE_HIGH:
      return "ERR_FILTER_PRESSURE_HIGH";
    case Result::ERR_MEMBRANE_PRESSURE_TIMEOUT:
      return "ERR_MEMBRANE_PRESSURE_TIMEOUT";
    case Result::ERR_MEMBRANE_PRESSURE_LOW:
      return "ERR_MEMBRANE_PRESSURE_LOW";
    case Result::ERR_MEMBRANE_PRESSURE_HIGH:
      return "ERR_MEMBRANE_PRESSURE_HIGH";
    case Result::ERR_FLOWRATE_TIMEOUT:
      return "ERR_FLOWRATE_TIMEOUT";
    case Result::ERR_FLOWRATE_LOW:
      return "ERR_FLOWRATE_LOW";
    case Result::ERR_SALINITY_TIMEOUT:
      return "ERR_SALINITY_TIMEOUT";
    case Result::ERR_SALINITY_HIGH:
      return "ERR_SALINITY_HIGH";
    case Result::ERR_PRODUCTION_TIMEOUT:
      return "ERR_PRODUCTION_TIMEOUT";
    default:
      return "UNKNOWN";
  }
}

int64_t Brineomatic::getNextFlushCountdown()
{
  int64_t countdown = nextFlushTime - esp_timer_get_time();
  if (currentStatus == Status::IDLE && autoFlushEnabled && countdown > 0)
    return countdown;

  return 0;
}

int64_t Brineomatic::getRuntimeElapsed()
{
  int64_t elapsed = esp_timer_get_time() - runtimeStart;

  if (currentStatus == Status::RUNNING && elapsed > 0)
    return elapsed;

  return 0;
}

int64_t Brineomatic::getFinishCountdown()
{
  int64_t countdown = (runtimeStart + desiredRuntime) - esp_timer_get_time();
  if (currentStatus == Status::RUNNING && desiredRuntime > 0 && countdown > 0)
    return countdown;

  return 0;
}

int64_t Brineomatic::getFlushElapsed()
{
  int64_t elapsed = esp_timer_get_time() - flushStart;
  if (currentStatus == Status::FLUSHING && elapsed > 0)
    return elapsed;

  return 0;
}

int64_t Brineomatic::getFlushCountdown()
{
  int64_t countdown = (flushStart + flushDuration) - esp_timer_get_time();
  if (currentStatus == Status::FLUSHING && countdown > 0)
    return countdown;

  return 0;
}

int64_t Brineomatic::getPickleElapsed()
{
  int64_t elapsed = esp_timer_get_time() - pickleStart;
  if (currentStatus == Status::PICKLING && elapsed > 0)
    return elapsed;

  return 0;
}

int64_t Brineomatic::getPickleCountdown()
{
  int64_t countdown = (pickleStart + pickleDuration) - esp_timer_get_time();
  if (currentStatus == Status::PICKLING && countdown > 0)
    return countdown;

  return 0;
}

int64_t Brineomatic::getDepickleElapsed()
{
  int64_t elapsed = esp_timer_get_time() - depickleStart;
  if (currentStatus == Status::DEPICKLING && elapsed > 0)
    return elapsed;

  return 0;
}

int64_t Brineomatic::getDepickleCountdown()
{
  int64_t countdown = (depickleStart + depickleDuration) - esp_timer_get_time();
  if (currentStatus == Status::DEPICKLING && countdown > 0)
    return countdown;

  return 0;
}

bool Brineomatic::hasHighPressureValve()
{
  return highPressureValve != nullptr;
}

void Brineomatic::manageHighPressureValve()
{
  float angle;

  if (currentStatus != Status::IDLE) {
    if (hasHighPressureValve()) {
      if (membranePressureTarget >= 0) {

        // change directions if we are too far off our target
        // this is only for RUNNING mode when we are close to our target
        if (currentStatus == Status::RUNNING) {
          if (currentMembranePressure < membranePressureTarget * 0.975 && membranePressurePID.GetDirection() == (uint8_t)QuickPID::Action::reverse) {
            // sendDebug("forward");
            membranePressurePID.SetControllerDirection(QuickPID::Action::direct);
          } else if (currentMembranePressure > membranePressureTarget * 1.025 && membranePressurePID.GetDirection() == (uint8_t)QuickPID::Action::direct) {
            // sendDebug("reverse");
            membranePressurePID.SetControllerDirection(QuickPID::Action::reverse);
          }
        }

        // run our PID calculations
        if (membranePressurePID.Compute()) {
          // forwards / Clockwise / Close valve / Pressure UP
          if (membranePressurePID.GetDirection() == 0)
            angle = map(membranePressurePIDOutput, 0, 255, highPressureValveCloseMin, highPressureValveCloseMax);
          // reverse / Counterclockwise / Open Valve / Pressure DOWN
          else
            angle = map(membranePressurePIDOutput, 0, 255, highPressureValveOpenMin, highPressureValveOpenMax);

          // actually control the valve
          highPressureValve->write(angle);
          // sendDebug("HP PID | current: %.0f / target: %.0f | p: % .3f / i: % .3f / d: % .3f / sum: % .3f | output: %.0f / angle: %.0f | reverse? %d", round(currentMembranePressure), round(membranePressureTarget), membranePressurePID.GetPterm(), membranePressurePID.GetIterm(), membranePressurePID.GetDterm(), membranePressurePID.GetOutputSum(), membranePressurePIDOutput, angle, membranePressurePID.GetDirection());
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
      // sendDebug("State: IDLE");
      //  Serial.println("State: IDLE");
      if (autoFlushEnabled && esp_timer_get_time() > nextFlushTime)
        currentStatus = Status::FLUSHING;
      break;

    //
    // RUNNING
    //
    case Status::RUNNING: {
      sendDebug("State: RUNNING");

      runtimeStart = esp_timer_get_time();
      currentVolume = 0;

      uint64_t lastRuntimeUpdate = runtimeStart;

      if (initializeHardware()) {
        runResult = Result::ERR_MEMBRANE_PRESSURE_TIMEOUT;
        currentStatus = Status::IDLE;
        return;
      }

      uint64_t boostPumpStart = esp_timer_get_time();
      if (hasBoostPump()) {
        sendDebug("Boost Pump Started");
        enableBoostPump();
        while (getFilterPressure() < getFilterPressureMinimum()) {
          if (checkStopFlag())
            return;

          if (esp_timer_get_time() - boostPumpStart > filterPressureTimeout) {
            currentStatus = Status::STOPPING;
            runResult = Result::ERR_FILTER_PRESSURE_TIMEOUT;
            return;
          }

          vTaskDelay(pdMS_TO_TICKS(100));
        }
        sendDebug("Boost Pump OK");
      }

      sendDebug("High Pressure Pump Started");
      enableHighPressurePump();
      setMembranePressureTarget(defaultMembranePressureTarget);

      uint64_t highPressurePumpStart = esp_timer_get_time();
      while (getMembranePressure() < getMembranePressureMinimum()) {

        // check this here in case our PID goes crazy
        if (checkMembranePressureHigh())
          return;

        if (checkStopFlag())
          return;

        if (esp_timer_get_time() - highPressurePumpStart > membranePressureTimeout) {
          currentStatus = Status::STOPPING;
          runResult = Result::ERR_MEMBRANE_PRESSURE_TIMEOUT;
          return;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
      }

      sendDebug("High Pressure Pump OK");

      if (waitForFlowrate())
        return;
      if (waitForSalinity())
        return;

      sendDebug("Closing Diverter Valve.");

      // opening the valve sometimes causes a small blip in either, let it stabilize again
      if (waitForFlowrate())
        return;
      if (waitForSalinity())
        return;

      sendDebug("Flow and Salinity OK");

      uint64_t productionStart = esp_timer_get_time();
      while (true) {

        if (checkFilterPressureLow())
          return;

        if (checkFilterPressureHigh())
          return;

        if (checkMembranePressureLow())
          return;

        if (checkMembranePressureHigh())
          return;

        if (checkFlowrateLow())
          return;

        if (checkSalinityHigh())
          return;

        if (checkStopFlag())
          return;

        if (esp_timer_get_time() - productionStart > productionTimeout) {
          currentStatus = Status::STOPPING;
          runResult = Result::ERR_PRODUCTION_TIMEOUT;
          return;
        }

        // are we going for time?
        if (desiredRuntime > 0 && getRuntimeElapsed() >= desiredRuntime)
          break;

        // are we going for volume?
        if (desiredVolume > 0 && getVolume() >= desiredVolume)
          break;

        // tank level means we're finished successfully
        if (checkTankLevel())
          break;

        // save our total runtime occasionally (every minute)
        if (esp_timer_get_time() - lastRuntimeUpdate > 60000000) {
          totalRuntime += esp_timer_get_time() - lastRuntimeUpdate;
          preferences.putULong64("bomTotRuntime", totalRuntime);
          lastRuntimeUpdate = esp_timer_get_time();
        }

        vTaskDelay(pdMS_TO_TICKS(100));
      }

      sendDebug("Finished making water.");

      // save our total volume produced
      preferences.putFloat("bomTotVolume", totalVolume);

      totalCycles++;
      preferences.putUInt("bomTotCycles", totalCycles);

      runResult = Result::SUCCESS;
      currentStatus = Status::STOPPING;

      break;
    }

    //
    // STOPPING
    //
    case Status::STOPPING: {
      // sendDebug("State: STOPPING");

      if (initializeHardware())
        currentStatus = Status::IDLE;
      else
        currentStatus = Status::FLUSHING;

      break;
    }

    //
    // FLUSHING
    //
    case Status::FLUSHING: {
      sendDebug("State: FLUSHING");

      stopFlag = false;
      flushStart = esp_timer_get_time();

      if (initializeHardware()) {
        currentStatus = Status::IDLE;
        flushResult = Result::ERR_MEMBRANE_PRESSURE_TIMEOUT;
        return;
      }

      openFlushValve();
      uint64_t flushValveStart = esp_timer_get_time();
      while (getFilterPressure() < getFilterPressureMinimum()) {
        if (stopFlag)
          break;

        if (esp_timer_get_time() - flushValveStart > filterPressureTimeout) {
          currentStatus = Status::STOPPING;
          runResult = Result::ERR_FLUSH_VALVE_TIMEOUT;
          return;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
      }

      // short little delay for fresh water valve to fully open
      vTaskDelay(pdMS_TO_TICKS(5000));

      if (!stopFlag) {
        enableHighPressurePump();

        while (getFlushElapsed() < flushDuration) {
          if (stopFlag)
            break;

          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }

      if (initializeHardware()) {
        currentStatus = Status::IDLE;
        flushResult = Result::ERR_MEMBRANE_PRESSURE_TIMEOUT;
        return;
      }

      if (autoFlushEnabled)
        nextFlushTime = esp_timer_get_time() + flushInterval;

      // keep track over restarts.
      preferences.putBool("bomPickled", false);

      if (stopFlag)
        flushResult = Result::USER_STOP;
      else
        flushResult = Result::SUCCESS;
      currentStatus = Status::IDLE;

      break;
    }

    //
    // PICKLING
    //
    case Status::PICKLING:
      sendDebug("State: PICKLING");

      pickleStart = esp_timer_get_time();

      if (initializeHardware()) {
        currentStatus = Status::IDLE;
        pickleResult = Result::ERR_MEMBRANE_PRESSURE_TIMEOUT;
        return;
      }

      enableHighPressurePump();
      while (getPickleElapsed() < pickleDuration) {
        if (stopFlag)
          break;

        vTaskDelay(pdMS_TO_TICKS(100));
      }

      if (initializeHardware()) {
        currentStatus = Status::IDLE;
        pickleResult = Result::ERR_MEMBRANE_PRESSURE_TIMEOUT;
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
      sendDebug("State: DEPICKLING");

      depickleStart = esp_timer_get_time();

      if (initializeHardware()) {
        currentStatus = Status::IDLE;
        depickleResult = Result::ERR_MEMBRANE_PRESSURE_TIMEOUT;
        return;
      }

      enableHighPressurePump();
      while (getDepickleElapsed() < depickleDuration) {
        if (stopFlag)
          break;

        vTaskDelay(pdMS_TO_TICKS(100));
      }

      if (initializeHardware()) {
        currentStatus = Status::IDLE;
        depickleResult = Result::ERR_MEMBRANE_PRESSURE_TIMEOUT;
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

bool Brineomatic::checkStopFlag()
{
  if (stopFlag) {
    currentStatus = Status::STOPPING;
    runResult = Result::USER_STOP;
    return true;
  }

  return false;
}

bool Brineomatic::checkMembranePressureHigh()
{
  if (getMembranePressure() > highPressureMaximum)
    membranePressureHighErrorCount++;
  else
    membranePressureHighErrorCount = 0;

  if (membranePressureHighErrorCount > 10) {
    currentStatus = Status::STOPPING;
    runResult = Result::ERR_MEMBRANE_PRESSURE_HIGH;
    return true;
  }

  return false;
}

bool Brineomatic::checkMembranePressureLow()
{
  if (getMembranePressure() < highPressureMinimum)
    membranePressureLowErrorCount++;
  else
    membranePressureLowErrorCount = 0;

  if (membranePressureLowErrorCount > 10) {

    currentStatus = Status::STOPPING;
    runResult = Result::ERR_MEMBRANE_PRESSURE_LOW;
    return true;
  }

  return false;
}

bool Brineomatic::checkFilterPressureHigh()
{
  if (getFilterPressure() > lowPressureMaximum)
    filterPressureHighErrorCount++;
  else
    filterPressureHighErrorCount = 0;

  if (filterPressureHighErrorCount > 10) {
    currentStatus = Status::STOPPING;
    runResult = Result::ERR_FILTER_PRESSURE_HIGH;
    return true;
  }

  return false;
}

bool Brineomatic::checkFilterPressureLow()
{
  if (getFilterPressure() < lowPressureMinimum)
    filterPressureLowErrorCount++;
  else
    filterPressureLowErrorCount = 0;

  if (filterPressureLowErrorCount > 10) {
    currentStatus = Status::STOPPING;
    runResult = Result::ERR_FILTER_PRESSURE_LOW;
    return true;
  }

  return false;
}

bool Brineomatic::checkFlowrateLow()
{
  if (getFlowrate() < getFlowrateMinimum())
    flowrateLowErrorCount++;
  else
    flowrateLowErrorCount = 0;

  if (flowrateLowErrorCount > 10) {
    currentStatus = Status::STOPPING;
    runResult = Result::ERR_FLOWRATE_LOW;
    return true;
  }

  return false;
}

bool Brineomatic::checkSalinityHigh()
{
  if (getSalinity() > getSalinityMaximum())
    salinityHighErrorCount++;
  else
    salinityHighErrorCount = 0;

  if (salinityHighErrorCount > 10) {
    currentStatus = Status::STOPPING;
    runResult = Result::ERR_SALINITY_HIGH;
    return true;
  }

  return false;
}

bool Brineomatic::waitForFlowrate()
{
  int flowReady = 0;
  uint64_t flowCheckStart = esp_timer_get_time();

  while (flowReady < 10) {
    if (checkMembranePressureHigh())
      return false;
    if (checkStopFlag())
      return false;

    if (getFlowrate() > getFlowrateMinimum())
      flowReady++;
    else
      flowReady = 0;

    if (esp_timer_get_time() - flowCheckStart > flowRateTimeout) {
      currentStatus = Status::STOPPING;
      runResult = Result::ERR_FLOWRATE_TIMEOUT;
      return true;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }

  return false;
}

bool Brineomatic::waitForSalinity()
{
  int salinityReady = 0;
  uint64_t salinityCheckStart = esp_timer_get_time();
  while (salinityReady < 10) {
    if (checkMembranePressureHigh())
      return false;
    if (checkStopFlag())
      return false;

    if (getSalinity() < getSalinityMaximum())
      salinityReady++;
    else
      salinityReady = 0;

    if (esp_timer_get_time() - salinityCheckStart > salinityTimeout) {
      currentStatus = Status::STOPPING;
      runResult = Result::ERR_SALINITY_TIMEOUT;
      return true;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }

  return false;
}

bool Brineomatic::checkTankLevel()
{
  return currentTankLevel >= 0.95;
}

#endif