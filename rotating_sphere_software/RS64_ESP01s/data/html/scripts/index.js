let _currentPath = "/"; // Startpfad

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
                populateFileList(data.files, data.directories);
            }
        })
        .catch(error => {
            console.error("❌ Fehler beim Laden der Verzeichnisdaten:", error);
        });
}

function populateFileList(files, directories) {
    const fileList = document.getElementById("fileList");
    fileList.innerHTML = '';

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
        fileList.appendChild(upItem);
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
        fileList.appendChild(dirItem);
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
        fileList.appendChild(fileItem);
    });

}

function selectFile(element, filename) {
    // Auswahl zurücksetzen
    document.querySelectorAll(".file-list li").forEach(li => li.classList.remove("selected"));
    // Neue Auswahl markieren
    element.classList.add("selected");

    // Server über Auswahl informieren
    fetch("/select-file", {
        method: "POST",
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ filename: _currentPath + filename })
    }).catch(err => console.error("Fehler beim Senden der Datei-Auswahl:", err));
}
  
function getPageData() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var data = JSON.parse(this.responseText);
            console.log("brightness: " + data.brightness);
            console.log("mode: " + data.mode);
            document.getElementById("brightnessSlider").value = data.brightness;
            document.getElementById("brightnessValue").innerHTML = data.brightness + " %";
        }
    };
    xhr.open("GET", "/getParam", true);
    xhr.send();
}

function updateSliderBrightness(element) {
    var sliderValue = document.getElementById("brightnessSlider").value;
    document.getElementById("brightnessValue").innerHTML = sliderValue + "%";
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/slider?value="+sliderValue, true);
    xhr.send();
}

function resetWifi() {
    window.location.href = "/resetWifi";
}

function playlist() {
    window.location.href = "/playlistManager";
}

function timeZone() {
    window.location.href = "/timeZone";
}

document.addEventListener("DOMContentLoaded", () => {
    loadDirectory(_currentPath);
});

document.addEventListener("DOMContentLoaded", function(event) { 
    getPageData();
});

// Event listener for "reset wifi" button
document.getElementById('reset-wifi').addEventListener('click', () => {
    resetWifi();
});

// Event listener for "playlist" button
document.getElementById('playlist').addEventListener('click', () => {
    playlist();
});

// Event listener for "file browser" button
document.getElementById('time-zone').addEventListener('click', () => {
    timeZone();
});

// Event listener for brightness slider
document.getElementById('brightnessSlider').addEventListener('change', () => {
    updateSliderBrightness();
});
