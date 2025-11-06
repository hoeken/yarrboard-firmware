#ifndef _CONFIG_H_SENDIT_REV_A
#define _CONFIG_H_SENDIT_REV_A

#define YB_HARDWARE_VERSION "SENDIT_REV_A"
#define YB_MANUFACTURER     "Yarrboard"

#define YB_IS_SENDIT        1
#define YB_BOARD_NAME       "Sendit"
#define YB_DEFAULT_HOSTNAME "sendit"

#define YB_HAS_STATUS_RGB
#define YB_STATUS_RGB_PIN 21

#define YB_HAS_ADC_CHANNELS
#define YB_ADC_DRIVER_ADS1115
#define YB_ADC_UNIT_LENGTH               8
#define YB_ADC_RUNNING_AVERAGE_SIZE      10
#define YB_ADC_CALIBRATION_TABLE_MAX     255
#define YB_I2C_SDA_PIN                   2
#define YB_I2C_SCL_PIN                   1
#define YB_ADC_VREF                      4.096
#define YB_ADC_VCC                       3.3
#define YB_ADC_CHANNEL_COUNT             8
#define YB_ADC_RESOLUTION                15 // positive only
#define YB_CHANNEL_VOLTAGE_I2C_ADDRESS_1 0x49
#define YB_CHANNEL_VOLTAGE_I2C_ADDRESS_2 0x48
#define YB_ADS1115_READY_PIN_1           4
#define YB_ADS1115_READY_PIN_2           5

#define YB_SENDIT_420MA_R1        165.0
#define YB_SENDIT_HIGH_DIVIDER_R1 13000.0
#define YB_SENDIT_HIGH_DIVIDER_R2 1500.0
#define YB_SENDIT_LOW_DIVIDER_R1  560.0
#define YB_SENDIT_LOW_DIVIDER_R2  1000.0

#define YB_HAS_SD_CARD
#define YB_SD_SCK         36
#define YB_SD_MOSI        35
#define YB_SD_MISO        37
#define YB_SD_CS          34
#define YB_SD_CARD_DETECT 33

#endif // _CONFIG_SENDIT_REV_A