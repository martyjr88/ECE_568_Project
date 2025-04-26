#include <WiFi.h>
#include <HTTPClient.h>

// My wifi if anyone wants it ig
//const char* ssid = "Weida Apt 1-444 N Grant St";
//const char* password = "NmvcSPgcwVm!";

// Charles's Wifi lol
const char* ssid = "ssh -X yourmom";
const char* password = "wifipassword123";

const char* serverIP = "192.168.0.12"; // My PC IP
const int serverPort = 8000;
const char* userKey = "fakey faker";

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.println("IP address: " + WiFi.localIP().toString());

  // Send POST request
  sendLoginKey(userKey);
}

void loop() {
}

void sendLoginKey(const char* key) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = String("http://") + serverIP + ":" + String(serverPort) + "/submit";
    String postData = "key=" + String(key);

    http.begin(url); 
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("Server response: ");
      Serial.println(response);
    } else {
      Serial.print("Error in POST request. Code: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}
