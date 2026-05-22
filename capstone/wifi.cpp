#include <Arduino.h>
#include <WiFi.h>

// ==========================
// WIFI CONFIG
// ==========================

const char* ssid = "NAMA_WIFI";
const char* password = "PASSWORD_WIFI";

// ==========================

void setup() {

  Serial.begin(9600);

  Serial.println();
  Serial.println("=== WIFI TEST ESP32 ===");

  // mulai koneksi WiFi
  WiFi.begin(ssid, password);

  Serial.print("Menghubungkan ke WiFi");

  // tunggu koneksi
  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {

  // cek koneksi WiFi
  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("WiFi Terputus!");
    Serial.println("Reconnect...");

    WiFi.disconnect();
    WiFi.begin(ssid, password);

    delay(5000);
  }

  // monitoring status
  Serial.println("WiFi OK");

  delay(3000);
}