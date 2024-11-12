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
float water_temperature = 25;

void brineomatic_setup()
{
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
  brineomatic_adc.setDataRate(2); // 32 samples per second.

  current_ads1115_channel = 0;
  brineomatic_adc.requestADC(current_ads1115_channel);

  gravityTds.setAref(YB_ADS1115_VREF); // reference voltage on ADC
  gravityTds.setAdcRange(15);          // 16 bit ADC, but its differential, so lose 1 bit.
  gravityTds.begin();                  // initialization

  char prefIndex[YB_PREF_KEY_LENGTH];

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
    detachInterrupt(digitalPinToInterrupt(YB_FLOWMETER_PIN));
    pulse_counter = 0;
    attachInterrupt(digitalPinToInterrupt(YB_FLOWMETER_PIN), flowmeter_interrupt, FALLING);

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
  gravityTds.setTemperature(water_temperature); // set the temperature and execute temperature compensation
  gravityTds.update(reading);                   // sample and calculate
  float tdsReading = gravityTds.getTdsValue();  // then get the value
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
  // enabled or no
  if (preferences.isKey("bomPickled"))
    isPickled = preferences.getBool("bomPickled");
  else
    isPickled = false;

  if (preferences.isKey("bomTotVolume"))
    totalVolume = preferences.getFloat("bomTotalVolume");
  else
    totalVolume = 0.0;

  if (preferences.isKey("bomTotRuntime"))
    totalRuntime = preferences.getFloat("bomTotRuntime");
  else
    totalRuntime = 0.0;

  autoFlushEnabled = true;
  diversionValveOpen = false;
  highPressurePumpEnabled = false;
  boostPumpEnabled = false;
  flushValveOpen = false;
  currentTemperature = 0.0;
  currentFlowrate = 0.0;
  currentVolume = 0.0;
  currentSalinity = 0.0;
  currentFilterPressure = 0.0;
  currentMembranePressure = 0.0;
  membranePressureTarget = -1;
  currentStatus = Status::STARTUP;

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

void Brineomatic::stop()
{
  if (currentStatus == Status::RUNNING || currentStatus == Status::FLUSHING || currentStatus == Status::PICKLING) {
    stopFlag = true;
  }
}

void Brineomatic::initializeHardware()
{
  openDiverterValve();

  // zero pressure and wait for it to drop
  if (membranePressureTarget > 0) {
    setMembranePressureTarget(0);
    while (getMembranePressure() > 50) {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    setMembranePressureTarget(-1);
  }

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
            sendDebug("forward");
            membranePressurePID.SetControllerDirection(QuickPID::Action::direct);
          } else if (currentMembranePressure > membranePressureTarget * 1.025 && membranePressurePID.GetDirection() == (uint8_t)QuickPID::Action::direct) {
            sendDebug("reverse");
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
          sendDebug("HP PID | current: %.0f / target: %.0f | p: % .3f / i: % .3f / d: % .3f / sum: % .3f | output: %.0f / angle: %.0f | reverse? %d", round(currentMembranePressure), round(membranePressureTarget), membranePressurePID.GetPterm(), membranePressurePID.GetIterm(), membranePressurePID.GetDterm(), membranePressurePID.GetOutputSum(), membranePressurePIDOutput, angle, membranePressurePID.GetDirection());
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
      // Serial.println("State: IDLE");
      if (autoFlushEnabled && esp_timer_get_time() > nextFlushTime)
        currentStatus = Status::FLUSHING;
      break;

    //
    // RUNNING
    //
    case Status::RUNNING: {
      // sendDebug("State: RUNNING");

      runtimeStart = esp_timer_get_time();
      currentVolume = 0;

      initializeHardware();

      if (hasBoostPump()) {
        // sendDebug("Boost Pump Started");
        enableBoostPump();
        while (getFilterPressure() < getFilterPressureMinimum()) {
          if (stopFlag) {
            initializeHardware();
            currentStatus = Status::FLUSHING;
            return;
          }
          vTaskDelay(pdMS_TO_TICKS(100));
        }
        // sendDebug("Boost Pump OK");
      }

      // sendDebug("High Pressure Pump Started");
      enableHighPressurePump();
      setMembranePressureTarget(defaultMembranePressureTarget);

      while (getMembranePressure() < getMembranePressureMinimum()) {
        if (getMembranePressure() > highPressureMaximum)
          stopFlag = true;
        if (stopFlag) {
          initializeHardware();
          currentStatus = Status::FLUSHING;
          return;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
      }

      // both flowrate and salinity need to be good
      bool ready = false;
      while (!ready) {
        // if (getMembranePressure() > highPressureMaximum)
        //   stopFlag = true;

        ready = true;
        if (stopFlag) {
          initializeHardware();
          currentStatus = Status::FLUSHING;
          return;
        }

        if (getFlowrate() < getFlowrateMinimum())
          ready = false;
        if (getSalinity() > getSalinityMaximum())
          ready = false;

        vTaskDelay(pdMS_TO_TICKS(100));
      }

      // sendDebug("Flow and Salinity OK");

      closeDiverterValve();

      while ((desiredRuntime == 0 && desiredVolume == 0) || (getRuntimeElapsed() < desiredRuntime) || (getVolume() < desiredVolume)) {

        // if (getMembranePressure() > highPressureMaximum)
        //   stopFlag = true;
        // if (getFlowrate() < getFlowrateMinimum())
        //   stopFlag = true;
        // if (getSalinity() > getSalinityMaximum())
        //   stopFlag = true;

        if (stopFlag) {
          currentStatus = Status::FLUSHING;
          initializeHardware();
          return;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
      }

      // save our total volume produced
      // preferences.putFloat("bomTotalVolume", totalVolume);

      // sendDebug("Finished making water.");

      initializeHardware();
      currentStatus = Status::FLUSHING;

      break;
    }

    //
    // FLUSHING
    //
    case Status::FLUSHING: {

      stopFlag = false;
      flushStart = esp_timer_get_time();

      initializeHardware();

      openFlushValve();
      while (getFilterPressure() < getFilterPressureMinimum()) {
        if (stopFlag)
          break;

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

      initializeHardware();

      if (autoFlushEnabled)
        nextFlushTime = esp_timer_get_time() + flushInterval;

      // keep track over restarts.
      isPickled = false;
      preferences.putBool("bomPickled", false);

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
      while (getPickleElapsed() < pickleDuration) {
        if (stopFlag)
          break;

        vTaskDelay(pdMS_TO_TICKS(100));
      }

      initializeHardware();

      currentStatus = Status::PICKLED;

      // keep track over restarts.
      preferences.putBool("bomPickled", true);

      break;
  }
}
#endif