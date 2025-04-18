// =========================================================================================================================================
//                                                 Rotating Sphere RS64c
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================


// ******************************************************************************************************************************************
//
//          This class manages the ressources of the microcontroller:
//              * Wifi
//              * clock
//
// ******************************************************************************************************************************************

#ifndef my_ESP_H
#define my_ESP_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <time.h>

#define TIME_ZONE_DEFAULT "CEST-1CET,M3.5.0/2:00:00,M10.5.0/3:00:00"

class my_ESP {
  private:
    void _iniWifi();                       // either connects to known Wifi or starts AP

    const char* _ssidPath = "/variables/ssid.txt";
    const char* _passwordPath = "/variables/password.txt";
    const char* _timezonePath = "/variables/timezone.txt";

    char _ssid[50];
    char _password[50];

  public:
    my_ESP();           // Constructor. hand over i2cRef

    void begin();                          // initiate Wifi
                                           // set time

    char timeZone[100];

    AsyncWebServer server{80};             // AsyncWebServer: Server object is public

    String ipAddress;

    tm localTime;                                // the structure tm holds the local time
    int Sec;   // Sekunden (0-59)
    int Min;   // Minuten (0-59)
    int Hour;  // Stunden (0-23)
    int Mday;  // Tag des Monats (1-31)
    int Mon;   // Monat (0-11)
    int Year;  // Jahr - 1900
    int Wday;  // Tag der Woche (0-6, Sonntag = 0)

    void configureTimeZone();
    void updateLocalTime();
    bool is_leap(int year);
    int64_t tm_to_unixtime(const tm& t); 
};

#endif
