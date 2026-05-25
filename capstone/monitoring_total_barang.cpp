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

const char* mqtt_user = "smartconveyor-iot";
const char* mqtt_password = "Mesinelektro123";

// ======================================
// MQTT CLIENT
// ======================================

WiFiClientSecure espClient;
PubSubClient client(espClient);

// ======================================
// SENSOR PHOTOELECTRIC
// ======================================

const int Photoelectric = 27;

bool lastState = HIGH;

int totalBarang = 0;

// debounce
unsigned long lastTrigger = 0;
const int debounceDelay = 200;

// ======================================
// FUNCTION DECLARATION
// ======================================

void connectWiFi();
void connectMQTT();
void ReadPhotoelectric();

// ======================================

void setup() {

  Serial.begin(9600);

  pinMode(Photoelectric, INPUT_PULLUP);

  // connect wifi
  connectWiFi();

  // SSL bypass
  espClient.setInsecure();

  // MQTT setup
  client.setServer(mqtt_server, 8883);

  Serial.println("System Ready!");
}

// ======================================

void loop() {

  // reconnect mqtt
  if (!client.connected()) {

    connectMQTT();
  }

  client.loop();

  ReadPhotoelectric();
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
// PHOTOELECTRIC SENSOR
// ======================================

void ReadPhotoelectric() {

  bool currentState = digitalRead(Photoelectric);

  // HIGH -> LOW
  if (lastState == HIGH && currentState == LOW) {

    // debounce
    if (millis() - lastTrigger > debounceDelay) {

      totalBarang++;

      Serial.println("====================");

      Serial.print("Barang Terdeteksi!");
      Serial.print(" | Total: ");

      Serial.println(totalBarang);

      // ==================================
      // MQTT PUBLISH
      // ==================================

      String payload = String(totalBarang);

      client.publish(
        "smartconveyor/totalbarang",
        payload.c_str()
      );

      Serial.println("MQTT Published");

      lastTrigger = millis();
    }
  }

  lastState = currentState;
}