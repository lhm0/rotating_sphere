// =========================================================================================================
// Rotating Display RS64c
// © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// You may use, adapt, share. If you share, "share alike".
// You may not use this software for commercial purposes.
// =========================================================================================================

#ifndef ASYNCWEBSERVER_REGEX
#define ASYNCWEBSERVER_REGEX
#endif

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266WiFi.h>

#include "webInterface.h"
#include "webinterface_data.h"

String webInterface::_currentPath = "/";

// =========================================================================================================
// Constructor
// =========================================================================================================

webInterface::webInterface() {}
// =========================================================================================================
// begin Method
// =========================================================================================================

void webInterface::begin() {

    File file = LittleFS.open(_ssidPath, "r");  // "r" für read

    if (!file) {
        Serial.println("error opening ssid.txt");
    } else {
        size_t len = file.readBytes(_ssid, sizeof(_ssid) - 1);
        _ssid[len] = '\0';  // terminate with /0
        file.close();
    }

    file = LittleFS.open(_passwordPath, "r");  // "r" für read

    if (!file) {
        Serial.println("error opening password.txt");
    } else {
        size_t len = file.readBytes(_password, sizeof(_password) - 1);
        _password[len] = '\0';  // terminate with /0
        file.close();
    }

    _startServer();
}

// =========================================================================================================
// _startServer() and WebSocket Handlers
// =========================================================================================================

