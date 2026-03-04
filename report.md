# 📋 VitalSync — Project Report

> **Project:** Smart Vital Signs Monitoring System
> **Version:** 1.0.0  
> **Date:** March 2026  
> **Author:** x606

---

## 1. Introduction

### 1.1 Project Overview

The **VitalSync** system is a complete IoT-based heart rate and body temperature monitoring system that bridges the gap between hardware sensing and modern web visualization. The system uses an **Arduino UNO** microcontroller with an analog pulse sensor and a **DS18B20 digital temperature sensor** to capture real-time health data, which is then streamed to a beautifully designed web dashboard through a **Node.js** server.

### 1.2 Objectives

- Build a reliable, real-time heart rate and body temperature monitoring system using affordable hardware
- Create a modern, responsive web interface for dual-sensor data visualization
- Implement bi-directional communication between hardware and software
- Provide health status classification and alert mechanisms (high BPM + fever detection)
- Enable session management and data export capabilities

### 1.3 Target Users

- Students and educators in biomedical engineering and IoT courses
- Health enthusiasts for personal monitoring
- Developers learning real-time web technologies with hardware integration
- Researchers needing a quick prototyping platform for biosignal acquisition

---

## 2. System Architecture

### 2.1 High-Level Architecture

The system follows a **three-tier architecture**:

```
┌────────────────────┐
│   HARDWARE LAYER   │    Arduino UNO + Pulse Sensor + DS18B20 + LEDs + Buzzer
│   (Data Capture)   │    Samples at 500Hz (pulse) + every 2s (temperature)
└────────┬───────────┘
         │  Serial (USB — 115200 baud)
         ▼
┌────────────────────┐
│   SERVER LAYER     │    Node.js + Express + Socket.IO + SerialPort
│   (Data Relay)     │    Parses serial data, broadcasts via WebSocket
└────────┬───────────┘
         │  WebSocket (Socket.IO)
         ▼
┌────────────────────┐
│   CLIENT LAYER     │    HTML5 + CSS3 + JavaScript + Chart.js
│   (Visualization)  │    Real-time dashboard with dual charts & controls
└────────────────────┘
```

### 2.2 Communication Protocol

| Channel          | Protocol              | Baud/Format                                |
| ---------------- | --------------------- | ------------------------------------------ |
| Arduino ↔ Server | Serial (USB)          | 115200 baud, line-based (`\r\n` delimiter) |
| Server ↔ Browser | WebSocket (Socket.IO) | JSON events over HTTP upgrade              |

### 2.3 Data Format

**Arduino → Server:**

- `BPM:<integer>` — Heart rate value (e.g., `BPM:72`)
- `TEMP:<float>` — Temperature in °C (e.g., `TEMP:36.5`)
- `STATUS:<state>` — Status message (e.g., `STATUS:READY`, `STATUS:NO_SIGNAL`)

**Server → Arduino:**

- `PAUSE\n`, `RESUME\n`, `RESET\n` — Control commands (uppercase + newline)

---

## 3. Hardware Design

### 3.1 Components Used

| #   | Component                | Quantity | Purpose                                                        |
| --- | ------------------------ | -------- | -------------------------------------------------------------- |
| 1   | Arduino UNO (ATmega328P) | 1        | Main microcontroller                                           |
| 2   | Analog Pulse Sensor      | 1        | Heart rate detection via photoplethysmography                  |
| 3   | DS18B20 Temp Sensor      | 1        | Digital temperature measurement (±0.5°C accuracy)              |
| 4   | 4.7kΩ Resistor           | 1        | Pull-up resistor for DS18B20 data line                         |
| 5   | Capacitor (220 nF)       | 1        | Reduces noise and smooths the sensor signal                    |
| 6   | Resistors (220Ω or 330Ω) | 3        | Limit the current flowing through LEDs and protect the circuit |
| 7   | Green LED                | 1        | Normal heart rate indicator (pin D7)                           |
| 8   | Yellow LED               | 1        | Beat flash indicator (pin D6)                                  |
| 9   | Blue LED                 | 1        | High heart rate indicator (pin D5)                             |
| 10  | Active Buzzer            | 1        | Audio alarm for abnormal BPM (pin D2)                          |
| 11  | USB Cable (Type-A to B)  | 1        | Serial communication & power                                   |

### 3.2 Pin Configuration

