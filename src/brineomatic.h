/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_BRINEOMATIC_H
#define YARR_BRINEOMATIC_H

#include "Flowmeter.h"
#include "GD20Modbus.h"
#include "adchelper.h"
#include "brineomatic_config.h"
#include "etl/deque.h"
#include <ADS1X15.h>
#include <ArduinoJson.h>
#include <DallasTemperature.h>
#include <GravityTDS.h>
#include <OneWire.h>
#include <QuickPID.h>

class YarrboardApp;
class RelayChannel;
class ServoChannel;
class StepperChannel;
class RelayController;
class ServoController;
class StepperController;

class Brineomatic
{
  public:
    enum class Status {
      STARTUP,
      MANUAL,
      IDLE,
      RUNNING,
      STOPPING,
      FLUSHING,
      PICKLING,
      PICKLED,
      DEPICKLING
    };

// Master list of all Result enum entries (single source of truth)
#define BOM_RESULT_LIST            \
  X(STARTUP)                       \
  X(SUCCESS)                       \
  X(SUCCESS_TIME)                  \
  X(SUCCESS_VOLUME)                \
  X(SUCCESS_TANK_LEVEL)            \
  X(SUCCESS_SALINITY)              \
  X(USER_STOP)                     \
  X(ERR_FILTER_PRESSURE_TIMEOUT)   \
  X(ERR_FILTER_PRESSURE_LOW)       \
  X(ERR_FILTER_PRESSURE_HIGH)      \
  X(ERR_MEMBRANE_PRESSURE_TIMEOUT) \
  X(ERR_MEMBRANE_PRESSURE_LOW)     \
  X(ERR_MEMBRANE_PRESSURE_HIGH)    \
  X(ERR_PRODUCT_FLOWRATE_TIMEOUT)  \
  X(ERR_PRODUCT_FLOWRATE_LOW)      \
  X(ERR_PRODUCT_FLOWRATE_HIGH)     \
  X(ERR_FLUSH_FLOWRATE_LOW)        \
  X(ERR_FLUSH_FILTER_PRESSURE_LOW) \
  X(ERR_FLUSH_VALVE_ON)            \
  X(ERR_FLUSH_TIMEOUT)             \
  X(ERR_BRINE_FLOWRATE_LOW)        \
  X(ERR_TOTAL_FLOWRATE_LOW)        \
  X(ERR_DIVERTER_VALVE_OPEN)       \
  X(ERR_PRODUCT_SALINITY_TIMEOUT)  \
  X(ERR_PRODUCT_SALINITY_HIGH)     \
  X(ERR_PRODUCTION_TIMEOUT)        \
  X(ERR_MOTOR_TEMPERATURE_HIGH)

    enum class Result {
#define X(name) name,
      BOM_RESULT_LIST
#undef X
    };

    // Static lookup tables
    static constexpr const char* const AUTOFLUSH_MODES[] = {"NONE", "TIME", "SALINITY", "VOLUME"};
    static constexpr const char* const TEMPERATURE_UNITS[] = {"celsius", "fahrenheit"};
    static constexpr const char* const PRESSURE_UNITS[] = {"pascal", "psi", "bar"};
    static constexpr const char* const VOLUME_UNITS[] = {"liters", "gallons"};
    static constexpr const char* const FLOWRATE_UNITS[] = {"lph", "gph"};

    static constexpr const char* BOOST_PUMP_CONTROLS[] = {"NONE", "MANUAL", "RELAY"};
    static constexpr const char* HIGH_PRESSURE_PUMP_CONTROLS[] = {"NONE", "MANUAL", "RELAY", "MODBUS"};
    static constexpr const char* HIGH_PRESSURE_PUMP_MODBUS_DEVICES[] = {"GD20"};
    static constexpr const char* HIGH_PRESSURE_VALVE_CONTROLS[] = {"NONE", "MANUAL", "STEPPER"};
    static constexpr const char* DIVERTER_VALVE_CONTROLS[] = {"NONE", "MANUAL", "SERVO"};
    static constexpr const char* FLUSH_VALVE_CONTROLS[] = {"NONE", "MANUAL", "RELAY"};
    static constexpr const char* COOLING_FAN_CONTROLS[] = {"NONE", "MANUAL", "RELAY"};

