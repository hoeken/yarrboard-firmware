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
      NONE,
      SERVO,
      STEPPER
    };

    bool isPickled;
    bool autoFlushEnabled;
    bool flushUseHighPressureMotor = false;

    RelayChannel* flushValve = NULL;
    RelayChannel* boostPump = NULL;
    RelayChannel* highPressurePump = NULL;
    RelayChannel* coolingFan = NULL;
    ServoChannel* diverterValve = NULL;
    ServoChannel* highPressureValveServo = NULL;
    StepperChannel* highPressureValveStepper = NULL;

    HighPressureValveControlMode highPressureValveMode;

    float diverterValveOpenAngle = 35;
    float diverterValveClosedAngle = 125;

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
    uint64_t totalRuntime;

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
    void startDuration(uint64_t duration);
    void startVolume(float volume);
    void flush(uint64_t duration);
    void pickle(uint64_t duration);
    void depickle(uint64_t duration);
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

    int64_t getNextFlushCountdown();
    int64_t getRuntimeElapsed();
    int64_t getFinishCountdown();
    int64_t getFlushElapsed();
    int64_t getFlushCountdown();
    int64_t getPickleElapsed();
    int64_t getPickleCountdown();
    int64_t getDepickleElapsed();
    int64_t getDepickleCountdown();

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
    uint64_t getTotalRuntime();
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
    void updateMQTT();

  private:
    Status currentStatus;
    Result runResult;
    Result flushResult;
    Result pickleResult;
    Result depickleResult;

    bool stopFlag = false;

    float desiredVolume = 0;

    // all these times are in microseconds
    uint64_t desiredRuntime = 0;
    uint64_t runtimeStart = 0;
    uint64_t runtimeElapsed = 0;
    uint64_t flushStart = 0;
    uint64_t flushDuration = 3ULL * 60 * 1000000; // 3 minute default, in microseconds
    uint64_t nextFlushTime = 0;
    uint64_t flushInterval = 3ULL * 24 * 60 * 60 * 1000000; // 3 day default, in microseconds
    uint64_t pickleStart = 0;
    uint64_t pickleDuration = 5ULL * 60 * 1000000; // 5 minute default, in microseconds
    uint64_t depickleStart = 0;
    uint64_t depickleDuration = 15ULL * 60 * 1000000; // 15 minute default, in microseconds

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

    float membranePressureTarget;
    float membranePressurePIDOutput;
    QuickPID membranePressurePID;

    float lowPressureMinimum = 2.5;              // PSI
    float lowPressureMaximum = 60.0;             // PSI
    float highPressureMinimum = 700.0;           // PSI
    float defaultMembranePressureTarget = 800.0; // PSI
    float highPressureMaximum = 900.0;           // PSI
    float productFlowrateMinimum = 120.0;        // LPH
    float productFlowrateMaximum = 160.0;        // LPH
    float flushFilterPressureMinimum = 20.0;     // PSI
    float flushFlowrateMinimum = 100.0;          // LPH
    float runTotalFlowrateMinimum = 300.0;       // LPH
    float pickleTotalFlowrateMinimum = 300.0;    // LPH
    float productSalinityMaximum = 500.0;        // PPM
    float motorTemperatureMaximum = 65.0;        // Celcius
    float tankLevelFull = 0.99;                  // 0 = empty, 1 = full
    float tankCapacity = 780;                    // Liters
    float coolingFanOnTemperature = 35.0;        // Celcius
    float coolingFanOffTemperature = 34.0;       // Celcius

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

    // how long to delay until we trigger the error
    uint32_t membranePressureHighTimeout = 2000;      // timeout for membrane pressure too high during run in ms
    uint32_t membranePressureLowTimeout = 2000;       // timeout for membrane pressure too low during run in ms
    uint32_t filterPressureHighTimeout = 2000;        // timeout for filter pressure too high during run in ms
    uint32_t filterPressureLowTimeout = 2000;         // timeout for filter pressure too low during run in ms
    uint32_t productFlowrateLowTimeout = 2500;        // timeout for product flowrate too low during run in ms
    uint32_t productFlowrateHighTimeout = 2500;       // timeout for product flowrate too high during run in ms
    uint32_t brineFlowrateLowTimeout = 2500;          // timeout for brine flowrate too low during run in ms
    uint32_t totalFlowrateLowTimeout = 2500;          // timeout for total flowrate too low in ms
    uint32_t diverterValveOpenTimeout = 5000;         // timeout for diverter valve opening failure in ms
    uint32_t productSalinityHighTimeout = 1000;       // timeout for salinity too high during run  in ms
    uint32_t motorTemperatureTimeout = 1000;          // timeout for motor overtemp in ms
    uint32_t flushFilterPressureLowTimeout = 2500;    // timeout for flush filter pressure in ms
    uint32_t flushFlowrateLowTimeout = 2500;          // timeout for flush flowrate in ms
    uint32_t flushValveOffTimeout = 10000;            // timeout for flush valve to turn off in ms
    uint32_t filterPressureTimeout = 30 * 1000;       // timeout for filter pressure in ms
    uint32_t membranePressureTimeout = 60 * 1000;     // timeout for membrane pressure to stabilize in ms
    uint32_t productFlowRateTimeout = 2 * 60 * 1000;  // timeout for product flowrate to stabilize in ms
    uint32_t productSalinityTimeout = 5 * 60 * 1000;  // timeout for salinity to stabilize in ms
    uint32_t productionTimeout = 12 * 60 * 60 * 1000; // maximum length of run run in ms

    bool checkStopFlag();
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
    bool checkMotorTemperature();
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