| Arduino Pin | Connected To        | Mode              |
| ----------- | ------------------- | ----------------- |
| A0          | Pulse Sensor Signal | Analog Input      |
| D4          | DS18B20 DATA        | Digital (OneWire) |
| D2          | Active Buzzer       | Digital Output    |
| D5          | Blue LED            | Digital Output    |
| D6          | Yellow LED          | Digital Output    |
| D7          | Green LED           | Digital Output    |
| D13         | Onboard LED         | Digital Output    |

### 3.3 Signal Processing Pipeline

```
Raw Analog ─► EMA Filter ─► Peak/Trough ─► Beat Detection ─► IBI Average ─► BPM
  (A0)        (α = 0.25)    Detection       (Threshold)      (10-sample)    Output
```

1. **Sampling:** Timer2 ISR fires every 2ms (500Hz sampling rate)
2. **EMA Filter:** `filtered = 0.25 × raw + 0.75 × previous` — removes high-frequency noise
3. **Adaptive Threshold:** Dynamically adjusts based on signal amplitude with constraints (100–900 ADC range)
4. **Beat Validation:** Rejects inter-beat intervals outside 300–1500ms (40–200 BPM range)
5. **BPM Smoothing:** Output smoothed with `0.7 × previous + 0.3 × new` ratio
6. **Skip Initial Beats:** First 3 beats are discarded for stability

---

## 4. Software Design

### 4.1 Server (`server.js`)

**Technology:** Node.js with Express, Socket.IO, and SerialPort

| Feature             | Implementation                                                 |
| ------------------- | -------------------------------------------------------------- |
| Static File Serving | Express serves `public/` directory                             |
| Serial Connection   | `SerialPort` library with `ReadlineParser` (`\r\n` delimiter)  |
| Data Parsing        | Regex-based parsing of `BPM:`, `TEMP:`, and `STATUS:` prefixes |
| Data Validation     | BPM: `> 0` and `< 300`; Temp: `> -50` and `< 125`              |
| Command Validation  | Whitelist: `['pause', 'resume', 'reset']`                      |
| Auto-Reconnect      | Up to 10 retries with 3-second delay                           |
| Configuration       | Environment variables via `.env` file                          |

**Key Design Decisions:**

- **No database** — data is ephemeral and streamed; CSV export handles persistence
- **Command whitelist** — prevents injection of arbitrary serial commands
- **Retry limit** — avoids infinite reconnection loops when hardware is unavailable

### 4.2 Frontend

#### 4.2.1 HTML (`index.html`)

- Semantic HTML5 structure with SEO meta tags
- Google Fonts: **Inter** (body) and **Space Mono** (data/monospace)
- Chart.js loaded from CDN with fallback to jsdelivr
- Inline SVG favicon (💓 emoji)

#### 4.2.2 CSS (`style.css`)

**Design System — CSS Custom Properties:**

| Variable   | Value     | Usage                        |
| ---------- | --------- | ---------------------------- |
| `--bg`     | `#0a0a0f` | Page background              |
| `--card`   | `#111118` | Card backgrounds             |
| `--red`    | `#ff2d55` | Primary accent (heart/alert) |
| `--green`  | `#00ff88` | Normal status                |
| `--yellow` | `#ffd60a` | Caution/paused               |
| `--blue`   | `#5ac8fa` | Secondary accent             |
| `--orange` | `#ff9500` | Temperature theme            |

**Key CSS Features:**

- Grid-based background pattern overlay using `::before` pseudo-element
- `clamp()` for responsive BPM font sizing: `clamp(5rem, 15vw, 9rem)`
- Smooth transitions on all interactive elements (0.25s–0.4s)
- Custom scrollbar styling
- Responsive breakpoints at 768px and 480px

**Animations:**

| Animation    | Duration      | Target                                 |
| ------------ | ------------- | -------------------------------------- |
| `heartbeat`  | 1s infinite   | Logo icon — mimics a real heartbeat    |
| `beat-pop`   | 0.3s          | BPM value — scales on each new reading |
| `pulse-dot`  | 1s infinite   | Status dot — blinks when active        |
| `blink`      | 1.5s infinite | Waiting message — fade in/out          |
| `temp-pulse` | 2s infinite   | Temperature icon — subtle scale pulse  |

#### 4.2.3 JavaScript (`app.js`)

**State Management:**

