#include <Arduino.h>

// ======================================
// TEST INDUCTIVE SENSOR ESP32
// ======================================

const int inductiveSensor = 26;

bool lastState = HIGH;

int totalLogam = 0;

// debounce
unsigned long lastTrigger = 0;
const int debounceDelay = 200;

void setup() {

  Serial.begin(115200);

  pinMode(inductiveSensor, INPUT_PULLUP);

  Serial.println("=== TEST INDUCTIVE SENSOR ===");
}

void loop() {

  bool currentState = digitalRead(inductiveSensor);

  // Deteksi perubahan HIGH -> LOW
  // (logam terdeteksi)
  if (lastState == HIGH && currentState == LOW) {

    if (millis() - lastTrigger > debounceDelay) {

      totalLogam++;

      Serial.println("====================");
      Serial.println("LOGAM TERDETEKSI!");
      Serial.print("Total Logam: ");
      Serial.println(totalLogam);

      lastTrigger = millis();
    }
  }

  lastState = currentState;
}