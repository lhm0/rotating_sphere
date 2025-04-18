#include "i2cslave.h"

// Static Variable Definition
FrameState I2CSlave::frameState = FrameState::WAIT_START;
uint8_t I2CSlave::rxBuffer[MAX_FRAME_SIZE];
size_t I2CSlave::rxPos = 0;
uint8_t I2CSlave::payloadLength = 0;
bool I2CSlave::commandReady = false;
FsFile I2CSlave::uploadFile;
char I2CSlave::finalFilename[256] = {0};
FsFile I2CSlave::downloadFile;
uint8_t I2CSlave::responseBuffer[64];
uint8_t I2CSlave::responseLength = 0;
char I2CSlave::selectedImagePath[256] = {0};
char I2CSlave::currentTimeZone[64] = "UTC";

void I2CSlave::begin() {
    gpio_init(I2C_SLAVE_SDA_PIN);
    gpio_set_function(I2C_SLAVE_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SLAVE_SDA_PIN);
 
    gpio_init(I2C_SLAVE_SCL_PIN);
    gpio_set_function(I2C_SLAVE_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SLAVE_SCL_PIN);
 
    i2c_init(i2c0, I2C_BAUDRATE);
    // configure I2C0 for slave mode
    i2c_slave_init(i2c0, I2C_SLAVE_ADDRESS, &onI2CEvent);
}

void I2CSlave::onI2CEvent(i2c_inst_t* i2c, i2c_slave_event_t event) {
    switch (event) {
    case I2C_SLAVE_RECEIVE: {
        uint8_t byte = i2c_read_byte(i2c);

        switch (frameState) {
        case FrameState::WAIT_START:
            if (byte == START_BYTE) {
                rxPos = 0;
                rxBuffer[rxPos++] = byte;
                frameState = FrameState::READ_HEADER;
            }
            break;

        case FrameState::READ_HEADER:
            rxBuffer[rxPos++] = byte;
            if (rxPos == 3) {
                payloadLength = rxBuffer[2];
                if (payloadLength > MAX_FRAME_SIZE - 4) {
                    resetFrame();
                } else {
                    frameState = FrameState::READ_PAYLOAD;
                }
            }
            break;

        case FrameState::READ_PAYLOAD:
            rxBuffer[rxPos++] = byte;
            if (rxPos == 3 + payloadLength + 1) {
                frameState = FrameState::WAIT_START;
                if (verifyCRC(rxBuffer, rxPos)) {
                    commandReady = true;
                }
                resetFrame();
            }
            break;

        default:
            resetFrame();
            break;
        }
        break;
    }

    case I2C_SLAVE_REQUEST:
        i2c_write_raw_blocking(i2c, I2CSlave::responseBuffer, I2CSlave::responseLength);
    break;


    case I2C_SLAVE_FINISH:
        break;
    }
}

void I2CSlave::i2cService() {
    if (commandReady) {
        handleCommand();
        commandReady = false;
    }
}