    bool isPickled;
    int64_t pickledOnTimestamp = 0;

    RelayChannel* flushValve = NULL;
    RelayChannel* boostPump = NULL;
    RelayChannel* highPressurePump = NULL;
    RelayChannel* coolingFan = NULL;
    ServoChannel* diverterValve = NULL;
    StepperChannel* highPressureValveStepper = NULL;

    OneWire oneWire;
    DallasTemperature ds18b20;
    DeviceAddress motorThermometer;

    Flowmeter productFlowmeter;
    Flowmeter brineFlowmeter;

    GravityTDS gravityTds;

    ADS1115 _adc;
    ADS1115Helper* adcHelper;

    GD20Modbus* gd20;

    // float membranePressurePIDOutput;
    // QuickPID membranePressurePID;
    // float KpRamp = 0;
    // float KiRamp = 0;
    // float KdRamp = 0;
    // float KpMaintain = 0;
    // float KiMaintain = 0;
    // float KdMaintain = 0;

    float currentVolume;
    float currentFlushVolume;

    uint32_t totalCycles;
    float totalVolume;
    uint32_t totalRuntime; // seconds

    Brineomatic(YarrboardApp& app, RelayController& relays, ServoController& servos, StepperController& steppers);
    void init();
    void initChannels();
    void initModbus();

    void loop();

    void measureMotorTemperature();
    void measureProductFlowmeter();
    void measureBrineFlowmeter();
    void measureProductSalinity();
    void measureBrineSalinity();
    void measureFilterPressure();
    void measureMembranePressure();

    void setMembranePressureTarget(float pressure);
    void setWaterTemperature(float temp);
    void setTankLevel(float level);

    void idle();
    void manual();
    void start();
    void startDuration(uint32_t duration);
    void startVolume(float volume);
    void flush();
    void flushDuration(uint32_t duration);
    void flushVolume(float volume);
    void pickle(uint32_t duration);
    void depickle(uint32_t duration);
    void stop();

    bool initializeHardware();

    bool autoflushEnabled();

    bool isBoostPumpOn();
    bool hasBoostPump();
    void enableBoostPump();
    void disableBoostPump();

    bool isHighPressurePumpOn();
    bool hasHighPressurePump();
    void enableHighPressurePump();
    void disableHighPressurePump();
    void modbusEnableHighPressurePump();
    void modbusDisableHighPressurePump();

    bool isDiverterValveOpen();
    bool hasDiverterValve();
    void openDiverterValve();
    void closeDiverterValve();

    bool isFlushValveOpen();
    bool hasFlushValve();
    void openFlushValve();
    void closeFlushValve();

    bool isCoolingFanOn();
    bool hasCoolingFan();
    void enableCoolingFan();
    void disableCoolingFan();
    void manageCoolingFan();

    bool hasHighPressureValve();
    void manageHighPressureValve();

    const char* getStatus();
    const char* resultToString(Result result);
    Result getRunResult();
    Result getFlushResult();
    Result getPickleResult();
    Result getDepickleResult();

    uint32_t getNextFlushCountdown();
    uint32_t getRuntimeElapsed();
    uint32_t getFinishCountdown();
    uint32_t getFlushElapsed();
    uint32_t getFlushCountdown();
    uint32_t getPickleElapsed();
    uint32_t getPickleCountdown();
    uint32_t getDepickleElapsed();
    uint32_t getDepickleCountdown();

