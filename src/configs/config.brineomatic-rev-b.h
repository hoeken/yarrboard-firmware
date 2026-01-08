#ifndef _CONFIG_H_BRINEOMATIC_REV_B
#define _CONFIG_H_BRINEOMATIC_REV_B

#define YB_HARDWARE_VERSION "BRINEOMATIC_REV_B"
#define YB_MANUFACTURER     "Yarrboard"

#define YB_IS_BRINEOMATIC   1
#define YB_BOARD_NAME       "Brineomatic"
#define YB_DEFAULT_HOSTNAME "brineomatic"
#define YB_HARDWARE_URL     "https://github.com/hoeken/brineomatic/releases/tag/Rev-B"
#define YB_PROJECT_NAME     "Brineomatic"
#define YB_PROJECT_URL      "https://github.com/hoeken/brineomatic"

#define YB_BOM_DATA_SIZE     300
#define YB_BOM_DATA_INTERVAL 5000

#define YB_BOM_PID_OUTPUT_MIN -255
#define YB_BOM_PID_OUTPUT_MAX 255

#define YB_HAS_STATUS_RGB
#define YB_STATUS_RGB_PIN 38

#define YB_HAS_RELAY_CHANNELS
#define YB_RELAY_CHANNEL_COUNT 4
#define YB_RELAY_CHANNEL_PINS  {17, 18, 21, 14}

#define YB_HAS_SERVO_CHANNELS
#define YB_SERVO_CHANNEL_COUNT 2
#define YB_SERVO_CHANNEL_PINS  {40, 41}

#define YB_DS18B20_MOTOR_PIN 4

#define YB_HAS_PIEZO
#define YB_PIEZO_PASSIVE
#define YB_PIEZO_PIN 39

#define YB_PRODUCT_FLOWMETER_PIN 1
#define YB_BRINE_FLOWMETER_PIN   2

// Omega BV2000TRN075B = 4700.0
// Amazon B07MY7H45V = 1260.0
// PPL calculations
// F=(21*Q) Q=L/Min
// PPL = 21 * 60

#define YB_ADC_DRIVER_ADS1115
#define YB_I2C_SDA_PIN         13
#define YB_I2C_SCL_PIN         12
#define YB_ADS1115_READY_PIN   42
#define YB_ADS1115_VREF        4.096
#define YB_ADS1115_ADDRESS     0x49
#define YB_ADS1115_SAMPLES     50
#define YB_ADS1115_WINDOW      500
#define YB_BRINE_TDS_CHANNEL   0
#define YB_PRODUCT_TDS_CHANNEL 1
#define YB_LP_SENSOR_CHANNEL   2
#define YB_HP_SENSOR_CHANNEL   3
#define YB_420_RESISTOR        165.0

#define YB_HAS_STEPPER_CHANNELS
#define YB_STEPPER_DRIVER_TMC2209
#define YB_STEPPER_SERIAL_PORT   Serial1
#define YB_STEPPER_SERIAL_SPEED  115200
#define YB_STEPPER_RX_PIN        10
#define YB_STEPPER_TX_PIN        9
#define YB_STEPPER_CHANNEL_COUNT 1
#define YB_STEPPER_MICROSTEPS    32
#define YB_STEPPER_STEP_PINS     {7}
#define YB_STEPPER_DIR_PINS      {6}
#define YB_STEPPER_ENABLE_PINS   {11}
#define YB_STEPPER_DIAG_PINS     {8}

#define YB_HAS_MODBUS
#define YB_MODBUS_SERIAL   Serial2
#define YB_MODBUS_RX       47
#define YB_MODBUS_TX       48
#define YB_MODBUS_DERE_PIN 5
#define YB_MODBUS_SPEED    9600

#define YB_BOOST_PUMP_RELAY_ID     1
#define YB_HIGH_PRESSURE_RELAY_ID  2
#define YB_DIVERTER_VALVE_RELAY_ID 2
#define YB_FLUSH_VALVE_RELAY_ID    3
#define YB_COOLING_FAN_RELAY_ID    4

#endif // _CONFIG_BRINEOMATIC_REV_B