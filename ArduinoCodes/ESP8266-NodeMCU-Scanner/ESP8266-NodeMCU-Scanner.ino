#include "melody.h"


// WiFi Credentials
const char* ssid = "ESP8266-AP";
const char* password = "123456789";

// Server address (use the IP or URL of your server)
const char* serverURL = "http://192.168.4.1/";

#define SW_RX D2
#define SW_TX D3

#define I2C_LCD_SDA D4
#define I2C_LCD_SCL D5

// Software Serial for communication with Arduino Nano
SoftwareSerial nanoSerial(SW_RX, SW_TX);  // RX, TX

// Initialize the LCD with the I2C address
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Timing variables for tasks
unsigned long lastUIDCheck = 0;
unsigned long lastWiFiDataFetch = 0;
const unsigned long uidInterval = 150;
const unsigned long wifiDataInterval = 250;

void setup() {
  Serial.begin(115200);
  nanoSerial.begin(9600);

  pinMode(BUZZER_PIN, OUTPUT);

  // Initialize the LCD
  Wire.begin(I2C_LCD_SDA, I2C_LCD_SCL);  // SDA, SCL

  lcd.init();
  lcd.backlight();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  lcd.setCursor(0, 0);
  lcd.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println("Connected to WiFi");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  delay(2000);

  lcd.clear();
  print_center(1, "Scan Your Card or,");
  print_center(2, "A Valid NFC Device");
}

void loop() {
  unsigned long currentMillis = millis();

  // Check for UID from Arduino Nano
  if (currentMillis - lastUIDCheck >= uidInterval) {
    lastUIDCheck = currentMillis;
    checkUIDTask();
  }

  // Fetch WiFi Data
  if (currentMillis - lastWiFiDataFetch >= wifiDataInterval) {
    lastWiFiDataFetch = currentMillis;
    fetchWiFiDataTask();
  }
}

void checkUIDTask() {
  if (nanoSerial.available()) {
    String uidHex = nanoSerial.readStringUntil('\n');
    uidHex.trim();

    if (uidHex.length() > 0) {
      // Create a JSON object with the RFID UID
      StaticJsonDocument<200> jsonDoc;
      jsonDoc["UID"] = uidHex;
      jsonDoc["room"] = 404;
      jsonDoc["dept"] = "ECE";

      // Serialize the JSON object to a string
      String jsonData;
      serializeJson(jsonDoc, jsonData);

      // Display the UID on the LCD
      lcd.clear();
      print_center(1, "Scan ID: " + uidHex);

      // Send the JSON data to the server
      if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;
        http.begin(client, serverURL);                       // Specify the server URL
        http.addHeader("Content-Type", "application/json");  // Specify content-type header

        int httpResponseCode = http.POST(jsonData);  // Send the JSON data
        if (httpResponseCode > 0) {
          String response = http.getString();  // Get the response to the request
          Serial.println("POST Response code: " + String(httpResponseCode));
          Serial.println("Response: " + response);
        } else {
          Serial.print("Error sending POST: ");
          Serial.println(httpResponseCode);
        }
        http.end();  // Free resources
      } else {
        Serial.println("WiFi disconnected");
      }
    }
  }
}

