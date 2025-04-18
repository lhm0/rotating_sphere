// =========================================================================================================================================
//                                                 Rotating Display RD56m
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================

#include "my_RP.h"

#include <Arduino.h>
#include <time.h>
#include "hardware/rtc.h" 

#define RP_CLK_GPIO 20
#define RP_CMD_GPIO 19
#define RP_DAT0_GPIO 15

// Configuration for the SD card
#define SD_CONFIG SdioConfig(RP_CLK_GPIO, RP_CMD_GPIO, RP_DAT0_GPIO)

#define I2C_SDA 12
#define I2C_SCL 13


// =========================================================================================================================================
//                                                      Constructor
// =========================================================================================================================================

my_RP::my_RP() {
}

// =========================================================================================================================================
//                                                      begin Method
// =========================================================================================================================================

void my_RP::begin() {
  // set time
  rtc_init();
  setRTC((time_t)0);

  // initialize SD card reader
  int tries = 0;
  while (!SDCardInit()) {
    tries++;
    if (tries>20) break;
    delay(100);
  }
  if (tries>20) {
    Serial.println("SD card initialization failed.");
    return;
  } else {
    Serial.print(tries);
    Serial.println(" tries. SD card initialization succeeded");
  }
  char path[] = "/";
  SDCardListDirectory(path);   
  
}

void my_RP::disableI2Cpins() {
  gpio_init(I2C_SDA);
  gpio_set_dir(I2C_SDA, GPIO_IN);
  gpio_disable_pulls(I2C_SDA);
  gpio_init(I2C_SCL);
  gpio_set_dir(I2C_SCL, GPIO_IN);
  gpio_disable_pulls(I2C_SCL);
}


// =========================================================================================================================================
//                                                      Time Methods
// helpful tutorial: https://randomnerdtutorials.com/esp32-ntp-timezones-daylight-saving/
// =========================================================================================================================================

void my_RP::setRTC(time_t unixTime) {
  struct tm *tm = localtime(&unixTime);

  // struct tm in datetime_t umwandeln
  datetime_t t = {
    .year  = (int16_t)(tm->tm_year + (int)1900),
    .month = (int8_t)(tm->tm_mon + (int)1),
    .day   = (int8_t)tm->tm_mday,
    .dotw  = (int8_t)tm->tm_wday, // 0 = Sonntag
    .hour  = (int8_t)tm->tm_hour,
    .min   = (int8_t)tm->tm_min,
    .sec   = (int8_t)tm->tm_sec
  };

  // RTC setzen
  rtc_set_datetime(&t);
}

void my_RP::getTime() {
  datetime_t dt;
  rtc_get_datetime(&dt);

  local.tm_year = (int16_t)dt.year - 1900;
  local.tm_mon  = (int8_t)dt.month - 1;
  local.tm_mday = (int8_t)dt.day;
  local.tm_hour = (int8_t)dt.hour;
  local.tm_min  = (int8_t)dt.min;
  local.tm_sec  = (int8_t)dt.sec;
  local.tm_isdst = -1;
}

// Initializes the SD card
bool my_RP::SDCardInit() {
  if (!SD.begin(SD_CONFIG)) {
      Serial.println("SD card initialization failed. (msg 01)");
      return false;
  }

  Serial.print("Card type: ");
  Serial.println(SD.fatType() == FAT_TYPE_EXFAT ? "exFAT" : "FAT");

  Serial.print("Card size: ");
  Serial.print(SD.card()->sectorCount() * 512E-9);
  Serial.println(" GB");

  return true;
}

void my_RP::SDCardListDirectory(const char* path) {
  FsFile dir = SD.open(path);
  if (!dir) {
      Serial.println("Verzeichnis konnte nicht geöffnet werden.");
      return;
  }
  
  if (!dir.isDirectory()) {
      Serial.println("Pfad ist kein Verzeichnis.");
      return;
  }
  
  FsFile file;
  while ((file = dir.openNextFile())) {
      char name[64];
      file.getName(name, sizeof(name));
  
      Serial.print(name);
      if (file.isDirectory()) {
          Serial.println(" <DIR>");
      } else {
          Serial.print("\t");
          Serial.println(file.size());
      }
      file.close();
  }
  dir.close();
}
