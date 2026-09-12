# ESP8266 Universal Remote (IR & RF Smart Lighting Controller)

A unified smart lighting hub running on an ESP8266 (NodeMCU v2 / ESP-12E). Bridges multiple infrared (IR) lamps and an RF-controlled overhead fixture into a single microcontroller framework with non-blocking hardware control, a responsive dark-themed web dashboard, REST API, ArduinoOTA wireless flashing, and simultaneous Serial CLI access.

---

## ⚡ Hardware Pinout & Circuit Interfacing

| Target Device | Interface Type | ESP8266 Pin | LM3046 / Driver Pin | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **IR Transmitter** | 38 kHz Modulated IR | **GPIO 14 (D5)** | Transistor / IR LED circuit | Drives 38 kHz carrier |
| **RF Power** | Transistor Low-Side Switch | **GPIO 12 (D6)** | Base $\rightarrow$ Collector | Remote Power Test Pad |
| **RF Warmer** | Transistor Low-Side Switch | **GPIO 4 (D2)** | Base $\rightarrow$ Collector | Remote Warmer Test Pad |
| **RF Dimmer** | Transistor Low-Side Switch | **GPIO 13 (D7)** | Base $\rightarrow$ Collector | Remote Dimmer Test Pad |
| **RF Cooler** | Transistor Low-Side Switch | **GPIO 16 (D0)** | Base $\rightarrow$ Collector | Remote Cooler Test Pad |
| **RF Brighter** | Transistor Low-Side Switch | **GPIO 5 (D1)** | Base $\rightarrow$ Collector | Remote Brighter Test Pad |

- **Transistor Array:** LM3046 NPN array with $1\text{ k}\Omega$ base resistors and $10\text{ k}\Omega$ pull-down resistors to ground. Substrate (Pin 13) and emitters connected to common ground.
- **Power:** ESP8266 3.3V rail supplies the RF remote PCB. Direct common ground links ESP8266 GND, LM3046 emitters/substrate, and the RF remote battery negative terminal.

---

## 🛠️ Protocols & Timings

1. **Poster Light (IR):** Standard NEC protocol (`irsend.sendNEC`).
   - ON: `0x7386, 0x03` | OFF: `0x7386, 0x98` | Color: `0x7386, 0x50`
2. **Donut Lamp (IR):** Standard NEC protocol.
   - ON: `0x0000, 0x40` | OFF: `0x0000, 0x41`
3. **Bedside Lamp (IR):** Custom 10-bit Pulse Distance Protocol (`irsend.sendGeneric`).
   - Power Toggle: `0x1F2` | Brighter: `0x3B2` | Dimmer: `0x1E2` | Warmer: `0x3D2` | Cooler: `0x0E2`
4. **RF Overhead Lamp:** Active HIGH 150 ms pulse with 100 ms cooldown.
5. **Non-Blocking Execution Engine:** All hardware timings and macro sequences run in a non-blocking `millis()` state machine, allowing HTTP client connections, Wi-Fi packets, and ArduinoOTA updates to execute without lag.

---

## 🚀 Quick Start & Flashing

### 1. Configure Wi-Fi Credentials
Edit `include/secrets.h` (or copy from `include/secrets.example.h`):
```cpp
#define WIFI_SSID "YourWiFiNetwork"
#define WIFI_PASSWORD "YourPassword"
#define OTA_PASSWORD "admin123"
```

### 2. Flashing via PlatformIO (USB / CP2102)
Connect the ESP8266 over USB and run:
```bash
pio run -t upload
pio device monitor -b 115200
```

### 3. Wireless Updates (ArduinoOTA)
Once the ESP8266 is connected to your Wi-Fi network, you can update firmware over-the-air:
In `platformio.ini`, uncomment:
```ini
upload_protocol = espota
upload_port = universal-remote.local
upload_flags =
    --auth=admin123
```
Then run:
```bash
pio run -t upload
```

---

## 📱 Web Dashboard & mDNS Discovery

Once booted and connected to your Wi-Fi, the web dashboard is available at:
- **mDNS:** `http://universal-remote.local/`
- **IP Address:** Printed to Serial on boot (e.g. `http://192.168.1.xxx/`)

### Features
- **Responsive Dark Mode:** Self-contained single-page application with touch feedback and card layout.
- **Smart Macros:** "ALL ON" and "ALL OFF" sequentially power devices on/off, intelligently checking assumed device states to prevent toggle misfires on the Bedside and Overhead lamps.
- **Manual State Sync:** Sync buttons to reconcile software state if a light was toggled via physical remote.
- **Zero External Dependencies:** 100% offline-ready embedded HTML/CSS/JS in `PROGMEM`.

---

## 🌐 REST API Endpoints

### 1. Trigger Command
```http
POST /api/command?action=<action_name>
```
**Supported Actions:**
- Poster: `poster_on`, `poster_off`, `poster_color`
- Donut: `donut_on`, `donut_off`
- Bedside: `bedside_power`, `bedside_brighter`, `bedside_dimmer`, `bedside_warmer`, `bedside_cooler`
- Overhead RF: `rf_power`, `rf_warmer`, `rf_dimmer`, `rf_cooler`, `rf_brighter`
- Macros: `all_on`, `all_off`

**Sample Response:**
```json
{
  "success": true,
  "action": "all_on",
  "states": {
    "poster": true,
    "donut": true,
    "bedside": true,
    "overhead": true,
    "busy": false
  }
}
```

### 2. Query Status
```http
GET /api/status
```
Returns JSON with current assumed states of all lights and queue busy status.

### 3. State Resynchronization
```http
POST /api/resync?device=<bedside|overhead|poster|donut>&state=<1|0>
```
Updates tracked state in memory without transmitting any IR or RF pulses.

---

## ☁️ HiveMQ Cloud MQTT & Worldwide Remote Portal

Control all fixtures from outside your local Wi-Fi without opening firewall ports.

### 1. Broker Setup (HiveMQ Cloud Free Serverless)
1. Register a free account at [hivemq.com/cloud](https://www.hivemq.com/cloud/) and create a cluster.
2. In the HiveMQ Console, navigate to **Access Management** and create client credentials (Username and Password).
3. Add your cluster URL and credentials to `include/secrets.h`:
```cpp
#define MQTT_HOST "your-cluster-id.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_USER "your_username"
#define MQTT_PASS "your_password"
#define MQTT_TOPIC_PREFIX "home/universal_remote"
```
4. Flash the firmware via OTA or USB.

### 2. MQTT Topics & Payloads
- **Inbound Commands:** `home/universal_remote/command` (Payload: string e.g. `bedside_dimmer`, `poster_on`, `all_off`)
- **Outbound State (Retained):** `home/universal_remote/status` (JSON: `{"poster":false,"donut":false,"bedside":true,"overhead":false,"busy":false}`)
- **Availability (LWT):** `home/universal_remote/availability` (`online` / `offline`)

### 3. Remote Web Portal (`portal/index.html`)
- A standalone HTML5 single-page application connecting directly to HiveMQ Cloud over secure WebSockets (`wss://`, port `8884`).
- Open `portal/index.html` in Chrome/Safari on any device (cellular or remote network), click ⚙️ Settings, enter your HiveMQ Cluster Host, Username, and Password.
- Can be deployed to GitHub Pages, Netlify, or saved to your phone's home screen for instant remote access.

