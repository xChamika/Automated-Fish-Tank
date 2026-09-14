# 🐠 ESP32 Automated Fish Tank (Blynk IoT)

An intelligent, non-blocking IoT aquarium management system powered by an ESP32, featuring local OLED carousel feedback, flash-persisted thresholds (`Preferences`), dual-sensor safety alarms, and full Blynk Cloud control.

---

## 🌟 Key Features
* **Multi-Sensor Telemetry**: Real-time water temperature (`DS18B20`) and percentage water level calculation (`HC-SR04`, mapped 5–30cm).
* **Non-Blocking Architecture (`millis()`)**:
  * **P1 (Continuous)**: Debounced physical feed button (`GPIO 14`).
  * **P2 (Every 2s)**: Sensor read, dual-alarm evaluation, OLED refresh, and telemetry push (`V0`/`V1`).
  * **P3 (Every 10s)**: Non-blocking Wi-Fi & Blynk cloud handshake check (3s timeout).
* **0.91" I2C OLED Carousel (128x32)**: Auto-rotates 5 full-screen icon/data slides every 5s (`Temp` ➔ `Level` ➔ `Limits` ➔ `Connectivity` ➔ `Feed Interval`), with priority overlay popups (`CONFIG SAVED`, `WiFi & Blynk Online`, `FEEDING FISH`).
* **Dual Alarms & Edge-Triggered Events**: Latching buzzer activation for threshold breaches + state-change edge notifications (`temp_alert`, `level_alert`).
* **Scheduled & Manual Feeding**: SG90 servo sweep (75° for 400ms) triggered via physical button, Blynk app button (`V2`), or auto-interval countdown (`V7`).
* **NVS Flash Persistence**: Custom thresholds (`tempLow/High`, `levelLow/High`, `feedHrs`) saved across reboots via ESP32 `Preferences`.

---

## 📌 Hardware Pin Mapping

| Component | Pin / Signal | ESP32 GPIO / Rail | Notes |
| :--- | :--- | :--- | :--- |
| **HC-SR04 Ultrasonic** | TRIG / ECHO | GPIO 5 / GPIO 18 | *Use 1k/2k voltage divider on Echo (5V➔3.3V)* |
| **DS18B20 Temp** | Data (DQ) | GPIO 4 | Requires 4.7kΩ pull-up to 3.3V |
| **SG90 Servo** | Signal | GPIO 13 | PWM sweep control |
| **Active Buzzer** | Positive (+) | GPIO 15 | Latching alarm driver |
| **Push Button** | Terminal 1 | GPIO 14 | Internal pullup (`INPUT_PULLUP` to GND) |
| **0.91" OLED (I2C)** | SDA / SCL | GPIO 21 / GPIO 22 | SSD1306 128x32 |

---

## ☁️ Blynk Virtual Pin Mapping (`TMPL6hDqZJ6Eu`)

| Pin | Type | Direction | Description |
| :--- | :--- | :--- | :--- |
| **V0** | Float | ESP32 ➔ App | Water Temperature (°C) |
| **V1** | Integer | ESP32 ➔ App | Water Level (%) |
| **V2** | Button | App ➔ ESP32 | Manual Feed Trigger (`1 = Feed`) |
| **V3 / V4** | Float | App ➔ ESP32 | Temperature Low / High Thresholds |
| **V5 / V6** | Integer | App ➔ ESP32 | Water Level Low / High Thresholds (%) |
| **V7** | Integer | App ➔ ESP32 | Auto-Feed Interval (Hours) |

---

## 🛠️ Dependencies (Arduino IDE / PlatformIO)
* `WiFi.h` (Built-in)
* `BlynkSimpleEsp32`
* `Wire.h` / `Adafruit_GFX.h` / `Adafruit_SSD1306`
* `OneWire.h` / `DallasTemperature.h`
* `ESP32Servo.h`
* `Preferences.h` (Built-in)
