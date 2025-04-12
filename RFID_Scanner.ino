/*
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Adapted for Adafruit Feather V2:
    - MISO: GPIO 21
    - MOSI: GPIO 19
    - SCK:  GPIO 5
    - NSS:  GPIO 27
  Complete project details at https://RandomNerdTutorials.com/esp32-mfrc522-rfid-reader-arduino/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <SPI.h>
#include <Preferences.h>

Preferences preferences;
// Choose GPIO 27 as the NSS (chip select) pin for the MFRC522 module.
MFRC522DriverPinSimple ss_pin(27);

// Create the SPI driver using our defined chip select.
MFRC522DriverSPI driver{ss_pin};

// Create MFRC522 instance with the SPI driver.
MFRC522 mfrc522{driver};

void setup() {
  Serial.begin(115200);         // Initialize serial communication.
  while (!Serial);              // Wait for the serial port to be ready (required on some boards).

  // Initialize SPI using the Adafruit Feather V2 pin configuration.
  // SPI.begin(SCK, MISO, MOSI, SS)
  SPI.begin(5, 21, 19, 27);  

  // Initialize the MFRC522 RFID reader.
  mfrc522.PCD_Init();

  // Dump details of the RFID reader to the Serial Monitor (for debugging).
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

 preferences.begin("rfid", false);

  // Save the UID using the key "uidString".
  preferences.putString("uidString", "0776fa85");

  // Retrieve the UID using the same key.
  String lastUID = preferences.getString("uidString", "none");
  Serial.println("Last UID: " + lastUID);

  }

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
		return;
	}

	// Select one of the cards.
	if (!mfrc522.PICC_ReadCardSerial()) {
		return;
	}

  Serial.print("Card UID: ");
  MFRC522Debug::PrintUID(Serial, (mfrc522.uid));
  Serial.println();

  // Save the UID on a String variable
  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      uidString += "0"; 
    }
    uidString += String(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println(uidString);

  delay(2000);  // Wait 2 seconds before scanning for the next card.
  printf("Test\n");
}
