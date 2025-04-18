#include "I2CFileTransfer.h"
#include "global.h"

I2CFileTransfer::I2CFileTransfer() {}

void I2CFileTransfer::reset() {
    if (sourceFile) sourceFile.close();
    if (destFile) destFile.close();
    offset = 0;
    mode = Mode::NONE;
    state = State::IDLE;
}

bool I2CFileTransfer::beginUpload(const char* source, const char* destination) {
    if (state != State::IDLE) return false;

    sourceFile = LittleFS.open(source, "r");
    if (!sourceFile) {
        Serial.println("❌ Konnte Quelldatei nicht öffnen");
        return false;
    }

    remotePath = destination;
    offset = 0;
    mode = Mode::UPLOAD;
    state = State::START_SENT;
    return true;
}

bool I2CFileTransfer::beginDownload(const char* remote, const char* local) {
    if (mode != Mode::NONE || state != State::IDLE) {
        Serial.println("⚠️ Download bereits aktiv oder anderer Modus belegt");
        return false;
    }

    // Pfade zwischenspeichern
    remotePath = remote;
    localPath = local;

    // Versuche, Zieldatei zu öffnen
    destFile = LittleFS.open(localPath.c_str(), "w");
    if (!destFile) {
        Serial.println("❌ Konnte lokale Datei zum Schreiben nicht öffnen");
        return false;
    }

    // Download-Befehl vorbereiten
    tempData = remotePath;  // für LIST_DIR und generelles Re-Use
    state = State::START_SENT;
    mode = Mode::DOWNLOAD;
    offset = 0;

    return true;
}


bool I2CFileTransfer::beginListRemoteDir(const char* remoteDir, const char* outputFile) {
    if (state != State::IDLE) return false;

    remotePath = "/dirinfo.json";  // erwartet vom Slave
    localPath = outputFile;
    tempData = remoteDir; // merke dir Zielverzeichnis
    offset = 0;

    mode = Mode::LIST_DIR;
    state = State::START_SENT;
    return true;
}

bool I2CFileTransfer::selectImageFile(const char* path) {
    if (state != State::IDLE) return false;

    uint8_t len = strlen(path);
    if (len == 0 || len > 255) return false;

    pendingOpcode = 0xE0;
    pendingPayload.clear();
    pendingPayload.push_back(len);
    pendingPayload.insert(pendingPayload.end(), path, path + len);

    mode = Mode::SELECT_IMAGE;
    state = State::START_SENT;
    return true;
}

bool I2CFileTransfer::setTime(uint32_t unixTimestamp) {
    if (state != State::IDLE) return false;

    pendingOpcode = 0xE1;
    pendingPayload.clear();
    pendingPayload.push_back(unixTimestamp & 0xFF);
    pendingPayload.push_back((unixTimestamp >> 8) & 0xFF);
    pendingPayload.push_back((unixTimestamp >> 16) & 0xFF);
    pendingPayload.push_back((unixTimestamp >> 24) & 0xFF);

    mode = Mode::SET_TIME;
    state = State::START_SENT;
    return true;
}

bool I2CFileTransfer::setTimeZone(const char* timeZone) {
    if (state != State::IDLE) return false;

    uint8_t len = strlen(timeZone);
    if (len == 0 || len > 255) return false;

    pendingOpcode = 0xE2;
    pendingPayload.clear();
    pendingPayload.push_back(len);
    pendingPayload.insert(pendingPayload.end(), timeZone, timeZone + len);

    mode = Mode::SET_TZ;
    state = State::START_SENT;
    return true;
}

bool I2CFileTransfer::setBrightness(uint32_t brightness) {
    if (state != State::IDLE) return false;

    pendingOpcode = 0xE3;
    pendingPayload.clear();
    pendingPayload.push_back(brightness & 0xFF);
    pendingPayload.push_back((brightness >> 8) & 0xFF);
    pendingPayload.push_back((brightness >> 16) & 0xFF);
    pendingPayload.push_back((brightness >> 24) & 0xFF);

    mode = Mode::SET_BRIGHTNESS;
    state = State::START_SENT;
    return true;
}

