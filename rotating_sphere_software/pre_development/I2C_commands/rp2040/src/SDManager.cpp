// =========================================================================================================================================
//                                                 Rotating Display RS64c
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================

#include "SDManager.h"

#define RP_CLK_GPIO 20
#define RP_CMD_GPIO 19
#define RP_DAT0_GPIO 15

// Configuration for the SD card
#define SD_CONFIG SdioConfig(RP_CLK_GPIO, RP_CMD_GPIO, RP_DAT0_GPIO)

// Initializes the SD card
bool SDManager::begin() {
    if (!SD.begin(SD_CONFIG)) {
        Serial.println("SD card initialization failed. (msg 01)");
        return false;
    }

    Serial.print("Card type: ");
    Serial.println(SD.fatType() == FAT_TYPE_EXFAT ? "exFAT" : "FAT");

    Serial.print("Card size: ");
    Serial.print(SD.card()->sectorCount() * 512E-9);
    Serial.println(" GB");

    return true;
}

void SDManager::listDirectory(const char* path) {
    FsFile dir = SD.open(path);
    if (!dir) {
        Serial.println("Verzeichnis konnte nicht geöffnet werden.");
        return;
    }
    
    if (!dir.isDirectory()) {
        Serial.println("Pfad ist kein Verzeichnis.");
        return;
    }
    
    FsFile file;
    while ((file = dir.openNextFile())) {
        char name[64];
        file.getName(name, sizeof(name));
    
        Serial.print(name);
        if (file.isDirectory()) {
            Serial.println(" <DIR>");
        } else {
            Serial.print("\t");
            Serial.println(file.size());
        }
        file.close();
    }
    dir.close();
}
    