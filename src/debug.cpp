/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "debug.h"

#ifdef YB_USB_SERIAL
USBCDC USBSerial;
#endif

bool has_coredump = false;

YarrboardPrint YBP;
StringPrint startupLogger;
WebsocketPrint networkLogger;

void debug_setup()
{
  // startup our serial
  Serial.begin(115200);
  Serial.setTimeout(50);
  YBP.addPrinter(Serial);

  // native usb serial too
#ifdef YB_USB_SERIAL
  USBSerial.begin();
  YBP.addPrinter(USBSerial);
  YBP.println("USB Serial Started");
#endif

  // startup log logs to a string for getting later
  YBP.addPrinter(startupLogger);
  esp_log_set_vprintf(debug_log_vprintf);

  YBP.println("Yarrboard");
  YBP.print("Hardware Version: ");
  YBP.println(YB_HARDWARE_VERSION);
  YBP.print("Firmware Version: ");
  YBP.println(YB_FIRMWARE_VERSION);
  YBP.printf("Firmware build: %s (%s)\n", GIT_HASH, BUILD_TIME);

  YBP.print("Last Reset: ");
  YBP.println(getResetReason());

  if (checkCoreDump()) {
    has_coredump = true;
    YBP.println("WARNING: Coredump Found.");
    String coredump = readCoreDump();

    File file = LittleFS.open("/coredump.txt", FILE_WRITE);
    file.print(coredump);
    file.close();
  }
}

String getResetReason()
{
  switch (esp_reset_reason()) {
    case ESP_RST_UNKNOWN:
      return ("Reset reason can not be determined");
    case ESP_RST_POWERON:
      return ("Reset due to power-on event");
    case ESP_RST_EXT:
      return ("Reset by external pin (not applicable for ESP32)");
    case ESP_RST_SW:
      return ("Software reset via esp_restart");
    case ESP_RST_PANIC:
      return ("Software reset due to exception/panic");
    case ESP_RST_INT_WDT:
      return ("Reset (software or hardware) due to interrupt watchdog");
    case ESP_RST_TASK_WDT:
      return ("Reset due to task watchdog");
    case ESP_RST_WDT:
      return ("Reset due to other watchdogs");
    case ESP_RST_DEEPSLEEP:
      return ("Reset after exiting deep sleep mode");
    case ESP_RST_BROWNOUT:
      return ("Brownout reset (software or hardware)");
    case ESP_RST_SDIO:
      return ("Reset over SDIO");
    default:
      return ("Unknown");
  }
}

bool checkCoreDump()
{
  size_t size = 0;
  size_t address = 0;
  if (esp_core_dump_image_get(&address, &size) == ESP_OK) {
    YBP.print("coredump size: ");
    YBP.println(size);
    const esp_partition_t* pt = NULL;
    pt = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP, "coredump");
    if (pt != NULL)
      return true;
    else
      return false;
  } else
    return false;
}

String readCoreDump()
{
  size_t size = 0;
  size_t address = 0;
  if (esp_core_dump_image_get(&address, &size) == ESP_OK) {
    const esp_partition_t* pt = NULL;
    pt = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP, "coredump");

    if (pt != NULL) {
      uint8_t bf[256];
      char str_dst[640];
      int16_t toRead;
      String return_str;

      for (int16_t i = 0; i < (size / 256) + 1; i++) {
        strcpy(str_dst, "");
        toRead = (size - i * 256) > 256 ? 256 : (size - i * 256);

        esp_err_t er = esp_partition_read(pt, i * 256, bf, toRead);
        if (er != ESP_OK) {
          YBP.printf("FAIL [%x]\n", er);
          break;
        }

        for (int16_t j = 0; j < 256; j++) {
          char str_tmp[3];

          sprintf(str_tmp, "%02x", bf[j]);
          strcat(str_dst, str_tmp);
        }

        return_str += str_dst;
      }
      return return_str;
    } else {
      return "Partition NULL";
    }
  } else {
    return "esp_core_dump_image_get() FAIL";
  }
}

bool deleteCoreDump()
{
  if (esp_core_dump_image_erase() == ESP_OK)
    return true;
  else
    return false;
}

void crash_me_hard()
{
  // provoke crash through writing to a nullpointer
  volatile uint32_t* aPtr = (uint32_t*)0x00000000;
  *aPtr = 0x1234567; // goodnight
}

int debug_log_vprintf(const char* fmt, va_list args)
{
  char buf[256];

  // Format the log into a buffer using the va_list
  int len = vsnprintf(buf, sizeof(buf), fmt, args);

  if (len > 0) {
    YBP.print(buf); // Use Print::print()
  }

  return len;
}