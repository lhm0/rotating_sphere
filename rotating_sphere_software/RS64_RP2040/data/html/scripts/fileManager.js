
//////////////////////////////////////////////////////////////////////////////////////////////////////
//               load and display file list
//////////////////////////////////////////////////////////////////////////////////////////////////////

// Define global variables
let currentPath = "/"; // current directory path
let fileList = document.getElementById("file-list"); // file list DOM element
let fileItems = []; // list of file item DOM elements
let selectedFile = ""; // name of the selected file/folder
let selectedFilePath = ""; // path of the selected file/folder

// Event listener for when the DOM is fully loaded
document.addEventListener('DOMContentLoaded', () => {
  console.log('Document loaded.');
  loadFileList();
});

// Load the file list and disk space information
function loadFileList() {
  console.log('Loading file list...');

  // Clear previous file list before adding new elements
  fileList.innerHTML = ''; 

  // Fetch the file list from the server
  fetchFileList()
      .then(fileListData => {
          // If the current path is not the root directory, add ".." at the beginning
          if (currentPath !== "/") {
              addParentDirectory();  // Add ".." to navigate to parent directory
          }

          // Display the sorted file list
          displayFileList(fileListData);
      })
      .catch(error => console.error('Error loading file list:', error));

  // Fetch disk space information
  fetchDiskSpace()
      .then(diskSpaceData => updateDiskSpaceDisplay(diskSpaceData))
      .catch(error => console.error('Error loading disk space:', error));
}

// Add the ".." item for navigating to the parent directory
function addParentDirectory() {
  const parentItem = document.createElement("div");
  parentItem.textContent = "..";
  parentItem.classList.add("directory");

  // Navigate to parent directory on click
  parentItem.onclick = () => {
    if (currentPath !== "/") {
      // Go up one directory level
      const parentPath = currentPath.split('/').slice(0, -2).join('/') + '/';
          
      // Ensure we don't go above root directory
      currentPath = parentPath === "" ? "/" : parentPath;

      // Reload the file list after changing directory
      loadFileList();
    }
  };

  // Append ".." to the file list
  fileList.appendChild(parentItem);
}

// Fetch the list of files and directories from the server
function fetchFileList() {
  return fetch(`/filelist?path=${encodeURIComponent(currentPath)}`)
      .then(response => response.json())
      .then(data => {
          // Sort files alphabetically (case insensitive)
          return data.sort((a, b) => a.localeCompare(b, undefined, { sensitivity: 'base' }));
      });
}

// Function to display the file list in the DOM
function displayFileList(fileListData) {
  // Create and append file/directory items to the DOM
  fileListData.forEach(file => {
      const item = document.createElement("div");
      item.textContent = file;

      // Add directory styling if it's a directory (no dot in the name)
      if (file.indexOf('.') === -1) {
          item.classList.add("directory");
      }

      // Click event to handle item selection
      item.onclick = () => {
          const allItems = fileList.querySelectorAll("div");
          allItems.forEach(el => el.classList.remove("selected"));
          item.classList.add("selected");
      };

      // Append each item to the file list
      fileList.appendChild(item);
  });
}

// Fetch disk space information from the server
function fetchDiskSpace() {
  return fetch('/diskspace')
      .then(response => response.json());
}

