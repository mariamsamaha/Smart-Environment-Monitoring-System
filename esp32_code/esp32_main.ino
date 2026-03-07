#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

#define DHTPIN    4
#define DHTTYPE   DHT22
#define LDR_PIN   34
#define PIR_PIN   27
#define TRIG_PIN  5
#define ECHO_PIN  18
#define LED_RED   26
#define LED_ORANGE 25
#define LED_GREEN 33
#define BUZZER    32
#define SERVO_PIN 13
#define RELAY_PIN 12

float temp_max  = 30.0;
int   light_min = 2000;
float dist_min  = 20.0;

const char* ssid        = "Wokwi-GUEST";
const char* password    = "";
const char* mqtt_server = "broker.hivemq.com";
const int   mqtt_port   = 1883;

DHT          dht(DHTPIN, DHTTYPE);
WiFiClient   espClient;
PubSubClient client(espClient);
Servo        myServo;

unsigned long lastSensorRead = 0;
unsigned long buzzerOnTime   = 0;
bool          buzzerActive   = false;
unsigned long lastHeartbeat = 0;


void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  StaticJsonDocument<200> doc;
  deserializeJson(doc, msg);
  String t = String(topic);

  if (t == "actuators/led") {
    String state = doc["state"];
    String color = doc["color"];
    if (color == "red")    digitalWrite(LED_RED,    state == "on" ? HIGH : LOW);
    if (color == "orange") digitalWrite(LED_ORANGE, state == "on" ? HIGH : LOW);
    if (color == "green")  digitalWrite(LED_GREEN,  state == "on" ? HIGH : LOW);
    Serial.printf("LED %s → %s\n", color.c_str(), state.c_str());
  }
  else if (t == "actuators/buzzer") {
    String state = doc["state"];
    digitalWrite(BUZZER, state == "on" ? HIGH : LOW);
    Serial.println("Buzzer → " + state);
  }
  else if (t == "actuators/servo") {
    int angle = doc["angle"];
    myServo.write(angle);
    Serial.printf("Servo → %d°\n", angle);
  }
  else if (t == "actuators/relay") {
    String state = doc["state"];
    digitalWrite(RELAY_PIN, state == "on" ? HIGH : LOW);
    Serial.println("Relay → " + state);
  }
  else if (t == "config/thresholds") {
    if (doc.containsKey("temp_max"))  temp_max  = doc["temp_max"];
    if (doc.containsKey("light_min")) light_min = doc["light_min"];
    if (doc.containsKey("dist_min"))  dist_min  = doc["dist_min"];
    Serial.printf("Thresholds updated → T:%.1f L:%d D:%.1f\n",
                   temp_max, light_min, dist_min);
  }
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32_SmartEnv")) {
      Serial.println("Connected!");
      client.subscribe("actuators/led");
      client.subscribe("actuators/buzzer");
      client.subscribe("actuators/servo");
      client.subscribe("actuators/relay");
      client.subscribe("config/thresholds");
    } else {
      Serial.print("Failed rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5s...");
      delay(5000);
    }
  }
}

float readDistance() {
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2.0;
}

void publishFloat(const char* topic, float value, const char* unit) {
  StaticJsonDocument<100> doc;
  char buffer[100];
  doc["value"] = value;
  doc["unit"]  = unit;
  serializeJson(doc, buffer);
  client.publish(topic, buffer);
}

void publishLight(int value) {
  StaticJsonDocument<100> doc;
  char buffer[100];
  doc["value"] = value;
  doc["level"] = (value > light_min) ? "bright" : "dark";
  serializeJson(doc, buffer);
  client.publish("sensors/light", buffer);
}

void publishMotion(bool detected) {
  StaticJsonDocument<50> doc;
  char buffer[50];
  doc["detected"] = detected;
  serializeJson(doc, buffer);
  client.publish("sensors/motion", buffer);
}

void applyAutoControl(float temp, int light, bool motion, float distance) {
  digitalWrite(LED_RED, temp > temp_max ? HIGH : LOW);

  digitalWrite(LED_ORANGE, light < light_min ? HIGH : LOW);

  digitalWrite(RELAY_PIN, temp > temp_max ? HIGH : LOW);

  if (motion && !buzzerActive) {
    digitalWrite(BUZZER, HIGH);
    buzzerOnTime  = millis();
    buzzerActive  = true;
    Serial.println("⚠ MOTION DETECTED — Buzzer ON");
  }
  if (buzzerActive && millis() - buzzerOnTime >= 2000) {
    digitalWrite(BUZZER, LOW);
    buzzerActive = false;
  }

  if (distance < dist_min) {
    myServo.write(90);
    Serial.println("⚠ OBJECT TOO CLOSE — Barrier OPEN");
  } else {
    myServo.write(0);
  }
}

void publishHeartbeat() {
  if (millis() - lastHeartbeat >= 10000) {
    StaticJsonDocument<150> doc;
    char buffer[150];
    doc["uptime"]      = millis() / 1000;   
    doc["rssi"]        = WiFi.RSSI();        
    doc["free_memory"] = ESP.getFreeHeap();  
    serializeJson(doc, buffer);
    client.publish("system/status", buffer);
    Serial.printf(" Heartbeat: %s\n", buffer);
    lastHeartbeat = millis();
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_RED,    OUTPUT);
  pinMode(LED_ORANGE, OUTPUT);
  pinMode(LED_GREEN,  OUTPUT);
  pinMode(BUZZER,     OUTPUT);
  pinMode(RELAY_PIN,  OUTPUT);

  pinMode(PIR_PIN,  INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  myServo.attach(SERVO_PIN);
  dht.begin();
  delay(2000);

  connectWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  connectMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!client.connected())           connectMQTT();
  client.loop();

  if (millis() - lastSensorRead >= 2000) {
    float temp     = dht.readTemperature();
    float hum      = dht.readHumidity();
    int   light    = analogRead(LDR_PIN);
    bool  motion   = digitalRead(PIR_PIN) == HIGH;
    float distance = readDistance();
    publishHeartbeat();

    if (!isnan(temp) && !isnan(hum)) {
      publishFloat("sensors/temperature", temp, "C");
      publishFloat("sensors/humidity",    hum,  "%");
      publishLight(light);
      publishMotion(motion);
      publishFloat("sensors/distance", distance, "cm");

      applyAutoControl(temp, light, motion, distance);

      Serial.println("─────");
      Serial.printf("Temp:     %.2f C  (limit: %.1f)\n", temp, temp_max);
      Serial.printf("Humidity: %.2f %%\n", hum);
      Serial.printf("Light:    %d  (limit: %d)\n", light, light_min);
      Serial.printf("Motion:   %s\n", motion ? "DETECTED" : "None");
      Serial.printf("Distance: %.2f cm  (limit: %.1f)\n", distance, dist_min);
      Serial.println("──────");
    }

    lastSensorRead = millis();
  }
}