- `isPaused` — tracks pause/resume state
- `maxBPM`, `minBPM`, `allBPMs[]` — BPM statistical accumulators
- `maxTemp`, `minTemp`, `allTemps[]` — temperature statistical accumulators
- `sessionStart`, `sessionTimerInterval` — session tracking
- `lastAlertTime` — throttle for alert sound (5-second minimum gap)

**Chart.js Configuration:**

- Two line charts (BPM + Temperature) with 30-point sliding windows
- Dynamic Y-axis: auto-adjusts to data range
- 400ms animation duration per update
- Custom tooltip styling matching the dark theme
- Temperature chart uses orange theme (`#ff9500`)

**Audio Alert:**

- Web Audio API oscillator (880Hz sine wave, 0.4s duration)
- Gain envelope with exponential ramp-down for natural sound
- Graceful fallback on audio context errors

---

## 5. Features Detailed

### 5.1 Real-Time BPM Display

The central dashboard card shows the current BPM value in large monospace font with a "beat-pop" animation triggered on each new reading. The card border and shadow change to red with a glow effect when BPM exceeds 100.

### 5.2 Statistics Panel

Four BPM stat cards + three temperature stat cards display:

- **Status** — Color-coded health classification (Low/Normal/High/Danger)
- **Max** — Highest BPM recorded in the session
- **Min** — Lowest BPM recorded in the session
- **Average** — Running average of the last 200 readings
- **Temp Max** — Highest temperature recorded (orange-themed)
- **Temp Min** — Lowest temperature recorded (orange-themed)
- **Avg Temp** — Running average temperature (orange-themed)

### 5.3 Live Chart

A Chart.js line chart plots the last 30 BPM readings with:

- Smooth bezier curves (`tension: 0.4`)
- Fill gradient below the line
- Auto-scaling axes based on data range
- Custom tooltips in the "Space Mono" font

### 5.4 Temperature Monitoring

The DS18B20 temperature sensor provides:

- **Real-time display** — Large orange-themed temperature value with °C unit
- **Fever detection** — Alert banner when temperature > 37.5°C
- **Temperature chart** — Separate line chart (orange theme) alongside BPM chart
- **Status badges** — Low (< 36°C, blue), Normal (36–37.5°C, green), Fever (> 37.5°C, orange)

### 5.5 Session Management

- **Session Start** — Timestamp of the first BPM reading
- **Duration** — Live counter updated every second
- **Reading Count** — Total number of readings in the session
- **Reset** — Clears all data and restarts the session

### 5.6 CSV Export

Exports a UTF-8 CSV file with BOM containing:

- Index, BPM value, status label, and temperature for each reading
- Summary statistics (Min/Max/Average BPM + Min/Max/Average Temperature)
- Filename format: `vitalsync-data-YYYY-MM-DD.csv`

### 5.7 Alert System

- **BPM Alert:** Red border + glow on BPM card, alert banner with warning message
- **Fever Alert:** Orange border + glow on temperature card, fever banner
- **Audio:** 880Hz sine wave beep via Web Audio API
- **Throttle:** Minimum 5-second gap between audio alerts

### 5.8 Arduino LED/Buzzer Feedback

| BPM Range | Green LED | Blue LED | Buzzer  |
| --------- | --------- | -------- | ------- |
| < 60      | OFF       | OFF      | 1 beep  |
| 60 – 100  | ON        | OFF      | Silent  |
| > 100     | OFF       | ON       | 2 beeps |

---

## 6. Technologies Used

| Layer          | Technology                  | Version    | Purpose                               |
| -------------- | --------------------------- | ---------- | ------------------------------------- |
| Hardware       | Arduino UNO                 | ATmega328P | Microcontroller                       |
| Firmware       | Arduino C++                 | —          | Sensor reading, ISR, serial protocol  |
| Temp Sensor    | OneWire + DallasTemperature | 2.3 + 3.9  | DS18B20 temperature reading           |
| Server Runtime | Node.js                     | 18+        | Server-side JavaScript                |
| Web Framework  | Express                     | 4.18.2     | Static file serving & HTTP            |
| WebSocket      | Socket.IO                   | 4.6.1      | Real-time bidirectional communication |
| Serial         | serialport                  | 12.0.0     | USB serial port access                |
| Serial Parser  | @serialport/parser-readline | 12.0.0     | Line-based serial parsing             |
| Charting       | Chart.js                    | 4.4.1      | Data visualization (CDN)              |
| Fonts          | Google Fonts                | —          | Inter + Space Mono                    |
| Audio          | Web Audio API               | —          | Browser-native alert sounds           |

