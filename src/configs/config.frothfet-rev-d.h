#ifndef _CONFIG_H_FROTHFET_REV_D
#define _CONFIG_H_FROTHFET_REV_D

#define YB_HARDWARE_VERSION "FROTHFET_REV_D"

#define YB_HAS_PWM_CHANNELS
#define YB_PWM_CHANNEL_COUNT 8
#define YB_PWM_CHANNEL_PINS {1, 2, 42, 41, 40, 39, 37, 36}
#define YB_PWM_CHANNEL_FREQUENCY 1000
#define YB_PWM_CHANNEL_RESOLUTION 10
#define YB_PWM_CHANNEL_ADC_DRIVER_MCP3564
#define YB_PWM_CHANNEL_ADC_CS 10
#define YB_PWM_CHANNEL_MAX_AMPS 20.0
// #define YB_PWM_CHANNEL_TOTAL_AMPS 100.0

#define YB_HAS_CHANNEL_VOLTAGE
#define YB_CHANNEL_VOLTAGE_ADC_DRIVER_ADS1015
#define YB_CHANNEL_VOLTAGE_I2C_ADDRESS                                         \
  {0x49, 0x49, 0x49, 0x49, 0x48, 0x48, 0x48, 0x48}
#define YB_CHANNEL_VOLTAGE_I2C_CHANNEL {0, 1, 2, 3, 0, 1, 2, 3}

#define YB_HAS_BUS_VOLTAGE
#define YB_BUS_VOLTAGE_MCP3425
#define YB_BUS_VOLTAGE_R1 300000.0
#define YB_BUS_VOLTAGE_R2 22000.0
#define YB_BUS_VOLTAGE_ADDRESS 0x68

#define YB_HAS_FANS
#define YB_FAN_COUNT 2
#define YB_FAN_MOSFET_PINS {15, 2}
#define YB_FAN_PWM_PINS {16, 6}
#define YB_FAN_TACH_PINS {17, 7}
#define YB_FAN_SINGLE_CHANNEL_THRESHOLD_START 5
#define YB_FAN_SINGLE_CHANNEL_THRESHOLD_END 10
#define YB_FAN_AVERAGE_CHANNEL_THRESHOLD_START 2
#define YB_FAN_AVERAGE_CHANNEL_THRESHOLD_END 4

#endif // _CONFIG_FROTHFET_REV_D