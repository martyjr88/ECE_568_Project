3-Factor Authentication Lock System
===================================

This project implements a secure three-factor authentication system using an ESP32 microcontroller. 
It combines a fingerprint sensor, RFID reader, and keypad to authenticate users and communicates with a 
local server for user management and logging.

Repository Contents:
--------------------
- authlock.ino  -> Code to be flashed onto the ESP32.
- server.py     -> Python server script to run on your computer for handling HTTP requests and storing data.

Getting Started:
----------------
Hardware Requirements:
- ESP32 Development Board
- MFRC522 RFID Reader
- Adafruit Optical Fingerprint Sensor
- SD Card Module (optional for logging)
- Access to a WiFi network

Software Requirements:
- Arduino IDE with the following libraries installed:
    * MFRC522v2
    * MFRC522DriverSPI
    * MFRC522DriverPinSimple
    * MFRC522Debug
    * Adafruit_Fingerprint
    * HardwareSerial
    * SPI
    * SD
    * HTTPClient
    * WiFi

- Python 3 with the following standard libraries:
    * http.server
    * urllib.parse
    * datetime

IP Address Configuration:
-------------------------
You must configure the correct IP address in BOTH files:

1. authlock.ino:
   - Set the IP address of the computer running server.py.

2. server.py:
   - Ensure the server is binding to the correct local IP address (or 0.0.0.0 for all interfaces).

Setup Instructions:
-------------------
1. Open authlock.ino in the Arduino IDE.
2. Select your ESP32 board and the correct COM port.
3. Flash the firmware onto the ESP32.

4. Run the Python server:
   python server.py

Authors:
--------
Dan Gui, Avanish Karlapudi, Hanna Letzring, Lucas Manalo, Marty Martin, Christopher Miotto, Moeyad Omer  
ECE 568 – Embedded Systems Design
