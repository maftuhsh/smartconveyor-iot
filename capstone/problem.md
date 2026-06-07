aku kan ada project smart conveyor pake iot jadi aku pake 2 sensor photoelectri buat deteksi semua object dan proximity inductive buat deteksi logam intinya projectku ini buat deteksi benda logam dan non logam jadi nanti ada 3 monitoring jadi total object yang di deteksi photo electric dan logam yang deteksi inductive dan non logam adalah hasil semua objek - logam paham kan? nah kek gitu tapi kemarin aku memiliki problem dimana pas detect photoelectri buat semua objek jadi dia masuk di total barang sekaligus logam padahal dia belum sampe di fase sensor inductivenya buat deteksi kan aneh itu bikin aku kesel 
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
ini progam cek sensor inductivenya udah bisa jalan sesuai 
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
ini photoelectric juga sama udah sesuai tapi pas aku satu di mqttnya buat monitor di web malah eror kek tadi 

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

<!DOCTYPE html>
<html lang="en">
<head>

  <meta charset="UTF-8" />

  <meta name="viewport"
        content="width=device-width, initial-scale=1.0"/>

  <title>Smart Conveyor Dashboard</title>

  <!-- GOOGLE FONT -->
  <link rel="preconnect"
        href="https://fonts.googleapis.com">

  <link rel="preconnect"
        href="https://fonts.gstatic.com"
        crossorigin>

  <link href="https://fonts.googleapis.com/css2?family=Poppins:wght@300;400;500;600;700&display=swap"
        rel="stylesheet">

  <!-- MQTT -->
  <script src="https://unpkg.com/mqtt/dist/mqtt.min.js"></script>

  <style>

    *{
      margin:0;
      padding:0;
      box-sizing:border-box;
      font-family:'Poppins',sans-serif;
    }

    body{

      background:
      radial-gradient(circle at top right,
      #1e3a8a 0%,transparent 25%),

      radial-gradient(circle at bottom left,
      #0891b2 0%,transparent 25%),

      #0f172a;

      color:white;

      min-height:100vh;

      overflow-x:hidden;
    }

    .container{

      width:100%;
      max-width:1400px;

      margin:auto;

      padding:20px;
    }

    /* =========================
        HEADER
    ========================= */

    .header{

      display:flex;

      justify-content:space-between;

      align-items:center;

      margin-bottom:25px;

      flex-wrap:wrap;

      gap:20px;
    }

    .title h1{

      font-size:2rem;
      font-weight:700;
    }

    .title p{

      color:#94a3b8;
      margin-top:5px;
    }

    .status{

      display:flex;

      align-items:center;

      gap:10px;

      background:rgba(30,41,59,0.8);

      padding:12px 18px;

      border-radius:14px;

      border:1px solid rgba(255,255,255,0.08);

      backdrop-filter:blur(10px);
    }

    .dot{

      width:12px;
      height:12px;

      border-radius:50%;

      background:#ef4444;

      box-shadow:0 0 10px #ef4444;

      transition:0.3s;
    }

    .dot.connected{

      background:#22c55e;

      box-shadow:0 0 12px #22c55e;
    }

    /* =========================
        GRID
    ========================= */

    .grid{

      display:grid;

      grid-template-columns:
      repeat(auto-fit,minmax(280px,1fr));

      gap:20px;

      margin-bottom:25px;
    }

    /* =========================
        CARD
    ========================= */

    .card{

      background:rgba(30,41,59,0.75);

      backdrop-filter:blur(14px);

      border-radius:24px;

      padding:24px;

      border:1px solid rgba(255,255,255,0.08);

      transition:0.3s;

      position:relative;

      overflow:hidden;
    }

    .card:hover{

      transform:translateY(-5px);

      border-color:#38bdf8;

      box-shadow:
      0 0 25px rgba(56,189,248,0.15);
    }

    .card::before{

      content:"";

      position:absolute;

      top:0;
      left:0;

      width:100%;
      height:5px;

      background:
      linear-gradient(
      90deg,
      #06b6d4,
      #3b82f6
      );
    }

    .card-title{

      font-size:1rem;

      color:#94a3b8;

      margin-bottom:12px;

      font-weight:500;
    }

    .card-value{

      font-size:3.5rem;

      font-weight:700;

      line-height:1;
    }

    .card-sub{

      margin-top:10px;

      color:#94a3b8;

      font-size:0.95rem;
    }

    /* =========================
        COLORS
    ========================= */

    .green{
      color:#22c55e;
    }

    .blue{
      color:#38bdf8;
    }

    .orange{
      color:#f59e0b;
    }

    /* =========================
        BIG CARD
    ========================= */

    .big-card{

      background:rgba(30,41,59,0.75);

      border-radius:24px;

      padding:24px;

      border:1px solid rgba(255,255,255,0.08);

      backdrop-filter:blur(14px);
    }

    .big-title{

      font-size:1.2rem;

      margin-bottom:20px;

      font-weight:600;
    }

    /* =========================
        LOG
    ========================= */

    .logs{

      max-height:350px;

      overflow-y:auto;

      display:flex;

      flex-direction:column;

      gap:12px;
    }

    .logs::-webkit-scrollbar{
      width:6px;
    }

    .logs::-webkit-scrollbar-thumb{

      background:#334155;

      border-radius:10px;
    }

    .log-item{

      background:#0f172a;

      padding:14px;

      border-radius:14px;

      border-left:4px solid #38bdf8;

      animation:fadeIn 0.3s ease;
    }

    .log-time{

      font-size:0.8rem;

      color:#94a3b8;

      margin-bottom:5px;
    }

    @keyframes fadeIn{

      from{

        opacity:0;
        transform:translateY(10px);
      }

      to{

        opacity:1;
        transform:translateY(0);
      }
    }

    /* =========================
        MOBILE
    ========================= */

    @media(max-width:768px){

      .header{

        flex-direction:column;

        align-items:flex-start;
      }

      .title h1{

        font-size:1.6rem;
      }

      .card-value{

        font-size:2.6rem;
      }

      .container{

        padding:15px;
      }
    }

  </style>

</head>

<body>

  <div class="container">

    <!-- =========================
          HEADER
    ========================= -->

    <div class="header">

      <div class="title">

        <h1>
          Smart Conveyor Dashboard
        </h1>

        <p>
          IoT Monitoring System • MQTT
        </p>

      </div>

      <div class="status">

        <div class="dot"
             id="statusDot"></div>

        <span id="statusText">
          Disconnected
        </span>

      </div>

    </div>

    <!-- =========================
          GRID
    ========================= -->

    <div class="grid">

      <!-- TOTAL BARANG -->

      <div class="card">

        <div class="card-title">
          Total Barang
        </div>

        <div class="card-value green"
             id="totalBarang">

          0

        </div>

        <div class="card-sub">
          Total barang conveyor
        </div>

      </div>

      <!-- LOGAM -->

      <div class="card">

        <div class="card-title">
          Barang Logam
        </div>

        <div class="card-value blue"
             id="logam">

          0

        </div>

        <div class="card-sub">
          Total barang logam
        </div>

      </div>

      <!-- NON LOGAM -->

      <div class="card">

        <div class="card-title">
          Non Logam
        </div>

        <div class="card-value orange"
             id="nonlogam">

          0

        </div>

        <div class="card-sub">
          Total barang non logam
        </div>

      </div>

      <!-- MQTT -->

      <div class="card">

        <div class="card-title">
          MQTT Status
        </div>

        <div class="card-value"
             id="mqttStatus"
             style="font-size:2rem;color:#ef4444;">

          OFFLINE

        </div>

        <div class="card-sub">
          Status broker MQTT
        </div>

      </div>

    </div>

    <!-- =========================
          LOG ACTIVITY
    ========================= -->

    <div class="big-card">

      <div class="big-title">
        Realtime Activity
      </div>

      <div class="logs"
           id="logs">

      </div>

    </div>

  </div>

  <script>

    // =====================================
    // MQTT CONFIG
    // =====================================

    const options = {

      username:
      "smartconveyor-iot",

      password:
      "Mesinelektro123",

      reconnectPeriod: 2000,

      connectTimeout: 4000,

      clean: true,

      clientId:
      "dashboard_" +
      Math.random()
      .toString(16)
      .substr(2,8)

    };

    // =====================================
    // MQTT CONNECT
    // =====================================

    const client = mqtt.connect(

      "wss://589230614b1342099527d504f560a5ef.s1.eu.hivemq.cloud:8884/mqtt",

      options
    );

    // =====================================
    // ELEMENT
    // =====================================

    const totalBarang =
    document.getElementById(
    "totalBarang");

    const logam =
    document.getElementById(
    "logam");

    const nonlogam =
    document.getElementById(
    "nonlogam");

    const mqttStatus =
    document.getElementById(
    "mqttStatus");

    const statusDot =
    document.getElementById(
    "statusDot");

    const statusText =
    document.getElementById(
    "statusText");

    const logs =
    document.getElementById(
    "logs");

    // =====================================
    // LAST VALUE
    // =====================================

    let lastTotal = -1;
    let lastLogam = -1;
    let lastNonLogam = -1;

    // =====================================
    // CONNECT
    // =====================================

    client.on("connect", () => {

      console.log(
      "MQTT Connected");

      statusDot.classList.add(
      "connected");

      statusText.innerText =
      "Connected";

      mqttStatus.innerText =
      "ONLINE";

      mqttStatus.style.color =
      "#22c55e";

      // =============================
      // SUBSCRIBE
      // =============================

      client.subscribe(
      "smartconveyor/totalbarang");

      client.subscribe(
      "smartconveyor/logam");

      client.subscribe(
      "smartconveyor/nonlogam");

      addLog(
      "MQTT Connected");

    });

    // =====================================
    // MQTT MESSAGE
    // =====================================

    client.on(
    "message",
    (topic, message) => {

      const value =
      parseInt(message.toString());

      console.log(
      topic,
      value
      );

      // =============================
      // TOTAL BARANG
      // =============================

      if(
      topic ===
      "smartconveyor/totalbarang"
      ){

        if(value !== lastTotal){

          totalBarang.innerText =
          value;

          addLog(
          `Total Barang = ${value}`);

          lastTotal = value;
        }
      }

      // =============================
      // LOGAM
      // =============================

      if(
      topic ===
      "smartconveyor/logam"
      ){

        if(value !== lastLogam){

          logam.innerText =
          value;

          addLog(
          `Barang Logam = ${value}`);

          lastLogam = value;
        }
      }

      // =============================
      // NON LOGAM
      // =============================

      if(
      topic ===
      "smartconveyor/nonlogam"
      ){

        if(value !== lastNonLogam){

          nonlogam.innerText =
          value;

          addLog(
          `Non Logam = ${value}`);

          lastNonLogam = value;
        }
      }

    });

    // =====================================
    // OFFLINE
    // =====================================

    client.on("offline", () => {

      statusDot.classList.remove(
      "connected");

      statusText.innerText =
      "Disconnected";

      mqttStatus.innerText =
      "OFFLINE";

      mqttStatus.style.color =
      "#ef4444";

      addLog(
      "MQTT Disconnected");

    });

    // =====================================
    // ERROR
    // =====================================

    client.on("error", (err) => {

      console.error(err);

      addLog(
      "MQTT Error");

    });

    // =====================================
    // RECONNECT
    // =====================================

    client.on("reconnect", () => {

      addLog(
      "Reconnect MQTT...");

    });

    // =====================================
    // LOG FUNCTION
    // =====================================

    function addLog(text){

      const item =
      document.createElement("div");

      item.classList.add(
      "log-item");

      const time =
      new Date()
      .toLocaleTimeString();

      item.innerHTML = `

        <div class="log-time">
          ${time}
        </div>

        <div>
          ${text}
        </div>

      `;

      logs.prepend(item);

      // maksimal 30 log
      if(logs.children.length > 30){

        logs.removeChild(
        logs.lastChild);
      }

    }

  </script>

</body>
</html> 

 coba kamu analisa masalahnya 