void I2CFileTransfer::update() {
    // Kein aktiver Vorgang? Nichts zu tun.
    if (state == State::IDLE || mode == Mode::NONE) return;

    // === LIST_DIR starten ===
    if (state == State::START_SENT && mode == Mode::LIST_DIR) {
        uint8_t pathLen = tempData.length();
        uint8_t payload[1 + pathLen];
        payload[0] = pathLen;
        memcpy(&payload[1], tempData.c_str(), pathLen);

        if (i2c.sendFramedCommand(0x30, payload, 1 + pathLen)) {
            delay(50);              // needed, as Slave only reacts with delay

            destFile = LittleFS.open(localPath.c_str(), "w");
            if (!destFile) {
                Serial.println("❌ Konnte Zieldatei nicht öffnen");
                state = State::ERROR;
                return;
            }

            state = State::TRANSFERRING;
            mode = Mode::DOWNLOAD;  // Wechsel zu regulärem Download-Mode

            offset = 0;
        } else {
            Serial.println("❌ LIST_DIR Übertragung fehlgeschlagen");
            state = State::ERROR;
        }
        return;
    }

    // === PING senden ===
    if (state == State::START_SENT && mode == Mode::PING) {
        if (i2c.ping()) {
            Serial.println("✅ PING erfolgreich");
            state = State::IDLE;
        } else {
            Serial.println("❌ PING fehlgeschlagen");
            state = State::ERROR;
        }
        mode = Mode::NONE;
        return;
    }

    // === Einfache Befehle: SELECT_IMAGE / SET_TIME / SET_TZ / SET_BRIGHTNESS ===
    if (state == State::START_SENT &&
        (mode == Mode::SELECT_IMAGE || mode == Mode::SET_TIME || mode == Mode::SET_TZ || mode == Mode::SET_BRIGHTNESS)) {

        if (i2c.sendFramedCommand(pendingOpcode, pendingPayload.data(), pendingPayload.size())) {
            Serial.printf("✅ Opcode 0x%02X gesendet\n", pendingOpcode);

            pendingPayload.clear();
            mode = Mode::NONE;
            state = State::IDLE; 
        } else {
            Serial.printf("❌ Fehler bei Opcode 0x%02X\n", pendingOpcode);

            pendingPayload.clear();
            mode = Mode::NONE;
            state = State::ERROR;
        }
        return;
    }

    // === START_UPLOAD senden ===
    if (mode == Mode::UPLOAD && state == State::START_SENT) {
        uint8_t pathLen = remotePath.length();
        uint8_t payload[1 + pathLen];
        payload[0] = pathLen;
        memcpy(&payload[1], remotePath.c_str(), pathLen);

        if (i2c.sendFramedCommand(0x10, payload, 1 + pathLen)) {
            state = State::TRANSFERRING;
        } else {
            Serial.println("❌ START_UPLOAD fehlgeschlagen");
            state = State::ERROR;
        }
        return;
    }

    // === Datei-Upload: Übertragung abgeschlossen? ===
    if (mode == Mode::UPLOAD && state == State::TRANSFERRING &&
        (!sourceFile || !sourceFile.available())) {
        sourceFile.close();

        if (i2c.endUpload()) {
            state = State::IDLE;        // ⬅️ Änderung: direkt zu IDLE
            mode = Mode::NONE;
        } else {
            Serial.println("❌ END_UPLOAD fehlgeschlagen");
            state = State::ERROR;
        }
        return;
    }

    // === Datei-Upload: Nächsten Chunk senden ===
    if (mode == Mode::UPLOAD && state == State::TRANSFERRING) {
        const uint8_t chunkSize = MAX_CHUNK_SIZE;
        uint8_t buffer[chunkSize];
        size_t readLen = sourceFile.read(buffer, chunkSize);

        if (i2c.uploadChunk(offset, buffer, readLen)) {
            offset += readLen;
        } else {
            Serial.println("❌ Fehler beim Senden eines Upload-Chunks");
            state = State::ERROR;
        }
        return;
    }

    // === START_DOWNLOAD senden ===
    if (mode == Mode::DOWNLOAD && state == State::START_SENT) {
        uint8_t pathLen = remotePath.length();
        uint8_t payload[1 + pathLen];
        payload[0] = pathLen;
        memcpy(&payload[1], remotePath.c_str(), pathLen);

        if (i2c.sendFramedCommand(0x20, payload, 1 + pathLen)) {
            Serial.println("📥 START_DOWNLOAD gesendet");

            destFile = LittleFS.open(localPath.c_str(), "w");
            if (!destFile) {
                Serial.println("❌ Konnte Zieldatei nicht öffnen");
                state = State::ERROR;
                return;
            }

            state = State::TRANSFERRING;
            offset = 0;
        } else {
            Serial.println("❌ START_DOWNLOAD fehlgeschlagen");
            state = State::ERROR;
        }
        return;
    }

    // === Datei-Download: Chunk lesen ===
    if (mode == Mode::DOWNLOAD && state == State::TRANSFERRING) {
        const uint8_t chunkSize = MAX_CHUNK_SIZE;
        uint8_t buffer[chunkSize];
        uint8_t received = 0;

        if (i2c.readChunk(offset, chunkSize, buffer, &received)) {
            if (received > 0) {
                destFile.write(buffer, received);
                offset += received;
            } else {
                destFile.close();
                state = State::IDLE;   // ✅ auch hier: direkt zu IDLE
                mode = Mode::NONE;
            }
        } else {
            Serial.println("❌ Fehler beim Lesen eines Download-Chunks");
            destFile.close();
            state = State::ERROR;
        }
        return;
    }
}

bool I2CFileTransfer::isBusy() const {
    return state == State::START_SENT || state == State::TRANSFERRING || state == State::END_SENT;
}

bool I2CFileTransfer::isDone() const { 
    return state == State::DONE && mode == Mode::NONE; 
}

bool I2CFileTransfer::isError() const {
    return state == State::ERROR;
}

bool I2CFileTransfer::isIdle() const {
    return state == State::IDLE && mode == Mode::NONE;
}

bool I2CFileTransfer::waitUntilDone(unsigned long timeoutMs) {
    unsigned long start = millis();
    while (!isIdle()) {
        update();
        delay(5);
        if (millis() - start > timeoutMs) {
            Serial.println("⏰ Timeout erreicht!");
            return false;
        }
    }
    return true;
}

bool I2CFileTransfer::ping() {
    if (state != State::IDLE) return false;  // Nur wenn Leerlauf

    mode = Mode::PING;           // Art des Vorgangs setzen
    state = State::START_SENT;   // Ausführung in update() anstoßen
    return true;
}