void fetchWiFiDataTask() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, serverURL);  // Specify the server URL

    int httpResponseCode = http.GET();  // Send the GET request
    if (httpResponseCode > 0) {
      String response = http.getString();  // Get the response to the request

      // Print the received JSON data to Serial Monitor
      Serial.println("Received JSON from server:");
      Serial.println(response);

      // Parse the received JSON
      StaticJsonDocument<200> jsonDoc;
      DeserializationError error = deserializeJson(jsonDoc, response);

      if (!error) {
        String toid = jsonDoc["to"].as<String>();

        if (toid != "404") return;

        String name = jsonDoc["name"].as<String>();
        String idnt = "Student";  // jsonDoc["idnt"].as<String>();
        int code = jsonDoc["code"].as<int>();

        // Clear the LCD and display the received text
        lcd.clear();

        // Handle different response codes
        switch (code) {
            case -1:
              display3l(name, idnt, "Too Late Dear! :(");
              playAccessDenied();
              break;
            case 0:
              display4l(name, idnt, "Attendance Accepted.", jsonDoc["course"].as<String>());
              playAccessGranted();
              break;
            case 1:
              display3l(name, idnt, "You're Too Early!");
              playAccessDenied();
              break;
            case 2:
              display4l(name, idnt, "You're in Wrong Room", "Corr. Room: " + jsonDoc["room"].as<String>());
              playAccessDenied();
              break;
            case 3:
              idnt = "Teacher";
              display3l(name, idnt, "You're in Wrong Room");
              playAccessDenied();
              break;
            case 4:
              display4l(name, idnt, "Left Classroom", "Classes Done: " + jsonDoc["cls_done"].as<String>());
              break;
            case 5:
              idnt = "Teacher";
              display3l(name, idnt, "Teacher Entered");
              break;
            case -5:
              idnt = "Teacher";
              display3l(name, idnt, "Teacher Left");
              break;
            case 400:
              display4l("", "", "Server Error", "Please try again");
              playAccessDenied();
              break;
            case 404:
              display3l("", "", "Invalid ID");
              playAccessDenied();
              break;
            case 500:
            case 501:
              display3l(name, idnt, "No Classes Today");
              playAccessDenied();
              break;
        }
        // }
      } else {
        Serial.println("Failed to parse JSON.");
      }
    } else {
      Serial.print("Error fetching GET: ");
      Serial.println(httpResponseCode);
    }
    http.end();  // Free resources
  } else {
    Serial.println("WiFi disconnected");
  }
}

void display3l(String t1, String t2, String t3) {
  lcd.setCursor(0, 0);
  lcd.print(t1);
  lcd.setCursor(0, 1);
  lcd.print(t2);
  lcd.setCursor(0, 2);
  lcd.print(t3);
}

void display4l(String t1, String t2, String t3, String t4) {
  display3l(t1, t2, t3);
  lcd.setCursor(0, 3);
  lcd.print(t4);
}

void print_center(int y, String t) {
  int x = (20 - t.length()) / 2;
  lcd.setCursor(x, y);
  lcd.print(t);
}


// #include <SPI.h>
// #include <MFRC522.h>
// #include <ArduinoJson.h>
// #include <Wire.h>
// #include <LiquidCrystal_I2C.h>
// #include <ESP8266WiFi.h>
// #include <ESP8266HTTPClient.h>

// // WiFi Credentials
// const char* ssid = "ESP8266-AP";
// const char* password = "123456789";

// // Server address (use the IP or URL of your server)
// const char* serverURL = "http://192.168.4.1/";

// // Define the pins for the RFID module
// #define SS_PIN D4
// #define RST_PIN D3

// MFRC522 rfid(SS_PIN, RST_PIN);

// // Initialize the LCD with the I2C address
// LiquidCrystal_I2C lcd(0x27, 20, 4);

// // Timing variables for tasks
// unsigned long lastRFIDCheck = 0;
// unsigned long lastWiFiDataFetch = 0;
// const unsigned long rfidInterval = 150;
// const unsigned long wifiDataInterval = 250;

// void setup() {
//   // Initialize the serial port
//   Serial.begin(115200);

//   // Initialize the SPI bus and RFID reader
//   SPI.begin();
//   rfid.PCD_Init();

//   // Initialize the LCD
//   Wire.begin(D2, D1);  // SDA, SCL
//   lcd.init();
//   lcd.backlight();

//   // Connect to WiFi
//   WiFi.begin(ssid, password);
//   lcd.setCursor(0, 1);
//   lcd.print("Connecting to WiFi");

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }

//   Serial.println("Connected to WiFi");
//   lcd.clear();
//   lcd.setCursor(0, 0);
//   lcd.print("WiFi Connected");
//   lcd.setCursor(0, 1);
//   lcd.print(WiFi.localIP());

//   delay(2000);

//   lcd.clear();
//   print_center(1, "Scan Your Card or,");
//   print_center(2, "A Valid NFC Device");
// }

// void loop() {
//   unsigned long currentMillis = millis();

//   // Check RFID
//   if (currentMillis - lastRFIDCheck >= rfidInterval) {
//     lastRFIDCheck = currentMillis;
//     readRFIDTask();
//   }

//   // Fetch WiFi Data
//   if (currentMillis - lastWiFiDataFetch >= wifiDataInterval) {
//     lastWiFiDataFetch = currentMillis;
//     fetchWiFiDataTask();
//   }
// }

// void readRFIDTask() {
//   // Check for incoming RFID tag
//   if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
//     // Read the RFID UID
//     String uidHex = "";
//     for (byte i = 0; i < rfid.uid.size; i++) {
//       uidHex += String(rfid.uid.uidByte[i], HEX);
//     }

