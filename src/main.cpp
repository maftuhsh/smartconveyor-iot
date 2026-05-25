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
// FUNCTION
// ======================================

void connectWiFi();
void connectMQTT();
void ReadConveyor();
void publishMQTT();

// ======================================

void setup() {

  Serial.begin(115200);

  pinMode(Photoelectric, INPUT_PULLUP);

  // inductive NPN
  pinMode(InductiveSensor, INPUT_PULLUP);

  // connect wifi
  connectWiFi();

  // SSL insecure
  espClient.setInsecure();

  // MQTT
  client.setServer(mqtt_server, 8883);

  Serial.println("System Ready!");
}

// ======================================

void loop() {

  if (!client.connected()) {

    connectMQTT();
  }

  client.loop();

  ReadConveyor();
}

// ======================================
// WIFI
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

    String clientID = "ESP32-";
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
// READ CONVEYOR
// ======================================

void ReadConveyor() {

  bool currentPhotoState =
  digitalRead(Photoelectric);

  // HIGH -> LOW
  if (lastPhotoState == HIGH &&
      currentPhotoState == LOW) {

    // debounce
    if (millis() - lastTrigger >
        debounceDelay) {

      totalBarang++;

      Serial.println("====================");

      Serial.println("Barang Masuk");

      // ==============================
      // TUNGGU BARANG KE INDUCTIVE
      // ==============================

      delay(400);

      // ==============================
      // CEK LOGAM
      // ==============================

      bool metalDetected = false;

      // anti noise
      for(int i=0; i<5; i++){

        if(digitalRead(
            InductiveSensor
          ) == LOW){

          metalDetected = true;
        }

        delay(10);
      }

      // ==============================
      // KLASIFIKASI
      // ==============================

      if(metalDetected){

        totalLogam++;

        Serial.println("LOGAM");

      } else {

        totalNonLogam++;

        Serial.println("NON LOGAM");
      }

      // ==============================
      // SERIAL MONITOR
      // ==============================

      Serial.print("Total Barang : ");
      Serial.println(totalBarang);

      Serial.print("Logam        : ");
      Serial.println(totalLogam);

      Serial.print("Non Logam    : ");
      Serial.println(totalNonLogam);

      // publish mqtt
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

  sprintf(totalStr, "%d", totalBarang);

  sprintf(logamStr, "%d", totalLogam);

  sprintf(nonlogamStr, "%d",
          totalNonLogam);

  client.publish(
    "smartconveyor/totalbarang",
    totalStr
  );

  client.publish(
    "smartconveyor/logam",
    logamStr
  );

  client.publish(
    "smartconveyor/nonlogam",
    nonlogamStr
  );

  Serial.println("MQTT Published!");
}