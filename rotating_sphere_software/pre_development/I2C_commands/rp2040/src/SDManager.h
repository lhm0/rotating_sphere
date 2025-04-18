// =========================================================================================================================================
//                                                 Rotating Display RS64c
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================



// ******************************************************************************************************************************************
//
//          this class provides methods for saving and loading data to and from the SPIFFS (SPI Flash File System)
//          
// ******************************************************************************************************************************************

#ifndef SDCARD_H
#define SDCARD_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SdFat.h>

extern SdFs SD;

class SDManager {
  private:

  public:
    bool begin();                                                 // Initializes the SD card    
    void listDirectory(const char* path);                         // list dir
  };

#endif
