// 

#ifndef I2C_H
#define I2C_H

#include <Arduino.h>
#include "i2c_slave.h"
#include "i2c_fifo.h"
#include "SDManager.h"
#include <SdFat.h>  

#define I2C_SLAVE_ADDRESS 0x17
#define I2C_BAUDRATE 400000
#define I2C_SLAVE_SDA_PIN 12
#define I2C_SLAVE_SCL_PIN 13


enum class FrameState {
    WAIT_START,
    READ_HEADER,
    READ_PAYLOAD,
    READ_CRC
};

class I2CSlave {
public:
    void begin();
    void i2cService(); // call in loop()

private:
    static void onI2CEvent(i2c_inst_t* i2c, i2c_slave_event_t event);

    static constexpr uint8_t START_BYTE = 0xAA;
    static constexpr size_t MAX_FRAME_SIZE = 128;

    static FrameState frameState;
    static uint8_t rxBuffer[MAX_FRAME_SIZE];
    static size_t rxPos;
    static uint8_t payloadLength;
    static bool commandReady;
    static FsFile uploadFile;
    static char finalFilename[256];
    static FsFile downloadFile;
    static uint8_t responseBuffer[64];
    static uint8_t responseLength;
    static char selectedImagePath[256];
    static char currentTimeZone[64];

    static void resetFrame();
    static bool verifyCRC(const uint8_t* data, size_t len);
    static void handleCommand();
    static void prepareResponse();
};

#endif
