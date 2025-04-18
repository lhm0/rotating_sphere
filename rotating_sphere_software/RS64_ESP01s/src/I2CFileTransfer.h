#pragma once
#include <vector>
#include <string> 
#include "I2CMaster.h"
#include <LittleFS.h>

class I2CFileTransfer {
public:

    enum class State {
        IDLE,
        START_SENT,
        TRANSFERRING,
        END_SENT,
        DONE,
        ERROR
    };

    State state = State::IDLE;

    enum class Mode {
        NONE,
        UPLOAD,
        DOWNLOAD,
        LIST_DIR,
        SELECT_IMAGE,
        SET_TIME,
        SET_TZ,
        SET_BRIGHTNESS,
        PING
    };

    Mode mode = Mode::NONE;

    I2CFileTransfer();

 
    bool beginUpload(const char* sourcePath, const char* destinationPath);
    bool beginDownload(const char* remotePath, const char* localPath);
    bool beginListRemoteDir(const char* remoteDirPath, const char* outputFile);

    // other functions
    bool selectImageFile(const char* path);
    bool setTime(uint32_t unixTimestamp);
    bool setTimeZone(const char* timeZone);
    bool setBrightness(uint32_t brightness);


    // Status
    void update();
    void reset();
    bool ping(); 
    bool isBusy() const;
    bool isDone() const;
    bool isError() const;
    bool isIdle() const;
    bool waitUntilDone(unsigned long timeoutMs = 5000);

private:
   

    uint8_t pendingOpcode = 0;
    std::vector<uint8_t> pendingPayload;
    String tempData;

    // Upload
    File sourceFile;
    String remotePath;

    // Download
    File destFile;
    String localPath;

    uint32_t offset = 0;
    static constexpr uint8_t chunkSize = 32;
};
