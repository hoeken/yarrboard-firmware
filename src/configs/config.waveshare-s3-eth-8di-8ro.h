#ifndef _CONFIG_H_ESP32_S3_ETH_8DI_8RO
#define _CONFIG_H_ESP32_S3_ETH_8DI_8RO

#define YB_HARDWARE_VERSION "ESP32-S3-ETH-8DI-8RO"
#define YB_MANUFACTURER     "Waveshare"

#define YB_HAS_STATUS_RGB
#define YB_STATUS_RGB_PIN 38

#define YB_HAS_PIEZO
#define YB_PIEZO_PASSIVE
#define YB_PIEZO_PIN 46

#define YB_HAS_RELAY_CHANNELS
#define YB_RELAY_DRIVER_TCA9554
#define YB_RELAY_DRIVER_TCA9554_ADDRESS 0x20
#define YB_I2C_SDA_PIN                  42
#define YB_I2C_SCL_PIN                  41
#define YB_I2C_SPEED                    100000
#define YB_RELAY_CHANNEL_COUNT          8
#define YB_RELAY_CHANNEL_PINS           {0, 1, 2, 3, 4, 5, 6, 7}

// Note: need to refresh the old input channel code
#define YB_HAS_DIGITAL_INPUT_CHANNELS
#define YB_DIGITAL_INPUT_CHANNEL_COUNT 8
#define YB_DIGITAL_INPUT_CHANNEL_PINS  {4, 5, 6, 7, 8, 9, 10, 11}

// Note: no sd card support has been written
#define YB_HAS_SD_CARD
#define YB_SD_D0  45
#define YB_SD_CMD 47
#define YB_SD_SCK 48

// Note: no ethernet support has been written
#define YB_HAS_ETHERNET
#define YB_ETHERNET_DRIVER_W5500
#define YB_ETH_INT  12
#define YB_ETH_MOSI 13
#define YB_ETH_MISO 14
#define YB_ETH_SCK  15
#define YB_ETH_CS   16

// Note: no RS485 support has been written
#define YB_HAS_RS485
#define YB_RS485_RX 18
#define YB_RS485_TX 17

#endif // _CONFIG_H_ESP32_S3_ETH_8DI_8RO