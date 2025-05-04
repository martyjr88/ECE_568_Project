/*****************************************************************************
 * @author  Dan Gui, Avanish Karlapudi, Hanna Letzring, Lucas Manalo, Marty Martin, Christopher Miotto, Moeyad Omer
 * @brief ECE 568 - 3 Factor Authenticate Lock System
 *****************************************************************************/
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <HTTPClient.h>
#include <WiFi.h>

// Pin Definitions
#define RX_PIN 7  // Connect to fingerprint sensor's TX
#define TX_PIN 8  // Connect to fingerprint sensor's RX
#define SD_MISO 32  // SD card MISO pin
#define SD_MOSI 33  // SD card MOSI pin
#define SD_SCK  25  // SD card SCK pin
#define SD_CS   26  // SD card CS pin

// Authentication State Machine
enum State {
  IDLE,
  AUTHENTICATION,
  RFID,
  PASSWORD,
  FINGERPRINT,
  UNLOCK,
  FAILURE,
  EDIT_USERS,
  EDIT_ADD_USER,
  EDIT_DELETE_USER
};

// User Data Structure
struct UserData {
  uint8_t fingerprintKey;
  String password;
  void clear() {
    fingerprintKey = 0;
    password = "";
  }
};

// Global Variables
State currentState = IDLE;
UserData currentUser;
uint8_t id;
uint8_t pressed1;
String rfid;
String status;
bool skip_print = false;
const char* ssid = "LucasMoneyMachine";
const char* password = "BadPassword";
const char* serverName = "http://172.20.10.6:80";

// ESP classes or whatever
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);
MFRC522DriverPinSimple ss_pin(27);
MFRC522DriverSPI driver{ss_pin};
MFRC522 mfrc522{driver};
SPIClass sdSPI(HSPI);

// Authentication Functions
String check_rfid();
bool check_rfid_sd(String rfid, UserData &user);
bool check_password(String pin, UserData &user);
bool check_fingerprint(uint8_t fp_id, UserData &user);
bool admin_pass();

// SD card functions
bool add_user_sd(String rfid,UserData &user);
bool delete_user_sd(String rfid);

// Fingerprint Functions
uint8_t getFingerprintEnroll();
uint8_t getFingerprintID();
int getFingerprintIDez();

// Server functions
bool send_key_to_server();

// Universal Read functionality 
uint8_t readnumber(void) {
  uint8_t num = 0;
  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

String readSerialLine() {
  while (Serial.available() && (Serial.peek() == '\r' || Serial.peek() == '\n')) {
    Serial.read();
  }
  while (!Serial.available());
  String ret = Serial.readStringUntil('\n');
  return ret;
}

void setup() {
  Serial.begin(115200);
  Serial.println("==================== Lock Setup ====================");

  // Initialize wifi connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Initialize state machine
  currentState = IDLE;
  currentUser.clear();
  
  // Setup SD Card
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if(!SD.begin(SD_CS, sdSPI)){
    Serial.println("SD Card Mount Failed!");
  }
  Serial.println("SD Card initialized :)");
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD card size: %lluMB\n", cardSize);

  // RFID Setup
  SPI.begin(5, 21, 19, 27); 
  mfrc522.PCD_Init();
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
  Serial.print("RFID Setup Complete\n");

  // Fingerprint Setup
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  while (!Serial); // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("Adafruit Fingerprint sensor enrollment");
  
  finger.begin(57600);  // FIngerprint sensor uses separate baud rate
  
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }
  Serial.println("==================== Setup Complete ====================");
  Serial.println("Starting Authentification Lock System\n\n");
}


