/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_BRINEOMATIC_H
#define YARR_BRINEOMATIC_H

#include "brineomatic_config.h"
#include "etl/deque.h"
#include <ArduinoJson.h>
#include <QuickPID.h>

class RelayChannel;
class ServoChannel;
class StepperChannel;

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

    // Static lookup tables
    static constexpr const char* const AUTOFLUSH_MODES[] = {"NONE", "TIME", "SALINITY", "MANUAL"};
    static constexpr const char* const TEMPERATURE_UNITS[] = {"celsius", "fahrenheit"};
    static constexpr const char* const PRESSURE_UNITS[] = {"pascal", "psi", "bar"};
    static constexpr const char* const VOLUME_UNITS[] = {"liters", "gallons"};
    static constexpr const char* const FLOWRATE_UNITS[] = {"lph", "gph"};

    static constexpr const char* BOOST_PUMP_CONTROLS[] = {"NONE", "MANUAL", "RELAY"};
    static constexpr const char* HIGH_PRESSURE_PUMP_CONTROLS[] = {"NONE", "MANUAL", "RELAY"};
    static constexpr const char* HIGH_PRESSURE_VALVE_CONTROLS[] = {"NONE", "MANUAL", "STEPPER"};
    static constexpr const char* DIVERTER_VALVE_CONTROLS[] = {"NONE", "MANUAL", "SERVO"};
    static constexpr const char* FLUSH_VALVE_CONTROLS[] = {"NONE", "MANUAL", "RELAY"};
    static constexpr const char* COOLING_FAN_CONTROLS[] = {"NONE", "MANUAL", "RELAY"};

    bool isPickled;

    RelayChannel* flushValve = NULL;
    RelayChannel* boostPump = NULL;
    RelayChannel* highPressurePump = NULL;
    RelayChannel* coolingFan = NULL;
    ServoChannel* diverterValve = NULL;
    StepperChannel* highPressureValveStepper = NULL;

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

    Brineomatic();
    void init();
    void initChannels();

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

    bool autoflushEnabled();

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

    bool validateConfigJSON(JsonVariantConst config, char* error, size_t err_size);
    bool validateGeneralConfigJSON(JsonVariantConst config, char* error, size_t err_size);
    bool validateHardwareConfigJSON(JsonVariantConst config, char* error, size_t err_size);
    bool validateSafeguardsConfigJSON(JsonVariantConst config, char* error, size_t err_size);

    void loadConfigJSON(JsonVariantConst config);
    void loadGeneralConfigJSON(JsonVariantConst config);
    void loadHardwareConfigJSON(JsonVariantConst config);
    void loadSafeguardsConfigJSON(JsonVariantConst config);

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
    String autoflushMode;
    float autoflushSalinity;
    uint32_t autoflushDuration;
    float autoflushVolume;
    uint32_t autoflushInterval;
    bool autoflushUseHighPressureMotor;

    float tankLevelFull;            // 0 = empty, 1 = full
    float tankCapacity;             // Liters
    float coolingFanOnTemperature;  // Celcius
    float coolingFanOffTemperature; // Celcius

    String temperatureUnits;
    String pressureUnits;
    String volumeUnits;
    String flowrateUnits;

    String successMelody;
    String errorMelody;

    String boostPumpControl;
    uint8_t boostPumpRelayId;

    String highPressurePumpControl;
    uint8_t highPressureRelayId;

    String highPressureValveControl;
    uint8_t highPressureValveStepperId;
    float highPressureValveStepperStepAngle;
    float highPressureValveStepperGearRatio;
    float highPressureValveStepperCloseAngle;
    float highPressureValveStepperCloseSpeed;
    float highPressureValveStepperOpenAngle;
    float highPressureValveStepperOpenSpeed;
    float membranePressureTarget; // PSI

    String diverterValveControl;
    uint8_t diverterValveServoId;
    float diverterValveOpenAngle;
    float diverterValveCloseAngle;

    String flushValveControl;
    uint8_t flushValveRelayId;

    String coolingFanControl;
    uint8_t coolingFanRelayId;

    bool hasMembranePressureSensor;
    float membranePressureSensorMin;
    float membranePressureSensorMax;

    bool hasFilterPressureSensor;
    float filterPressureSensorMin;
    float filterPressureSensorMax;

    bool hasProductTDSSensor;
    bool hasBrineTDSSensor;

    bool hasProductFlowSensor;
    float productFlowmeterPPL;

    bool hasBrineFlowSensor;
    float brineFlowmeterPPL;

    bool hasMotorTemperatureSensor;

    bool enableMembranePressureHighCheck;
    float membranePressureHighThreshold;
    uint32_t membranePressureHighDelay;

    bool enableMembranePressureLowCheck;
    float membranePressureLowThreshold;
    uint32_t membranePressureLowDelay;

    bool enableFilterPressureHighCheck;
    float filterPressureHighThreshold;
    uint32_t filterPressureHighDelay;

    bool enableFilterPressureLowCheck;
    float filterPressureLowThreshold;
    uint32_t filterPressureLowDelay;

    bool enableProductFlowrateHighCheck;
    float productFlowrateHighThreshold;
    uint32_t productFlowrateHighDelay;

    bool enableProductFlowrateLowCheck;
    float productFlowrateLowThreshold;
    uint32_t productFlowrateLowDelay;

    bool enableRunTotalFlowrateLowCheck;
    float runTotalFlowrateLowThreshold;
    uint32_t runTotalFlowrateLowDelay;

    bool enablePickleTotalFlowrateLowCheck;
    float pickleTotalFlowrateLowThreshold;
    uint32_t pickleTotalFlowrateLowDelay;

    bool enableDiverterValveClosedCheck;
    float diverterValveClosedDelay;

    bool enableProductSalinityHighCheck;
    float productSalinityHighThreshold;
    uint32_t productSalinityHighDelay;

    bool enableMotorTemperatureCheck;
    float motorTemperatureHighThreshold;
    uint32_t motorTemperatureHighDelay;

    bool enableFlushFlowrateLowCheck;
    float flushFlowrateLowThreshold;
    uint32_t flushFlowrateLowDelay;

    bool enableFlushFilterPressureLowCheck;
    float flushFilterPressureLowThreshold;
    uint32_t flushFilterPressureLowDelay;

    bool enableFlushValveOffCheck;
    float flushValveOffThreshold;
    uint32_t flushValveOffDelay;

    uint32_t flushTimeout;             // timeout for flush cycle in ms
    uint32_t membranePressureTimeout;  // timeout for membrane pressure to stabilize in ms
    uint32_t productFlowrateTimeout;   // timeout for product flowrate to stabilize in ms
    uint32_t productSalinityTimeout;   // timeout for salinity to stabilize in ms
    uint32_t productionRuntimeTimeout; // maximum length of run in ms

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

extern Brineomatic wm;

template <class X, class M, class N, class O, class Q>
X map_generic(X x, M in_min, N in_max, O out_min, Q out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif /* !YARR_BRINEOMATIC_H */