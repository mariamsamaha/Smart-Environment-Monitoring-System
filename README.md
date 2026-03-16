# Smart Environment Monitoring & Control System

## What This Project Does

We built a system that watches a room and responds to what it finds. Temperature, humidity, light level, motion, and distance are all read by an ESP32 every two seconds and sent over Wi-Fi to a Node-RED dashboard. The dashboard displays everything live, triggers alerts when something is out of range, and lets you override any actuator manually from your browser.

The whole thing runs on the HiveMQ public MQTT broker, which means the Wokwi simulator and Node-RED can talk to each other without any local network configuration.

---

## Repository Structure

```
TeamName_SmartEnv/
├── esp32_code/
│   └── smart_env.ino          # Arduino sketch — sensors, MQTT, auto-control
├── nodered_flow/
│   └── flow.json              # Node-RED exported flow (98 nodes, F1–F10)
├── diagrams/
│   └── wiring.png             # Full wiring diagram from Wokwi
├── screenshots/
│   └── ...                    # Dashboard screenshots and demo video
└── report.pdf                 # Full project report
```

---

## Hardware

### Sensors

| Component | Model | Pin(s) | Measures |
|-----------|-------|--------|----------|
| Temperature & Humidity | DHT22 | GPIO 4 | °C and %RH |
| Light | LDR / Photoresistor | GPIO 34 (ADC) | Raw light level (0–4095) |
| Motion | HC-SR501 (PIR) | GPIO 27 | Presence detection |
| Distance | HC-SR04 (Ultrasonic) | TRIG: GPIO 5 / ECHO: GPIO 18 | Distance in cm |

### Actuators

| Component | Model | Pin | Triggered By |
|-----------|-------|-----|--------------|
| Red LED | — | GPIO 26 | Temperature above threshold |
| Orange LED | — | GPIO 25 | Low light (dark condition) |
| Green LED | — | GPIO 33 | System OK — all readings normal |
| Buzzer | — | GPIO 32 | Motion detected (2-second burst) |
| Servo Motor | SG90 | GPIO 13 | Object too close — simulates barrier |
| Relay | — | GPIO 12 | Temperature above threshold |

---

## Software

### ESP32 Libraries

- `DHT.h` — reads the DHT22 sensor
- `WiFi.h` — Wi-Fi connection
- `PubSubClient.h` — MQTT client
- `ArduinoJson.h` — JSON serialisation
- `ESP32Servo.h` — servo motor control

### Node-RED Modules

- `node-red-dashboard` v3.6.6
- `node-red-node-ui-table` v0.4.5

---

## MQTT Topics

### ESP32 → Node-RED (sensor data)

| Topic | Payload |
|-------|---------|
| `sensors/temperature` | `{"value": 24.5, "unit": "C"}` |
| `sensors/humidity` | `{"value": 61, "unit": "%"}` |
| `sensors/light` | `{"value": 2800, "level": "bright"}` |
| `sensors/motion` | `{"detected": true}` |
| `sensors/distance` | `{"value": 35.2, "unit": "cm"}` |
| `system/status` | `{"uptime": 120, "rssi": -55, "free_memory": 210000}` |

### Node-RED → ESP32 (actuator commands)

| Topic | Payload |
|-------|---------|
| `actuators/led` | `{"state": "on", "color": "red"}` |
| `actuators/buzzer` | `{"state": "on", "duration": 2000}` |
| `actuators/servo` | `{"angle": 90}` |
| `actuators/relay` | `{"state": "on"}` |

### Node-RED → ESP32 (configuration)

| Topic | Payload | Retain |
|-------|---------|--------|
| `config/thresholds` | `{"temp_max": 30, "light_min": 2000, "dist_min": 20}` | Yes |
| `config/interval` | `{"value": 2000}` | Yes |

---

## Automatic Control Rules