void loop() {
  switch (currentState) {
    case IDLE: {
      currentUser.clear();
      if(!skip_print){
        Serial.println("{1} Authenticate User\n{2} Edit User");
      }
      else{
        skip_print = false;
      }
      while (!Serial.available());
      char entry = Serial.read();
      Serial.read(); // consume newline if present
      if (entry == '1') currentState = AUTHENTICATION;
      else if (entry == '2') currentState = EDIT_USERS;
      else if (entry == '\r' || entry == '\n'){
        skip_print = true;
        currentState = IDLE;
      }
      else {
        Serial.println("Invalid entry");
        currentState = IDLE;
      }
      break;
    }
    case AUTHENTICATION: {
      Serial.println("Scan RFID Tag:");
      currentState = RFID;
      break;
    }
    case RFID: {
      rfid = check_rfid();
      if (check_rfid_sd(rfid, currentUser)) {
        currentState = PASSWORD;
      } else {
        currentState = FAILURE;
      }
      break;
    }
    case PASSWORD: {
      Serial.println("Enter 6-digit PIN:");
      String pass = readSerialLine();
      if (check_password(pass, currentUser)) {
        currentState = FINGERPRINT;
      } else {
        currentState = FAILURE;
      }
      break;
    }
    case FINGERPRINT: {
      Serial.println("Place Finger on Sensor\nPress 1:");
      pressed1 = readnumber();
      uint8_t fp_id;
      if (pressed1 == 1){
        fp_id = getFingerprintID();
        delay(100);  
      }
      if (check_fingerprint(fp_id, currentUser)) {
        currentState = UNLOCK;
      } else {
        currentState = FAILURE;
      }
      break;
    }
    case UNLOCK: {
      status = "Success";
      Serial.println("Unlocking... Sending key to server");
      if (send_key_to_server()) { // waits until key send finished
        Serial.println("Access granted");
        currentState = IDLE;
      }
      break;
    }
    case FAILURE: {
      status = "Failure";
      Serial.println("Authentication failed");
      if (send_key_to_server()) { // waits until key send finished
        currentState = IDLE;
      }
      currentState = IDLE;
      break;
    }
    case EDIT_USERS: {
      // String adminPass = "";
      // while(adminPass == ""){
      //   while (!Serial.available());
      //   adminPass = Serial.readStringUntil('\n');
      // }

      // bool works = (adminPass == "admin");

      if (admin_pass()){ 
        Serial.println("{1} Add User\n{2} Delete User");
        while (!Serial.available());
        char adminEntry = Serial.read();
        Serial.read();
        if (adminEntry == '1') {
          currentState = EDIT_ADD_USER;
        } else if (adminEntry == '2') {
          currentState = EDIT_DELETE_USER;
        } else {
          Serial.println("Invalid entry");
          currentState = EDIT_USERS;
        }
      } 
      // else if(adminPass == ""){
      //   Serial.println("No password entered");
      //   currentState = EDIT_USERS;
      // }
      else {
        Serial.println("Admin Authentication Failed");
        currentState = IDLE;
      }
      break;
    }
    case EDIT_ADD_USER: {
      Serial.println("Scan New RFID Tag:");
      String rfid = check_rfid();
 
      Serial.println("Enter new 6-digit PIN:");
      currentUser.password = readSerialLine();

      /*----------------FINGER PRINT---------------------*/
      Serial.println("Enter ID for fingerprint (1-127):");
      currentUser.fingerprintKey = readnumber();
      if (currentUser.fingerprintKey == 0) {// ID #0 not allowed, try again!
        return;
      }
      uint8_t fingersuccess;
      while (! (fingersuccess = getFingerprintEnroll()) );

      if(fingersuccess != true) {
        Serial.println("Failed to add user due to fingerprint");
      }
      /*-------------------------------------------------*/
      else if (add_user_sd(rfid, currentUser)) {
        Serial.println("User successfully added");
        currentState = IDLE;
      } else {
        Serial.println("Failed to add user");
        // stay in EDIT_ADD_USER to try again
      }
      break;
    }
    case EDIT_DELETE_USER: {
      Serial.println("Scan RFID Tag to Delete:");
      String rfid = check_rfid();
      check_rfid_sd(rfid, currentUser);

      if (delete_user_sd(rfid)) {
        deleteFingerprint(currentUser.fingerprintKey);
        Serial.println("User successfully deleted");
        currentState = IDLE;

      } else {
        Serial.println("Failed to delete user SD");
        currentState = IDLE;
        // stay in EDIT_DELETE_USER to try again
      }
      break;
    }
  }
}


