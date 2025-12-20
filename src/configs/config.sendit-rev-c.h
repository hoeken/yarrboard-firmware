#ifndef _CONFIG_H_SENDIT_REV_C
#define _CONFIG_H_SENDIT_REV_C

#define YB_HARDWARE_VERSION "SENDIT_REV_C"
#define YB_MANUFACTURER     "Yarrboard"

#define YB_IS_SENDIT        1
#define YB_BOARD_NAME       "Sendit"
#define YB_DEFAULT_HOSTNAME "sendit"
#define YB_HARDWARE_URL     "https://github.com/hoeken/sendit/releases/tag/REV-C"
#define YB_PROJECT_NAME     "Sendit"
#define YB_PROJECT_URL      "https://github.com/hoeken/sendit"

#define YB_HAS_STATUS_RGB
#define YB_STATUS_RGB_PIN 38

#define YB_HAS_PIEZO
#define YB_PIEZO_PASSIVE
#define YB_PIEZO_PIN 48

#define YB_HAS_ADC_CHANNELS
#define YB_ADC_DRIVER_ADS1115
#define YB_ADC_UNIT_LENGTH               8
#define YB_ADC_RUNNING_AVERAGE_SIZE      10
#define YB_ADC_CALIBRATION_TABLE_MAX     255
#define YB_I2C_SDA_PIN                   8
#define YB_I2C_SCL_PIN                   9
#define YB_ADC_VREF                      ADS1x15_GAIN_2048MV_FSRANGE_V
#define YB_ADC_GAIN                      ADS1X15_GAIN_2048MV
#define YB_ADC_VCC                       3.3
#define YB_ADC_CHANNEL_COUNT             8
#define YB_ADC_RESOLUTION                15 // positive only
#define YB_CHANNEL_VOLTAGE_I2C_ADDRESS_1 0x49
#define YB_CHANNEL_VOLTAGE_I2C_ADDRESS_2 0x48
#define YB_ADS1115_READY_PIN_1           21
#define YB_ADS1115_READY_PIN_2           14

#define YB_SENDIT_420MA_R1        100.0
#define YB_SENDIT_HIGH_DIVIDER_R1 28000.0
#define YB_SENDIT_HIGH_DIVIDER_R2 2000.0
#define YB_SENDIT_LOW_DIVIDER_R1  18000.0
#define YB_SENDIT_LOW_DIVIDER_R2  10000.0

#endif // _CONFIG_SENDIT_REV_B