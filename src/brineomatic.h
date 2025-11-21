/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_BRINEOMATIC_H
#define YARR_BRINEOMATIC_H

#include "etl/deque.h"
#include <ArduinoJson.h>
#include <QuickPID.h>

class RelayChannel;
class ServoChannel;
class StepperChannel;

extern etl::deque<float, YB_BOM_DATA_SIZE> motor_temperature_data;
extern etl::deque<float, YB_BOM_DATA_SIZE> water_temperature_data;
extern etl::deque<float, YB_BOM_DATA_SIZE> filter_pressure_data;
extern etl::deque<float, YB_BOM_DATA_SIZE> membrane_pressure_data;
extern etl::deque<float, YB_BOM_DATA_SIZE> product_salinity_data;
extern etl::deque<float, YB_BOM_DATA_SIZE> brine_salinity_data;
extern etl::deque<float, YB_BOM_DATA_SIZE> product_flowrate_data;
extern etl::deque<float, YB_BOM_DATA_SIZE> brine_flowrate_data;
extern etl::deque<float, YB_BOM_DATA_SIZE> tank_level_data;

void brineomatic_setup();
void brineomatic_loop();

void brineomatic_state_machine(void* pvParameters);

void measure_product_flowmeter();
void measure_brine_flowmeter();
void measure_temperature();
void measure_product_salinity(int16_t reading);
void measure_brine_salinity(int16_t reading);
void measure_filter_pressure(int16_t reading);
void measure_membrane_pressure(int16_t reading);

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

    enum class HighPressureValveControlMode {
      MANUAL,
      SERVO,
      STEPPER_POSITION,
      STEPPER_PID
    };

    bool isPickled;
    bool autoFlushEnabled;
    bool flushUseHighPressureMotor = false;

    RelayChannel* flushValve = NULL;
    RelayChannel* boostPump = NULL;
    RelayChannel* highPressurePump = NULL;
    RelayChannel* coolingFan = NULL;
    ServoChannel* diverterValve = NULL;

    HighPressureValveControlMode highPressureValveMode;
    ServoChannel* highPressureValveServo = NULL;
    StepperChannel* highPressureValveStepper = NULL;

    float highPressureValveOpenMin;
    float highPressureValveOpenMax;
    float highPressureValveCloseMin;
    float highPressureValveCloseMax;
    float highPressureValveMaintainOpenMin;
    float highPressureValveMaintainOpenMax;
    float highPressureValveMaintainCloseMin;
    float highPressureValveMaintainCloseMax;

    float KpRamp = 0;
    float KiRamp = 0;
    float KdRamp = 0;
    float KpMaintain = 0;
    float KiMaintain = 0;
    float KdMaintain = 0;

    float currentVolume;
    float currentFlushVolume;

    uint32_t totalCycles;
    float totalVolume;
    uint32_t totalRuntime; // seconds

    Brineomatic();
    void init();

    void setFilterPressure(float pressure);
    void setMembranePressure(float pressure);
    void setMembranePressureTarget(float pressure);
    void setProductFlowrate(float flowrate);
    void setBrineFlowrate(float flowrate);
    void setMotorTemperature(float temp);
    void setWaterTemperature(float temp);
    void setProductSalinity(float salinity);
    void setBrineSalinity(float salinity);
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

    bool isBoostPumpOn();
    bool hasBoostPump();
    void enableBoostPump();
    void disableBoostPump();

    bool isHighPressurePumpOn();
    bool hasHighPressurePump();
    void enableHighPressurePump();
    void disableHighPressurePump();

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

    void updateMQTT();

  private:
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
    uint32_t lastAutoFlushTime = 0;
    uint32_t pickleStart = 0;
    uint32_t depickleStart = 0;
    uint32_t pickleDuration;
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

    float membranePressurePIDOutput;
    QuickPID membranePressurePID;

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
    float tankLevelFull = 0.99;            // 0 = empty, 1 = full
    float tankCapacity = 780;              // Liters
    float coolingFanOnTemperature = 35.0;  // Celcius
    float coolingFanOffTemperature = 34.0; // Celcius

    const char* autoflushMode;
    float autoflushSalinity = 750.0;
    uint32_t autoflushDuration = 3 * 60 * 1000;
    float autoflushVolume = 15.0;
    uint32_t autoflushInterval = 3 * 24 * 60 * 60 * 1000;

    const char* temperatureUnits = "celsius";
    const char* pressureUnits = "psi";
    const char* volumeUnits = "liters";
    const char* flowrateUnits = "lph";

    const char* successMelody = "SUCCESS";
    const char* errorMelody = "ERROR";

    const char* boostPumpControl = "none";
    uint8_t boostPumpRelayId = 1;

    const char* highPressurePumpControl = "relay";
    uint8_t highPressureRelayId = 4;

    const char* highPressureValveControl = "stepper_position";
    uint8_t highPressureValveStepperId = 1;
    float highPressureValveStepperStepAngle = 1.8;
    float highPressureValveStepperGearRatio = 3.0;
    float highPressureValveStepperCloseAngle = 1660.0;
    float highPressureValveStepperCloseSpeed = 10.0;
    float highPressureValveStepperOpenAngle = 0.0;
    float highPressureValveStepperOpenSpeed = 40.0;
    float membranePressureTarget = 800.0; // PSI

    const char* diverterValveControl = "servo";
    uint8_t diverterValveServoId = 1;
    float diverterValveOpenAngle = 35;
    float diverterValveCloseAngle = 125;

    const char* flushValveControl = "relay";
    uint8_t flushValveRelayId = 2;

    const char* coolingFanControl = "relay";
    uint8_t coolingFanRelayId = 3;

    bool hasMembranePressureSensor = true;
    float membranePressureSensorMin = 0.0;
    float membranePressureSensorMax = YB_HP_SENSOR_MAX;

    bool hasFilterPressureSensor = true;
    float filterPressureSensorMin = 0.0;
    float filterPressureSensorMax = YB_LP_SENSOR_MAX;

    bool hasProductTDSSensor = true;
    bool hasBrineTDSSensor = true;

    bool hasProductFlowSensor = true;
    float productFlowmeterPPL = YB_PRODUCT_FLOWMETER_DEFAULT_PPL;

    bool hasBrineFlowSensor = true;
    float brineFlowmeterPPL = YB_BRINE_FLOWMETER_DEFAULT_PPL;

    bool hasMotorTemperatureSensor = true;

    bool enableMembranePressureHighCheck = true;
    float membranePressureHighThreshold = 900.0;
    uint32_t membranePressureHighDelay = 2000;

    bool enableMembranePressureLowCheck = true;
    float membranePressureLowThreshold = 700.0;
    uint32_t membranePressureLowDelay = 2000;

    bool enableFilterPressureHighCheck = true;
    float filterPressureHighThreshold = 60.0;
    uint32_t filterPressureHighDelay = 2000;

    bool enableFilterPressureLowCheck = true;
    float filterPressureLowThreshold = 2.5;
    uint32_t filterPressureLowDelay = 2000;

    bool enableProductFlowrateHighCheck = true;
    float productFlowrateHighThreshold = 165.0;
    uint32_t productFlowrateHighDelay = 10 * 1000;

    bool enableProductFlowrateLowCheck = true;
    float productFlowrateLowThreshold = 120.0;
    uint32_t productFlowrateLowDelay = 10 * 1000;

    bool enableRunTotalFlowrateLowCheck = true;
    float runTotalFlowrateLowThreshold = 300.0;
    uint32_t runTotalFlowrateLowDelay = 2500;

    bool enablePickleTotalFlowrateLowCheck = true;
    float pickleTotalFlowrateLowThreshold = 300.0;
    uint32_t pickleTotalFlowrateLowDelay = 2500;

    bool enableDiverterValveClosedCheck = true;
    float diverterValveClosedDelay = 5000;

    bool enableProductSalinityHighCheck = true;
    float productSalinityHighThreshold = 500.0;
    uint32_t productSalinityHighDelay = 1000;

    bool enableMotorTemperatureCheck = true;
    float motorTemperatureHighThreshold = 65.0;
    uint32_t motorTemperatureHighDelay = 1000;

    bool enableFlushFlowrateLowCheck = true;
    float flushFlowrateLowThreshold = 100.0;
    uint32_t flushFlowrateLowDelay = 2500;

    bool enableFlushFilterPressureLowCheck = true;
    float flushFilterPressureLowThreshold = 15.0;
    uint32_t flushFilterPressureLowDelay = 2500;

    bool enableFlushValveOffCheck = true;
    float flushValveOffThreshold = 2.0;
    uint32_t flushValveOffDelay = 15 * 1000;

    uint32_t flushTimeout = 5 * 60 * 1000;                   // timeout for flush cycle in ms
    uint32_t membranePressureTimeout = 60 * 1000;            // timeout for membrane pressure to stabilize in ms
    uint32_t productFlowrateTimeout = 2 * 60 * 1000;         // timeout for product flowrate to stabilize in ms
    uint32_t productSalinityTimeout = 5 * 60 * 1000;         // timeout for salinity to stabilize in ms
    uint32_t productionRuntimeTimeout = 12 * 60 * 60 * 1000; // maximum length of run in ms

    //
    //  Old config - needs conversion
    //

    // convert to boost pump error check
    uint32_t filterPressureTimeout = 30 * 1000; // timeout for filter pressure in ms

    void resetErrorTimers();

    bool checkStopFlag(Result& result);
    bool checkTankLevel();
    bool checkMembranePressureHigh();
    bool checkMembranePressureLow();
    bool checkFilterPressureHigh();
    bool checkFilterPressureLow();
    bool checkProductFlowrateLow();
    bool checkProductFlowrateHigh();
    bool checkBrineFlowrateLow(float flowrate, Result& result);
    bool checkTotalFlowrateLow(float flowrate);
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

extern Brineomatic wm;

template <class X, class M, class N, class O, class Q>
X map_generic(X x, M in_min, N in_max, O out_min, Q out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif /* !YARR_BRINEOMATIC_H */