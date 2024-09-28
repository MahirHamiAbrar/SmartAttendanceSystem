#include "melody.h"

#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>

// WiFi Credentials
const char* ssid = "ESP8266-AP";
const char* password = "123456789";

// Server address (use the IP or URL of your server)
const char* serverURL = "http://192.168.4.1/";

// Define the pins for the RFID module (adjusted for ESP32)
#define SS_PIN 5
#define RST_PIN 4

#define LED_C1 12
#define LED_C2 13
#define LED_LAB 14

MFRC522 rfid(SS_PIN, RST_PIN);

// Initialize the LCD with the I2C address
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Timing variables
unsigned long previousRFIDMillis = 0;
unsigned long previousFetchMillis = 0;
const long rfidInterval = 150;
const long fetchInterval = 250;

void setup() {
  // Initialize the serial port
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);

  // Initialize the SPI bus and RFID reader
  SPI.begin();
  rfid.PCD_Init();

  // set pinmodes
  pinMode(LED_C1, OUTPUT);
  pinMode(LED_C2, OUTPUT);
  pinMode(LED_LAB, OUTPUT);

  digitalWrite(LED_C1, LOW);
  digitalWrite(LED_C2, LOW);
  digitalWrite(LED_LAB, LOW);

  // Initialize the LCD
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
  delay(10);
  print_center(1, "Scan Your Card or,");
  print_center(2, "A Valid NFC Device");
  delay(10);
}

void loop() {
  // Handle RFID reading
  unsigned long currentMillis = millis();
  if (currentMillis - previousRFIDMillis >= rfidInterval) {
    previousRFIDMillis = currentMillis;
    readRFID();
  }

  // Handle fetching WiFi data
  if (currentMillis - previousFetchMillis >= fetchInterval) {
    previousFetchMillis = currentMillis;
    fetchWiFiData();
  }
}

// Function to handle RFID reading and sending data to the server
void readRFID() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    // Read the RFID UID
    String uidHex = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      uidHex += String(rfid.uid.uidByte[i], HEX);
    }

    // Create a JSON object with the RFID UID
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["UID"] = uidHex;
    jsonDoc["room"] = 203;
    jsonDoc["dept"] = "CSE";

    // Serialize the JSON object to a string
    String jsonData;
    serializeJson(jsonDoc, jsonData);

    // Display the UID on the LCD
    lcd.clear();
    print_center(1, "Scan ID: " + uidHex);

    // Send the JSON data to the server
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverURL);  // Specify the server URL
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

    // Halt PICC (stop reading)
    rfid.PICC_HaltA();
  }
}

// Function to fetch JSON data from the server and update the LCD
void fetchWiFiData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);  // Specify the server URL

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
        // Handle JSON data and control LEDs or display info on the LCD
        if (jsonDoc.containsKey("led")) {
          String led = jsonDoc["led"].as<String>();
          bool status = jsonDoc["on"].as<bool>();
          auto state = (status ? HIGH : LOW);

          Serial.println("LED = " + led + "; Status: " + (status ? "ON" : "OFF"));
          
          if (led == "c1") digitalWrite(LED_C1, state);
          else if (led == "c2") digitalWrite(LED_C2, state);
          else if (led == "lab") digitalWrite(LED_LAB, state);
        } else {
          String toid = jsonDoc["to"].as<String>();
          if (toid != "203") return;

          String name = jsonDoc["name"].as<String>();
          String idnt = "Student";
          int code = jsonDoc["code"].as<int>();

          lcd.clear();
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
        }
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
