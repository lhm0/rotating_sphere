// =========================================================================================================================================
//                                                 Rotating Display RD56m
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================


// ******************************************************************************************************************************************
//
//          This class manages the ressources of the mikrocontroller:
//              * Wifi
//              * I2C Interface
//              * clock
//              * data and file storage
//
// ******************************************************************************************************************************************

#ifndef my_RP_H
#define my_RP_H

#include <Arduino.h>
#include <time.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SdFat.h>

extern SdFs SD;

#define TIME_ZONE "CET-1CEST,M3.5.0/02,M10.5.0/03" // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv    
                                                   // this is only the default (my home ;-)       

class my_RP {
  private:

  public:
    my_RP();                               // Constructor

    void begin();                          // set time
    void disableI2Cpins();

    String timeZone = TIME_ZONE;

    tm local;                                // the structure tm holds time information 

    void setRTC(time_t unixTime);          // set RTC to Unix time
    void getTime();     
    bool SDCardInit();                                                 // Initializes the SD card    
    void SDCardListDirectory(const char* path);                        // list dir

};

#endif
