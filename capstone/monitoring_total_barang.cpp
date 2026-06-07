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
// SERVO 360 CONFIG (BERBASIS DURASI WAKTU)
// ======================================================
Servo pusherServo;
const int servoPin = 25; 

// Parameter Kontrol Nilai Kecepatan/Arah Servo 360°
const int speedMaju   = 180; // Berputar penuh arah maju (mendorong)
const int speedMundur = 0;   // Berputar penuh arah mundur (menarik)
const int speedDiam   = 90;  // Nilai tengah untuk menghentikan motor total

// === KALIBRASI JARAK REL (Ubah nilai ini untuk menyesuaikan panjang lintasan) ===
const unsigned long durasiMaju   = 1000; // Waktu mendorong (milidetik)
const unsigned long durasiMundur = 1050; // Waktu menarik (dibuat sedikit lebih lama agar kembali mentok pas)

// State Machine Variables
unsigned long waktuPemicuServo = 0;
bool servoHarusMaju = false;
bool servoHarusMundur = false;

// ======================================================
// SENSOR PIN
// ======================================================
const int Photoelectric = 27;
const int inductiveSensor = 26;

// ======================================================
// VARIABLE
// ======================================================
bool lastPhotoState = HIGH;

int totalBarang = 0;
int totalLogam = 0;
int totalNonLogam = 0;

// Debounce timer untuk Photoelectric
unsigned long lastTrigger = 0;
const int debounceDelay = 300;

// SISTEM ANTREAN GERBANG
bool adaBarangDiConveyor = false; 

// Flag pengunci khusus Inductive agar tidak spamming MQTT saat besi menempel
bool sedangMendeteksiLogam = false;

// ======================================================
// FUNCTION DECLARATION
// ======================================================
void connectWiFi();
void connectMQTT();
void ReadSensors();
void HandleServoPusher(); 

// ======================================================
// SETUP
// ======================================================
void setup() {
  Serial.begin(115200);

  pinMode(Photoelectric, INPUT_PULLUP);
  pinMode(inductiveSensor, INPUT_PULLUP);

  // Alokasi timer internal ESP32 untuk Servo PWM
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  pusherServo.setPeriodHertz(50); 
  pusherServo.attach(servoPin, 500, 2400); 
  
  // WAJIB: Saat awal menyala, servo 360 wajib dipaksa DIAM (90)
  pusherServo.write(speedDiam); 

  connectWiFi();
  
  // Perbaikan memori buffer untuk enkripsi TLS/SSL MQTT HiveMQ
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
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  ReadSensors();
  HandleServoPusher(); 
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

    String clientID = "ConveyorZiqNew-";
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
// SENSOR MONITORING (ANTI-DELAY & PASTI SINKRON)
// ======================================================
void ReadSensors() {
  bool currentPhotoState = digitalRead(Photoelectric);
  bool currentInductiveState = digitalRead(inductiveSensor);

  // ------------------------------------------------------
  // JALUR 1: PHOTOELECTRIC (KENDALIKAN TOTAL BARANG & NON-LOGAM DI AWAL)
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

      client.publish("smartconveyor/totalbarang", String(totalBarang).c_str());
      client.publish("smartconveyor/nonlogam", String(totalNonLogam).c_str());

      lastTrigger = millis();
    }
  }
  lastPhotoState = currentPhotoState;

  // ------------------------------------------------------
  // JALUR 2: INDUCTIVE (INSTAN REFRESH MQTT + TRIGGER SERVO)
  // ------------------------------------------------------
  if (currentInductiveState == HIGH) {
    
    if (adaBarangDiConveyor && !sedangMendeteksiLogam) {
      
      totalLogam++; 

      if (totalNonLogam > 0) {
        totalNonLogam--;
      }

      Serial.println("\n=========================================");
      Serial.println("[INDUCTIVE] -> FIX LOGAM SAH TERDETEKSI!");
      Serial.println("[SYSTEM] -> Menghitung Mundur Jeda 1 Detik Menuju Hentakan...");
      Serial.print("TOTAL LOGAM : "); Serial.println(totalLogam);
      Serial.print("NON LOGAM   : "); Serial.println(totalNonLogam);
      Serial.println("=========================================");

      client.publish("smartconveyor/logam", String(totalLogam).c_str());
      client.publish("smartconveyor/nonlogam", String(totalNonLogam).c_str());

      // REKAYASA WAKTU SERVO
      waktuPemicuServo = millis(); 
      servoHarusMaju = true;       

      sedangMendeteksiLogam = true; 
      adaBarangDiConveyor = false; 
    }
  } else {
    sedangMendeteksiLogam = false;
  }
}

// ======================================================
// FUNGSI KENDALI AKTUATOR SERVO 360 (MURNI MILLIS - ANTI OVERLAP)
// ======================================================
void HandleServoPusher() {
  
  // FASE 1: Menunggu jeda 1 detik setelah sensor aktif, lalu MULAI MAJU
  if (servoHarusMaju && (millis() - waktuPemicuServo >= 1000)) {
    pusherServo.write(speedMaju); // Servo berputar maju (mendorong rel keluar)
    Serial.println("\n>>> [ACTUATOR] -> SERVO MAJU (MENDORONG LONGAL)...");
    
    servoHarusMaju = false;
    servoHarusMundur = true;
    waktuPemicuServo = millis(); // Catat waktu MULAI MAJU
  }

  // FASE 2: Setelah berputar maju selama durasiMaju, langsung BALIK ARAH MUNDUR
  if (servoHarusMundur && (millis() - waktuPemicuServo >= durasiMaju)) {
    pusherServo.write(speedMundur); // Servo langsung balik arah berputar mundur (menarik rel)
    Serial.println(">>> [ACTUATOR] -> SERVO MUNDUR (MENARIK PANGKAL)...");
    
    servoHarusMundur = false;
    waktuPemicuServo = millis(); // Catat waktu MULAI MUNDUR
  }

  // FASE 3: Setelah berputar mundur selama durasiMundur, REM TOTAL (DIAM)
  // (Fase ini jalan otomatis jika servo sedang tidak diperintahkan maju/mundur)
  if (!servoHarusMaju && !servoHarusMundur && (pusherServo.read() == speedMundur) && (millis() - waktuPemicuServo >= durasiMundur)) {
    pusherServo.write(speedDiam); // REM MUTLAK: Servo diam total di pangkal
    Serial.println(">>> [ACTUATOR] -> SERVO STANDBY (DIAM TOTAL).");
  }
}