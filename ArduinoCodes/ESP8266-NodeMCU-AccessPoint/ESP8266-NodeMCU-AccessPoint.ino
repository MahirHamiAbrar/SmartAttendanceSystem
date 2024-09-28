// this version is ment for ESP32 boards not ESP8266-NodeMCU boards!

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// Define the Access Point credentials
const char* ssid = "ESP8266-AP";
const char* password = "123456789";

// Create an instance of the server on port 80
WebServer server(80);

// Global variable to hold JSON data
String globalJsonData = "{}";

// To store multiple client responses
WiFiClient clients[2];  // Assuming 2 clients for now
bool sendData = false;

void setup() {
  // Initialize the Serial port
  Serial.begin(115200);

  // Set up the Access Point
  WiFi.softAP(ssid, password);

  // Print the IP address of the Access Point
  Serial.println("['Access Point started");
  Serial.print("IP address: ");
  Serial.print(WiFi.softAPIP());
  Serial.println("']");

  // Define the handler for the root URL (GET request)
  server.on("/", HTTP_GET, handleGet);

  // Define the handler for the root URL (POST request)
  server.on("/", HTTP_POST, handlePost);

  // Start the server
  server.begin();
}

unsigned long long int pm1 = 0, pm2 = 0;
int delay1 = 100, delay2 = 50;

void loop() {
  unsigned long long int cm = millis();

  if (cm - pm1 >= delay1) {
    checkSerial();
    pm1 = cm;
  }

  if (cm - pm2 >= delay2) {
    handleClientRequests();
    pm2 = cm;
  }
}

void checkSerial() {
  if (Serial.available()) {
    String serialData = Serial.readStringUntil('\n');
    serialData.trim();

    if (serialData != "") {
      // Update the global JSON data with the incoming serial data
      globalJsonData = serialData;
      sendData = true;
      
      // Broadcast the data from the Serial port to all connected clients
      broadcastToClients(globalJsonData);
    }
  }
}

void handleClientRequests() {
  server.handleClient();
  checkForNewClients();
}

void handleGet() {
  if (sendData) {
    // Send the global JSON data to all clients
    server.send(200, "application/json", globalJsonData);
    sendData = false;
  }
}

void handlePost() {
  // Handle POST request
  if (server.hasArg("plain")) {
    String jsonData = server.arg("plain");

    // Parse JSON data
    StaticJsonDocument<200> jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, jsonData);

    if (!error) {
      // Print received JSON to Serial Monitor
      serializeJson(jsonDoc, Serial);
      Serial.println();  // Print a newline for better readability

      // Store the received JSON data in the global variable
      globalJsonData = jsonData;

      // Send response back to client
      server.send(200, "text/plain", "Data received");

      // Broadcast the data to all connected clients
      broadcastToClients(globalJsonData);

    } else {
      Serial.println("{'code': -404, 'message': 'Failed to parse JSON.'}");
      server.send(400, "text/plain", "Failed to parse JSON.");
    }
  } else {
    server.send(400, "text/plain", "No data received");
  }
}

// Function to check for new clients and store them in the array
void checkForNewClients() {
  WiFiClient newClient = server.client();
  if (newClient) {
    for (int i = 0; i < 2; i++) {
      if (!clients[i]) {
        clients[i] = newClient;
        Serial.println("['New Client Connected.']");
        break;
      }
    }
  }
}

// Function to broadcast data to all connected clients
void broadcastToClients(const String &data) {
  for (int i = 0; i < 2; i++) {
    if (clients[i] && clients[i].connected()) {
      clients[i].println(data);  // Send data to each client
      Serial.println("Sent: " + data);
    } else if (clients[i]) {
      clients[i].stop();  // If client disconnected, free up the slot
      clients[i] = WiFiClient();
      Serial.print("['Client ");
      Serial.print(i+1);
      Serial.println(" disconnected.']");
    }
  }
}