void I2CSlave::handleCommand() {
    uint8_t cmd = rxBuffer[1];
    uint8_t len = rxBuffer[2];
    const uint8_t* payload = &rxBuffer[3];

    switch (cmd) {
    case 0x05: {            // PING 
        Serial.printf("PING empfangen\n");
        break;
    }

    case 0x10: { // START_UPLOAD
        if (len < 1) {
            Serial.printf("START_UPLOAD – zu kurz\n");
            break;
        }
    
        uint8_t nameLen = payload[0];
        if (len < 1 + nameLen) {
            Serial.printf("START_UPLOAD – Filename unvollständig\n");
            break;
        }
    
        char filename[256] = {0};
        memcpy(filename, &payload[1], nameLen);
        filename[nameLen] = '\0';
    
        Serial.printf("START_UPLOAD → Ziel: %s\n", filename);
    
        // Alte temp-Datei entfernen
        if (SD.exists("upload.tmp")) {
            SD.remove("upload.tmp");
        }
    
        FsFile f = SD.open("upload.tmp", FILE_WRITE);
        if (!f) {
            Serial.printf("Konnte upload.tmp nicht öffnen\n");
            break;
        }

        if (uploadFile) uploadFile.close();
        uploadFile = std::move(f); 
    
        strncpy(finalFilename, filename, sizeof(finalFilename) - 1);
    
        break;
    }

    case 0x11: { // UPLOAD_CHUNK
        if (len < 5) {
            Serial.printf("UPLOAD_CHUNK – ungültige Länge\n");
            break;
        }
    
        uint32_t offset = payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24);
        uint8_t chunkLen = payload[4];
        const uint8_t* data = &payload[5];
    
        if (len < 5 + chunkLen) {
            Serial.printf("UPLOAD_CHUNK – Payload zu kurz\n");
            break;
        }
    
        Serial.printf("UPLOAD_CHUNK: offset=%lu, len=%u → ", offset, chunkLen);
        for (uint8_t i = 0; i < chunkLen; i++) {
            putchar(data[i]);
        }
        Serial.printf("\n");
    
        if (uploadFile) {
            uploadFile.seek(offset);
            size_t written = uploadFile.write(data, chunkLen);
            if (written != chunkLen) {
                Serial.printf("⚠️ Fehler beim Schreiben: %u von %u Bytes\n", written, chunkLen);
            } else {
                Serial.printf("✔️  %u Bytes erfolgreich geschrieben\n", written);
            }
        } else {
            Serial.println("❌ Datei nicht geöffnet – uploadFile ungültig");
        }
    
        break;
    }
    
    case 0x12: { // END_UPLOAD
        Serial.printf("END_UPLOAD aufgerufen\n");
    
        if (uploadFile) {
            uploadFile.close();
            Serial.printf("Temp-Datei geschlossen\n");
        }
    
        if (!SD.exists("upload.tmp")) {
            Serial.printf("upload.tmp existiert nicht!\n");
            break;
        }
    
        if (SD.exists(finalFilename)) {
            SD.remove(finalFilename);
            Serial.printf("Alte Datei %s gelöscht\n", finalFilename);
        }
    
        if (!SD.rename("upload.tmp", finalFilename)) {
            Serial.printf("Fehler beim Umbenennen!\n");
            break;
        }
    
        Serial.printf("Upload abgeschlossen: %s\n", finalFilename);
        break;
    }

    case 0x20: { // START_DOWNLOAD
        if (len < 1) {
            Serial.println("START_DOWNLOAD – zu kurz");
            break;
        }
    
        uint8_t nameLen = payload[0];
        if (len < 1 + nameLen) {
            Serial.println("START_DOWNLOAD – Pfad unvollständig");
            break;
        }
    
        char filename[256] = {0};
        memcpy(filename, &payload[1], nameLen);
        filename[nameLen] = '\0';
    
        Serial.printf("START_DOWNLOAD → Datei: %s\n", filename);
    
        if (downloadFile) {
            downloadFile.close();
        }
    
        if (!SD.exists(filename)) {
            Serial.println("❌ Datei existiert nicht");
            break;
        }
    
        downloadFile = SD.open(filename, FILE_READ);
        if (!downloadFile) {
            Serial.println("❌ Konnte Datei nicht öffnen");
            break;
        }
    
        Serial.println("START_DOWNLOAD erfolgreich");
        break;
    }

    case 0x21: { // READ_CHUNK
        if (len != 5) {
            Serial.println("READ_CHUNK – ungültige Länge");
            break;
        }
        
        if (!downloadFile) {
            Serial.println("READ_CHUNK – keine Datei geöffnet");
            break;
        }
        
        uint32_t offset = payload[0] |
            (payload[1] << 8) |
            (payload[2] << 16) |
            (payload[3] << 24);
        uint8_t length = payload[4];
        
        Serial.printf("READ_CHUNK: offset=%lu, len=%u\n", offset, length);
        
        // Datei an richtige Stelle positionieren
        downloadFile.seek(offset);
        
        // Lese Daten aus Datei in Response-Buffer ab Position 3
        int bytesRead = downloadFile.read(&responseBuffer[3], length);
        if (bytesRead < 0) bytesRead = 0;
        
        // Header in den Response-Buffer schreiben
        responseBuffer[0] = 0xBB;          // Antwort-Startbyte
        responseBuffer[1] = 0x00;          // Status OK
        responseBuffer[2] = bytesRead;     // tatsächliche Länge der Daten
        
        // CRC berechnen über Header + Daten
        uint8_t crc = 0;
        for (int i = 0; i < 3 + bytesRead; i++) {
            crc ^= responseBuffer[i];
        }
        responseBuffer[3 + bytesRead] = crc;
        
        responseLength = 4 + bytesRead;
        
        Serial.printf("→ Antwort vorbereitet (%d Datenbytes + 1 CRC-Byte)\n", bytesRead);
        break;
    }

    case 0x22: { // END_DOWNLOAD
        Serial.println("END_DOWNLOAD aufgerufen");
    
        if (downloadFile) {
            downloadFile.close();
            Serial.println("Download-Datei geschlossen");
        } else {
            Serial.println("Keine Datei zum Schließen");
        }
    
        // Status zurücksetzen
        // (z. B. state = IDLE, falls du eine State Machine nutzt)
    
        // Bestätigungsantwort vorbereiten
        responseBuffer[0] = 0xBB;
        responseBuffer[1] = 0x00; // Status OK
        responseBuffer[2] = 0x00; // 0 Datenbytes
        responseBuffer[3] = responseBuffer[0] ^ responseBuffer[1] ^ responseBuffer[2];
        responseLength = 4;
    
        break;
    }    

    case 0x30: { // LIST_DIR
        if (len < 1) {
            Serial.println("LIST_DIR – ungültige Länge");
            break;
        }
    
        uint8_t nameLen = payload[0];
        if (len < 1 + nameLen || nameLen >= 255) {
            Serial.println("LIST_DIR – Pfad unvollständig oder zu lang");
            break;
        }
    
        char dirPath[256] = {0};
        memcpy(dirPath, &payload[1], nameLen);
        dirPath[nameLen] = '\0';
    
        Serial.printf("LIST_DIR → Verzeichnis: %s\n", dirPath);
    
        FsFile dir = SD.open(dirPath);
        if (!dir || !dir.isDirectory()) {
            Serial.println("❌ Pfad ist kein Verzeichnis oder konnte nicht geöffnet werden");
            break;
        }
    
        SD.remove("/dirinfo.json");
        FsFile out = SD.open("/dirinfo.json", FILE_WRITE);
        if (!out) {
            Serial.println("❌ Konnte /dirinfo.json nicht erstellen");
            dir.close();
            break;
        }
    
        out.println("{");
        out.print("  \"files\": [");
    
        bool first = true;
        dir.rewind();
        while (FsFile entry = dir.openNextFile()) {
            if (!entry.isDirectory()) {
                char nameBuf[64];
                entry.getName(nameBuf, sizeof(nameBuf));
                if (!first) out.print(", ");
                out.printf("\"%s\"", nameBuf);
                first = false;
            }
            entry.close();
        }
    
        out.println("],");
        out.print("  \"directories\": [");
    
        first = true;
        dir.rewind();
        while (FsFile entry = dir.openNextFile()) {
            if (entry.isDirectory()) {
                char nameBuf[64];
                entry.getName(nameBuf, sizeof(nameBuf));
                if (!first) out.print(", ");
                out.printf("\"%s\"", nameBuf);
                first = false;
            }
            entry.close();
        }
    
        out.println("]");
        out.println("}");
    
        out.close();
        dir.close();
    
        Serial.println("✅ LIST_DIR abgeschlossen (→ /dirinfo.json erstellt)");
    
        // Datei vorbereiten für folgenden Download
        if (downloadFile) downloadFile.close();
        downloadFile = SD.open("/dirinfo.json", FILE_READ);
    
        if (!downloadFile) {
            Serial.println("❌ Konnte /dirinfo.json nicht zum Download öffnen");
        } else {
            Serial.println("📂 /dirinfo.json für Download geöffnet");
        }
    
        break;
    }

    case 0xE0: {
        if (len < 1 || len > 255) {
            Serial.println("selectImageFile – ungültige Länge");
            break;
        }
    
        uint8_t pathLen = payload[0];
        if (len < 1 + pathLen) {
            Serial.println("selectImageFile – Pfad zu kurz");
            break;
        }
    
        memcpy(selectedImagePath, &payload[1], pathLen);
        selectedImagePath[pathLen] = '\0';
    
        Serial.printf("🖼️ Bild-Dateipfad ausgewählt: %s\n", selectedImagePath);
        break;
    }
    
    case 0xE1: {
        if (len != 4) {
            Serial.println("setTime – ungültige Länge");
            break;
        }
    
        uint32_t ts = payload[0] |
                      (payload[1] << 8) |
                      (payload[2] << 16) |
                      (payload[3] << 24);
    
        // Hier die RTC oder Systemzeit setzen
        Serial.printf("⏰ Uhrzeit gesetzt: %lu (Unix)\n", ts);
    
        break;
    }
    
    case 0xE2: { // setTimeZone
        if (len < 1 || len > 63) {
            Serial.println("setTimeZone – ungültige Länge");
            break;
        }
    
        uint8_t tzLen = payload[0];
        if (len < 1 + tzLen) {
            Serial.println("setTimeZone – Payload zu kurz");
            break;
        }
    
        memcpy(currentTimeZone, &payload[1], tzLen);
        currentTimeZone[tzLen] = '\0';
    
        Serial.printf("🌍 Zeitzone gesetzt: %s\n", currentTimeZone);
    
        // timeZone anwenden
    
        break;
    }    
                            
    default:
        Serial.printf("Unbekannter Befehl: 0x%02X\n", cmd);
        break;
    }
}

void I2CSlave::resetFrame() {
    rxPos = 0;
    payloadLength = 0;
    frameState = FrameState::WAIT_START;
}

bool I2CSlave::verifyCRC(const uint8_t* data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len - 1; i++) {
        crc ^= data[i];
    }
    return crc == data[len - 1];
}
