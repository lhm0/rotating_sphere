# Rotating LED Sphere – RP2040 POV Display

![Rotating LED Sphere](images/Title_page.jpg)

## 📌 Project Overview

This project describes the design and construction of a **rotating LED sphere** using the **persistence-of-vision (POV)** principle.  
The device is controlled by a **Raspberry Pi RP2040 microcontroller** and is capable of displaying **videos, animated GIFs**, and **static images** on a spherical 3D surface.

The project is inspired by the Las Vegas Sphere – scaled down to desktop size, but packed with smart engineering like **wireless power transmission**, **real-time SDIO video streaming**, and **web-based remote control**.

All hardware, software, PCB layouts and 3D printable parts are **open source** and included in this repository.

---

## ✨ Features

- 🟡 64 RGB LEDs arranged to form a 170 mm spherical POV display  
- 📊 Display resolution: **128 × 256 pixels** (via interlacing)  
- 🎞️ Supports **.rs64** video/image format (GIFs, animations, stills)  
- 💾 Real-time video streaming from **SD card via SDIO (4-bit)**  
- ⚙️ Driven by an **RP2040** using fast **PIO-based multiplexing**  
- 🌐 Optional **ESP01s** module creates a web-based control interface  
- 🔌 **Wireless power** using a Royer converter – no slip rings needed  
- 🧠 Designed to be **easy to build**, with a minimal part count  
- 🧰 All files included: **KiCad PCB**, **PlatformIO code**, **3D models**

---

## 🧱 System Components

- **RP2040 microcontroller** (via Raspberry Pi Pico or custom PCB)  
- **ESP01s (optional)** for Wi-Fi control and web interface  
- **SD card slot** (with high-speed SDIO interface)  
- **24-bit shift register** and **MOSFETs** for LED driving  
- **3D-printed mechanical components** (rotor, housing, mounts)  
- **Wireless power transmission** with printed transmitter/receiver coils  
- **Hall sensor** for rotation angle synchronization  
- **Custom `.rs64` video format** with 8 brightness levels via PWM

---

## 🛠️ How to Build

The build process is documented in a [step-by-step video tutorial](https://www.youtube.com/yourvideolink), covering:

1. Mechanical design & 3D printing  
2. Rotor balancing and alignment  
3. Soldering the LED matrix  
4. Power & wireless energy transmission  
5. Programming the RP2040 and ESP01s  
6. Creating `.rs64` files from GIFs and videos  
7. Using the web interface

For detailed instructions, see the [`docs/`](./docs) folder.

---

## 📜 License

This project is licensed under the **Creative Commons BY-NC-SA 4.0** license.  
You are free to **use, adapt, and share** this work for **non-commercial** purposes,  
as long as you give proper credit and share your work under the same terms.

> ✏️ Especially welcomed: use in education, maker communities, and open-source development!

---

