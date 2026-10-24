# ❤️ ESP32 Real-Time ECG Monitoring System

A real-time ECG monitoring project using **ESP32 and the AD8232 ECG sensor**. It displays ECG waveforms on a web dashboard using WebSockets, allowing wireless monitoring through a smartphone or computer.

## ✨ Features

* 📈 Real-time ECG waveform visualization.
* 📶 Wi-Fi Access Point mode — no internet or router required.
* 🔄 WebSocket communication for continuous data streaming.
* ❤️ ECG signal monitoring with lead-off detection.
* 📊 Displays current, average, and peak ECG values.
* 💓 Basic heart-rate estimation (BPM).
* 🎚️ Adjustable graph gain and display speed.
* 📱 Responsive web dashboard for mobile and desktop.
* 🧪 Simulated waveform for basic display testing when the ADC reading is near maximum.

## 🛠️ Components Required

* ESP32 Development Board
* AD8232 ECG Sensor Module
* ECG Electrodes and Leads
* Jumper Wires
* USB Cable for programming

## 🔌 Pin Connections

| AD8232 Pin | ESP32 GPIO |
| ---------- | ---------- |
| OUTPUT     | GPIO 34    |
| LO+        | GPIO 2     |
| LO-        | GPIO 4     |
| 3.3V       | 3.3V       |
| GND        | GND        |

**Note:** These GPIO assignments are for the ESP32 board targeted by the code. GPIO 34 is input-only and is suitable for analog readings on the original ESP32.

## 📚 Required Libraries

Install the following libraries through the Arduino IDE Library Manager:

* `WebSockets` by Markus Sattler
* `ArduinoJson`
* `WiFi` (included with the ESP32 board package)
* `WebServer` (included with the ESP32 board package)

## 🚀 Getting Started

1. Install the ESP32 board package and required libraries in Arduino IDE.
2. Connect the AD8232 module according to the wiring table.
3. Select your ESP32 board and the appropriate COM port.
4. Upload the code.
5. Open the Serial Monitor at **115200 baud**.
6. Connect your smartphone or computer to the Wi-Fi network:

   * **SSID:** `ECG_Monitor_ESP32`
   * **Password:** `12345678`
7. Open a browser and visit **http://192.168.4.1**.
8. Start monitoring and view the ECG waveform.

## ⚙️ Technical Specifications

* **ECG Sampling Rate:** 250 Hz (configured in the code)
* **ADC Resolution:** 12-bit
* **Web Server:** HTTP, port 80
* **WebSocket Server:** Port 81
* **Communication:** ESP32 Wi-Fi Access Point
* **Data Format:** JSON

## ⚠️ Important Disclaimer

This project is intended for **educational and experimental purposes only**. It is not a certified medical device and must not be used for clinical diagnosis or medical decisions.

The displayed heart rate is estimated using a basic peak-detection algorithm and may be inaccurate. The simulated waveform is for testing the display only and does not represent actual ECG measurements.

**Electrical safety:** Do not connect electrodes to a person while the ESP32 is connected to non-isolated mains-powered equipment. Use appropriate isolation and battery-powered operation for human testing.

## 📜 License

Open-source project for educational and personal use.
