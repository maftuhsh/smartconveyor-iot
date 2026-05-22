#include <Arduino.h>
#include <ESP32Servo.h>

// ======================
// PIN
// ======================

const int inductiveSensor = 26;
const int servoPin = 18;

// ======================
// SERVO
// ======================

Servo pusherServo;

// posisi servo
const int servoNormal = 0;
const int servoPush = 90;

// ======================
// TIMER
// ======================

bool logamDetected = false;

unsigned long detectTime = 0;

// delay barang menuju pusher
const unsigned long delayPusher = 2000; // 2 detik

void setup() {

  Serial.begin(115200);

  pinMode(inductiveSensor, INPUT_PULLUP);

  pusherServo.attach(servoPin);

  // posisi awal
  pusherServo.write(servoNormal);

  Serial.println("SYSTEM READY");
}

void loop() {

  bool sensorState = digitalRead(inductiveSensor);

  // =========================
  // DETEKSI LOGAM
  // =========================

  if (sensorState == LOW && !logamDetected) {

    logamDetected = true;

    detectTime = millis();

    Serial.println("LOGAM TERDETEKSI");
  }

  // =========================
  // DELAY MENUJU PUSHER
  // =========================

  if (logamDetected &&
      millis() - detectTime >= delayPusher) {

    Serial.println("PUSHER AKTIF");

    // dorong barang
    pusherServo.write(servoPush);

    delay(500);

    // kembali
    pusherServo.write(servoNormal);

    Serial.println("PUSHER RESET");

    logamDetected = false;
  }
}