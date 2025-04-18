#include "I2CMaster.h"
#include "global.h"

bool I2CMaster::begin() {
    Wire.begin(2, 0);                         // SDA = GPIO2, SCL = GPIO0
    return true;
}

bool I2CMaster::sendFramedCommand(uint8_t command, const uint8_t* payload, uint8_t payloadLength) {
    if (payloadLength > MAX_FRAME_SIZE - 4) {
        Serial.printf("❌ Payload zu groß: %u Bytes (max: %u)\n", payloadLength, MAX_FRAME_SIZE - 4);
        return false;
    }

    uint8_t frame[MAX_FRAME_SIZE];
    size_t pos = 0;
    frame[pos++] = START_BYTE;
    frame[pos++] = command;
    frame[pos++] = payloadLength;

    uint8_t crc = START_BYTE ^ command ^ payloadLength;

    for (uint8_t i = 0; i < payloadLength; ++i) {
        frame[pos++] = payload[i];
        crc ^= payload[i];
    }

    frame[pos++] = crc;

    Wire.beginTransmission(slaveAddress);
    Wire.write(frame, pos);
    int result = Wire.endTransmission(true);

    return result == 0;
}

bool I2CMaster::ping() {
    return sendFramedCommand(0x05, nullptr, 0);
}

bool I2CMaster::startUpload(const char* destinationPath) {
    uint8_t nameLen = strlen(destinationPath);
    if (nameLen == 0 || nameLen > 255) return false;

    uint8_t payload[1 + nameLen];
    payload[0] = nameLen;
    memcpy(&payload[1], destinationPath, nameLen);

    return sendFramedCommand(0x10, payload, 1 + nameLen);
}

bool I2CMaster::uploadChunk(uint32_t offset, const uint8_t* data, uint8_t length) {
    uint8_t payload[5 + MAX_CHUNK_SIZE];
    payload[0] = offset & 0xFF;
    payload[1] = (offset >> 8) & 0xFF;
    payload[2] = (offset >> 16) & 0xFF;
    payload[3] = (offset >> 24) & 0xFF;
    payload[4] = length;
    memcpy(&payload[5], data, length);

    bool success = sendFramedCommand(0x11, payload, 5 + length);
    if (!success) {
        Serial.println("❌ uploadChunk(): sendFramedCommand() fehlgeschlagen");
    }
    return success;
}

bool I2CMaster::endUpload() {
    return sendFramedCommand(0x12, nullptr, 0);
}

bool I2CMaster::startDownload(const char* filePath) {
    uint8_t pathLen = strlen(filePath);
    if (pathLen == 0 || pathLen > 255) return false;

    uint8_t payload[1 + pathLen];
    payload[0] = pathLen;
    memcpy(&payload[1], filePath, pathLen);

    return sendFramedCommand(0x20, payload, 1 + pathLen);
}

bool I2CMaster::readChunk(uint32_t offset, uint8_t maxLength, uint8_t* buffer, uint8_t* received) {
    *received = 0;
    if (maxLength == 0 || maxLength > MAX_CHUNK_SIZE) return false;

    // 1. Anfrage vorbereiten
    uint8_t payload[5] = {
        (uint8_t)(offset & 0xFF),
        (uint8_t)((offset >> 8) & 0xFF),
        (uint8_t)((offset >> 16) & 0xFF),
        (uint8_t)((offset >> 24) & 0xFF),
        maxLength
    };

    if (!sendFramedCommand(0x21, payload, 5)) {
        Serial.println("❌ READ_CHUNK: Senden fehlgeschlagen");
        return false;
    }

    delay(2);  // kurze Slave-Verarbeitungszeit

    const uint8_t maxExpected = 3 + maxLength + 1;  // Header + Daten + CRC
    Wire.requestFrom(slaveAddress, maxExpected);

    // 🔁 Warte auf vollständige Antwort
    unsigned long startWait = millis();
    while (Wire.available() < 4) {  // Mindestens Header + CRC
        if (millis() - startWait > 100) {
            Serial.println("⏱️ Timeout beim Warten auf minimale Antwort!");
            return false;
        }
        delay(1);
    }

    // 🧠 Daten einlesen
    uint8_t response[3 + MAX_CHUNK_SIZE + 1] = {0};
    uint8_t actualLen = 0;

    while (Wire.available() && actualLen < sizeof(response)) {
        response[actualLen++] = Wire.read();
    }

    // Header prüfen
    if (actualLen < 4) {
        Serial.printf("❌ Antwort zu kurz (%d Bytes)\n", actualLen);
        return false;
    }

    uint8_t start   = response[0];
    uint8_t status  = response[1];
    uint8_t dataLen = response[2];

    if (start != START_RESPONSE) {
        Serial.printf("❌ Ungültiger Start-Byte: 0x%02X\n", start);
        return false;
    }

    if (dataLen > maxLength) {
        Serial.printf("❌ Datenlänge zu groß (%u > %u)\n", dataLen, maxLength);
        return false;
    }

    if (actualLen < 3 + dataLen + 1) {
        Serial.printf("❌ Antwort unvollständig (%d von %d Bytes)\n", actualLen, 3 + dataLen + 1);
        return false;
    }

    // 🔒 CRC prüfen
    uint8_t crc = 0;
    crc ^= start; 
    crc ^= status;
    crc ^= dataLen;
    for (uint8_t i = 0; i < dataLen; ++i) {
        buffer[i] = response[3 + i];
        crc ^= buffer[i];
    }
    uint8_t crcReceived = response[3 + dataLen];

    if (crc != crcReceived) {
        Serial.printf("❌ CRC-Fehler: berechnet=0x%02X, empfangen=0x%02X\n", crc, crcReceived);
        return false;
    }

    *received = dataLen;
    return true;
}

bool I2CMaster::endDownload() {
    return sendFramedCommand(0x22, nullptr, 0); // kein Payload
}

