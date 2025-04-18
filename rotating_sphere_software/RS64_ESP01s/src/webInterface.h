// =========================================================================================================================================
//                                                 Rotating Display RS64c
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================



// ******************************************************************************************************************************************
//
//          This class provides the user interface for controlling the rotating sphere
//
// ******************************************************************************************************************************************


#ifndef webInterface_H
#define webInterface_H

#include <Arduino.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "my_ESP.h"
#include "global.h"

class webInterface {
  private:
    const char* _ssidPath = "/variables/ssid.txt";
    const char* _passwordPath = "/variables/password.txt";
    const char* _timezonePath = "/variables/timezone.txt";

    char _ssid[50];
    char _password[50];

    AsyncWebServer _server{80};                 // server object

    static String _currentPath;

    void _startServer();
    void _listFiles(const char *dirname);
 

  public:
    webInterface();                                             // constructor. hand over i2c instance

    void begin();                                               // starts the web server

    int clockMode=0;
    int brightness=50;
};

#endif    
