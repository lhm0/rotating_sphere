// =========================================================================================================================================
//                                                 Rotating Sphere RS64c
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================

#include <Arduino.h>
#include "LED_control.h"
#include "i2cslave.h"
#include "globals.h"

I2CSlave I2C;                        // instance of i2c

Playlist::PlaylistEntry entry;

void setup(){
  // configure I2C pins as input in order to allow ESP01s to properly boot
  myRP.disableI2Cpins();

  // Serial port for debugging purposes
  Serial.begin(115200);
  delay(1000);
  Serial.println("start ............");

  // initialize RTC and SD card
  myRP.begin();
 
  // initialize pio and LED interrupts
  LEDs.begin();

  // allow ESP01s some time for booting
  delay(2000);      

  // initialize i2c
  I2C.begin();

  // generate playlist
  playlist.autoCreate("/");

  entry = playlist.nextTitle();
  if (entry.file != nullptr) {
      strncpy(playlist.playingNowTitle, entry.file, sizeof(playlist.playingNowTitle) - 1);
      playlist.playingNowTitle[sizeof(playlist.playingNowTitle) - 1] = '\0';  // Sicherheit: null-terminieren
  }

  Serial.print("📸 First file in playlist: '");
  Serial.println(playlist.playingNowTitle);

  // open image file
  if (!playlist.openPlaylistTitle(playlist.playingNowTitle)) {
    Serial.println("error opening image file");
  }

}

void setup1() {

}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 

void loop() {
  static uint32_t frameDisplayed=0;
  static uint64_t timeStampDataRead=0;
  static uint64_t timeStampStatusMessage=0;
  static int repetition = 0;

  uint64_t timeStamp = millis();
  if ((timeStamp - timeStampDataRead)>playlist.timePerFrame) {

    frameDisplayed++;
    if ((frameDisplayed >= playlist.framesMax)||(playlist.updateNow)) {
      playlist.updateNow = false;

      playlist.filePlayingNow.close();

      repetition++;
      if (repetition>=playlist.playingNowRepetitions) {
        repetition=0;
        entry = playlist.nextTitle();
      }
      if (entry.file != nullptr) {
          strncpy(playlist.playingNowTitle, entry.file, sizeof(playlist.playingNowTitle) - 1);
          playlist.playingNowTitle[sizeof(playlist.playingNowTitle) - 1] = '\0';  // Sicherheit: null-terminieren
      }
      playlist.playingNowRepetitions = entry.repetition;
      BMP.clockOn = entry.clockOn;

      if (!playlist.openPlaylistTitle(playlist.playingNowTitle)) {
        Serial.println("error opening image file");
      }
      frameDisplayed=0;
    }  

    if (!playlist.loadNextImage(LEDs.lines)) {
      Serial.println("Error reading image file (msg 03)");
    }
  
    timeStampDataRead = millis();
  }

  // send status message via Serial.print() every second
  if ((millis()-timeStampStatusMessage)>1000) {
    myRP.getTime();
    Serial.printf("%02d:%02d:%02d,  frame: %d\n", myRP.local.tm_hour, myRP.local.tm_min, myRP.local.tm_sec, frameDisplayed);
    timeStampStatusMessage=millis();
  }

  // check i2c commands
  I2C.i2cService();         // check i2c 
}

void loop1() {
  static uint32_t last_update = 0;
  if ((millis()-last_update)>1000) {
    last_update = millis();
    BMP.generateBMP(1);
    BMP.convertBMPtoLines();
  }
}
