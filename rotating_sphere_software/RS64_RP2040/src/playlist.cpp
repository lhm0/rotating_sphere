#include "playlist.h"
#include "globals.h"

#define JSON_BUFFER_SIZE 2048  // Define an appropriate buffer size

Playlist::Playlist() {
    currentIndex = 0;
}

bool Playlist::openPlaylistTitle(const char* path) {
    filePlayingNow = SD.open(path, O_RDONLY);
    char buffer[8];  // Buffer for reading 8 bytes (2x uint32_t
    int bytesRead= filePlayingNow.read(buffer, sizeof(buffer));
  
    if (bytesRead == 8) { // Ensure full data is read
      memcpy(&framesMax, buffer, 4);
      memcpy(&timePerFrame, buffer+4, 4);
      Serial.printf("timePerFrame: %d\n", timePerFrame);
      Serial.printf("framesMax: %d\n", framesMax);
      return true;
    } else { 
      Serial.println("Error reading timePerFrame and framesMax (msg 02)");
      return false;
    }
}

bool Playlist::loadPlaylist(String playlistPath) {
    updateNow = true;                                       // may be used by main loop.
    playlist.clear();

    if (playlistPath.endsWith(".RS64")) {
        // Sonderfall: Nur eine Datei als Playlist behandeln
        JsonArray array = playlist.to<JsonArray>();
        JsonObject entry = array.add<JsonObject>();
        entry["Nr"] = 0;
        entry["File"] = playlistPath;
        entry["Repetition"] = 1;
        entry["clock"] = "off";

        currentIndex = 0;

        serializeJsonPretty(playlist, Serial);  // Debug-Ausgabe
        return true;
    }

    // Standardfall: .play-Datei laden
    char buffer[JSON_BUFFER_SIZE];
    FsFile playListFile = SD.open(playlistPath.c_str(),  O_RDONLY);

    if (!playListFile) {
        return false;
    }

    int bytesRead = playListFile.read(buffer, sizeof(buffer)-1);
    playListFile.close();

    if (bytesRead <= 0) {
        return false;
    }

    buffer[bytesRead] = '\0';

    DeserializationError error = deserializeJson(playlist, buffer);
    if (error || !playlist.is<JsonArray>()) {
        return false;
    }

    currentIndex = 0;

    serializeJsonPretty(playlist, Serial);  // Debug-Ausgabe
    return true;
}

bool Playlist::savePlaylist(String playlistPath) {
    char buffer[JSON_BUFFER_SIZE];
    size_t jsonSize = serializeJson(playlist, buffer, sizeof(buffer));
    if (jsonSize == 0) {
        return false;
    }

    FsFile playListFile = SD.open(playlistPath.c_str(), O_RDWR | O_CREAT | O_TRUNC); 
    bool success = false;
    if (playListFile) {
        size_t written = playListFile.write(buffer, jsonSize);
        if (written == jsonSize) {
            Serial.println("✅ JSON saved");
            success = true;
        } else {
            Serial.println("⚠️ failed to save JSON");
            success = false;
        }
    } else {
        Serial.println("❌ failed to save JSON");
        success = false;
    }

    playListFile.close();
    return success;
}

Playlist::PlaylistEntry Playlist::nextTitle() {
    PlaylistEntry entry = {nullptr, 1, false};  // Default-Werte

    if (!playlist.is<JsonArray>()) return entry;

    JsonArray array = playlist.as<JsonArray>();
    if (array.size() == 0) return entry;

    // Zugriff auf aktuellen Titel
    JsonObject current = array[currentIndex];
    entry.file = current["File"];
    entry.repetition = current["Repetition"] | 1;  // Default: 1
    const char* clockValue = current["clock"] | "off";
    entry.clockOn = (strcmp(clockValue, "on") == 0);

    // Index für nächsten Aufruf vorbereiten
    currentIndex++;
    if (currentIndex >= array.size()) {
        currentIndex = 0;
    }

    return entry;
}

bool Playlist::autoCreate(const char* path) {
    playlist.clear();
    JsonArray array = playlist.to<JsonArray>();

    FsFile dir;
    if (!dir.open(path, O_RDONLY)) {
        return false;
    }

    int index = 0;
    FsFile file;
    while (file.openNext(&dir, O_RDONLY)) {
        char filename[64];
        file.getName(filename, sizeof(filename));
        file.close();

        if (filename[0] == '.') continue;

        String fileStr = String(filename);
        if (fileStr.endsWith(".RS64")) {
            JsonObject entry = array.add<JsonObject>();
            entry["Nr"] = index;
            entry["File"] = fileStr;
            entry["Repetition"] = 1;
            entry["clock"] = "off";
            index++;
        }
    }
    dir.close();

    serializeJsonPretty(playlist, Serial);
    currentIndex = 0;
    return savePlaylist("/auto_created.play");
}

bool Playlist::loadNextImage(uint8_t (*lines)[1792][3]) {
    // Calculate expected file size
    int bytesToRead = sizeof(uint8_t) * 8 * 1792 * 3; // 8 * 1792 * 3 bytes

    int bytesRead = filePlayingNow.read(lines, bytesToRead);  // schreibt 100 Bytes ab [3][5][10]
    
    if (bytesRead == bytesToRead) {
        return true;
    } else {
        Serial.print("Error reading data. Bytes read: (msg 11)");
        Serial.println(bytesRead);
        return false;
    }
}