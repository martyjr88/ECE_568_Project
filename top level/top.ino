
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
  String fingerprintKey;
  String password;
  void clear() {
    fingerprintKey = "";
    password = "";
  }
};

UserData currentUser;

String check_rfid();
bool check_rfid_sd(String rfid, UserData &user);
bool check_password(String pin, UserData &user);
String get_fingerprint();
bool check_fingerprint(String key, UserData &user);
bool send_key_to_server();
bool admin_pass();
bool add_user_sd(String rfid, UserData &user);
bool delete_user_sd(String rfid);

String readSerialLine() {
  while (!Serial.available());
  return Serial.readStringUntil('\n');
}

void setup() {
  Serial.begin(115200);
  currentState = IDLE;
  currentUser.clear();
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
      Serial.println("Scan fingerprint");
      String fpKey = get_fingerprint();
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
      Serial.println("Enter new user's 6-digit PIN:");
      currentUser.password = readSerialLine();
      Serial.println("Scan new user's fingerprint");
      currentUser.fingerprintKey = get_fingerprint();
      if (add_user_sd(rfid, currentUser)) {
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
  return readSerialLine(); // input into serial for now
}

bool check_rfid_sd(String rfid, UserData &user) {
  if (rfid == "12345") { // also temporary, replace with call for checking the sd card
    user.fingerprintKey = "abcd";
    user.password = "123456";
    return true;
  }
  return false;
}

bool check_password(String pin, UserData &user) {
  return pin == user.password;
}

String get_fingerprint() {
  Serial.println("Scan Fingerprint"); // input into serial for now
  return readSerialLine();
}

bool check_fingerprint(String key, UserData &user) {
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
