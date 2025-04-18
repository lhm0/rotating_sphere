#include <Arduino.h>
#include <LittleFS.h>
#include "I2CMaster.h"
#include "I2CFileTransfer.h"

I2CMaster i2c;
I2CFileTransfer fileTransfer(i2c);

enum class TestStage {
  PING,
  SELECT_IMAGE,
  SET_TIME,
  SET_TZ,
  LIST_DIR,
  DONE
};

TestStage testStage = TestStage::PING;
bool actionStarted = false;

void setup() {
  Serial.begin(115200);
  delay(2000);

  i2c.begin();
  delay(500);

  if (!LittleFS.begin()) {
    Serial.println("❌ Fehler: LittleFS konnte nicht gemountet werden.");
    return;
  }
  Serial.println("✅ LittleFS bereit.");
}

void loop() {
  fileTransfer.update();  // I2C-Prozesse verarbeiten

  static bool actionStarted = false;
  static bool downloadStarted = false;
  static bool fileShown = false;
  static bool downloadFailed = false;
  static bool listStarted = false;
  static bool listFinished = false;

  // Schritt 1: Upload starten
  if (!actionStarted && fileTransfer.state == I2CFileTransfer::State::IDLE) {
      Serial.println("🚀 Starte Dateiübertragungs-Test...");
      const char* localUpload = "/html/index.html";
      const char* remoteUpload = "/uploaded_index_test.html";
      if (fileTransfer.beginUpload(localUpload, remoteUpload)) {
          Serial.println("📤 Upload gestartet...");
          actionStarted = true;
      } else {
          Serial.println("❌ Upload-Start fehlgeschlagen.");
      }
  }

  // Schritt 2: Wenn Upload fertig, starte Download
  if (actionStarted && !downloadStarted && !downloadFailed && fileTransfer.isIdle()) {
      Serial.println("📤 Upload abgeschlossen. Starte Download...");
      const char* remoteDownload = "/uploaded_index_test.html";
      const char* localDownload = "/html/index_downloaded.html";
      if (fileTransfer.beginDownload(remoteDownload, localDownload)) {
          Serial.println("📥 Download gestartet...");
          downloadStarted = true;
      } else {
          Serial.println("❌ Download-Start fehlgeschlagen.");
          downloadFailed = true;  // ➕ VERHINDERE ENDLOSSCHLEIFE!
      }
  }

  // Schritt 3: Wenn alles abgeschlossen, zeige Datei
  if (downloadStarted && fileTransfer.state == I2CFileTransfer::State::IDLE && fileTransfer.mode == I2CFileTransfer::Mode::NONE && !fileShown) {
      Serial.println("📄 Zeige heruntergeladene Datei:");

      File downloaded = LittleFS.open("/html/index_downloaded.html", "r");
      if (!downloaded) {
          Serial.println("❌ Konnte heruntergeladene Datei nicht öffnen.");
      } else {
          while (downloaded.available()) {
              Serial.write(downloaded.read());
          }
          downloaded.close();
          Serial.println("\n✅ Datei-Anzeige abgeschlossen.");
      }

      fileShown = true;
  }

  // Schritt 4: Wenn Datei-Anzeige erfolgt ist, starte LIST_DIR
  if (fileShown && !listStarted && fileTransfer.isIdle()) {
      Serial.println("📂 Starte Verzeichnisabfrage auf Slave...");
      const char* remoteDir = "/";
      const char* localFile = "/html/remote_dir.json";
      if (fileTransfer.beginListRemoteDir(remoteDir, localFile)) {
          Serial.println("📥 LIST_DIR gestartet...");
          listStarted = true;
      } else {
          Serial.println("❌ LIST_DIR konnte nicht gestartet werden");
      }
  }

  // Schritt 5: Wenn LIST_DIR fertig, zeige Datei
  if (listStarted && fileTransfer.isIdle() && !listFinished) {
      Serial.println("📄 Zeige remote_dir.json:");
      File f = LittleFS.open("/html/remote_dir.json", "r");
      if (!f) {
          Serial.println("❌ Konnte remote_dir.json nicht öffnen.");
      } else {
          while (f.available()) {
              Serial.write(f.read());
          }
          f.close();
          Serial.println("\n✅ Verzeichnisanzeige abgeschlossen.");
      }
      listFinished = true;
  }

  delay(10);
}
