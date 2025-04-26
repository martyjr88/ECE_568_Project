
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

State currentState = IDLE;

struct UserData {
  String rfid;
  uint8_t fingerprintKey;
  String password;
  void clear() {
    fingerprintKey = 0;
    password = "123456";
  }
};

UserData currentUser;

String check_rfid();
bool check_rfid_sd(String rfid, UserData &user);
bool check_password(String pin, UserData &user);
bool check_fingerprint(uint8_t key, UserData &user);
bool send_key_to_server();
bool admin_pass();
bool add_user_sd(String rfid, UserData &user);
bool delete_user_sd(String rfid);

/*---------rfid stuff------------*/
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>

MFRC522DriverPinSimple ss_pin(27);
MFRC522DriverSPI driver{ss_pin};
MFRC522 mfrc522{driver};

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);
 
#define RX_PIN 7  // Connect to fingerprint sensor's TX
#define TX_PIN 8  // Connect to fingerprint sensor's RX

uint8_t id;
uint8_t pressed1;
uint8_t getFingerprintEnroll();
uint8_t getFingerprintID();
int getFingerprintIDez();

uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}
/*------------------------------*/

String readSerialLine() {
  while (!Serial.available());
  return Serial.readStringUntil('\n');
}

void setup() {
  Serial.begin(115200);
  currentState = IDLE;
  currentUser.clear();
  Serial.print("Beginning Setup\n");
  /* ----------RFID SETUP-------------------*/
  SPI.begin(5, 21, 19, 27); 
  mfrc522.PCD_Init();
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
  Serial.print("RFID Setup Complete\n");
  /*------------FINGERPRINT SETUP--------------*/
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  while (!Serial); // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("Adafruit Fingerprint sensor enrollment");
  
  // set the data rate for the sensor serial port
  finger.begin(57600);
  
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }
  Serial.print("Setup Complete\n");
  /*--------------------------------------------*/
}


void loop() {
  switch (currentState) {
    case IDLE: {
      currentUser.clear();
      Serial.println("Enter 1 for authentication, 2 for edit user:");
      while (!Serial.available());
      char entry = Serial.read();
      Serial.read(); // consume newline if present
      if (entry == '1') currentState = AUTHENTICATION;
      else if (entry == '2') currentState = EDIT_USERS;
      else {
        Serial.println("Invalid entry");
        currentState = IDLE;
      }
      break;
    }
    case AUTHENTICATION: {
      Serial.println("Tap RFID card");
      currentState = RFID;
      break;
    }
    case RFID: {
      String rfid = check_rfid();
      if (check_rfid_sd(rfid, currentUser)) {
        currentState = PASSWORD;
      } else {
        currentState = FAILURE;
      }
      break;
    }
    case PASSWORD: {
      Serial.println("Enter 6-digit PIN:");
      String pin = readSerialLine();
      if (check_password(pin, currentUser)) {
        currentState = FINGERPRINT;
      } else {
        currentState = FAILURE;
      }
      break;
    }
    case FINGERPRINT: {
      uint8_t fpKey = 0;
      Serial.println("Place Finger on Sensor and Press 1");
      pressed1 = readnumber();
      if (pressed1 == 1){
        fpKey = getFingerprintID();
        delay(100);  
      }

      if (check_fingerprint(fpKey, currentUser)) {
        currentState = UNLOCK;
      } else {
        currentState = FAILURE;
      }
      break;
    }
    case UNLOCK: {
      Serial.println("Unlocking... Sending key to server");
      if (send_key_to_server()) { // waits until key send finished
        Serial.println("Access granted");
        currentState = IDLE;
      }
      break;
    }
    case FAILURE: {
      Serial.println("Authentication failed");
      currentState = IDLE;
      break;
    }
    case EDIT_USERS: {
      Serial.println("Enter admin password:");
      if (admin_pass()) {
        Serial.println("Press 1 to add user, 2 to delete user:");
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
      } else {
        Serial.println("Admin authentication failed");
        currentState = IDLE;
      }
      break;
    }
    case EDIT_ADD_USER: {
      Serial.println("Tap new user's RFID tag");
      String rfid = check_rfid();
      currentUser.rfid = rfid;
 
      Serial.println("Enter new user's 6-digit PIN:");
      currentUser.password = readSerialLine();

      /*----------------FINGER PRINT---------------------*/
      Serial.println("Ready to enroll a fingerprint!");
      Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");
      currentUser.fingerprintKey = readnumber();
      if (currentUser.fingerprintKey == 0) {// ID #0 not allowed, try again!
        return;
      }
      Serial.print("Enrolling ID #");
      Serial.println(currentUser.fingerprintKey);
      uint8_t fingersuccess;
      while (! (fingersuccess = getFingerprintEnroll()) );

      if(fingersuccess != true) {
        Serial.println("Failed to add user");
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
      Serial.println("Tap RFID tag of user to delete");
      String rfid = check_rfid();
      if (delete_user_sd(rfid)) {
        Serial.println("User successfully deleted");
        currentState = IDLE;
      } else {
        Serial.println("Failed to delete user");
        // stay in EDIT_DELETE_USER to try again
      }
      break;
    }
  }
}


String check_rfid() {
  Serial.println("Tap RFID Tag");
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
  Serial.printf("Card UID: %s\n", rfid_string);

  return rfid_string; //std::to_string(mfrc522.uid); // input into serial for now
}

bool check_rfid_sd(String rfid, UserData &user) {
  if (rfid == "12345") { // also temporary, replace with call for checking the sd card
    user.fingerprintKey = 1;
    user.password = "123456";
    return true;
  }
return true;
  // return false;
}

bool check_password(String pin, UserData &user) {
  return pin == user.password;
}

bool check_fingerprint(uint8_t key, UserData &user) {
  return key == user.fingerprintKey;
}

bool send_key_to_server() {
  delay(1000); //call to send some sort of success key to server here
  return true;
}

bool admin_pass() {
  String pass = readSerialLine();
  return pass == "admin"; // the admin password
}

bool add_user_sd(String rfid, UserData &user) {
  // call to add new user to sd here
  return true;
}

bool delete_user_sd(String rfid) {
  // call to delete user from sd here
  return true;
}


/*-----------------------------FINGERPRINT SCANNER FUNCTIONS--------------------------------*/
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
/*----------------------------------------------------------------------------*/