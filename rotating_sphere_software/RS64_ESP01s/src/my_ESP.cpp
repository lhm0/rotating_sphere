// =========================================================================================================================================
//                                                 Rotating Display RD40
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================

#include <Arduino.h>
#include "my_ESP.h"
#include "global.h"

// =========================================================================================================================================
//                                                      Constructor
// =========================================================================================================================================

my_ESP::my_ESP(){}

// =========================================================================================================================================
//                                                      begin Method
// =========================================================================================================================================

void my_ESP::begin() {

  // initiate Wifi
  _iniWifi();

  // configure time zone
  configureTimeZone();
  updateLocalTime();
  
  time_t unixTime = tm_to_unixtime(localTime);

  if (unixTime < 1000000000) {
    Serial.println("❌ Ungültige lokale Zeit – wurde NTP gesetzt?");
    return;
  }

  Serial.printf("📤 Setze Slave-Zeit auf UNIX-Zeit: %lu\n", (long unsigned int)unixTime);

  // ⏱️ Zeit setzen (asynchron)
  if (!fileTransfer.setTime(unixTime)) {
    Serial.println("❌ setTime() konnte nicht gestartet werden.");
    return;
  }

  fileTransfer.update();
  delay(10);       // attention - the i2c state machine is not running, while in setup(). Therefore,
                    // there is only limited i2c functionality available here

}

// =========================================================================================================================================
//                                                      Wifi Methods
// =========================================================================================================================================

void my_ESP::_iniWifi() {
  /*
    read SSID and password, if available
  */
 File file = LittleFS.open(_ssidPath, "r");  // "r" für read

 if (!file) {
     Serial.println("error opening ssid.txt");
 } else {
     size_t len = file.readBytes(_ssid, sizeof(_ssid) - 1);
     _ssid[len] = '\0';  // terminate with /0
     file.close();
     Serial.printf("ssid read from file: %s\n", _ssid);
 }

 file = LittleFS.open(_passwordPath, "r");  // "r" für read

 if (!file) {
     Serial.println("error opening password.txt");
 } else {
     size_t len = file.readBytes(_password, sizeof(_password) - 1);
     _password[len] = '\0';  // terminate with /0
     file.close();
     Serial.printf("password read from file: %s\n", _password);

 }

 file = LittleFS.open(_timezonePath, "r");  // "r" für read

 if (!file) {
     Serial.println("error opening timezone.txt");
 } else {
     size_t len = file.readBytes(timeZone, sizeof(timeZone) - 1);
     timeZone[len] = '\0';  // terminate with /0
     file.close();
     Serial.printf("timezone read from file: %s\n", timeZone);

 }

 /*
    try to connect to WiFi, if ssid and password are valid
  */
  if ((strlen(_ssid)!=0)||(strlen(_password)!=0)) { 
    WiFi.begin(_ssid, _password);

    Serial.print("connecting to WiFi ...");

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(1000);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      ipAddress = WiFi.localIP().toString();
      Serial.println("WiFi connected");
      Serial.println(ipAddress);
    } else {
      Serial.println("Wifi connection failed...");
    }
  }

  if (WiFi.status() != WL_CONNECTED) {

    WiFi.mode(WIFI_OFF);
    delay(1000);
    ESP.eraseConfig();                      // supposed to fix connectivity issue of ESP8266
    WiFi.mode(WIFI_AP);                     //
    IPAddress apIP(192, 168, 4, 1);         //
    IPAddress subnet(255,255,255,0);        //
    WiFi.softAPConfig(apIP, apIP, subnet);  //

    strcpy(_ssid, "RS64c");
    WiFi.softAP(_ssid);                     //

    ipAddress = WiFi.softAPIP().toString();

    Serial.println("Access Point started.");
    Serial.print("IP-Adresse: ");
    Serial.println(ipAddress);  
  }

}

// =========================================================================================================================================
//                                                      Time Methods
//                                  https://werner.rothschopf.net/202011_arduino_esp8266_ntp_en.htm
// =========================================================================================================================================

void my_ESP::configureTimeZone() {
  File file = LittleFS.open(_timezonePath, "r");  // "r" für read

  if (!file) {
      Serial.println("error opening timezone.txt");
      const char* defaultTimeZone = TIME_ZONE_DEFAULT;
      strcpy(timeZone, defaultTimeZone);  
  } else {
      size_t len = file.readBytes(timeZone, sizeof(timeZone) - 1);
      timeZone[len] = '\0';  // terminate with /0
      file.close();
      Serial.printf("timezone read from file: %s\n", timeZone);
  }
 
  const char* ntpServer = "pool.ntp.org";
  configTime(0, 0, ntpServer);              // Connect to NTP server with 0 TZ offset
  setenv("TZ", timeZone, 1);               // Set time zone
  tzset();
}

// Update Local Time
void my_ESP::updateLocalTime() {
  time_t now;
  time(&now);                       // get UNIX time
  localtime_r(&now, &localTime);    // convert to local time. Result will be in localTime

  Hour = localTime.tm_hour;
  Min = localTime.tm_min;
  Sec = localTime.tm_sec;
  Mday = localTime.tm_mday;
  Mon = localTime.tm_mon + 1;
  Year = localTime.tm_year + 1900;
  Wday = localTime.tm_wday;
}

bool my_ESP::is_leap(int year) {
  return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

int64_t my_ESP::tm_to_unixtime(const tm& t) {
  static const int days_before_month[12] = {
      0,   31,  59,  90, 120, 151,
      181, 212, 243, 273, 304, 334
  };

  int year = t.tm_year + 1900;
  int month = t.tm_mon;  // 0–11

  // Tage seit 1970-01-01
  int64_t days = 0;

  // Ganze Jahre seit 1970
  for (int y = 1970; y < year; y++) {
      days += is_leap(y) ? 366 : 365;
  }

  // Ganze Monate im aktuellen Jahr
  days += days_before_month[month];

  // Schaltjahrkorrektur für dieses Jahr, nach Februar
  if (month > 1 && is_leap(year)) {
      days += 1;
  }

  // Tage im aktuellen Monat (tm_mday: 1–31)
  days += t.tm_mday - 1;

  // Gesamtzeit in Sekunden
  int64_t seconds = days * 86400
                  + t.tm_hour * 3600
                  + t.tm_min * 60
                  + t.tm_sec;

  return seconds;
}