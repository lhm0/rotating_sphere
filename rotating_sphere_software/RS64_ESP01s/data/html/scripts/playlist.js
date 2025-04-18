document.addEventListener("DOMContentLoaded", function () {
    let playlistFileList = document.getElementById("playlistFileList");
    let playlistContent = document.getElementById("playlistContent");
  
    let _currentPath = "/";
    let selectedFile = null;

    let selectedPlaylistRow = null;

    function selectPlaylistRow(row) {
        if (selectedPlaylistRow) {
          selectedPlaylistRow.classList.remove("selected");
        }
        row.classList.add("selected");
        selectedPlaylistRow = row;
      }
  
    function loadDirectory(path) {
      _currentPath = path;
      const listUrl = '/list-files?path=' + encodeURIComponent(path);
      const pollUrl = '/download-list-files';
    
      // 1. Startet den I2C-Vorgang über den Server
      fetch(listUrl)
        .then(response => {
            if (!response.ok) throw new Error("LIST_DIR-Start fehlgeschlagen");
            console.log("📂 LIST_DIR-Vorgang gestartet...");
            pollForDirectoryData(pollUrl, 0); // beginne Polling
        })
        .catch(error => {
            console.error("❌ Fehler beim Starten des Verzeichnis-Ladevorgangs:", error);
        });
    }

    function pollForDirectoryData(url, attempt) {
      const maxAttempts = 20;
      const retryDelay = 300; // ms
  
      fetch(url)
          .then(response => {
              if (response.status === 202) {
                  if (attempt < maxAttempts) {
                      console.log(`⏳ Warten auf Datei... (${attempt + 1}/${maxAttempts})`);
                      setTimeout(() => pollForDirectoryData(url, attempt + 1), retryDelay);
                  } else {
                      console.error("⏰ Timeout beim Warten auf Verzeichnisdatei.");
                  }
              } else if (response.ok) {
                  return response.json();
              } else {
                  throw new Error("Fehler beim Abrufen der Verzeichnisliste");
              }
          })
          .then(data => {
              if (data) {
                  console.log("✅ Verzeichnisdaten empfangen:", data);
                  populatePlaylistFileList(data.files, data.directories);
              }
          })
          .catch(error => {
              console.error("❌ Fehler beim Laden der Verzeichnisdaten:", error);
          });
    }  

    function populatePlaylistFileList(files, directories) {
        playlistFileList = document.getElementById("playlistFileList");
        playlistFileList.innerHTML = '';
    
        // ".." hinzufügen, wenn nicht im Root
        if (_currentPath !== "/") {
            const upItem = document.createElement("li");
            upItem.textContent = "..";
            upItem.classList.add("directory-entry");
            upItem.addEventListener("click", () => {
                const parts = _currentPath.split("/").filter(p => p);
                parts.pop(); // letzten Teil entfernen
                const newPath = "/" + (parts.length ? parts.join("/") + "/" : "");
                loadDirectory(newPath);
            });
            playlistFileList.appendChild(upItem);
        }
    
        // Ordner anzeigen (klickbar)
        directories.forEach(dir => {
            const dirItem = document.createElement("li");
            dirItem.textContent = `${dir}`;
            dirItem.style.color = "#2196F3";
            dirItem.style.fontStyle = "italic";
            dirItem.style.fontWeight = "bold"; 
    
            dirItem.classList.add("directory-entry");
            dirItem.addEventListener("click", () => {
                const newPath = _currentPath + dir + "/";
                loadDirectory(newPath);
            });
            playlistFileList.appendChild(dirItem);
        });
    
        // Dateien anzeigen
        files.forEach(file => {
            const fileItem = document.createElement("li");
            fileItem.textContent = file;
    
            if ((file.toLowerCase().endsWith(".rs64"))||(file.toLowerCase().endsWith(".play"))) {
                // Nur .RS64-Dateien und .play Dateien auswählbar machen
                fileItem.addEventListener("click", () => selectFile(fileItem, file));
            } else {
                // Optional: ausgegraut darstellen oder Cursor deaktivieren
                fileItem.style.opacity = 0.5;
                fileItem.style.cursor = "default";
            }
            playlistFileList.appendChild(fileItem);
        });
    }

    function selectFile(element, filename) {
        // Auswahl zurücksetzen
        document.querySelectorAll(".file-list li").forEach(li => li.classList.remove("selected"));
        // Neue Auswahl markieren
        element.classList.add("selected");
        selectedFile = filename;
    }    

    function pollForPlaylistData(url, attempt) {
      const maxAttempts = 20;
      const retryDelay = 300; // ms
  
      fetch(url)
        .then(response => {
          if (response.status === 202) {
            if (attempt < maxAttempts) {
              console.log(`⏳ Warten auf Playlist-Daten... (${attempt + 1}/${maxAttempts})`);
              setTimeout(() => pollForPlaylistData(url, attempt + 1), retryDelay);
            } else {
              console.error("⏰ Timeout beim Laden der Playlist-Datei.");
              alert("Fehler: Playlist konnte nicht geladen werden.");
            }
          } else if (response.ok) {
            return response.json();
          } else {
            throw new Error("Fehler beim Abrufen der Playlist-Datei");
          }
        })
        .then(data => {
          if (data) {
            console.log("✅ Playlist-Daten empfangen:", data);
            renderPlaylist(data);
            document.getElementById("currentPlaylistName").textContent = "Playlist: " + selectedFile;
          }
        })
        .catch(err => {
          console.error("❌ Fehler beim Laden der Playlist:", err);
        });
    }
  
    // Button: select
    document.getElementById("select-playlist").addEventListener("click", () => {
      if (!selectedFile || !selectedFile.toLowerCase().endsWith(".play")) {
          alert("Bitte zuerst eine .play Datei aus der Liste auswählen.");
          return;
      }
  
      const startUrl = '/get_playlist?file=' + encodeURIComponent(selectedFile);
      const pollUrl = '/download_playlist';
  
      fetch(startUrl)
          .then(response => {
              if (!response.ok) throw new Error("Playlist-Download konnte nicht gestartet werden.");
              console.log("🎵 Playlist-Download gestartet...");
              pollForPlaylistData(pollUrl, 0);
          })
          .catch(err => {
              console.error("❌ Fehler beim Starten des Playlist-Downloads:", err);
          });
    });
  
    document.getElementById("new-playlist").addEventListener("click", () => {
        const name = prompt("Name der neuen Playlist eingeben:");
        if (!name) return; // Abbrechen oder leer
      
        let filename = name.trim();
        if (!filename.toLowerCase().endsWith(".play")) {
          filename += ".play";
        }
      
        selectedFile = filename;
      
        // Anzeige leeren
        renderPlaylist([]);
      
        // Playlist-Name anzeigen
        const nameDisplay = document.getElementById("currentPlaylistName");
        if (nameDisplay) {
          nameDisplay.textContent = "Playlist: " + selectedFile;
        }
      });

      document.getElementById("save-playlist").addEventListener("click", () => {
        const displayText = document.getElementById("currentPlaylistName")?.textContent || "";
        const filename = displayText.replace(/^Playlist:\s*/, "").trim();
      
        if (!filename || !filename.toLowerCase().endsWith(".play")) {
          alert("Kein gültiger Playlist-Name vorhanden.");
          return;
        }
      
        const playlistData = collectPlaylistData();
      
        fetch('/save_playlist', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json'
          },
          body: JSON.stringify({
            filename: filename, // ✅ der aus dem Textfeld gelesene Dateiname
            content: playlistData
          })
        })
          .then(response => {
            if (!response.ok) throw new Error("Fehler beim Speichern der Playlist.");
            return response.text();
          })
          .then(message => {
            alert("Playlist gespeichert: " + message);
          })
          .catch(error => {
            console.error("Fehler beim Speichern:", error);
            alert("Fehler beim Speichern der Playlist.");
          });
      });
      
    function renderPlaylist(playlistData) {
      playlistContent.innerHTML = "";
      selectedPlaylistRow = null;
  
      playlistData.forEach((item, index) => {
        const row = document.createElement("div");
        row.className = "playlist-row";

        row.addEventListener("click", () => selectPlaylistRow(row));

        // Titel
        const nameCol = document.createElement("div");
        nameCol.className = "playlist-col name";
        nameCol.textContent = item.File || `Track ${index + 1}`;
  
        // Clock (Checkbox)
        const clockCol = document.createElement("div");
        clockCol.className = "playlist-col clock";
        const clockCheckbox = document.createElement("input");
        clockCheckbox.type = "checkbox";
        clockCheckbox.checked = item.clock === "on"; // ✅ Hier verbessert
        clockCheckbox.addEventListener("change", (e) => {
            item.clock = e.target.checked ? "on" : "off"; // ✅ Hier auch angepasst
        });
        clockCol.appendChild(clockCheckbox);
        row.appendChild(clockCol);
  
        // Repetition (Number input)
        const repetitionCol = document.createElement("div");
        repetitionCol.className = "playlist-col repetition";

        const repetitionInput = document.createElement("input");
        repetitionInput.type = "number";
        repetitionInput.value = item.Repetition || 1; // Großes R wie im JSON
        repetitionInput.min = 1;
        repetitionInput.addEventListener("input", (e) => {
          item.Repetition = parseInt(e.target.value) || 1; // ebenfalls mit großem R
        });
        
        repetitionCol.appendChild(repetitionInput);
  
        row.appendChild(nameCol);
        row.appendChild(clockCol);
        row.appendChild(repetitionCol);
  
        playlistContent.appendChild(row);
      });
    }
  
    // Initiales Laden des Verzeichnisses
    loadDirectory("/");

    document.getElementById("move-up").addEventListener("click", () => {
        moveSelectedRow(-1); // -1 = nach oben
    });
  
    document.getElementById("move-down").addEventListener("click", () => {
        moveSelectedRow(1); // +1 = nach unten
    });

    document.getElementById("add-track").addEventListener("click", () => {
        addTrack();
    });
      
    document.getElementById("delete-track").addEventListener("click", () => {
        deleteSelectedTrack();
    });

    // Event listener for done button
    document.getElementById('done').addEventListener('click', () => {
	    configDone();
    });

    function moveSelectedRow(direction) {
        const rows = Array.from(document.querySelectorAll("#playlistContent .playlist-row"));
        const selectedIndex = rows.findIndex(row => row.classList.contains("selected"));
      
        if (selectedIndex === -1) return; // kein Eintrag ausgewählt
      
        const newIndex = selectedIndex + direction;
        if (newIndex < 0 || newIndex >= rows.length) return; // außerhalb der Liste
      
        const container = document.getElementById("playlistContent");
        const selectedRow = rows[selectedIndex];
        const targetRow = rows[newIndex];
      
        // Tauschen im DOM
        if (direction === -1) {
          container.insertBefore(selectedRow, targetRow);
        } else {
          container.insertBefore(targetRow, selectedRow);
        }
      
        // Selektion neu setzen
        rows.forEach(row => row.classList.remove("selected"));
        selectedRow.classList.add("selected");
      
        // [NEU] Nummern aktualisieren
        updatePlaylistRowNumbers();
    }
    
    function updatePlaylistRowNumbers() {
        const rows = Array.from(document.querySelectorAll("#playlistContent .playlist-row"));
        rows.forEach((row, index) => {
          const nameCol = row.querySelector(".playlist-col.name");
          if (nameCol && nameCol.textContent.startsWith("Track")) {
            nameCol.textContent = `Track ${index + 1}`;
          }
        });
    }  
    
    function deleteSelectedTrack() {
        const selected = document.querySelector("#playlistContent .playlist-row.selected");
        if (!selected) {
            alert("Bitte zuerst einen Track in der Playlist auswählen, um ihn zu löschen.");
            return;
        }
          
        selected.remove();
        updatePlaylistRowNumbers();
        selectedPlaylistRow = null;
    }
          
    function addTrack() {
        const selected = document.querySelector("#playlistFileList li.selected");
        if (!selected || !selected.textContent.toLowerCase().endsWith(".rs64")) {
            alert("Bitte eine .RS64 Datei im oberen Fenster auswählen, um sie zur Playlist hinzuzufügen.");
            return;
        }
          
        const fileName = selected.textContent;
        const container = document.getElementById("playlistContent");
          
        const row = document.createElement("div");
        row.className = "playlist-row";
          
        row.addEventListener("click", () => selectPlaylistRow(row)); // korrekt eingebaut
          
        const nameCol = document.createElement("div");
        nameCol.className = "playlist-col name";
        nameCol.textContent = fileName;
        row.appendChild(nameCol);
          
        const clockCol = document.createElement("div");
        clockCol.className = "playlist-col clock";
        const clockCheckbox = document.createElement("input");
        clockCheckbox.type = "checkbox";
        clockCheckbox.checked = false;
        clockCol.appendChild(clockCheckbox);
        row.appendChild(clockCol);
          
        const repetitionCol = document.createElement("div");
        repetitionCol.className = "playlist-col repetition";
        const repetitionInput = document.createElement("input");
        repetitionInput.type = "number";
        repetitionInput.min = 1;
        repetitionInput.value = 1;
        repetitionCol.appendChild(repetitionInput);
        row.appendChild(repetitionCol);
          
        container.appendChild(row);
          
        updatePlaylistRowNumbers();
    }      

    function collectPlaylistData() {
        const rows = document.querySelectorAll("#playlistContent .playlist-row");
        const data = [];
          
        rows.forEach((row, index) => {
            const file = row.querySelector(".playlist-col.name")?.textContent.trim() || "";
            const repetition = parseInt(row.querySelector("input[type='number']")?.value || "1");
            const clock = row.querySelector("input[type='checkbox']")?.checked ? "on" : "off";
          
            data.push({
                Nr: index,
                File: file,
                Repetition: repetition,
                clock: clock
            });
        });
          
        return data;
    }    
          
    function configDone() {
        window.location.href = "/configDone";
    }
});
  
