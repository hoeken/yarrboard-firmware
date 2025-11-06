#ifndef _CONFIG_H_SENDIT_REV_B
#define _CONFIG_H_SENDIT_REV_B

#define YB_HARDWARE_VERSION "SENDIT_REV_B"
#define YB_MANUFACTURER     "Yarrboard"

#define YB_IS_SENDIT        1
#define YB_BOARD_NAME       "Sendit"
#define YB_DEFAULT_HOSTNAME "sendit"

#define YB_HAS_STATUS_RGB
#define YB_STATUS_RGB_PIN 38

#define YB_HAS_PIEZO
#define YB_PIEZO_PASSIVE
#define YB_PIEZO_PIN 14

#define YB_HAS_ADC_CHANNELS
#define YB_ADC_CHANNEL_COUNT         8
#define YB_ADC_DRIVER_MCP3564        1
#define YB_ADC_RESOLUTION            23
#define YB_ADC_UNIT_LENGTH           8
#define YB_ADC_RUNNING_AVERAGE_SIZE  10
#define YB_ADC_CALIBRATION_TABLE_MAX 255
#define YB_ADC_VREF                  3.3
#define YB_ADC_VCC                   3.3
#define YB_ADC_SCK                   12
#define YB_ADC_MOSI                  11
#define YB_ADC_MISO                  10
#define YB_ADC_CS                    13
#define YB_ADC_IRQ                   9

#define YB_SENDIT_420MA_R1        165.0
#define YB_SENDIT_HIGH_DIVIDER_R1 130000.0
#define YB_SENDIT_HIGH_DIVIDER_R2 15000.0
#define YB_SENDIT_LOW_DIVIDER_R1  560.0
#define YB_SENDIT_LOW_DIVIDER_R2  1000.0

#endif // _CONFIG_SENDIT_REV_B