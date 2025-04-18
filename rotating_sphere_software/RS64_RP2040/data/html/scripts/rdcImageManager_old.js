////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                Handling of the File select box
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////

let currentPath = "/"; // current directory path
let fileList = document.getElementById("file-list"); // file list DOM element
let fileItems = []; // list of file item DOM elements
let selectedFile = ""; // name of the selected file/folder
let selectedFilePath = ""; // path of the selected file/folder

let scrollDirection = 1; // Standardmäßig positiv für normale Scrollrichtung
let touchStartX = 0; //                                                     
let touchStartY = 0; //                                                     
let mousedownTime = 0; // time when mouse button was pressed down           
let mouseupTime = 0; // time when mouse button was released                 
let clickHandled = false; //                                                
let longClickTimeout; // timeout ID for handling long click events          
let isTouchDevice = false; // flag to indicate if device is touch-enabled   
let touchMoved; //                                                          


// Load the list of files and directories in the current path
function loadFileList() {
    console.log('Loading file list...');

    // Send an HTTP GET request to the server to retrieve a list of files and directories in the current path
    fetch(`/filelist?path=${encodeURIComponent(currentPath)}`)
    .then(response => response.json())
    .then(data => {
        data.sort((a, b) => a.localeCompare(b, undefined, { sensitivity: 'base' }));
        console.log('Received sorted file list:', data);

        // Clear any previous elements from the file list display
        fileList.innerHTML = '';

        // If the current path is not the root directory, add a ".." directory to represent the parent directory
        if (currentPath !== "/") {
            const parentItem = document.createElement("div");
            parentItem.textContent = "..";
            parentItem.classList.add("directory");
            parentItem.onclick = () => {
                // Update the path to the parent directory and reload the file list display
                currentPath = currentPath.split('/').slice(0, -2).join('/') + '/';
                loadFileList();
            };
            fileList.appendChild(parentItem);
        }

        // Create DOM elements for each file or directory in the list and add them to the file list display
        const fileItems = data.map(file => {
            const item = document.createElement("div");
            item.textContent = file;

            // Add the "directory" class if it is a directory, to visually highlight it
            if (file.indexOf('.') === -1) {
                item.classList.add("directory");
            }

            // Add a click handler to select the element on click
            item.onclick = () => {
                // Remove the "selected" class from any other elements in the file list
                fileItems.forEach(el => el.classList.remove("selected"));
                // Add the "selected" class to the clicked element
                item.classList.add("selected");
            };
            return item;
        });

        // Add all created file and directory elements to the file list display
        fileItems.forEach(item => fileList.appendChild(item));
    })
    .catch(error => {
        console.error('Error loading file list:', error);
    });

    // Send an HTTP GET request to the server to retrieve information about the available disk space
    fetch('/diskspace')
    .then(response => response.json())
    .then(data => {
        const { totalBytes, usedBytes } = data;
        console.log(`Total Bytes: ${totalBytes}, Used Bytes: ${usedBytes}`);

        // Update the disk space display with the retrieved information
        const diskSpaceDisplay = document.getElementById('diskSpace');
        diskSpaceDisplay.textContent = "SD memory total: " + Math.floor(totalBytes / 1048576) + " MB, free: " + Math.floor((totalBytes - usedBytes) / 1048576) + " MB";
      })
    .catch(error => console.error(error));
}

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

