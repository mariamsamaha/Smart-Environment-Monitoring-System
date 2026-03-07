# Smart Environment Monitoring & Control System

This project uses an ESP32 microcontroller and multiple sensors to monitor environmental conditions and detect motion or obstacles. The system collects data such as temperature, humidity, light intensity, motion, and distance, and responds using LEDs, a buzzer, and a servo motor

---

# Phase 1 – Project Setup & Hardware Integration

## Objective
The goal of Phase 1 is to set up the hardware components and establish communication between the ESP32 and the sensors

## Components Used

| Component | Description |
|-----------|-------------|
| ESP32 | Main microcontroller |
| DHT22 | Temperature and humidity sensor |
| Photoresistor (LDR module) | Light intensity detection |
| HC-SR501 | PIR motion sensor |
| HC-SR04 | Ultrasonic distance sensor |
| LEDs | Status indicators |
| Buzzer | Audio alert |
| Servo Motor | Mechanical movement |
| Relay Module | Control external device |

---

## Hardware Setup

1. Connect all sensors to the ESP32 according to the pin configuration.
2. Power sensors using **3.3V** and **GND**.
3. Upload test code to verify each sensor is working.
4. Monitor readings through the Serial Monitor in Wokwi.

---

# Phase 2 – Sensor Data Collection & Basic Logic

## Objective
The goal of Phase 2 is to read sensor data and implement basic logic for monitoring and alerts

## Sensor Functions

| Sensor | Function | Data Measured | Example Logic |
|------|------|------|------|
| DHT22 | Measures environmental conditions | Temperature, Humidity | Display temperature and humidity values in Serial Monitor |
| Photoresistor (LDR) | Detects light intensity | Light level | If light level is low → turn on LED |
| HC-SR501 | Detects motion | Motion presence | If motion is detected → activate buzzer or LED |
| HC-SR04 | Measures distance to objects | Distance | If object distance is below threshold → trigger alert |

---

## Simulation

The system was tested using the Wokwi simulator to verify sensor functionality and system responses before hardware implementation