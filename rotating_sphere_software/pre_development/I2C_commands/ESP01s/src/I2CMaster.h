// 

#ifndef I2CMASTER_H
#define I2CMASTER_H

#include <Arduino.h>
#include <FS.h>  
#include <LittleFS.h>
#include <Wire.h>

#define MAX_CHUNK_SIZE 60
#define START_RESPONSE 0xBB

class I2CMaster {
public:
    bool begin();
    bool ping();
    bool startUpload(const char* destinationPath);
    bool uploadChunk(uint32_t offset, const uint8_t* data, uint8_t length);
    bool endUpload();
    bool startDownload(const char* filePath);
    bool readChunk(uint32_t offset, uint8_t length, uint8_t* buffer, uint8_t* actualLen);
    bool endDownload();       
    bool sendFramedCommand(uint8_t command, const uint8_t* payload, uint8_t payloadLength);

private:
    const uint8_t slaveAddress = 0x17;
    static constexpr uint8_t START_BYTE = 0xAA;
    static constexpr size_t MAX_FRAME_SIZE = 128;
};
  
#endif
