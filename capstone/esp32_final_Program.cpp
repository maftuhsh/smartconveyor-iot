#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP32Servo.h> 

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
// SERVO 360 CONFIG
// ======================================================
Servo pusherServo;
const int servoPin = 25; 

const int speedMaju   = 180; 
const int speedMundur = 0;   
const int speedDiam   = 90;  

const unsigned long durasiMaju   = 1000; 
const unsigned long durasiMundur = 1050; 

unsigned long waktuPemicuServo = 0;
bool servoHarusMaju = false;
bool servoHarusMundur = false;

// ======================================================
// SENSOR PIN
// ======================================================
const int Photoelectric = 27;
const int inductiveSensor = 26;

// ======================================================
// VARIABLE (KEMBALI KE LOGIKA ASLI LU)
// ======================================================
bool lastPhotoState = HIGH;

int totalBarang = 0;
int totalLogam = 0;
int totalNonLogam = 0;

unsigned long lastTrigger = 0;
const int debounceDelay = 300;

// SISTEM ANTREAN GERBANG ASLI LU (WAJIB ADA)
bool adaBarangDiConveyor = false; 
bool sedangMendeteksiLogam = false;

// Timer Reconnect WiFi Background
unsigned long waktuCekWiFi = 0;
const unsigned long jedaCekWiFi = 10000; 

// ======================================================
// FUNCTION DECLARATION
// ======================================================
void HandleWiFiDanMQTT();
void ReadSensors();
void HandleServoPusher(); 

// ======================================================
// SETUP
// ======================================================
void setup() {
  Serial.begin(115200);

  pinMode(Photoelectric, INPUT_PULLUP);
  pinMode(inductiveSensor, INPUT_PULLUP);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  pusherServo.setPeriodHertz(50); 
  pusherServo.attach(servoPin, 500, 2400); 
  
  pusherServo.write(speedDiam); 

  WiFi.begin(ssid, password);
  
  espClient.setInsecure(); 
  client.setBufferSize(512); 
  client.setServer(mqtt_server, 8883);

  Serial.println("=================================");
  Serial.println("SYSTEM READY - MICRO SERVO 360 MODE");
  Serial.println("=================================");
}

// ======================================================
// LOOP
// ======================================================
void loop() {
  HandleWiFiDanMQTT();
  ReadSensors();
  HandleServoPusher(); 
}

// ======================================================
// MANAGEMENT KONEKSI (NON-BLOCKING)
// ======================================================
void HandleWiFiDanMQTT() {
  if (millis() - waktuCekWiFi >= jedaCekWiFi) {
    waktuCekWiFi = millis();
    
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, password); 
    } else {
      if (!client.connected()) {
        String clientID = "ConveyorZiqNew-";
        clientID += String(random(0xffff), HEX);
        client.connect(clientID.c_str(), mqtt_user, mqtt_password);
      }
    }
  }

  if (WiFi.status() == WL_CONNECTED && client.connected()) {
    client.loop();
  }
}

// ======================================================
// SENSOR MONITORING (100% LOGIKA ASLI UTUH)
// ======================================================
void ReadSensors() {
  bool currentPhotoState = digitalRead(Photoelectric);
  bool currentInductiveState = digitalRead(inductiveSensor);

  // ------------------------------------------------------
  // JALUR 1: PHOTOELECTRIC (Sistem Antrean Aktif)
  // ------------------------------------------------------
  if (lastPhotoState == HIGH && currentPhotoState == LOW) {
    if (millis() - lastTrigger > debounceDelay) {
      
      if (adaBarangDiConveyor) {
        Serial.println("[SYSTEM] -> Objek sebelumnya dipastikan NON-LOGAM.");
      }

      totalBarang++;
      totalNonLogam++; 
      adaBarangDiConveyor = true; 

      Serial.println("\n=========================================");
      Serial.println("[PHOTOELECTRIC] -> OBJEK BARU MASUK CONVEYOR");
      Serial.print("TOTAL BARANG : "); Serial.println(totalBarang);
      Serial.print("NON LOGAM    : "); Serial.println(totalNonLogam);
      Serial.println("=========================================");

      if (WiFi.status() == WL_CONNECTED && client.connected()) {
        client.publish("smartconveyor/totalbarang", String(totalBarang).c_str());
        client.publish("smartconveyor/nonlogam", String(totalNonLogam).c_str());
      }

      lastTrigger = millis();
    }
  }
  lastPhotoState = currentPhotoState;

  // ------------------------------------------------------
  // JALUR 2: INDUCTIVE (Dengan Pengunci Antrean Asli Lu)
  // ------------------------------------------------------
  if (currentInductiveState == HIGH) {
    if (adaBarangDiConveyor && !sedangMendeteksiLogam) {
      
      totalLogam++; 
      if (totalNonLogam > 0) {
        totalNonLogam--;
      }

      Serial.println("\n=========================================");
      Serial.println("[INDUCTIVE] -> FIX LOGAM SAH TERDETEKSI!");
      Serial.print("TOTAL LOGAM : "); Serial.println(totalLogam);
      Serial.print("NON LOGAM   : "); Serial.println(totalNonLogam);
      Serial.println("=========================================");

      if (WiFi.status() == WL_CONNECTED && client.connected()) {
        client.publish("smartconveyor/logam", String(totalLogam).c_str());
        client.publish("smartconveyor/nonlogam", String(totalNonLogam).c_str());
      }

      waktuPemicuServo = millis(); 
      servoHarusMaju = true;       
      sedangMendeteksiLogam = true; 
      adaBarangDiConveyor = false; // Antrean dimatikan agar tidak double hitung
    }
  } else {
    sedangMendeteksiLogam = false;
  }
}

// ======================================================
// FUNGSI KENDALI AKTUATOR SERVO 360 (MURNI ASLI LU)
// ======================================================
void HandleServoPusher() {
  if (servoHarusMaju && (millis() - waktuPemicuServo >= 1200)) {
    pusherServo.write(speedMaju); 
    Serial.println("\n>>> [ACTUATOR] -> SERVO MAJU (MENDORONG LOGAM)...");
    
    servoHarusMaju = false;
    servoHarusMundur = true;
    waktuPemicuServo = millis(); 
  }

  if (servoHarusMundur && (millis() - waktuPemicuServo >= durasiMaju)) {
    pusherServo.write(speedMundur); 
    Serial.println(">>> [ACTUATOR] -> SERVO MUNDUR (MENARIK PANGKAL)...");
    
    servoHarusMundur = false;
    waktuPemicuServo = millis(); 
  }

  if (!servoHarusMaju && !servoHarusMundur && (pusherServo.read() == speedMundur) && (millis() - waktuPemicuServo >= durasiMundur)) {
    pusherServo.write(speedDiam); 
    Serial.println(">>> [ACTUATOR] -> SERVO STANDBY (DIAM TOTAL).");
  }
}