    float getFilterPressure();
    float getFilterPressureMinimum();
    float getMembranePressure();
    float getMembranePressureMinimum();
    float getProductFlowrate();
    float getProductFlowrateMinimum();
    float getBrineFlowrate();
    float getTotalFlowrate();
    float getTotalFlowrateMinimum();
    uint32_t getTotalCycles();
    float getVolume();
    float getFlushVolume();
    float getTotalVolume();
    uint32_t getTotalRuntime();
    float getMotorTemperature();
    float getWaterTemperature();
    float getProductSalinity();
    float getProductSalinityMaximum();
    float getBrineSalinity();
    float getTankLevel();
    float getTankCapacity();
    float getMotorTemperatureMaximum();

    void runStateMachine();

    void generateUpdateJSON(JsonVariant output);
    void generateConfigJSON(JsonVariant output);

    bool validateConfigJSON(JsonVariant config, char* error, size_t err_size);
    bool validateGeneralConfigJSON(JsonVariant config, char* error, size_t err_size);
    bool validateHardwareConfigJSON(JsonVariant config, char* error, size_t err_size);
    bool validateSafeguardsConfigJSON(JsonVariant config, char* error, size_t err_size);

    void loadConfigJSON(JsonVariant config);
    void loadGeneralConfigJSON(JsonVariant config);
    void loadHardwareConfigJSON(JsonVariant config);
    void loadSafeguardsConfigJSON(JsonVariant config);

    void updateMQTT();

  private:
    YarrboardApp& _app;
    RelayController& _relays;
    ServoController& _servos;
    StepperController& _steppers;

    Status currentStatus;
    Result runResult;
    Result flushResult;
    Result pickleResult;
    Result depickleResult;

    bool stopFlag = false;

    float desiredFlushVolume = 0;
    uint32_t desiredFlushDuration = 0;
    uint32_t desiredRuntime = 0;
    float desiredVolume = 0;

    // all these times are in milliseconds
    uint32_t runtimeStart = 0;
    uint32_t runtimeElapsed = 0;
    uint32_t flushStart = 0;
    uint32_t lastAutoflushTimeMillis = 0;
    int64_t lastAutoflushTimeNTP = 0;
    uint32_t pickleStart = 0;
    uint32_t pickleDuration;
    uint32_t depickleStart = 0;
    uint32_t depickleDuration;

    bool boostPumpOnState;
    bool highPressurePumpOnState;
    bool diverterValveOpenState;
    bool flushValveOpenState;
    bool coolingFanOnState;

    float currentTankLevel;
    float currentWaterTemperature;
    float currentMotorTemperature;
    float currentProductFlowrate;
    float currentBrineFlowrate;
    float currentProductSalinity;
    float currentBrineSalinity;
    float currentFilterPressure;
    float currentMembranePressure;
    float currentMembranePressureTarget;

    // tracking when we first saw the error condition
    uint32_t membranePressureHighStart = 0;
    uint32_t membranePressureLowStart = 0;
    uint32_t filterPressureHighStart = 0;
    uint32_t filterPressureLowStart = 0;
    uint32_t productFlowrateLowStart = 0;
    uint32_t productFlowrateHighStart = 0;
    uint32_t brineFlowrateLowStart = 0;
    uint32_t totalFlowrateLowStart = 0;
    uint32_t flushFilterPressureLowStart = 0;
    uint32_t flushFlowrateLowStart = 0;
    uint32_t diverterValveOpenStart = 0;
    uint32_t productSalinityHighStart = 0;
    uint32_t motorTemperatureStart = 0;

    //
    // Configuration variables
    //
    String autoflushMode = YB_AUTOFLUSH_MODE;
    float autoflushSalinity = YB_AUTOFLUSH_SALINITY;
    uint32_t autoflushDuration = YB_AUTOFLUSH_DURATION;
    float autoflushVolume = YB_AUTOFLUSH_VOLUME;
    uint32_t autoflushInterval = YB_AUTOFLUSH_INTERVAL;
    bool autoflushUseHighPressureMotor = YB_AUTOFLUSH_USE_HIGH_PRESSURE_MOTOR;

