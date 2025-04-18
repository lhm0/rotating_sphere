// =========================================================================================================
//                                                 Rotating Sphere RS64c
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// You may use, adapt, share. If you share, "share alike".
// You may not use this software for commercial purposes.
// =========================================================================================================

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "webInterface.h"
#include "global.h"

webInterface wi;                            // create instance of webInterface. Hand over instance of i2cCommands


// Setup and Loop
void setup() {
  Serial.begin(115200);                     // Initialize serial communication for debugging
 
  delay(500);
  i2c.begin();
  delay(500);

  if (!LittleFS.begin()) {
    Serial.println("error: LittleFS mounting failed!");
    return;
  }
  Serial.println("LittleFS mounted successfully.");

  ESP01s.begin();                           // init Wifi and time
  wi.begin();                               // start web server
}

void loop() {
  fileTransfer.update();
  delay(50);

  static int lastDisplayedSecond = 0;  // Speichert die letzte Sekunde

  ESP01s.updateLocalTime();
  if (ESP01s.Sec != lastDisplayedSecond) {  // Nur wenn sich die Sekunde geändert hat
    lastDisplayedSecond = ESP01s.Sec;

    Serial.printf("local time: %02d:%02d:%02d\n", ESP01s.Hour, ESP01s.Min, ESP01s.Sec);  // Print time to serial monitor
    delay(10);
  }
}

