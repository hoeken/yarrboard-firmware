#ifndef _CONFIG_H_SENDIT_REV_A
#define _CONFIG_H_SENDIT_REV_A

#define YB_HARDWARE_VERSION "SENDIT_REV_A"

#define YB_IS_SENDIT

#define YB_DEFAULT_HOSTNAME "sendit"
#define YB_DEFAULT_AP_SSID  "sendit"

#define YB_HAS_STATUS_WS2818
#define YB_STATUS_WS2818_PIN 21

#define YB_HAS_ADC_CHANNELS
#define YB_ADC_DRIVER_ADS1115
#define YB_I2C_SDA_PIN                   2
#define YB_I2C_SCL_PIN                   1
#define YB_ADS1115_VREF                  4.096
#define YB_ADC_CHANNEL_COUNT             8
#define YB_ADC_RESOLUTION                15 // positive only
#define YB_CHANNEL_VOLTAGE_I2C_ADDRESS_1 0x49
#define YB_CHANNEL_VOLTAGE_I2C_ADDRESS_2 0x48
#define YB_ADS1115_READY_PIN_1           4
#define YB_ADS1115_READY_PIN_2           5

#endif // _CONFIG_SENDIT_REV_A