---

## 7. Testing & Validation

### 7.1 Hardware Validation

- ✅ Pulse sensor correctly reads analog signal on A0
- ✅ EMA filter eliminates high-frequency noise
- ✅ Beat detection works reliably with finger placement
- ✅ BPM values match manual pulse count (±3 BPM accuracy)
- ✅ LEDs and buzzer respond correctly to BPM ranges
- ✅ PAUSE, RESUME, and RESET commands function as expected
- ✅ No-signal timeout triggers after 2500ms of silence

### 7.2 Server Validation

- ✅ Serial data parsing correctly extracts BPM and STATUS messages
- ✅ Invalid BPM values (≤ 0 or ≥ 300) are filtered out
- ✅ Invalid control commands are rejected with console warning
- ✅ Auto-reconnect works up to 10 retries
- ✅ Multiple browser clients receive data simultaneously
- ✅ Environment variable configuration works correctly

### 7.3 Frontend Validation

- ✅ Real-time BPM display updates on each reading
- ✅ Chart renders correctly with sliding window
- ✅ Statistics (Max, Min, Average) compute accurately
- ✅ Pause/Resume toggles UI state and sends command to Arduino
- ✅ Reset clears all data and timer
- ✅ CSV export generates valid file with correct data
- ✅ Fullscreen toggle works in Chrome, Firefox, and Edge
- ✅ Responsive layout tested on 320px – 1920px viewports
- ✅ Alert banner and sound trigger at BPM > 100
- ✅ Temperature card, chart, and fever alert work correctly
- ✅ Temperature data included in CSV export

---

## 8. Challenges & Solutions

| Challenge                               | Solution                                                                     |
| --------------------------------------- | ---------------------------------------------------------------------------- |
| **Noisy pulse sensor signal**           | Applied EMA filter (α = 0.25) in the ISR for real-time smoothing             |
| **Unstable initial BPM readings**       | Skip first 3 beats after start/resume, then apply 0.7/0.3 smoothing ratio    |
| **analogRead in ISR is blocking**       | Accepted as a known trade-off; ISR runs at 2ms intervals which is sufficient |
| **Buzzer getting stuck ON**             | Moved `handleBuzzer()` before any early returns in `loop()`                  |
| **Threshold drift causing false beats** | Constrained threshold to 100–900 ADC range, enforced minimum amplitude of 10 |
| **Serial connection drops**             | Implemented auto-reconnect with retry limit (10 attempts × 3s delay)         |
| **Arbitrary serial command injection**  | Added command whitelist validation on the server                             |
| **Chart.js CDN failure**                | Added fallback to jsdelivr CDN with dynamic script injection                 |

---

## 9. Future Improvements

- [ ] Add user authentication and multi-session history with database storage
- [ ] Implement HRV (Heart Rate Variability) analysis
- [ ] Add SpO2 (blood oxygen) sensor support
- [ ] Create a mobile PWA (Progressive Web App) version
- [ ] Add MQTT support for wireless/remote monitoring
- [ ] Implement data annotation and tagging features
- [ ] Add PDF report generation with charts
- [ ] Support Bluetooth Low Energy (BLE) for wireless Arduino communication

---

## 10. Conclusion

The **VitalSync** project successfully demonstrates a complete end-to-end IoT health monitoring solution. By combining affordable hardware (Arduino + analog pulse sensor + DS18B20 temperature sensor) with modern web technologies (Node.js, Socket.IO, Chart.js), the system delivers a reliable, real-time heart rate and body temperature monitoring experience with a professional-grade user interface.

The project showcases proficiency in:

- **Embedded Systems** — Timer-based ISR, EMA signal filtering, adaptive beat detection, OneWire temperature sensing
- **Full-Stack Development** — Node.js server, WebSocket communication, responsive frontend
- **IoT Architecture** — Multi-sensor hardware-software integration via serial protocol
- **UI/UX Design** — Dark theme design system, dual-chart visualization, animations, and accessibility

The modular architecture makes it extensible for future enhancements such as database integration, mobile support, and additional biosensor inputs.

---

<p align="center">
  <em>VitalSync v1.0.0 — Smart Health Monitoring System</em><br>
  <strong>Made with 💓 by x606</strong>
</p>