    float tankLevelFull = 0.99;            // 0 = empty, 1 = full
    float tankCapacity = YB_TANK_CAPACITY; // Liters

    String temperatureUnits = YB_TEMPERATURE_UNITS;
    String pressureUnits = YB_PRESSURE_UNITS;
    String volumeUnits = YB_VOLUME_UNITS;
    String flowrateUnits = YB_FLOWRATE_UNITS;

    String successMelody = YB_SUCCESS_MELODY;
    String errorMelody = YB_ERROR_MELODY;

    String boostPumpControl = YB_BOOST_PUMP_CONTROL;
    uint8_t boostPumpRelayId = YB_BOOST_PUMP_RELAY_ID;

    String highPressurePumpControl = YB_HIGH_PRESSURE_PUMP_CONTROL;
    uint8_t highPressureRelayId = YB_HIGH_PRESSURE_RELAY_ID;
    String highPressurePumpModbusDevice = YB_HIGH_PRESSURE_PUMP_MODBUS_DEVICE;
    uint8_t highPressurePumpModbusSlaveId = YB_HIGH_PRESSURE_PUMP_MODBUS_SLAVE_ID;
    float highPressurePumpModbusFrequency = YB_HIGH_PRESSURE_PUMP_MODBUS_FREQUENCY;

    String highPressureValveControl = YB_HIGH_PRESSURE_VALVE_CONTROL;
    uint8_t highPressureValveStepperId = YB_HIGH_PRESSURE_VALVE_STEPPER_ID;
    float highPressureValveStepperStepAngle = YB_HIGH_PRESSURE_VALVE_STEPPER_STEP_ANGLE;
    float highPressureValveStepperGearRatio = YB_HIGH_PRESSURE_VALVE_STEPPER_GEAR_RATIO;
    float highPressureValveStepperCloseAngle = YB_HIGH_PRESSURE_VALVE_STEPPER_CLOSE_ANGLE;
    float highPressureValveStepperCloseSpeed = YB_HIGH_PRESSURE_VALVE_STEPPER_CLOSE_SPEED;
    float highPressureValveStepperOpenAngle = YB_HIGH_PRESSURE_VALVE_STEPPER_OPEN_ANGLE;
    float highPressureValveStepperOpenSpeed = YB_HIGH_PRESSURE_VALVE_STEPPER_OPEN_SPEED;
    float membranePressureTarget = YB_MEMBRANE_PRESSURE_TARGET; // PSI

    String diverterValveControl = YB_DIVERTER_VALVE_CONTROL;
    uint8_t diverterValveServoId = YB_DIVERTER_VALVE_SERVO_ID;
    float diverterValveOpenAngle = YB_DIVERTER_VALVE_OPEN_ANGLE;
    float diverterValveCloseAngle = YB_DIVERTER_VALVE_CLOSE_ANGLE;

    String flushValveControl = YB_FLUSH_VALVE_CONTROL;
    uint8_t flushValveRelayId = YB_FLUSH_VALVE_RELAY_ID;

    String coolingFanControl = YB_COOLING_FAN_CONTROL;
    uint8_t coolingFanRelayId = YB_COOLING_FAN_RELAY_ID;
    float coolingFanOnTemperature = YB_COOLING_FAN_ON_TEMPERATURE;   // Celcius
    float coolingFanOffTemperature = YB_COOLING_FAN_OFF_TEMPERATURE; // Celcius

    bool hasMembranePressureSensor = YB_HAS_MEMBRANE_PRESSURE_SENSOR;
    float membranePressureSensorMin = YB_MEMBRANE_PRESSURE_SENSOR_MIN;
    float membranePressureSensorMax = YB_MEMBRANE_PRESSURE_SENSOR_MAX;

