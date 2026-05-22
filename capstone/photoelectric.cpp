// ======================================
// TEST SENSOR PHOTOELECTRIC ESP32
// ======================================
#include <Arduino.h>
const int photoSensor = 27;

bool lastState = HIGH;
int totalBarang = 0;

// debounce timer
unsigned long lastTrigger = 0;
const int debounceDelay = 200;

void setup() {

  Serial.begin(9600);

  pinMode(photoSensor, INPUT_PULLUP);

  Serial.println("=== TEST PHOTOELECTRIC SENSOR ===");
}

void loop() {

  bool currentState = digitalRead(photoSensor);

  // Deteksi perubahan HIGH -> LOW
  if (lastState == HIGH && currentState == LOW) {

    // debounce
    if (millis() - lastTrigger > debounceDelay) {

      totalBarang++;

      Serial.println("====================");
      Serial.print("Objek Terdeteksi!");
      Serial.print(" | Total Barang: ");
      Serial.println(totalBarang);

      lastTrigger = millis();
    }
  }

  lastState = currentState;
}