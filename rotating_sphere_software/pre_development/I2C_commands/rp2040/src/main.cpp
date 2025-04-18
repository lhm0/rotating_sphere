// =========================================================================================================================================
//                                                 Rotating Sphere RS64c
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================

#include <Arduino.h>
#include "SDManager.h"
#include "i2cslave.h"

I2CSlave I2C;                        // instance of i2c
SdFs SD;
SDManager SDM;

void setup(){

  // Serial port for debugging purposes
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("start ............");

  // initialize SD card reader
  int tries = 0;
  while (!SDM.begin()) {
    tries++;
    if (tries>20) break;
    delay(100);
  }
  if (tries>20) {
    Serial.println("SD card initialization failed. (msg 04)");
    return;
  } else {
    Serial.print(tries);
    Serial.println(" tries. SD card initialization succeeded");
  }
  char path[] = "/";
  SDM.listDirectory(path);   

  // initialize i2c
  I2C.begin();
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 

void loop() {
  // check i2c commands
  I2C.i2cService();         // check i2c 
  delay(1);
}