//     // Create a JSON object with the RFID UID
//     StaticJsonDocument<200> jsonDoc;
//     jsonDoc["UID"] = uidHex;
//     jsonDoc["room"] = 203;
//     jsonDoc["dept"] = "CSE";

//     // Serialize the JSON object to a string
//     String jsonData;
//     serializeJson(jsonDoc, jsonData);

//     // Display the UID on the LCD
//     lcd.clear();
//     print_center(1, "Scan ID: " + uidHex);

//     // Send the JSON data to the server
//     if (WiFi.status() == WL_CONNECTED) {
//       WiFiClient client;
//       HTTPClient http;
//       http.begin(client, serverURL);  // Specify the server URL
//       http.addHeader("Content-Type", "application/json");  // Specify content-type header

//       int httpResponseCode = http.POST(jsonData);  // Send the JSON data
//       if (httpResponseCode > 0) {
//         String response = http.getString();  // Get the response to the request
//         Serial.println("POST Response code: " + String(httpResponseCode));
//         Serial.println("Response: " + response);
//       } else {
//         Serial.print("Error sending POST: ");
//         Serial.println(httpResponseCode);
//       }
//       http.end();  // Free resources
//     } else {
//       Serial.println("WiFi disconnected");
//     }

//     // Halt PICC (stop reading)
//     rfid.PICC_HaltA();
//   }
// }

// void fetchWiFiDataTask() {
//   if (WiFi.status() == WL_CONNECTED) {
//     WiFiClient client;
//     HTTPClient http;
//     http.begin(client, serverURL);  // Specify the server URL

//     int httpResponseCode = http.GET();  // Send the GET request
//     if (httpResponseCode > 0) {
//       String response = http.getString();  // Get the response to the request

//       // Print the received JSON data to Serial Monitor
//       Serial.println("Received JSON from server:");
//       Serial.println(response);

//       // Parse the received JSON
//       StaticJsonDocument<200> jsonDoc;
//       DeserializationError error = deserializeJson(jsonDoc, response);

//       if (!error) {
//         // If the key "resp" exists, it's probably a response from the server
//         // display the data on the display
//         if (!jsonDoc.containsKey("resp")) {
//           String name = "Student-x"; //jsonDoc["name"].as<String>();
//           String idnt = "Student"; //jsonDoc["idnt"].as<String>();
//           int code = jsonDoc["code"].as<int>();

//           // Clear the LCD and display the received text
//           lcd.clear();

//           // Handle different response codes
//           switch (code) {
//             case -1:
//               display3l(name, idnt, "Too Late Dear! :(");
//               break;
//             case -2:
//               display3l(name, idnt, "You're Too Early!");
//               break;
//             case -3:
//               {
//                 String room = jsonDoc["room"].as<String>();
//                 display4l(name, idnt, "You're in Wrong Room", room);
//               }
//               break;
//             case -4:
//               {
//                 String classes_done = jsonDoc["cls_done"].as<String>();
//                 display4l(name, idnt, "Left The Room.", classes_done);
//               }
//               break;
//             case 0:
//               {
//                 String course = jsonDoc["course"].as<String>();
//                 display4l(name, idnt, "Attendance Accepted.", course);
//               }
//               break;
//             case -100:
//               {
//                 String room = jsonDoc["room"].as<String>();
//                 display4l(name, idnt, "You're in Wrong Room.", room);
//               }
//               break;
//           }
//         }
//       } else {
//         Serial.println("Failed to parse JSON.");
//       }
//     } else {
//       Serial.print("Error fetching GET: ");
//       Serial.println(httpResponseCode);
//     }
//     http.end();  // Free resources
//   } else {
//     Serial.println("WiFi disconnected");
//   }
// }

// void display3l(String t1, String t2, String t3) {
//   lcd.setCursor(0, 0);
//   lcd.print(t1);
//   lcd.setCursor(0, 1);
//   lcd.print(t2);
//   lcd.setCursor(0, 2);
//   lcd.print(t3);
// }

// void display4l(String t1, String t2, String t3, String t4) {
//   display3l(t1, t2, t3);
//   lcd.setCursor(0, 3);
//   lcd.print(t4);
// }

// void print_center(int y, String t) {
//   int x = (20 - t.length()) / 2;
//   lcd.setCursor(x, y);
//   lcd.print(t);
// }