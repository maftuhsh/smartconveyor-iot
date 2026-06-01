#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// ======================================================
// WIFI CONFIG
// ======================================================
const char* ssid = "esp32cam";
const char* password = "00000000";

// ======================================================
// MQTT CONFIG
// ======================================================
const char* mqtt_server = "589230614b1342099527d504f560a5ef.s1.eu.hivemq.cloud";
const char* mqtt_user = "smartconveyor-iot";
const char* mqtt_password = "Mesinelektro123";

// ======================================================
// MQTT CLIENT
// ======================================================
WiFiClientSecure espClient;
PubSubClient client(espClient);

// ======================================================
// SENSOR PIN
// ======================================================
// Photoelectric
const int Photoelectric = 27;
// Inductive Sensor
const int inductiveSensor = 26;

// ======================================================
// VARIABLE
// ======================================================
bool lastPhotoState = HIGH;

int totalBarang = 0;
int totalLogam = 0;
int totalNonLogam = 0;

// debounce
unsigned long lastTrigger = 0;
const int debounceDelay = 300;

// ======================================================
// FUNCTION DECLARATION
// ======================================================
void connectWiFi();
void connectMQTT();
void ReadPhotoelectric();

// ======================================================
// SETUP
// ======================================================
void setup() {
  // Menaikkan baudrate ke 115200 agar komunikasi data serial lebih cepat
  Serial.begin(115200);

  // ==========================================
  // SENSOR MODE
  // ==========================================
  pinMode(Photoelectric, INPUT_PULLUP);

  // FIX: Wajib menggunakan INPUT_PULLUP agar pin tidak mengambang (floating)
  // dan kebal dari noise yang dihasilkan oleh modul WiFi ESP32
  pinMode(inductiveSensor, INPUT_PULLUP);

  // ==========================================
  // WIFI
  // ==========================================
  connectWiFi();

  // ==========================================
  // SSL
  // ==========================================
  espClient.setInsecure();

  // ==========================================
  // MQTT
  // ==========================================
  client.setServer(mqtt_server, 8883);

  Serial.println("=================================");
  Serial.println("SYSTEM READY");
  Serial.println("=================================");
}

// ======================================================
// LOOP
// ======================================================
void loop() {
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  ReadPhotoelectric();
}

// ======================================================
// WIFI CONNECT
// ======================================================
void connectWiFi() {
  Serial.print("Connecting WiFi");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// ======================================================
// MQTT CONNECT
// ======================================================
void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting MQTT...");

    String clientID = "ESP32Client-";
    clientID += String(random(0xffff), HEX);

    if (client.connect(clientID.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("MQTT CONNECTED");
    } else {
      Serial.print("FAILED rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// ======================================================
// PHOTOELECTRIC SENSOR
// ======================================================
void ReadPhotoelectric() {
  bool currentPhotoState = digitalRead(Photoelectric);

  // ==========================================
  // DETEKSI BARANG (HIGH ke LOW)
  // ==========================================
  if (lastPhotoState == HIGH && currentPhotoState == LOW) {
    if (millis() - lastTrigger > debounceDelay) {
      
      totalBarang++;

      // ======================================
      // BACA INDUCTIVE DENGAN FILTER KETAT
      // ======================================
      bool metalDetected = false;
      int lowCount = 0;

      // Beri sedikit jeda waktu (50 milidetik) agar posisi barang 
      // benar-benar pas berada di bawah sensor inductive setelah memicu photoelectric
      delay(50); 

      // Sampling diperbanyak menjadi 30 kali untuk akurasi ekstra
      for (int i = 0; i < 30; i++) {
        if (digitalRead(inductiveSensor) == LOW) {
          lowCount++;
        }
        delay(2); 
      }

      Serial.print("INDUCTIVE LOW COUNT = ");
      Serial.println(lowCount);

      // Threshold disesuaikan: Jika minimal 22 dari 30 sampling bernilai LOW,
      // maka dikonfirmasi sebagai LOGAM asli (bukan karena noise listrik)
      if (lowCount >= 22) {
        metalDetected = true;
      }

      // ======================================
      // KLASIFIKASI
      // ======================================
      if (metalDetected) {
        totalLogam++;
      } else {
        totalNonLogam++;
      }

      // ======================================
      // SERIAL MONITOR
      // ======================================
      Serial.println("=======================");
      Serial.println("BARANG TERDETEKSI");

      if (metalDetected) {
        Serial.println("JENIS: LOGAM");
      } else {
        Serial.println("JENIS: NON LOGAM");
      }

      Serial.print("TOTAL BARANG : ");
      Serial.println(totalBarang);
      Serial.print("TOTAL LOGAM : ");
      Serial.println(totalLogam);
      Serial.print("TOTAL NON LOGAM : ");
      Serial.println(totalNonLogam);

      // ======================================
      // MQTT PUBLISH
      // ======================================
      client.publish("smartconveyor/totalbarang", String(totalBarang).c_str());
      client.publish("smartconveyor/logam", String(totalLogam).c_str());
      client.publish("smartconveyor/nonlogam", String(totalNonLogam).c_str());

      Serial.println("MQTT PUBLISHED");

      lastTrigger = millis();
    }
  }

  lastPhotoState = currentPhotoState;
}