// Update the disk space display
function updateDiskSpaceDisplay(diskSpaceData) {
  const { totalBytes, usedBytes } = diskSpaceData;
  const diskSpaceDisplay = document.getElementById('diskSpace');
  diskSpaceDisplay.textContent = `SD memory total: ${Math.floor(totalBytes / 1048576)} MB, free: ${Math.floor((totalBytes - usedBytes) / 1048576)} MB`;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//               manage short and long clicks on file list
//////////////////////////////////////////////////////////////////////////////////////////////////////

// global variables
//
let scrollDirection = 1; // positiv for normal scroll direction
let touchStartX = 0; //                                                     
let touchStartY = 0; //                                                     
let mousedownTime = 0; // time when mouse button was pressed down           
let mouseupTime = 0; // time when mouse button was released                 
let clickHandled = false; //                                                
let longClickTimeout; // timeout ID for handling long click events          
let isTouchDevice = false; // flag to indicate if device is touch-enabled   
let touchMoved; //                                                          

// Event listener for mousedown event on file list
fileList.onmousedown = (event) => {
  if (event.target !== fileList && !isTouchDevice && !clickHandled) {
    mousedownTime = new Date().getTime();
    clickHandled = true;

    longClickTimeout = setTimeout(() => {
      handleLongClick(event);
    }, 500);
  }
};

// Event listener for mouseup event on file list
fileList.onmouseup = (event) => {
  if (event.target !== fileList && !isTouchDevice && clickHandled) {
    clearTimeout(longClickTimeout);

    mouseupTime = new Date().getTime();
    const timeDifference = mouseupTime - mousedownTime;

    if (timeDifference < 500) {
      handleShortClick(event);
    }

    // Wartezeit von 500 ms, bevor weitere Klicks akzeptiert werden
    setTimeout(() => {
      clickHandled = false;
    }, 500);
  }
};

// Event listener for touchstart event on file list
fileList.addEventListener('touchstart', (event) => {
  if (event.target !== fileList && !clickHandled) {
    isTouchDevice = true;
    touchStartX = event.touches[0].clientX;
    touchStartY = event.touches[0].clientY;
    touchMoved = false;
    clickHandled = true;

    mousedownTime = new Date().getTime();

    longClickTimeout = setTimeout(() => {
      handleLongClick(event);
    }, 500);
  }
});

// Event listener for touchmove event on file list
fileList.addEventListener('touchmove', (event) => {
  if (event.target !== fileList) {
    const touchEndX = event.touches[0].clientX;
    const touchEndY = event.touches[0].clientY;
    const touchDistance = Math.sqrt(
      Math.pow(touchEndX - touchStartX, 2) + Math.pow(touchEndY - touchStartY, 2)
    );

    if (touchDistance > 10) {
      touchMoved = true;
      clearTimeout(longClickTimeout);
    }
  }
});

// Event listener for touchend event on file list
fileList.addEventListener('touchend', (event) => {
  if (event.target !== fileList && clickHandled) {
    clearTimeout(longClickTimeout);

    mouseupTime = new Date().getTime();
    const timeDifference = mouseupTime - mousedownTime;

    if (!touchMoved && timeDifference < 500) {
      handleShortClick(event);
    }

    // Wartezeit von 500 ms, bevor weitere Klicks akzeptiert werden
    setTimeout(() => {
      clickHandled = false;
    }, 500);
  }

  touchMoved = false;
});

// Check if it is a touch-enabled device
if ('ontouchstart' in window || navigator.msMaxTouchPoints) {
    scrollDirection = -1; // Reverse scroll direction for touch-enabled devices
}  
  
// Handle short click event on file item
function handleShortClick(event) {
  fileItems.forEach(item => {
    item.classList.remove("selected");
  });

  event.target.classList.add("selected");
  const fileName = event.target.textContent;
  
  if (fileName.indexOf('.') === -1) {
    currentPath += fileName + "/";
    console.log("Current path: " + currentPath);
    loadFileList();
  }

  previewFile(currentPath + fileName);
}

// Handle long click event on file item
function handleLongClick(event) {
  selectedFile = event.target.textContent;
  if (selectedFile.indexOf("..") !== -1) {
    // Do nothing
  } else if (selectedFile.indexOf(".") !== -1) {
    selectedFilePath = currentPath + selectedFile;
    document.getElementById("text-selected-file").value = "Selected file: " + selectedFilePath;
    previewFile(selectedFilePath);
  } else {
    selectedFilePath = currentPath + selectedFile +"/";
    document.getElementById("text-selected-file").value = "Selected folder: " + selectedFilePath;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     delete a file or folder
//////////////////////////////////////////////////////////////////////////////////////////////////////

// Event listener for delete button
document.getElementById("delete").onclick = () => {
  if (selectedFilePath !== "") {
    deleteFile(selectedFilePath);
  } else {
    alert("Click and hold to select a file or folder.");
  }
  clearSelectedFilePath();
};

// Delete a file or folder with the specified name
function deleteFile(filename) {
    // Show a confirmation dialog before deleting the file
    const confirmDeletion = confirm(`Delete '${filename}': Are you sure?`);
    if (confirmDeletion) {
        // Send an HTTP POST request to the server to delete the specified file
        fetch(`/deletefile?filename=${filename}`)
        .then(response => response.json())
        .then(data => {
            console.log('File deleted:', filename);
            // Reload the file list display to reflect the deleted file
            loadFileList();
        })
        .catch(error => {
            console.error('Error deleting file:', error);
        });
    }
}

// Clear the currently selected file path
function clearSelectedFilePath() {
  selectedFilePath = "";
  document.getElementById("text-selected-file").value = selectedFilePath;
  document.getElementById("image-preview").style.display = "none";
  document.getElementsByClassName("text4")[0].style.display = "none";
  document.getElementById('fileSize').textContent = "";
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     rename a file or folder
//////////////////////////////////////////////////////////////////////////////////////////////////////

// Event listener for rename button
document.getElementById("rename").onclick = () => {
  if (selectedFilePath !== "") {
    renameFile(selectedFilePath);
  } else {
    alert("Click and hold to select a file or folder.");
  }
  clearSelectedFilePath();
};

// Rename a file or folder with the specified name
function renameFile(filename) {
    console.log("old path: " + filename);

    let isFolder;
    let folderName = "";
    let FileName = "";
    let originPath = "";
    let oldFileName = filename;
    let newFileName = "";
    if (filename.endsWith("/")) { // Check if it is a folder
        isFolder = true;
        var lastSlashIndex = filename.lastIndexOf("/");
        var secondLastSlashIndex = filename.lastIndexOf("/", lastSlashIndex - 1);

        folderName = filename.substring(secondLastSlashIndex + 1, lastSlashIndex);
        console.log("folderName = " + folderName);

        originPath = filename.substring(0, secondLastSlashIndex + 1);

        var newFolderName = prompt("rename folder '"+originPath+folderName+"'. Enter new name:", folderName); // User input of new folder name
        newFileName = originPath + newFolderName + "/"; // Construct the new path with the new folder name
    } else { // If it's not a folder, it's a file
        isFolder = false;
        var lastSlashIndex = filename.lastIndexOf("/");
        fileName = filename.substring(lastSlashIndex + 1);
        originPath = filename.substring(0, lastSlashIndex + 1);

        var newName = prompt("Please rename file:", fileName); // User input of new file name
        newFileName = originPath + newName; // Construct the new path with the new file name
    }
    
    console.log("new path: " + newFileName);

    // Send an HTTP POST request to the server to rename the specified file
    fetch(`/renamefile?oldfilename=${oldFileName}&newfilename=${newFileName}`)
        .then(response => response.json())
        .then(data => {
            console.log('File renamed:', oldFileName, ' => ', newFileName);
            // Reload the file list display to reflect the renamed file
            loadFileList();
        })
        .catch(error => {
            console.error('Error renaming file:', error);
        });
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     copy a file or folder
//////////////////////////////////////////////////////////////////////////////////////////////////////

// Event listener for copy button
document.getElementById("copy").onclick = () => {
  if (selectedFilePath !== "") {
    copyFile(selectedFilePath, currentPath);
  } else {
    alert("Click and hold to select a file or folder.");
  }
  clearSelectedFilePath();
};

// Copy a file or folder from the source path to the destination path
function copyFile(sourceFile, destPath) {
    // Send an HTTP POST request to the server to copy the specified file
    console.log('copyFile:', sourceFile, ' to destination: ', destPath);

    fetch(`/copyfile?source=${sourceFile}&destination=${destPath}&moveflag=0`)
        .then(response => response.json())
        .then(data => {
            console.log('file copied:', sourceFile);
            // Reload the file list display to reflect the copied file
            loadFileList();
        })
        .catch(error => {
            console.error('Error copying file:', error);
        });
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     move a file or folder
//////////////////////////////////////////////////////////////////////////////////////////////////////

// Event listener for move button
document.getElementById("move").onclick = () => {
  if (selectedFilePath !== "") {
    moveFile(selectedFilePath, currentPath);
  } else {
    alert("Click and hold to select a file or folder.");
  }
  clearSelectedFilePath();
};

// Move a file or folder from the source path to the destination path
function moveFile(sourceFile, destPath) {
    // Send an HTTP POST request to the server to move the specified file
    fetch(`/copyfile?source=${sourceFile}&destination=${destPath}&moveflag=1`)
        .then(response => response.json())
        .then(data => {
            console.log('file moved:', sourceFile);
            // Reload the file list display to reflect the moved file
            loadFileList();
        })
        .catch(error => {
            console.error('Error moving file:', error);
        });
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     create a new folder
//////////////////////////////////////////////////////////////////////////////////////////////////////

// Event listener for make directory button
document.getElementById("make-dir").onclick = () => {
  mkDir();
};

// Create a new directory in the current path
function mkDir() {
    // Prompt user to enter directory name
    var directory = prompt("Please enter the directory name:", "");
    if (directory == null || directory == "") {
        // Cancel if user didn't enter anything
        return;
    } else {
        // Trim any leading or trailing whitespace from the input
        directory = directory.trim();
        // Construct the full path for the new directory
        directory = currentPath + directory + "/";
        // Send an HTTP POST request to the server to create the new directory
        fetch(`/mkdir?filename=${directory}`)
            .then(response => response.json())
            .then(data => {
                console.log('Directory created:', directory);
                // Reload the file list display to reflect the new directory
                loadFileList();
            })
            .catch(error => {
                console.error('Error creating directory:', error);
            });
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     upload a file to the server
//////////////////////////////////////////////////////////////////////////////////////////////////////

// Event listener for upload file button
document.getElementById('upload-file').addEventListener('click', () => {
  uploadFile();
});

// Upload a file to the server
function uploadFile() {
  // Create a new input element for selecting a file
  const fileInput = document.createElement('input');
  fileInput.type = 'file';

  let totalSpace = 0;
  let usedSpace = 0;

  // Determine free disk space
  fetch('/diskspace')
      .then(response => response.json())
      .then(data => {
          const { totalBytes, usedBytes } = data;
          console.log("Total space: " + totalBytes + "   Used space: " + usedBytes);
          totalSpace = totalBytes;
          usedSpace = usedBytes;
          // Open file selection only after space info is loaded
          fileInput.click();
      })
      .catch(error => {
          console.error('Failed to retrieve disk space:', error);
      });

  // Add an event listener to the input element to handle file selection
  fileInput.addEventListener('change', (event) => {
    const file = event.target.files[0];
    const fileSize = file.size;

    // Check if there is enough disk space to upload the file
    if (fileSize > (totalSpace - usedSpace)) {
      console.log("ERROR: Not enough disk space");
      openInfoBox(totalSpace, usedSpace, fileSize);
    } else {
      // Create a new FormData object to send the file as a binary payload
      const formData = new FormData();
      formData.append('file', file);

      // Create an XMLHttpRequest to track upload progress
      const xhr = new XMLHttpRequest();

      // Progress event listener
      xhr.upload.addEventListener("progress", uploadProgress, false);
      xhr.upload.addEventListener("load", uploadComplete, false);

      // Send the file
      xhr.open('POST', `/uploadfile?path=${encodeURIComponent(currentPath)}`);
      xhr.send(formData);
    } 
  });

   // Click the file input element to open the file selection dialog
   fileInput.click();
}

function uploadProgress(event) {
  if (event.lengthComputable) {
      const percentComplete = (event.loaded / event.total) * 100;
      console.log(`Upload progress: ${percentComplete.toFixed(2)}%`);
      // for large files a progress bar can be added here.
  } else {
      console.log("Unable to compute progress information since the total size is unknown.");
  }
}

function uploadComplete(event) {
  console.log(`Upload complete`);
  loadFileList();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     download a file from the server
//////////////////////////////////////////////////////////////////////////////////////////////////////

// Event listener for download file button
document.getElementById('download-file').addEventListener('click', () => {
  console.log("selectedFilePath = " + selectedFilePath);
  if (selectedFilePath !== "") {
    if (selectedFilePath.endsWith("/")) {
      alert("Can only download files, not folders.");
    } else {
      downloadFile(selectedFilePath);
    }
  } else {
    alert("Click and hold to select a file.");
  }
  clearSelectedFilePath();
});

// Download a file from the server using the specified URL
function downloadFile(fileUrl) {
    console.log("fileUrl = " + fileUrl);
  
    // Send an HTTP GET request to the server to retrieve the file data as a blob object
    fetch('/downloadfile?filename=' + fileUrl)
    .then(response => response.blob())
    .then(blob => {
        var fileName = getFileNameFromUrl(fileUrl); // Extract the filename from the URL
        var link = document.createElement('a');
        link.href = window.URL.createObjectURL(blob);
        link.download = fileName;
        link.click(); // Clicks the "Download" link to download the file
      })
      .catch(function(error) {
        console.log('Problem beim Herunterladen der Datei:', error);
      });
}  

// Extract the file name from a given URL
function getFileNameFromUrl(url) {
    // Splits the URL by "/" and returns the last element of the resulting array, which is the filename
    var parts = url.split("/");
    return parts[parts.length - 1];
  }

// Open a dialog box showing information about the current directory
function openInfoBox(totalBytes, usedBytes, fileSize) {
    const message = `Not enough flash memory\n\ntotal: ${totalBytes} bytes\nfree: ${(totalBytes-usedBytes)} bytes\nFile: ${fileSize} bytes`;
    alert(message);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     request file size from the server
//////////////////////////////////////////////////////////////////////////////////////////////////////

// Get the size of a file with the specified name
function getFileSize(filename) {
    console.log("getFileSize: " + filename);
  
    return fetch(`/fileSize?filename=${filename}`)
      .then(response => response.json()) // Parse response as JSON
      .then(data => {
        console.log(filename + " fileSize: " + data.size);
        return data.size;
      })
      .catch(error => console.error(error));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     preview file content
//////////////////////////////////////////////////////////////////////////////////////////////////////

// global variables
// Create a canvas element and resize the image to 110x110 pixels.
const canvasPreview = document.createElement('canvas');
canvasPreview.width = 110;
canvasPreview.height = 110;
const ctx = canvasPreview.getContext('2d');

// Get the pixel buffer of the resized image and store it in the global variable.
const imageDataPreview = ctx.getImageData(0, 0, canvasPreview.width, canvasPreview.height);

// let previewPixels = new Uint8ClampedArray(imageDataPreview.data);

// Preview the contents of a file with the specified name
function previewFile(fileName) {

  // If the file is an image, download it and display it in an <img> element.
  if (/\.(gif|jpe?g|tiff?|png|webp|bmp)$/i.test(fileName)) {
    fetch('/downloadfile?filename=' + encodeURIComponent(fileName))
    .then(response => response.blob())
    .then(blob => {
      // Hide the textarea and show the image element
      var textareaElement = document.getElementsByTagName("textarea")[0];
      textareaElement.style.display = "none";     
      var imgElement = document.getElementsByTagName("img")[0];
      imgElement.style.display = "block";

      // Create a URL for the blob and set it as the image source
      const objectURL = URL.createObjectURL(blob);
      document.getElementById("image-preview").src = objectURL;
    })
    .catch(error => {
      console.error('Error downloading file:', error);
    });
  }

  // If the file is an rdc file, download it and display it in an <img> element.
  if (/\.(rdc)$/i.test(fileName)) {
    console.log("previewFile1: " + '/downloadfile?filename=' + encodeURIComponent(fileName));
    fetch('/downloadfilepart1?filename=' + encodeURIComponent(fileName))
    .then(response => response.blob())
    .then(blob => {
      const reader = new FileReader();
      reader.onloadend = function () {
        const arrayBuffer = reader.result;
        const dataView = new DataView(arrayBuffer);

        // Create a new Uint8Array to store the file data in the "pixels" array
        const pixels = new Uint8Array(dataView.buffer, dataView.byteOffset, dataView.byteLength);

        // Hide the textarea and show the image element
        var textareaElement = document.getElementsByTagName("textarea")[0];
        textareaElement.style.display = "none";
        var imgElement = document.getElementById("image-preview");
        imgElement.style.display = "block";
  
        let x, y, y_byte, y_start, byte_no;
        let xd, yd;

        const firstFourBytes = pixels.slice(0, 4);
        const dataViewForIntX = new DataView(firstFourBytes.buffer);
        xd = dataViewForIntX.getUint32(0, true); // true für little-endian, false für big-endian
        const nextFourBytes = pixels.slice(4, 8);
        const dataViewForIntY = new DataView(nextFourBytes.buffer);
        yd = dataViewForIntY.getUint32(0, true); // true für little-endian, false für big-endian

        console.log("file xdim: "+xd);
        console.log("file ydim: "+yd);  

        let previewTemp = new Uint8Array(xd*yd*4);

        for (x = 0; x < xd; x++) {
          for (y = 0; y < yd; y++) {
            y_byte=Math.floor(3*y/2);
            y_start=(3*y)%2;
            byte_no=12 + x * Math.ceil(yd*1.5) + y_byte;
            const pixelIndex = 4 * x + (yd-1-y) * xd*4;
            let pixelValueR, pixelValueG, pixelValueB;
            if (y_start==0) {
              pixelValueR = 36*(pixels[byte_no]&0x0F);
              pixelValueG = 36*((pixels[byte_no]&0xF0)>>4);
              pixelValueB = 36*(pixels[byte_no+1]&0x0F);
            }
            else {
              pixelValueR = 36*(pixels[byte_no] >> 4);
              pixelValueG = 36*(pixels[byte_no+1] & 0x0F);
              pixelValueB = 36*(pixels[byte_no+1] >>4);
            }

            previewTemp[pixelIndex] = pixelValueR; // Red channel
            previewTemp[pixelIndex + 1] = pixelValueG; // Green channel
            previewTemp[pixelIndex + 2] = pixelValueB; // Blue channel
            previewTemp[pixelIndex + 3] = 255; // Alpha channel (fully opaque)
          }
        }

        // Display the modified image in the HTML element.
        displayRDC('image-preview', previewTemp, xd, yd);
      };
      reader.readAsArrayBuffer(blob);
    })
    .catch(error => {
      console.error('Error downloading file:', error);
    });
  }

  // If the file is a plain text file, download it and display the contents in a <textarea> element.
  if (/\.(txt|css|html|js)$/i.test(fileName)) {
    fetch('/downloadfile?filename=' + encodeURIComponent(fileName))
    .then(response => response.text())
    .then(text => {
      // Hide the image element and show the textarea
      var imgElement = document.getElementsByTagName("img")[0];
      imgElement.style.display = "none";
      var textareaElement = document.getElementsByTagName("textarea")[0];
      textareaElement.style.display = "inline";
            
      // Set the file contents as the textarea value
      document.getElementById("text-box").value = text;
    });
  } 

  // Request the file size and display it.
  if (/\.(gif|jpe?g|tiff?|png|webp|bmp|txt|css|html|js|rdc)$/i.test(fileName)) {
    console.log("request fileSize of " + fileName);

    // Request the file size and create a Promise to handle the result.
    const fileSizePromise = getFileSize(fileName);
    fileSizePromise.then(fileSize => {
      // Update the file size display with the filename and size in bytes.
      const fileSizeDisplay = document.getElementById('fileSize');
      fileSizeDisplay.textContent = getFileNameFromUrl(fileName) + " (" + fileSize + "bytes)";
    })
    .catch(error => {
      console.error(error);
    });
  }
}

// Displays the RDC image in an HTML element.
function displayRDC(elementId, pixels, pixelsWidth, pixelsHeight) {
  // Create a canvas element and set its dimensions to match the image size.
  const canvas = document.createElement('canvas');
  canvas.width = pixelsWidth;
  canvas.height = pixelsHeight;

  // Draw the image onto the canvas.
  const ctx = canvas.getContext('2d');
  const imageData = ctx.createImageData(canvas.width, canvas.height);
  imageData.data.set(pixels);
  ctx.putImageData(imageData, 0, 0);

  // Set the source of the <img> element to the data URL of the canvas.
  const dataUrl = canvas.toDataURL();
  const imgElement = document.getElementById(elementId);
  imgElement.src = dataUrl;
  imgElement.style.display = 'block';
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     done button
//////////////////////////////////////////////////////////////////////////////////////////////////////

// Event listener for file manager done button
document.getElementById("fm-done").onclick = () => {
  window.location.href = "/configDone";
};