| Condition | Actuator | Response |
|-----------|----------|----------|
| Temperature > temp_max (default 30°C) | Red LED + Relay | Switch ON |
| Light < light_min (default 2000 raw) | Orange LED | Switch ON |
| All conditions within thresholds + no motion | Green LED | Switch ON (system OK) |
| PIR detects motion | Buzzer | Sounds for 2 seconds |
| Distance < dist_min (default 20 cm) | Servo | Rotates to 90° (barrier open) |

All thresholds are adjustable from the dashboard without restarting the ESP32.

---

## Dashboard Tabs

| Tab | What's There |
|-----|-------------|
| Dashboard | Five sensor gauges + motion indicator + live temperature/humidity chart |
| Control | Manual switches for all actuators, servo slider, threshold inputs, reporting interval |
| Alerts | Alert history table (last 50 events with timestamps) |
| Status | ESP32 uptime, RSSI, free heap, and watchdog connection status |
| Data & Notifications | Historical chart (1h/6h/24h), CSV download links, Telegram bot setup |

---

## Setup

### Running the Wokwi Simulation

1. Open [wokwi.com](https://wokwi.com) and create a new ESP32 project.
2. Paste the contents of `esp32_code/smart_env.ino` into the sketch editor.
3. Add these libraries in `libraries.txt`:
   ```
   PubSubClient
   DHT sensor library
   ArduinoJson
   ESP32Servo
   ```
4. The sketch connects to `Wokwi-GUEST` Wi-Fi automatically — no password needed.
5. Click the play button. The ESP32 will connect to HiveMQ and start publishing.

### Running Node-RED

1. Make sure Node.js is installed, then install Node-RED:
   ```bash
   npm install -g --unsafe-perm node-red
   ```
2. Install the dashboard and table modules:
   ```bash
   cd ~/.node-red
   npm install node-red-dashboard node-red-node-ui-table
   ```
3. Start Node-RED:
   ```bash
   node-red
   ```
4. Open `http://localhost:1880` in your browser.
5. Go to the hamburger menu → Import → paste the contents of `nodered_flow/flow.json`.
6. Click Deploy.
7. Open the dashboard at `http://localhost:1880/ui`.

### Telegram Alerts (Optional)

1. Search for `@BotFather` on Telegram, create a new bot, and copy the token.
2. Search for `@userinfobot`, send any message, and copy your numeric chat ID.
3. Paste both into the fields on the **Data & Notifications** tab of the dashboard.
4. Click **Send Test** to verify the connection.

---

## How the Watchdog Works

The ESP32 publishes a heartbeat to `system/status` every 10 seconds. Node-RED records the arrival time and checks it every 5 seconds. If no heartbeat arrives for 30 seconds, the Status tab displays **ESP32 OFFLINE**. When communication resumes, it automatically switches back to showing the uptime and signal strength.

---

## Phase Summary

### Phase 1 — Hardware Setup
Connected all sensors and actuators to the ESP32 according to the pin mapping above. Verified each sensor independently using the Wokwi serial monitor before adding MQTT.

### Phase 2 — Sensor Reading & Basic Logic
Implemented the 2-second sensor reading cycle using millis() (not delay(), which would block MQTT). Added automatic actuator rules based on configurable thresholds. Verified all sensor outputs through the Node-RED debug panel before building the dashboard.

### Phase 3 — Node-RED Dashboard & MQTT Integration
Built the full dashboard with gauges, charts, manual controls, alert system, data logging, and watchdog. All function nodes were audited to confirm correct wiring — a programmatic JSON check caught several disconnected gauge and switch nodes that appeared correct visually but had empty wires arrays.

---

## Known Limitations

- The system uses a public MQTT broker (HiveMQ). Any device that knows the topic names can publish or subscribe. For production use, replace with a local Mosquitto broker with authentication.
- CSV data is stored in Node-RED process memory and written to `/tmp/smartenv.csv`. Restarting Node-RED clears the in-memory log (the file remains).
- The Telegram cooldown key is derived from the alert message text. Very similar alert messages may share a cooldown bucket, reducing notification frequency slightly.