    bool hasFilterPressureSensor = YB_HAS_FILTER_PRESSURE_SENSOR;
    float filterPressureSensorMin = YB_FILTER_PRESSURE_SENSOR_MIN;
    float filterPressureSensorMax = YB_FILTER_PRESSURE_SENSOR_MAX;

    bool hasProductTDSSensor = YB_HAS_PRODUCT_TDS_SENSOR;
    bool hasBrineTDSSensor = YB_HAS_BRINE_TDS_SENSOR;

    bool hasProductFlowSensor = YB_HAS_PRODUCT_FLOW_SENSOR;
    float productFlowmeterPPL = YB_PRODUCT_FLOWMETER_PPL;

    bool hasBrineFlowSensor = YB_HAS_BRINE_FLOW_SENSOR;
    float brineFlowmeterPPL = YB_BRINE_FLOWMETER_PPL;

    bool hasMotorTemperatureSensor = YB_HAS_MOTOR_TEMPERATURE_SENSOR;

    uint32_t flushTimeout = YB_FLUSH_TIMEOUT;                          // timeout for flush cycle in ms
    uint32_t membranePressureTimeout = YB_MEMBRANE_PRESSURE_TIMEOUT;   // timeout for membrane pressure to stabilize in ms
    uint32_t productFlowrateTimeout = YB_PRODUCT_FLOWRATE_TIMEOUT;     // timeout for product flowrate to stabilize in ms
    uint32_t productSalinityTimeout = YB_PRODUCT_SALINITY_TIMEOUT;     // timeout for salinity to stabilize in ms
    uint32_t productionRuntimeTimeout = YB_PRODUCTION_RUNTIME_TIMEOUT; // maximum length of run in ms

    bool enableMembranePressureHighCheck = YB_ENABLE_MEMBRANE_PRESSURE_HIGH_CHECK;
    float membranePressureHighThreshold = YB_MEMBRANE_PRESSURE_HIGH_THRESHOLD;
    uint32_t membranePressureHighDelay = YB_MEMBRANE_PRESSURE_HIGH_DELAY;

    bool enableMembranePressureLowCheck = YB_ENABLE_MEMBRANE_PRESSURE_LOW_CHECK;
    float membranePressureLowThreshold = YB_MEMBRANE_PRESSURE_LOW_THRESHOLD;
    uint32_t membranePressureLowDelay = YB_MEMBRANE_PRESSURE_LOW_DELAY;

    bool enableFilterPressureHighCheck = YB_ENABLE_FILTER_PRESSURE_HIGH_CHECK;
    float filterPressureHighThreshold = YB_FILTER_PRESSURE_HIGH_THRESHOLD;
    uint32_t filterPressureHighDelay = YB_FILTER_PRESSURE_HIGH_DELAY;

    bool enableFilterPressureLowCheck = YB_ENABLE_FILTER_PRESSURE_LOW_CHECK;
    float filterPressureLowThreshold = YB_FILTER_PRESSURE_LOW_THRESHOLD;
    uint32_t filterPressureLowDelay = YB_FILTER_PRESSURE_LOW_DELAY;

    bool enableProductFlowrateHighCheck = YB_ENABLE_PRODUCT_FLOWRATE_HIGH_CHECK;
    float productFlowrateHighThreshold = YB_PRODUCT_FLOWRATE_HIGH_THRESHOLD;
    uint32_t productFlowrateHighDelay = YB_PRODUCT_FLOWRATE_HIGH_DELAY;

    bool enableProductFlowrateLowCheck = YB_ENABLE_PRODUCT_FLOWRATE_LOW_CHECK;
    float productFlowrateLowThreshold = YB_PRODUCT_FLOWRATE_LOW_THRESHOLD;
    uint32_t productFlowrateLowDelay = YB_PRODUCT_FLOWRATE_LOW_DELAY;