// Delete a file without confirmation
function deleteFile(filename) {
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

  
// Extract the file name from a given URL
function getFileNameFromUrl(url) {
    // Splits the URL by "/" and returns the last element of the resulting array, which is the filename
    var parts = url.split("/");
    return parts[parts.length - 1];
}

// Replaces the file extension of a filename with the extension ".rdc".
function replaceFileExtension(filename) {
    const dotIndex = filename.lastIndexOf('.');
    if (dotIndex !== -1) {
      const basename = filename.substring(0, dotIndex);
      return `${basename}.rdc`;
    } else {
      return `${filename}.rdc`;
    }
}

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

// Preview the contents of a file with the specified name
function previewFile(fileName) {
  console.log("previewFile: "+fileName);

  // If the file is an image, download it and display it in an <img> element.
  if (/\.(gif|jpe?g|tiff?|png|webp|bmp)$/i.test(fileName)) {
    console.log("previewFile1: "+'/downloadfile?filename=' + encodeURIComponent(fileName));
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

// Clear the currently selected file path
function clearSelectedFilePath() {
  selectedFilePath = "";
  document.getElementById("text-selected-file").value = selectedFilePath;
  document.getElementById("image-preview").style.display = "none";
  document.getElementsByClassName("text4")[0].style.display = "none";
  document.getElementById('fileSize').textContent = "";
}

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
  console.log('Short click event triggered.'); // Überprüfen Sie, ob diese Zeile im Konsolenprotokoll erscheint.

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


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                Image re-scaling and format conversion
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////

let originalPixels; // Declare a global variable to store the original pixel buffer of the image.
let originalPixelsHeight;
let originalPixelsWidth;
let originalImage;
let imageFile;
let isGif = false;
let frameN = 0;

let currentPixels; // Declare a global variable to store the current pixel buffer of the displayed image.
let currentPixelsWidth = 110;
let currentPixelsHeight = 110;
let rdcFileName; // filename of the RDC image
let gifRes = 1; // gif resolution = 1: all images are converted, 2: every second image converted, 3: every third image converted

let canvasWidth = 110; // width of the canvas
let canvasHeight = 110; // height of the canvas
document.getElementById('xdim').value = canvasWidth;
document.getElementById('ydim').value = canvasHeight;


// functions for processing selected file. Checks on data format, rescales and displays the (first) image
async function processSelectedFile(file) {
  const reader = new FileReader();

  return new Promise((resolve, reject) => {
    reader.onload = async (event) => {
      const img = new Image();

      img.onload = async () => {
        console.log(`Bild "${file.name}" wurde erfolgreich geladen. Dateigröße: ${file.size} Bytes`);

        originalImage = new Image();
        originalImage.src = img.src;
        originalImage.width = img.width;
        originalImage.height = img.height;

        imageFile = new File([file], file.name, { type: file.type });
        if (file.type === 'image/gif') {
          isGif = true;
          frameN = 0;
        } else {
          isGif = false;
        }
        numberOfFrames = 1;

        try {
          var manipulatedPixels = await processImage(originalImage, imageFile, isGif, frameN);
          displayRDC('image-rdc', manipulatedPixels, currentPixelsWidth, currentPixelsHeight);

          // Update the displayed filename
          rdcFileName = document.getElementById('rdc-filename');
          rdcFileName.textContent = replaceFileExtension(file.name);

          // Update the displayed number of frames
          const numFramesElement = document.getElementById('num-frames');
          numFramesElement.textContent = ("images: " + numberOfFrames.toString());

          resolve(); // Erfolgreich beenden
        } catch (error) {
          console.error(error);
          reject(error); // Mit Fehler beenden
        }
      };

      img.src = event.target.result;
    };

    reader.readAsDataURL(file);
  });
}

// function for handling of file select
async function handleFileSelect() {
  const input = document.createElement('input');
  input.type = 'file';
  input.accept = 'image/*';

  input.onchange = async (event) => {
    const file = event.target.files[0];

    try {
      await processSelectedFile(file);

    } catch (error) {
      console.error('Error processing selected file:', error);
    }
  };

  input.click();
}


// Function to process the image with aspect ratio preservation or cropping
async function processImage(img, file, isGif, frameNumber) {
  const scaleCheckbox = document.getElementById('scale');

  return new Promise(async (resolve, reject) => {
    if (!scaleCheckbox) {
      reject(new Error('Checkbox not found.'));
      return;
    }

    let manipulatedPixels;

    if (isGif) {
      try {
        img = await extractGifFrame(file, frameNumber);
      } catch (error) {
        reject(error);
        return;
      }
    }

    const originalImage = new Image();
    originalImage.src = img.src;
    originalImage.width = img.width;
    originalImage.height = img.height;

    originalImage.onload = function () {
      if (scaleCheckbox.checked) {
        // if checkbox is checked, image will be scaled
        const canvas = document.createElement('canvas');
        const ctx = canvas.getContext('2d');
        canvas.width = currentPixelsWidth;
        canvas.height = currentPixelsHeight;
        ctx.drawImage(originalImage, 0, 0, currentPixelsWidth, currentPixelsHeight);
        const currentImageData = ctx.getImageData(0, 0, currentPixelsWidth, currentPixelsHeight);
        manipulatedPixels = new Uint8ClampedArray(currentImageData.data);
        resolve(manipulatedPixels);
      } else {
        // if checkbox is not checked, image will be cropped
        const canvas = document.createElement('canvas');
        const ctx = canvas.getContext('2d');
        const targetWidth = currentPixelsWidth;
        const targetHeight = currentPixelsHeight;
        const scaleX = targetWidth / originalImage.width;
        const scaleY = targetHeight / originalImage.height;
        const scale = Math.max(scaleX, scaleY);
        const scaledWidth = originalImage.width * scale;
        const scaledHeight = originalImage.height * scale;
        const offsetX = (targetWidth - scaledWidth) / 2;
        const offsetY = (targetHeight - scaledHeight) / 2;
        canvas.width = targetWidth;
        canvas.height = targetHeight;
        ctx.drawImage(originalImage, offsetX, offsetY, scaledWidth, scaledHeight);
        const currentImageData = ctx.getImageData(0, 0, targetWidth, targetHeight);
        manipulatedPixels = new Uint8ClampedArray(currentImageData.data);
        resolve(manipulatedPixels);
      }
    };
  });
}



async function loadImg(filePath) {
  try {
    // Fetch the image file from the server.
    const response = await fetch('/downloadfile?filename=' + filePath);
    const blob = await response.blob();

    // Rufe die Funktion processSelectedFile auf und übergebe das geladene Bild.
    await processSelectedFile(new File([blob], filePath, { type: blob.type }));

  } catch (error) {
    console.error('Error loading image:', error);
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


// create RDC data structure and upload file to server
async function saveRDC(fileName) {
  try {
    let fs;
    let paramOffset = 0;
    let response; 
    let imagesSaved = 0;

    if (gifRes>numberOfFrames) {
      gifRes = 1;
      document.getElementById('option1').checked = true;
    }
    console.log("gif resolution: "+gifRes);

    let corrNumberOfFrames = 1 + Math.floor((numberOfFrames - 1)/ gifRes);
    console.log("corrected number of frames: "+corrNumberOfFrames);

    // display status
    document.getElementById('num-frames').textContent = ("saving......");    

    // calculate file size. the first 12 bytes will be 3 unsigned integers (xdim, ydim, no. of images)
    fs = Math.ceil(currentPixelsHeight * 1.5) * currentPixelsWidth *  corrNumberOfFrames + 12;
    console.log("file size of rdcData: "+fs+" bytes");

    // Create an array to hold the RDC data
    let rdcData = new Uint8Array(fs);

    // write currentPixelsWidth, currentPixelsHeight, numberOfFrames as unsigned int into the first 12 Bytes
    const dataView = new DataView(rdcData.buffer);
    dataView.setUint32(0, currentPixelsWidth, true);       // "true" means "Little Endian" sequence
    dataView.setUint32(4, currentPixelsHeight, true);
    dataView.setUint32(8, corrNumberOfFrames, true);

    for (let i=0; i<numberOfFrames; i++) {

      paramOffset = Math.ceil(currentPixelsHeight * 1.5) * currentPixelsWidth * Math.floor(i/gifRes) +12;
    
      // Load to be saved RDC
      const pixelsToBeSaved = await processImage(originalImage, imageFile, isGif, i);

      if ((i % gifRes)==0) {
        // Loop through each column of the image
        for (let x = 0; x < currentPixelsWidth; x++) {
          // Loop through each row of the column
          for (let y = 0; y < currentPixelsHeight; y++) {
            // Calculate the index in the currentPixels array
            const pixelIndex = x + (currentPixelsHeight-1-y) * currentPixelsWidth; 
  
            // Get the pixel values (RGB) from currentPixels
            let pixelR = Math.floor(pixelsToBeSaved[pixelIndex*4]/32);
            let pixelG = Math.floor(pixelsToBeSaved[pixelIndex*4+1]/32);
            let pixelB = Math.floor(pixelsToBeSaved[pixelIndex*4+2]/32);
  
            // Calculate the byte index and bit position within the byte
            let y_byte = Math.floor(3*y/2);
            let y_start = (3*y)%2;
            let byte_no = x * Math.ceil(currentPixelsHeight * 1.5) + y_byte + paramOffset;

            if (y_start==0) {
              rdcData[byte_no] &= 0xF0;
              rdcData[byte_no] |= pixelR;
              pixelG<<=4;
              rdcData[byte_no] &= 0x0F;
              rdcData[byte_no] |= pixelG;
              rdcData[byte_no+1] &= 0xF0;
              rdcData[byte_no+1] |= pixelB;
            }
            else {
              pixelR<<=4;
              rdcData[byte_no] &= 0x0F;
              rdcData[byte_no] |= pixelR;
              rdcData[byte_no+1] &= 0xF0;
              rdcData[byte_no+1] |= pixelG;
              pixelB<<=4;
              rdcData[byte_no+1] &= 0x0F;
              rdcData[byte_no+1] |= pixelB;
            }
          }
        }
        imagesSaved++;
      }
    }

    // Create a new FormData object to send the file as a binary payload
    const formData = new FormData();
    formData.append('file', new Blob([rdcData], { type: 'application/octet-stream' }), fileName);
    
    // Send an HTTP POST request to the server to upload the file
    response = await fetch(`/uploadfile?path=${encodeURIComponent(fileName)}`, {
      method: 'POST',
      body: formData,
    }); 
    
    if (response.ok) {
      console.log('File uploaded successfully!');
    } else {
      console.error('Failed to upload file!');
    }

    // display status
    const numFramesElement = document.getElementById('num-frames');
    numFramesElement.textContent = (corrNumberOfFrames.toString()+" images saved");   
    
    // Reload the file list display to reflect the uploaded file
    loadFileList();
        
  } catch (error) {
    console.error('Error occurred while uploading file:', error);
  }
}

// Event listener for xdim input field
document.getElementById('xdim').addEventListener('input', async function() {
  const newXDim = parseInt(this.value, 10);
  if (!isNaN(newXDim)) {
    currentPixelsWidth = newXDim;
  }

  try {
    var manipulatedPixels = await processImage(originalImage, imageFile, isGif, frameN); // scale the image
    displayRDC('image-rdc', manipulatedPixels, currentPixelsWidth, currentPixelsHeight);
  } catch (error) {
    console.error("Error processing image:", error);
  }
});

// Event listener for ydim input field
document.getElementById('ydim').addEventListener('input', async function() {
  const newYDim = parseInt(this.value, 10);
  if (!isNaN(newYDim)) {
    currentPixelsHeight = newYDim;
  }

  try {
    var manipulatedPixels = await processImage(originalImage, imageFile, isGif, frameN); // scale the image
    displayRDC('image-rdc', manipulatedPixels, currentPixelsWidth, currentPixelsHeight);
  } catch (error) {
    console.error("Error processing image:", error);
  }
});

document.getElementById('scale').addEventListener('change', async function () {

  try {
    var manipulatedPixels = await processImage(originalImage, imageFile, isGif, frameN); // scale the image
    displayRDC('image-rdc', manipulatedPixels, currentPixelsWidth, currentPixelsHeight);
  } catch (error) {
    console.error("Error processing image:", error);
  }
});

// add click handler for image-rd40
document.getElementById('image-rdc').addEventListener('click', async function () {
  if (isGif == true) {
    frameN++;
    if (frameN >= numberOfFrames) frameN = 0;
  }
  try {
    var manipulatedPixels = await processImage(originalImage, imageFile, isGif, frameN); // scale the image
    displayRDC('image-rdc', manipulatedPixels, currentPixelsWidth, currentPixelsHeight);
  } catch (error) {
    console.error("Error processing image:", error);
  }
});

// process radio buttons
document.getElementById('option1').addEventListener('change', function () {
  gifRes = 1;
});

document.getElementById('option2').addEventListener('change', function () {
  gifRes = 2;
});

document.getElementById('option3').addEventListener('change', function () {
  gifRes = 3;
});


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                processing of GIF file
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////


let prev;                     // previous image canvas
let curr;                     // current image canvas
let currFrameDisposal;        // disposal method of current frame
let currFrameX;               // x of current frame
let currFrameY;               // y of current frame
let currFrameWidth;           // width of current frame
let currFrameHeight;          // height of current
let numberOfFrames = 1; 

async function extractGifFrame(file, frameNumber) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();

    const processFile = async () => {
      try {
        const arrayBuffer = reader.result;
        const gifReader = new GifReader(new Uint8Array(arrayBuffer));
        const numFrames = gifReader.numFrames();

        if (frameNumber < 0 || frameNumber >= numFrames) {
          reject(new Error(`Invalid frame number: ${frameNumber}`));
          return;
        }

        const width = gifReader.width;
        const height = gifReader.height;

        // if frameNumber == 0, generate empty canvas "prev"
        if (frameNumber === 0 && !prev && !curr){
          prev = document.createElement('canvas');
          prev.width = width;
          prev.height = height;
          curr = document.createElement('canvas');
          curr.width = width;
          curr.height = height;
          numberOfFrames = numFrames;
          console.log("ini variable prev");
        } 

        var prevCtx = prev.getContext('2d');
        var currCtx = curr.getContext('2d');
        
        if (frameNumber === 0) {
          prevCtx.fillStyle = 'white';                        // set prev to background color (white)
          prevCtx.fillRect(0, 0, prev.width, prev.height);
          currCtx.fillStyle = 'white';                        // set prev to background color (white)
          currCtx.fillRect(0, 0, curr.width, curr.height);
          console.log("prev und curr zurückgesetzt");
        }  else {
          //
          //                                           handle disposal of the previous frame
          // there is some good explanation here: https://legacy.imagemagick.org/Usage/anim_basics/ (search for "Frame Disposal Methods")
          //
          if (currFrameDisposal === 0) {            // no disposal specified
            prevCtx.drawImage(curr, 0, 0);          // do nothing. leave image as is. curr => prev
          }
          if (currFrameDisposal === 1) {            // do not dispose
            prevCtx.drawImage(curr, 0, 0);          // do nothing. leave image as is. curr => prev
          }
          if (currFrameDisposal === 2) {            // restore to background color
            prevCtx.fillStyle = 'white';            // this is the background color
            prevCtx.fillRect(currFrameX, currFrameY, currFrameWidth, currFrameHeight);   // delete used area
          }
          if (currFrameDisposal === 3) {            // restore to previous
                                                    // leave prev unchanged 
          }
          currCtx.drawImage(prev, 0, 0);            // disposal complete. prev is now the starting point for current
        }

        var canvas = document.createElement('canvas');
        canvas.width = width;
        canvas.height = height;
        var ctx = canvas.getContext('2d');
        var imgData = ctx.createImageData(width, height);

        gifReader.decodeAndBlitFrameRGBA(frameNumber, imgData.data);
        ctx.putImageData(imgData, 0, 0);

        const frameInfo = gifReader.frameInfo(frameNumber);
        currFrameDisposal = frameInfo.disposal;
        currFrameX = frameInfo.x;
        currFrameY = frameInfo.y;
        currFrameWidth = frameInfo.width;
        currFrameHeight = frameInfo.height;

        console.log("frameNumber: "+frameNumber+"  disposal: "+frameInfo.disposal);

        currCtx.drawImage(
          canvas,
          frameInfo.x, frameInfo.y, frameInfo.width, frameInfo.height,        // Source rectangle
          frameInfo.x, frameInfo.y, frameInfo.width, frameInfo.height         // Destination rectangle
        );

          // Create a new Image element from curr
        const img = new Image();
        img.src = curr.toDataURL();
        img.width = curr.width;
        img.height = curr.height;
                
        if (frameNumber === numberOfFrames - 1) {
          prev = null;
          curr = null;
        }

        resolve(img);
      } catch (error) {
        reject(error);
      }
    };

    reader.onload = processFile;
    reader.readAsArrayBuffer(file);
  });
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                        File selection of images
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////


function getRDCNames() {
  fetch('/getRDCNames')
    .then((response) => {
      if (response.ok) {
        return response.json();
      } else {
        throw new Error('Fehler bei der Anfrage. Status: ' + response.status);
      }
    })
    .then((data) => {
      if (data.clockface!="") document.getElementById("rdc-clock-face").textContent = getFileNameFromUrl(data.clockface);
      else document.getElementById("rdc-clock-face").textContent = "default";
      document.getElementById("rdc-logo").textContent = getFileNameFromUrl(data.logo);
      document.getElementById("rdc-image").textContent = getFileNameFromUrl(data.image);
    })
    .catch((error) => {
      console.error('Fehler bei der Anfrage:', error);
    });
}

// Extract the file name from a given URL
function getFileNameFromUrl(url) {
  // Splits the URL by "/" and returns the last element of the resulting array, which is the filename
  var parts = url.split("/");
  return parts[parts.length - 1];
}

// Create a canvas element and resize the image to 110x110 pixels.
const canvasPreview = document.createElement('canvas');
canvasPreview.width = 110;
canvasPreview.height = 110;
const ctx = canvasPreview.getContext('2d');

// Get the pixel buffer of the resized image and store it in the global variable.
const imageDataPreview = ctx.getImageData(0, 0, canvasPreview.width, canvasPreview.height);
//previewPixels = new Uint8ClampedArray(imageDataPreview.data);
originalPixels = new Uint8ClampedArray(imageDataPreview.data);
currentPixels = new Uint8ClampedArray(imageDataPreview.data);

// Event listener for when the DOM is fully loaded
document.addEventListener('DOMContentLoaded', () => {
    console.log('Document loaded.');
    document.getElementById('option1').checked = true;
    loadFileList();
    getRDCNames();
});


// Event listener for load button
document.getElementById("load-img").onclick = async () => {
  if (selectedFilePath !== "") {
    await loadImg(selectedFilePath)
  } else {
    await handleFileSelect();
  }
  clearSelectedFilePath();
};

// Event listener for "save-rdc" button
document.getElementById("save-rdc").onclick = () => {
    console.log("rdcFileName: " + rdcFileName.textContent);
    console.log("currentPath: " + currentPath);
    console.log("path: " + currentPath + rdcFileName.textContent);    
    saveRDC(rdcFileName.textContent);
};
  
// Event listener for done button
document.getElementById("fm-done").onclick = () => {
  window.location.href = "/configDone";
};

// Event listener for "select clock face" button
document.getElementById("select-clock-face").onclick = () => {
  if (selectedFilePath !== "") {
        let rememberSelectedFilePath = selectedFilePath;
        // Send an HTTP POST request to the server to delete the specified file
        fetch(`/selectclockface?filename=${selectedFilePath}`)
        .then((response) => {
          if (response.ok) {
            document.getElementById("rdc-clock-face").textContent = getFileNameFromUrl(rememberSelectedFilePath);
            console.log("rememberSelectedFilePath: " + getFileNameFromUrl(rememberSelectedFilePath));
          } else {
            document.getElementById("rdc-clock-face").textContent = "default";
            console.log("Fehler bei der Anfrage. Status: " + response.status);
          }
        })
        .catch((error) => {
          // Hier kannst du Fehlerbehandlung durchführen
          console.log("Fehler bei der Anfrage:" + error);
        });
  } else {
    alert("Click and hold to select a .rdc file.");
  }
  clearSelectedFilePath();
};


// Event listener for "clear clock face" button
document.getElementById("clr-clock-face").onclick = () => {
  
  deleteFile("/variables/clockFacePath.txt");
  document.getElementById("rdc-clock-face").textContent = "default";

};

// Event listener for "select logo" button
document.getElementById("select-logo").onclick = () => {
  if (selectedFilePath !== "") {
        let rememberSelectedFilePath = selectedFilePath;
        // Send an HTTP POST request to the server to delete the specified file
        fetch(`/selectlogo?filename=${selectedFilePath}`)
        .then((response) => {
          if (response.ok) {
            document.getElementById("rdc-logo").textContent = getFileNameFromUrl(rememberSelectedFilePath);
            console.log("rememberSelectedFilePath: " + getFileNameFromUrl(rememberSelectedFilePath));
          } else {
            console.log("Fehler bei der Anfrage. Status: " + response.status);
          }
        })
        .catch((error) => {
          // Hier kannst du Fehlerbehandlung durchführen
          console.log("Fehler bei der Anfrage:" + error);
        });
  } else {
    alert("Click and hold to select a .rdc file.");
  }
  clearSelectedFilePath();
};

// Event listener for "select image" button
document.getElementById("select-image").onclick = () => {
  if (selectedFilePath !== "") {
        let rememberSelectedFilePath = selectedFilePath;
        // Send an HTTP POST request to the server to delete the specified file
        fetch(`/selectimage?filename=${selectedFilePath}`)
        .then((response) => {
          if (response.ok) {
            document.getElementById("rdc-image").textContent = getFileNameFromUrl(rememberSelectedFilePath);
            console.log("rememberSelectedFilePath: " + getFileNameFromUrl(rememberSelectedFilePath));
          } else {
            console.log("Fehler bei der Anfrage. Status: " + response.status);
          }
        })
        .catch((error) => {
          // Hier kannst du Fehlerbehandlung durchführen
          console.log("Fehler bei der Anfrage:" + error);
        });
  } else {
    alert("Click and hold to select a .rdc file.");
  }
  clearSelectedFilePath();
};




