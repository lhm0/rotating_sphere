/*
 * Copyright (c) 2021 Valentin Milea <valentin.milea@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

 #include <Arduino.h>
 #include <pico/stdlib.h>
 #include <stdio.h>
 #include <string.h>
 #include <i2c.h>
 

static uint64_t timestamp = 0; 

i2c I2C;                // instance of i2c

void setup() {
    Serial.begin(115200);
    I2C.setup_slave();
}

 void loop() {
    if ((time_us_64()-timestamp)>1000000) {
        timestamp = time_us_64();
        char text[12] = {0};

        text[0] = 0x2E;
        for (int i = 1; i < 9; i++) text[i]=I2C.context.mem[i];
        Serial.println(I2C.count);
        Serial.println(text); 

    } 
 }
 
 