# 📋 Pulse Dashboard — Project Report

> **Project:** Smart Heart Rate Monitoring System  
> **Version:** 1.0.0  
> **Date:** March 2026  
> **Author:** x606

---

## 1. Introduction

### 1.1 Project Overview

The **Pulse Dashboard** is a complete IoT-based heart rate monitoring system that bridges the gap between hardware sensing and modern web visualization. The system uses an **Arduino UNO** microcontroller with an analog pulse sensor to capture real-time heart rate data, which is then streamed to a beautifully designed web dashboard through a **Node.js** server.

### 1.2 Objectives

- Build a reliable, real-time heart rate monitoring system using affordable hardware
- Create a modern, responsive web interface for data visualization
- Implement bi-directional communication between hardware and software
- Provide health status classification and alert mechanisms
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
│   HARDWARE LAYER   │    Arduino UNO + Pulse Sensor + LEDs + Buzzer
│   (Data Capture)   │    Samples at 500Hz using Timer2 ISR
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
│   (Visualization)  │    Real-time dashboard with controls
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
| 3   | Capacitor (220 nF)       | 1        | Reduces noise and smooths the sensor signal                    |
| 4   | Resistors (220Ω or 330Ω) | 3        | Limit the current flowing through LEDs and protect the circuit |
| 5   | Green LED                | 1        | Normal heart rate indicator (pin D7)                           |
| 6   | Yellow LED               | 1        | Beat flash indicator (pin D6)                                  |
| 7   | Blue LED                 | 1        | High heart rate indicator (pin D5)                             |
| 8   | Active Buzzer            | 1        | Audio alarm for abnormal BPM (pin D2)                          |
| 9   | USB Cable (Type-A to B)  | 1        | Serial communication & power                                   |

### 3.2 Pin Configuration

| Arduino Pin | Connected To        | Mode           |
| ----------- | ------------------- | -------------- |
| A0          | Pulse Sensor Signal | Analog Input   |
| D2          | Active Buzzer       | Digital Output |
| D5          | Blue LED            | Digital Output |
| D6          | Yellow LED          | Digital Output |
| D7          | Green LED           | Digital Output |
| D13         | Onboard LED         | Digital Output |

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

| Feature             | Implementation                                                |
| ------------------- | ------------------------------------------------------------- |
| Static File Serving | Express serves `public/` directory                            |
| Serial Connection   | `SerialPort` library with `ReadlineParser` (`\r\n` delimiter) |
| Data Parsing        | Regex-based parsing of `BPM:` and `STATUS:` prefixes          |
| Data Validation     | BPM range check: `> 0` and `< 300`                            |
| Command Validation  | Whitelist: `['pause', 'resume', 'reset']`                     |
| Auto-Reconnect      | Up to 10 retries with 3-second delay                          |
| Configuration       | Environment variables via `.env` file                         |

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

**Key CSS Features:**

- Grid-based background pattern overlay using `::before` pseudo-element
- `clamp()` for responsive BPM font sizing: `clamp(5rem, 15vw, 9rem)`
- Smooth transitions on all interactive elements (0.25s–0.4s)
- Custom scrollbar styling
- Responsive breakpoints at 768px and 480px

**Animations:**

| Animation   | Duration      | Target                                 |
| ----------- | ------------- | -------------------------------------- |
| `heartbeat` | 1s infinite   | Logo icon — mimics a real heartbeat    |
| `beat-pop`  | 0.3s          | BPM value — scales on each new reading |
| `pulse-dot` | 1s infinite   | Status dot — blinks when active        |
| `blink`     | 1.5s infinite | Waiting message — fade in/out          |

#### 4.2.3 JavaScript (`app.js`)

**State Management:**

- `isPaused` — tracks pause/resume state
- `maxBPM`, `minBPM`, `allBPMs[]` — statistical accumulators
- `sessionStart`, `sessionTimerInterval` — session tracking
- `lastAlertTime` — throttle for alert sound (5-second minimum gap)

**Chart.js Configuration:**

- Line chart with 30-point sliding window
- Dynamic Y-axis: `suggestedMin = min - 15`, `suggestedMax = max + 15`
- 400ms animation duration per update
- Custom tooltip styling matching the dark theme

**Audio Alert:**

- Web Audio API oscillator (880Hz sine wave, 0.4s duration)
- Gain envelope with exponential ramp-down for natural sound
- Graceful fallback on audio context errors

---

## 5. Features Detailed

### 5.1 Real-Time BPM Display

The central dashboard card shows the current BPM value in large monospace font with a "beat-pop" animation triggered on each new reading. The card border and shadow change to red with a glow effect when BPM exceeds 100.

### 5.2 Statistics Panel

Four stat cards display:

- **Status** — Color-coded health classification (Low/Normal/High/Danger)
- **Max** — Highest BPM recorded in the session
- **Min** — Lowest BPM recorded in the session
- **Average** — Running average of the last 200 readings

### 5.3 Live Chart

A Chart.js line chart plots the last 30 BPM readings with:

- Smooth bezier curves (`tension: 0.4`)
- Fill gradient below the line
- Auto-scaling axes based on data range
- Custom tooltips in the "Space Mono" font

### 5.4 Session Management

- **Session Start** — Timestamp of the first BPM reading
- **Duration** — Live counter updated every second
- **Reading Count** — Total number of readings in the session
- **Reset** — Clears all data and restarts the session

### 5.5 CSV Export

Exports a UTF-8 CSV file with BOM containing:

- Index, BPM value, and status label for each reading
- Summary statistics (Min, Max, Average, Total Readings)
- Filename format: `pulse-data-YYYY-MM-DD.csv`

### 5.6 Alert System

- **Visual:** Red border + glow on BPM card, alert banner with warning message
- **Audio:** 880Hz sine wave beep via Web Audio API
- **Throttle:** Minimum 5-second gap between audio alerts

### 5.7 Arduino LED/Buzzer Feedback

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

The **Pulse Dashboard** project successfully demonstrates a complete end-to-end IoT health monitoring solution. By combining affordable hardware (Arduino + analog pulse sensor) with modern web technologies (Node.js, Socket.IO, Chart.js), the system delivers a reliable, real-time heart rate monitoring experience with a professional-grade user interface.

The project showcases proficiency in:

- **Embedded Systems** — Timer-based ISR, EMA signal filtering, adaptive beat detection
- **Full-Stack Development** — Node.js server, WebSocket communication, responsive frontend
- **IoT Architecture** — Hardware-software integration via serial protocol
- **UI/UX Design** — Dark theme design system, animations, and accessibility

The modular architecture makes it extensible for future enhancements such as database integration, mobile support, and additional biosensor inputs.

---

<p align="center">
  <em>Pulse Dashboard v1.0.0 — Smart Heart Rate Monitoring System</em><br>
  <strong>Made with 💓 by x606</strong>
</p>