String check_rfid() {
  // if (!mfrc522.PICC_IsNewCardPresent()) {
	// 	return;
	// }
  while(!mfrc522.PICC_IsNewCardPresent()) {}
	// Select one of the cards.
	while (!mfrc522.PICC_ReadCardSerial()) {}


  String rfid_string = "";
  for(byte i = 0; i < mfrc522.uid.size; i++){
    rfid_string += String(mfrc522.uid.uidByte[i], HEX);
  }
  // MFRC522Debug::PrintUID(Serial, (mfrc522.uid));
  return rfid_string; //std::to_string(mfrc522.uid); // input into serial for now
}

bool send_key_to_server() {
  // adjust to send success or failure
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Create the full URL
    // String serverPath = String(serverName) + "/submit?key=" + rfid;
    String serverPath = String(serverName) + "/submit?key=" + rfid + "&status=" + status;
    
    http.begin(serverPath);
    int httpResponseCode = http.GET();  // Perform the GET request
    
    while(httpResponseCode != 0){}

    String response = http.getString();
    http.end();  // Free resources
    return true;
  } else {
    Serial.println("WiFi not connected");
  }
  return false;
}

bool admin_pass() {
  Serial.println("Enter admin password:");
  String pass = readSerialLine();
  if(pass == "") Serial.println("No Password Entered");
  pass.trim();
  return (pass == "admin"); // the admin password
}


/*-----------------------------FINGERPRINT SCANNER FUNCTIONS--------------------------------*/
/***************************************************
  This is an example sketch for our optical Fingerprint sensor

  Designed specifically to work with the Adafruit BMP085 Breakout
  ----> http://www.adafruit.com/products/751

  These displays use TTL Serial to communicate, 2 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  Small bug-fix by Michael cochez

  BSD license, all text above must be included in any redistribution
 ****************************************************/
uint8_t getFingerprintEnroll() 
{
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(currentUser.fingerprintKey);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(currentUser.fingerprintKey);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(currentUser.fingerprintKey);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.println(currentUser.fingerprintKey);
  p = finger.storeModel(currentUser.fingerprintKey);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}

uint8_t getFingerprintID() 
{
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}

uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
  }

  return p;
}
/*----------------------------------------------------------------------------*/

/*---------------------------SD FUNCTIONS-------------------------------------*/

bool check_rfid_sd(String filename, UserData &user) {
  //filename = filename.substring(0, 8); //truncate to 8
  filename = "/" + String(filename) + ".txt";
  if (!SD.exists(filename)) {
    return false;
  }

  File file = SD.open(filename, FILE_READ);
  if (!file) {
    return false; // Couldn't open it for some reason
  }

  String content = file.readStringUntil('\n');
  file.close();

  int commaIndex = content.indexOf(',');
  if (commaIndex == -1) {
    return false; // Invalid file format
  }

  String keyString = content.substring(0, commaIndex);
  keyString.trim();
  user.fingerprintKey = (uint8_t) keyString.toInt();
  user.password = content.substring(commaIndex + 1);

  // Remove any trailing newline or carriage return from password
  user.password.trim();

  return true;
}

bool check_fingerprint(uint8_t fp_id, UserData &user) {
  return (fp_id == user.fingerprintKey);
}


bool check_password(String pin, UserData &user) {
  pin.trim();
  user.password.trim();
  return (pin == user.password);
}

bool add_user_sd(String filename,UserData &user) {
  //filename = filename.substring(0, 8); //truncate to 8
  filename = "/" + filename + ".txt";
  File file = SD.open(filename.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing!");
    return false; // Couldn't open/create the file
  }

  String content = String(user.fingerprintKey) + "," + user.password;
  file.println(content);
  file.close();

  return true;
}

bool delete_user_sd(String filename) {
  filename = "/" + filename + ".txt";
  if (SD.exists(filename)) {
    if (SD.remove(filename)) {
      // Serial.println("File deleted successfully.");
      return true;
    } 
    else {
      Serial.println("Failed to delete user.");
      return false;
    }
  }
  Serial.println("User does not exist.");
  return false;
}
