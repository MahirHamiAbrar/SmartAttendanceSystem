#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

// Define the Access Point credentials
const char* ssid = "ESP8266-AP";
const char* password = "123456789";

// Create an instance of the server on port 80
ESP8266WebServer server(80);

// Global variable to hold JSON data
String globalJsonData = "{}";

bool sendData = false;

void setup() {
  // Initialize the Serial port
  Serial.begin(115200);

  // Set up the Access Point
  WiFi.softAP(ssid, password);

  // Print the IP address of the Access Point
  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // Define the handler for the root URL (GET request)
  server.on("/", HTTP_GET, []() {
    if (sendData) {
      // Send the global JSON data to clients
      server.send(200, "application/json", globalJsonData);
      sendData = false;
    }
  });


  // Define the handler for the root URL (POST request)
  server.on("/", HTTP_POST, []() {
    // Handle POST request
    if (server.hasArg("plain")) {
      String jsonData = server.arg("plain");

      // Parse JSON data
      StaticJsonDocument<200> jsonDoc;
      DeserializationError error = deserializeJson(jsonDoc, jsonData);

      if (!error) {
        // Print received JSON to Serial Monitor
        // Serial.println("Received JSON:");
        // serializeJsonPretty(jsonDoc, Serial);
        serializeJson(jsonDoc, Serial);
        Serial.println();  // Print a newline for better readability

        // Store the received JSON data in the global variable
        globalJsonData = jsonData;

        // Send response back to client
        server.send(200, "text/plain", "Data received");
      } else {
        Serial.println("{'code': -404, 'message': 'Failed to parse JSON.'}");
        server.send(400, "text/plain", "Failed to parse JSON.");
      }
    } else {
      server.send(400, "text/plain", "No data received");
    }
  });

  // Start the server
  server.begin();
}

void loop() {
  // Handle client requests
  server.handleClient();

  // Check if there's incoming data from Serial
  if (Serial.available()) {
    String serialData = Serial.readStringUntil('\n');
    serialData.trim();

    if (serialData != "") {
      // Update the global JSON data with the incoming serial data
      globalJsonData = serialData;
      // Serial.println("Updated global JSON data from Serial:");
      // Serial.println(globalJsonData);
      sendData = true;
    }
  }
}




// //YWROBOT
// //Compatible with the Arduino IDE 1.0
// //Library version:1.1
// #include <LiquidCrystal_I2C.h>

// LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// void setup()
// {

//   Serial.begin(9600);
//   Serial.println("LCD Init");
//   lcd.init();                      // initialize the lcd 
//   // Print a message to the LCD.
//   lcd.backlight();
//   lcd.setCursor(3,0);
//   lcd.print("Hello, world!");
//   lcd.setCursor(2,1);
//   lcd.print("Ywrobot Arduino!");
//    lcd.setCursor(0,2);
//   lcd.print("Arduino LCM IIC 2004");
//    lcd.setCursor(2,3);
//   lcd.print("Power By Ec-yuan!");

//   Serial.println("LCD Init Done");
// }


// void loop()
// {
// }
