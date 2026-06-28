#include <WiFi.h>
#include <DHT.h>
#include <ThingSpeak.h>

// ─── WiFi Credentials ───────────────────────────────
const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// ─── ThingSpeak Config ───────────────────────────────
unsigned long channelID  = 12345;          // your channel ID
const char* writeAPIKey  = "********";     // your write API key

// ─── Pin Definitions ─────────────────────────────────
#define DHT22_PIN   4
#define MQ135_PIN   34
#define RELAY_PIN   26

// ─── DHT Setup ───────────────────────────────────────
DHT dht22(DHT22_PIN, DHT22);

// ─── Thresholds for relay auto-control ───────────────
#define TEMP_THRESHOLD     35.0   // °C — relay ON if above this
#define AQ_THRESHOLD       600    // raw ADC — relay ON if above this

WiFiClient client;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // relay OFF initially

  dht22.begin();

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi Connected!");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client);
}

void loop() {
  // ─── Read DHT22 ──────────────────────────────────
  float temp22 = dht22.readTemperature();
  float humi22 = dht22.readHumidity();

  // ─── Read MQ135 (raw analog 0–4095) ──────────────
  int airQuality = analogRead(MQ135_PIN);

  // ─── Validate readings ────────────────────────────
  if (isnan(temp22) || isnan(humi22)) {
    Serial.println("⚠️ DHT22 read failed!");
    temp22 = -1; humi22 = -1;
  }

  // ─── Relay Logic ──────────────────────────────────
  bool relayState = false;
  if (temp22 > TEMP_THRESHOLD || airQuality > AQ_THRESHOLD) {
    digitalWrite(RELAY_PIN, HIGH);
    relayState = true;
    Serial.println("🔴 Relay ON — High Temp or Bad Air!");
  } else {
    digitalWrite(RELAY_PIN, LOW);
    relayState = false;
    Serial.println("🟢 Relay OFF — Conditions normal");
  }

  // ─── Serial Monitor Output ────────────────────────
  Serial.println("================================");
  Serial.printf("DHT22 → Temp: %.1f°C | Humidity: %.1f%%\n", temp22, humi22);
  Serial.printf("MQ135 → Air Quality (raw): %d\n", airQuality);
  Serial.printf("Relay → %s\n", relayState ? "ON" : "OFF");
  Serial.println("================================");

  // ─── Send to ThingSpeak ───────────────────────────
  ThingSpeak.setField(1, temp22);
  ThingSpeak.setField(2, humi22);
  ThingSpeak.setField(3, airQuality);
  ThingSpeak.setField(4, relayState ? 1 : 0);

  int response = ThingSpeak.writeFields(channelID, writeAPIKey);
  if (response == 200) {
    Serial.println("✅ Data sent to ThingSpeak!");
  } else {
    Serial.printf("❌ ThingSpeak error: %d\n", response);
  }

  delay(20000);  // 20 seconds (ThingSpeak free tier limit)
}