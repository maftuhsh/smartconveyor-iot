#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// ======================================
// WIFI CONFIG
// ======================================

const char* ssid = "KAMAR SEBELAH";
const char* password = "tetapsama";

// ======================================
// MQTT CONFIG
// ======================================

const char* mqtt_server =
"589230614b1342099527d504f560a5ef.s1.eu.hivemq.cloud";

const char* mqtt_user =
"smartconveyor-iot";

const char* mqtt_password =
"Mesinelektro123";

// ======================================
// MQTT CLIENT
// ======================================

WiFiClientSecure espClient;
PubSubClient client(espClient);

// ======================================
// SENSOR PIN
// ======================================

const int Photoelectric = 27;
const int InductiveSensor = 26;

// ======================================
// COUNTER
// ======================================

int totalBarang = 0;
int totalLogam = 0;
int totalNonLogam = 0;

// ======================================
// STATE
// ======================================

bool lastPhotoState = HIGH;

// debounce
unsigned long lastTrigger = 0;
const int debounceDelay = 500;

// ======================================
// FUNCTION DECLARATION
// ======================================

void connectWiFi();
void connectMQTT();
void ReadConveyor();
void publishMQTT();

// ======================================

void setup() {

  Serial.begin(9600);

  // ==================================
  // PIN MODE
  // ==================================

  pinMode(Photoelectric, INPUT_PULLUP);

  // inductive NPN
  pinMode(InductiveSensor, INPUT_PULLUP);

  // ==================================
  // WIFI
  // ==================================

  connectWiFi();

  // ==================================
  // SSL BYPASS
  // ==================================

  espClient.setInsecure();

  // ==================================
  // MQTT
  // ==================================

  client.setServer(mqtt_server, 8883);

  Serial.println("========================");
  Serial.println("SMART CONVEYOR READY");
  Serial.println("========================");
}

// ======================================

void loop() {

  // reconnect mqtt
  if (!client.connected()) {

    connectMQTT();
  }

  client.loop();

  ReadConveyor();
}

// ======================================
// WIFI CONNECT
// ======================================

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

// ======================================
// MQTT CONNECT
// ======================================

void connectMQTT() {

  while (!client.connected()) {

    Serial.print("Connecting MQTT...");

    String clientID = "ESP32Client-";
    clientID += String(random(0xffff), HEX);

    if (client.connect(
          clientID.c_str(),
          mqtt_user,
          mqtt_password
        )) {

      Serial.println("MQTT Connected!");

    } else {

      Serial.print("Failed rc=");
      Serial.println(client.state());

      delay(2000);
    }
  }
}

// ======================================
// MAIN CONVEYOR SYSTEM
// ======================================

void ReadConveyor() {

  bool currentPhotoState =
  digitalRead(Photoelectric);

  // ==================================
  // DETEKSI BARANG
  // ==================================

  if (lastPhotoState == HIGH &&
      currentPhotoState == LOW) {

    // debounce
    if (millis() - lastTrigger >
        debounceDelay) {

      totalBarang++;

      Serial.println("====================");
      Serial.println("BARANG TERDETEKSI");

      // ==================================
      // DETEKSI LOGAM
      // ==================================

      bool metalDetected = false;

      int detectCount = 0;

      unsigned long startTime =
      millis();

      // monitoring inductive selama 800ms
      while (millis() - startTime < 800) {

        // LOW = logam terdeteksi
        if (digitalRead(
              InductiveSensor
            ) == LOW) {

          detectCount++;
        }

        delay(5);
      }

      // ==================================
      // VALIDASI LOGAM
      // ==================================

      // harus LOW berkali-kali
      // supaya noise tidak dianggap logam

      if (detectCount > 20) {

        metalDetected = true;
      }

      // ==================================
      // HASIL KLASIFIKASI
      // ==================================

      if (metalDetected) {

        totalLogam++;

        Serial.println("STATUS : LOGAM");

      } else {

        Serial.println("STATUS : NON LOGAM");
      }

      // ==================================
      // HITUNG NON LOGAM
      // ==================================

      totalNonLogam =
      totalBarang - totalLogam;

      // ==================================
      // SERIAL MONITOR
      // ==================================

      Serial.print("Total Barang : ");
      Serial.println(totalBarang);

      Serial.print("Total Logam  : ");
      Serial.println(totalLogam);

      Serial.print("Non Logam    : ");
      Serial.println(totalNonLogam);

      // ==================================
      // MQTT PUBLISH
      // ==================================

      publishMQTT();

      lastTrigger = millis();
    }
  }

  lastPhotoState = currentPhotoState;
}

// ======================================
// MQTT PUBLISH
// ======================================

void publishMQTT() {

  char totalStr[10];
  char logamStr[10];
  char nonlogamStr[10];

  sprintf(totalStr, "%d",
          totalBarang);

  sprintf(logamStr, "%d",
          totalLogam);

  sprintf(nonlogamStr, "%d",
          totalNonLogam);

  // TOTAL BARANG
  client.publish(
    "smartconveyor/totalbarang",
    totalStr
  );

  // LOGAM
  client.publish(
    "smartconveyor/logam",
    logamStr
  );

  // NON LOGAM
  client.publish(
    "smartconveyor/nonlogam",
    nonlogamStr
  );

  Serial.println("MQTT Published!");
}