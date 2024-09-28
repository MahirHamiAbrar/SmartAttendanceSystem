#ifndef MELODY_H
#define MELODY_H

#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Arduino.h>

#define BUZZER_PIN D8  // Define your buzzer pin

// // Define note frequencies (C4, D4, E4, etc.)
// #define NOTE_C4  261
// #define NOTE_D4  294
// #define NOTE_E4  329
// #define NOTE_F4  349
// #define NOTE_G4  392
// #define NOTE_A4  440
// #define NOTE_B4  493
// #define NOTE_C5  523

// // Melody for Access Granted (C4-E4-G4)
// int grantedMelody[] = { NOTE_C4, NOTE_E4, NOTE_G4 };
// int grantedDurations[] = { 200, 200, 400 };  // Note durations (in ms)

// // Melody for Access Denied (C4-A4-F4)
// int deniedMelody[] = { NOTE_C4, NOTE_A4, NOTE_F4 };
// int deniedDurations[] = { 200, 200, 400 };  // Note durations (in ms)

// class MelodyPlayer {
//   private:
//     int *melody;
//     int *durations;
//     int size;
//     unsigned long previousMillis = 0;
//     int currentNote = 0;
//     bool playing = false;

//   public:
//     MelodyPlayer(int *melody, int *durations, int size)
//       : melody(melody), durations(durations), size(size) {}

//     void play() {
//       if (currentNote < size && !playing) {
//         tone(BUZZER_PIN, melody[currentNote]);  // Play current note
//         playing = true;
//         previousMillis = millis();
//       }
//     }

//     void update() {
//       if (playing && (millis() - previousMillis >= durations[currentNote])) {
//         noTone(BUZZER_PIN);  // Stop the tone
//         playing = false;
//         currentNote++;
//         previousMillis = millis();  // Reset timing for next note
//       }
//     }

//     bool isPlaying() {
//       return currentNote < size;  // Return true if melody is still playing
//     }

//     void reset() {
//       currentNote = 0;
//       playing = false;
//     }
// };

// // Create instances of MelodyPlayer for both melodies
// MelodyPlayer grantedPlayer(grantedMelody, grantedDurations, sizeof(grantedMelody) / sizeof(int));
// MelodyPlayer deniedPlayer(deniedMelody, deniedDurations, sizeof(deniedMelody) / sizeof(int));


void playAccessGranted() {
  // grantedPlayer.reset();
  // grantedPlayer.play();

  digitalWrite(BUZZER_PIN, 1);
  delay(300);
  digitalWrite(BUZZER_PIN, 0);
}

void playAccessDenied() {
  // deniedPlayer.reset();
  // deniedPlayer.play();
  digitalWrite(BUZZER_PIN, 1);
  delay(100);
  digitalWrite(BUZZER_PIN, 0);
  delay(100);
  digitalWrite(BUZZER_PIN, 1);
  delay(100);
  digitalWrite(BUZZER_PIN, 0);
  delay(100);
  digitalWrite(BUZZER_PIN, 1);
  delay(100);
  digitalWrite(BUZZER_PIN, 0);
  delay(100);
}

// void updateMelodies() {
//   grantedPlayer.update();
//   deniedPlayer.update();
// }

#endif  // MELODY_H
