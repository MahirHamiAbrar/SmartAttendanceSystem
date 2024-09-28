#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>

#define SS_PIN 10
#define RST_PIN 9

MFRC522 rfid(SS_PIN, RST_PIN);
SoftwareSerial espSerial(2, 3);  // RX, TX

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("Arduino Nano RFID Reader");
}

void loop() {
  // Check for new cards
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    // Read the RFID UID
    String uidHex = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      uidHex += String(rfid.uid.uidByte[i], HEX);
    }

    // Send UID via Software Serial
    espSerial.println(uidHex);
    Serial.println("Sent UID: " + uidHex);

    // Halt PICC
    rfid.PICC_HaltA();
    // Stop encryption on PCD
    rfid.PCD_StopCrypto1();
  }
}