    bool enableRunTotalFlowrateLowCheck = YB_ENABLE_RUN_TOTAL_FLOWRATE_LOW_CHECK;
    float runTotalFlowrateLowThreshold = YB_RUN_TOTAL_FLOWRATE_LOW_THRESHOLD;
    uint32_t runTotalFlowrateLowDelay = YB_RUN_TOTAL_FLOWRATE_LOW_DELAY;

    bool enablePickleTotalFlowrateLowCheck = YB_ENABLE_PICKLE_TOTAL_FLOWRATE_LOW_CHECK;
    float pickleTotalFlowrateLowThreshold = YB_PICKLE_TOTAL_FLOWRATE_LOW_THRESHOLD;
    uint32_t pickleTotalFlowrateLowDelay = YB_PICKLE_TOTAL_FLOWRATE_LOW_DELAY;

    bool enableDiverterValveClosedCheck = YB_ENABLE_DIVERTER_VALVE_CLOSED_CHECK;
    float diverterValveClosedDelay = YB_DIVERTER_VALVE_CLOSED_DELAY;

    bool enableProductSalinityHighCheck = YB_ENABLE_PRODUCT_SALINITY_HIGH_CHECK;
    float productSalinityHighThreshold = YB_PRODUCT_SALINITY_HIGH_THRESHOLD;
    uint32_t productSalinityHighDelay = YB_PRODUCT_SALINITY_HIGH_DELAY;

    bool enableMotorTemperatureCheck = YB_ENABLE_MOTOR_TEMPERATURE_CHECK;
    float motorTemperatureHighThreshold = YB_MOTOR_TEMPERATURE_HIGH_THRESHOLD;
    uint32_t motorTemperatureHighDelay = YB_MOTOR_TEMPERATURE_HIGH_DELAY;

    bool enableFlushFlowrateLowCheck = YB_ENABLE_FLUSH_FLOWRATE_LOW_CHECK;
    float flushFlowrateLowThreshold = YB_FLUSH_FLOWRATE_LOW_THRESHOLD;
    uint32_t flushFlowrateLowDelay = YB_FLUSH_FLOWRATE_LOW_DELAY;

    bool enableFlushFilterPressureLowCheck = YB_ENABLE_FLUSH_FILTER_PRESSURE_LOW_CHECK;
    float flushFilterPressureLowThreshold = YB_FLUSH_FILTER_PRESSURE_LOW_THRESHOLD;
    uint32_t flushFilterPressureLowDelay = YB_FLUSH_FILTER_PRESSURE_LOW_DELAY;

    bool enableFlushValveOffCheck = YB_ENABLE_FLUSH_VALVE_OFF_CHECK;
    float flushValveOffThreshold = YB_FLUSH_VALVE_OFF_THRESHOLD;
    uint32_t flushValveOffDelay = YB_FLUSH_VALVE_OFF_DELAY;

    void resetErrorTimers();

    bool checkStopFlag(Result& result);
    bool checkTankLevel();
    bool checkMembranePressureHigh();
    bool checkMembranePressureLow();
    bool checkFilterPressureHigh();
    bool checkFilterPressureLow();
    bool checkProductFlowrateLow();
    bool checkProductFlowrateHigh();
    bool checkPickleTotalFlowrateLow(Result& result);
    bool checkRunTotalFlowrateLow();
    bool checkFlushFilterPressureLow();
    bool checkFlushFlowrateLow();
    bool checkDiverterValveClosed();
    bool checkProductSalinityHigh();
    bool checkMotorTemperature(Result& result);
    bool waitForMembranePressure();
    bool waitForProductFlowrate();
    bool waitForProductSalinity();
    bool waitForFlushValveOff();

    bool checkTimedError(bool condition,
      uint32_t& startTime,
      uint32_t timeout,
      Result errorResult,
      Result& result);
};

template <class X, class M, class N, class O, class Q>
X map_generic(X x, M in_min, N in_max, O out_min, Q out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif /* !YARR_BRINEOMATIC_H */