void webInterface::_startServer() {
    // Index Page
    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println("index.html requested");
        request->send(LittleFS, "/html/index.html", "text/html");
    });

    _server.on("/index.css", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/html/css/index.css", "text/css");
    });

    _server.on("/index.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/html/scripts/index.js", "text/javascript");
    });

    _server.on("/favicon.ico", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "OK");
    });

    _server.on("/getParam", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["mode"] = clockMode;
        doc["brightness"] = brightness;

        Serial.print("Mode uploaded: ");
        Serial.println(clockMode);

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    _server.on("/configDone", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/html/index.html", "text/html");
    });

    _server.on("/slider", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("value")) {
            brightness = (request->getParam("value")->value()).toInt();
            if (!fileTransfer.setBrightness((uint32_t)brightness)) {          ;
                Serial.println("failed to upload brightness to RP");
            }
            Serial.printf("Slider value = %d\n", brightness);
        } else {
            Serial.println("No slider value sent");
        }
        request->send(200, "text/plain", "OK");
    });

    _server.on("/select-file", HTTP_POST, [this](AsyncWebServerRequest *request) {}, NULL,
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {

        // JSON-Daten parsen
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data, len);

        if (error) {
            Serial.println("Fehler beim Parsen von select-file JSON");
            request->send(400, "application/json", "{\"error\":\"invalid json\"}");
            return;
        }

        const char* selectedFile = doc["filename"];
        if (selectedFile == nullptr || strlen(selectedFile) == 0) {
            request->send(400, "application/json", "{\"error\":\"missing filename\"}");
            return;
        }

        Serial.print("Datei ausgewählt: ");
        Serial.println(selectedFile);

        if (!fileTransfer.selectImageFile(selectedFile)) {
            Serial.printf("error: could not select image file %s\n", selectedFile);
        }

        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });


    // Reset WiFi Page
    _server.on("/resetWifi", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/html/resetWifi.html", "text/html");
    });

    _server.on("/resetWifi.css", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/html/css/resetWifi.css", "text/css");
    });

    _server.on("/resetWifi.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/html/scripts/resetWifi.js", "text/javascript");
    });


    _server.on("/reset", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "OK");
        if (LittleFS.exists(_ssidPath)) {
            // delete file
            if (!LittleFS.remove(_ssidPath)) {
              Serial.println("error deleting ssid.txt");
            }
        }
        if (LittleFS.exists(_passwordPath)) {
            // delete file
            if (!LittleFS.remove(_passwordPath)) {
              Serial.println("error deleting password.txt");
            }
        }
    });

    _server.on("/getWifiParam", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;

        bool success = true;
        File file = LittleFS.open(_ssidPath, "r");  // "r" für read

        if (!file) {
            Serial.println("error opening ssid.txt");
            success = false;
        } else {
            size_t len = file.readBytes(_ssid, sizeof(_ssid) - 1);
            _ssid[len] = '\0';  // terminate with /0
            file.close();
        }

        file = LittleFS.open(_passwordPath, "r");  // "r" für read

        if (!file) {
            Serial.println("error opening password.txt");
            success=false;
        } else {
            size_t len = file.readBytes(_password, sizeof(_password) - 1);
            _password[len] = '\0';  // terminate with /0
            file.close();
        }


        if (success) 
        {
            doc["ssid"] = String(_ssid);
            doc["password"] = String(_password);

            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
        } else {
            request->send(404);
        }
    });

    _server.on("/uploadWifiParam", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("value1") && request->hasParam("value2")) {
            request->getParam("value1")->value().toCharArray(_ssid, sizeof(_ssid));
            request->getParam("value2")->value().toCharArray(_password, sizeof(_password));

            File file = LittleFS.open(_ssidPath, "w");  // "w" = write (truncate + create if not exist)

            if (!file) {
                Serial.println("error opening ssid.txt");
            } else {
                file.print(_ssid);
                file.close();
            }

            file = LittleFS.open(_passwordPath, "w");  // "w" = write (truncate + create if not exist)

            if (!file) {
                Serial.println("error opening password.txt");
            } else {
                file.print(_password);
                file.close();
            }

        }
        request->send(200, "text/plain", "OK");
    });

    _server.on("/list-files", HTTP_GET, [this](AsyncWebServerRequest *request) {

        _currentPath = request->getParam("path")->value();
        String bufferPath = "/remote-dir.json";
    
        Serial.println("📂 Starte LIST_DIR via I2C...");
        
        if (fileTransfer.beginListRemoteDir(_currentPath.c_str(), bufferPath.c_str())) {
            Serial.println("➡️ LIST_DIR Kommando gesendet.");
            request->send(200, "text/plain", "LIST_DIR gestartet.");
        } else {
            Serial.println("❌ Fehler beim Start von LIST_DIR.");
            request->send(500, "text/plain", "Fehler beim Starten von LIST_DIR.");
        }
    });

    _server.on("/download-list-files", HTTP_GET, [this](AsyncWebServerRequest *request) {

        String bufferPath = "/remote-dir.json";
    
        if (fileTransfer.isIdle()) {
            Serial.println("✅ LIST_DIR abgeschlossen, lese Datei...");
    
            File bufferFile = LittleFS.open(bufferPath.c_str(), "r");
            String json = "";
    
            if (bufferFile) {
                json = bufferFile.readString();
                bufferFile.close();
                request->send(200, "application/json", json);
            } else {
                Serial.println("❌ Datei konnte nicht geöffnet werden: " + bufferPath);
                request->send(500, "text/plain", "Datei nicht gefunden.");
            }
    
        } else {
            Serial.println("⏳ LIST_DIR noch nicht abgeschlossen...");
            request->send(202, "text/plain", "LIST_DIR läuft noch. Bitte später erneut anfragen.");
        }
    });

    // Time Zone Page
    _server.on("/timeZone", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/html/timeZone.html", "text/html");
    });

    _server.on("/timeZone.css", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/html/css/timeZone.css", "text/css");
    });

    _server.on("/timeZone.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/html/scripts/timeZone.js", "text/javascript");
    });

    _server.on("/timeZoneData", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String part;
        if (request->hasParam("part")) {
            part = request->getParam("part")->value();
        }

        Serial.print("Call /timeZoneData: ");
        Serial.println(part);

        JsonDocument doc;
        JsonArray timeZoneArray = doc.to<JsonArray>();

        int part_i = part.toInt() - 1;
        for (int i = part_i * 20; i < (20 + part_i * 17); i++) {
            char location[50];
            char timeZone[50];
            char timeDifference[50];

            for (byte k = 0; k < 50; k++) {
                location[k] = pgm_read_byte_near(timeZones[0][i] + k);
                timeZone[k] = pgm_read_byte_near(timeZones[1][i] + k);
                timeDifference[k] = pgm_read_byte_near(timeZones[2][i] + k);
            }

            char entry[150];
            strcpy(entry, location);
            strcat(entry, ": ");
            strcat(entry, timeZone);
            strcat(entry, " (");
            strcat(entry, timeDifference);
            strcat(entry, ")");

            JsonObject timeZoneObj = timeZoneArray.add<JsonObject>();
            timeZoneObj["entry"] = entry;
        }

        String jsonString;
        serializeJson(doc, jsonString);
        request->send(200, "application/json", jsonString);
    });

    _server.on("/timeZoneUpdate", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("value")) {
            String newTimeZone = request->getParam("value")->value();
            int newTimeZone_i = newTimeZone.toInt();
            Serial.println("New time zone = " + newTimeZone);

            char timeZone[100];
            for (byte k = 0; k < 49; k++) {
                timeZone[k] = pgm_read_byte_near(timeZones[3][newTimeZone_i - 1] + k);
            }
            timeZone[49] = '\0';  // Terminierung sicherstellen
            
            File file = LittleFS.open(_timezonePath, "w");  // "w" = write (truncate + create if not exist)

            if (!file) {
                Serial.println("error opening timezone.txt");
            } else {
                file.print(timeZone);
                file.close();
            }

            ESP01s.configureTimeZone();
            Serial.println(timeZone);
            
            ESP01s.updateLocalTime();
            time_t unixTime = ESP01s.tm_to_unixtime(ESP01s.localTime);
            if (!fileTransfer.setTime(unixTime)) {          ;
                Serial.println("failed to upload time to RP");
            }

        } else {
            Serial.println("No time zone sent");
        }
        request->send(200, "text/plain", "OK");
    });

    _server.on("/server-time", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String jsonResponse = "{";
        jsonResponse += "\"hour\": " + String(ESP01s.Hour) + ",";
        jsonResponse += "\"minute\": " + String(ESP01s.Min) + ",";
        jsonResponse += "\"second\": " + String(ESP01s.Sec);
        jsonResponse += "}";

        request->send(200, "application/json", jsonResponse);
    });

    // Playlist Manager Page
    _server.on("/playlistManager", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/html/playlist.html", "text/html");
    });

    _server.on("/playlist.css", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/html/css/playlist.css", "text/css");
    });

    _server.on("/playlist.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println("playlist.js requested");
        request->send(LittleFS, "/html/scripts/playlist.js", "text/javascript");
    });

    _server.on("/get_playlist", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->hasParam("file")) {
            request->send(400, "text/plain", "Missing 'file' parameter");
            return;
        }
    
        String path = request->getParam("file")->value();
        Serial.printf("🎵 get_playlist gestartet für Datei: %s\n", path.c_str());
    
        String bufferPath = "/buffer.dat";  // Zwischenspeicher im LittleFS
    
        if (fileTransfer.beginDownload(path.c_str(), bufferPath.c_str())) {
            Serial.println("➡️ Download über I2C gestartet.");
            request->send(200, "text/plain", "Download gestartet.");
        } else {
            Serial.println("❌ Fehler beim Starten des Downloads.");
            request->send(500, "text/plain", "Download konnte nicht gestartet werden.");
        }
    });

    _server.on("/download_playlist", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String bufferPath = "/buffer.dat";
    
        if (fileTransfer.isIdle()) {
            File bufferFile = LittleFS.open(bufferPath.c_str(), "r");
            String json = "";
    
            if (bufferFile) {
                json = bufferFile.readString();
                bufferFile.close();
                Serial.println("✅ Playlist-Datei erfolgreich geladen:");
                Serial.println(json);
                request->send(200, "application/json", json);
            } else {
                Serial.println("❌ Fehler beim Öffnen von /buffer.dat");
                request->send(500, "text/plain", "Datei konnte nicht geöffnet werden.");
            }
        } else {
            Serial.println("⏳ Playlist-Datei wird noch geladen...");
            request->send(202, "text/plain", "Download läuft noch. Bitte später erneut anfragen.");
        }
    });    

    _server.on("/save_playlist", HTTP_POST,
        [this](AsyncWebServerRequest *request) {}, // leerer Handler
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    
        static String jsonAccum;  // Daten über mehrere Chunks sammeln (falls nötig)
    
        // JSON Stückweise aufbauen
        for (size_t i = 0; i < len; i++) {
            jsonAccum += (char)data[i];
        }
    
        // Letzter Chunk erreicht?
        if (index + len == total) {
            JsonDocument doc; 
            DeserializationError error = deserializeJson(doc, jsonAccum);
    
            if (error) {
                Serial.print("❌ JSON-Fehler: ");
                Serial.println(error.c_str());
                request->send(400, "text/plain", "Fehlerhaftes JSON");
                jsonAccum = "";
                return;
            }
    
            // Dateiname extrahieren
            String filename = doc["filename"] | "/unnamed.play";
            if (!filename.startsWith("/")) filename = "/" + filename;
    
            // Datei schreiben
            File file = LittleFS.open(filename, "w");
            if (!file) {
                request->send(500, "text/plain", "Datei konnte nicht geöffnet werden.");
                jsonAccum = "";
                return;
            }
    
            JsonArray contentArray = doc["content"].as<JsonArray>();
            serializeJson(contentArray, file);
            file.close();
            jsonAccum = "";
    
            // 🔁 Starte I2C-Upload (asynchron)
            if (fileTransfer.beginUpload(filename.c_str(), filename.c_str())) {
                Serial.println("📤 Upload via I2C gestartet.");
                request->send(200, "text/plain", "Playlist gespeichert und Upload gestartet.");
            } else {
                Serial.println("❌ Upload-Start fehlgeschlagen.");
                request->send(500, "text/plain", "Fehler beim Starten des Uploads.");
            }
        }
    });    
    
    // Start Server
    _server.begin();
    Serial.println("Server started");
}


void webInterface::_listFiles(const char *dirname) {
    Serial.printf("Listing files in directory: %s\n", dirname);

    File root = LittleFS.open(dirname, "r");

    if (!root || !root.isDirectory()) {
        Serial.println("Kein Verzeichnis oder Fehler beim Öffnen");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        Serial.print("  FILE: ");
        Serial.print(file.name());
        Serial.print("  SIZE: ");
        Serial.println(file.size());
        file = root.openNextFile();
    }
}
