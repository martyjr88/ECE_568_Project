#include <SPI.h>
#include <SD.h>

#define SD_CS 14
struct UserData {
  uint8_t fingerprintKey;
  String password;
  void clear() {
    fingerprintKey = -1;
    password = "";
  }
};



bool read(File &file, String pin, UserData &user) {
  if (!file) {
    Serial.println("File not open!");
    return false;
  }

  file.seek(0); // Start from beginning

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim(); // Clean up extra \r or spaces

    int firstComma = line.indexOf(',');
    int secondComma = line.indexOf(',', firstComma + 1);

    if (firstComma == -1 || secondComma == -1) {
      continue; // Bad/malformed line, skip
    }

    // Extract columns
    String col1 = line.substring(0, firstComma);          // PIN
    String col2 = line.substring(firstComma + 1, secondComma); // fingerprintKey
    String col3 = line.substring(secondComma + 1);         // password

    if (col1 == pin) {
      // Fill in the user struct
      user.fingerprintKey = (uint8_t)col2.toInt(); // Convert string to int safely
      user.password = col3;
      return true; // Found and populated
    }
  }
  return false; // Not found
}

bool check_rfid_sd(File &file,String rfid, UserData &user) {
  return(read(file,rfid,user)); // will tell you if RFID IS IN THERE
}

bool check_fingerprint(File &file,String rfid, UserData &user) {
  UserData user2;
  bool worked = read(file,rfid,user2);
  if(worked) {
    if(user2.fingerprintKey == user.fingerprintKey) return true;
  }
  return false;
}


bool check_password(File &file, String pin, UserData &user) {
  UserData user2;
  bool worked = read(file,pin,user2);
  if(worked) {
    if(user2.password == user.password) return true;
  }
  return false;
}



bool add_user_sd(File &file,String RFID,UserData &user) {
  String strNumber = String(user.fingerprintKey);
  if (file) {
    file.print(RFID);
    file.print(",");   // Add comma if not last column
    file.print(strNumber);
    file.print(",");   // Add comma if not last column
    file.print(user.password);
    Serial.println("WORKED LIL BRO");
    file.println();    // Newline if third column
    file.flush(); // Force write to SD card
    return true;
  }
  return false;
}








void setup() {
  Serial.begin(115200);
  if(!SD.begin(SD_CS)){
    Serial.println("Card Mount Failed!");
    return;
  }
  Serial.println("Card initialized :)");
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD card size: %lluMB\n", cardSize);
  File file = SD.open("/POGGERS.csv", FILE_WRITE);

  /////////////////////////// ////////////////////////////////
  //write
  UserData data; 
  data.fingerprintKey = 111;
  data.password = "passer";
  bool worked = add_user_sd(file,"123", data);
  Serial.printf("DID IT WORK : %d \n", worked);

  data.fingerprintKey = 222;
  data.password = "passer2";
  worked = add_user_sd(file,"4567", data);
  Serial.printf("DID IT WORK : %d \n", worked);

  data.fingerprintKey = 255;
  data.password = "passer3";
  worked = add_user_sd(file,"8910", data);
  Serial.printf("DID IT WORK : %d \n", worked);

  file.close(); // close it



  /////////////////////////// READ / CHECK RFID LOOK AT RFID IT CALLS READ AND READ WORKS 
  // NOW open it for reading
  file = SD.open("/POGGERS.csv", FILE_READ);

  UserData user;
  user.clear(); // Reset it before

  if (read(file, "123", user)) {
  Serial.println("User found:");
  Serial.print("Fingerprint Key: ");
  Serial.println(user.fingerprintKey);
  Serial.print("Password: ");
  Serial.println(user.password);
  } 
  else {
  Serial.println("User not found.");
  }


  user.clear(); // Reset it before

  if (read(file, "123", user)) { // SHOULD BE FOUND
  Serial.println("User found:");
  Serial.print("Fingerprint Key: ");
  Serial.println(user.fingerprintKey);
  Serial.print("Password: ");
  Serial.println(user.password);
  } 
  else {
  Serial.println("User not found.");
  }


  user.clear(); // Reset it before

  if (read(file, "4567", user)) { // SHOULD BE FOUND
  Serial.println("User found:");
  Serial.print("Fingerprint Key: ");
  Serial.println(user.fingerprintKey);
  Serial.print("Password: ");
  Serial.println(user.password);
  } 

  if (read(file, "8911", user)) { // SHOULD NOT BE FOUND
  Serial.println("User found:");
  Serial.print("Fingerprint Key: ");
  Serial.println(user.fingerprintKey);
  Serial.print("Password: ");
  Serial.println(user.password);
  } 
  else {
  Serial.println("User not found.");
  }

  ///////////////////////////
  // CHECK PASSWORD

  // PASSES EXPECTED TO DO SO
  user.fingerprintKey = 111;
  user.password = "passer";
  if (check_password(file, "123", user)) {
  Serial.println("MATCH PASSWORD");
  } 
  else {
  Serial.println("NO MATCH PASSWORD");
  }

  // FAILS(EXPECTED TO DO SO)
  if (check_password(file, "559", user)) {
  Serial.println("MATCH PASSWORD");
  } 
  else {
  Serial.println("NO MATCH PASSWORD");
  }



  ////////////////////////////
  //// CHECK FINGERPRING
  user.fingerprintKey = 111;
  user.password = "passer";
  if (check_fingerprint(file, "123", user)) {
  Serial.println("MATCH FP");
  } 
  else {
  Serial.println("NO MATCH FP");
  }

  // FAILS(EXPECTED TO DO SO)
  if (check_fingerprint(file, "559", user)) {
  Serial.println("MATCH FP ");
  } 
  else {
  Serial.println("NO MATCH FP");
  }

  file.close();
}






void loop() {
}













// void writeToCSV(File &file,String RFID, uint8_t fingerprint, String password) {

//   String strNumber = String(fingerprint);
//   if (file) {
//     file.print(RFID);
//     file.print(",");   // Add comma if not last column
//     file.print(strNumber);
//     file.print(",");   // Add comma if not last column
//     file.print(password);
//     Serial.println("WORKED LIL BRO");
//   } 
//   else {
//     Serial.println("Failed to open file for writing");
//   }
//   file.println();    // Newline if third column
//   file.flush(); // Force write to SD card
// }