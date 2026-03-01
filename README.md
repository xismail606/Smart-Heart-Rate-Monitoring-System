<p align="center">
  <img src="https://img.shields.io/badge/💓-Pulse%20Dashboard-ff2d55?style=for-the-badge&labelColor=0a0a0f" alt="Pulse Dashboard" />
</p>

<h1 align="center">Pulse Dashboard</h1>
<p align="center"><strong>Real-Time Heart Rate Monitoring System</strong></p>
<p align="center">
  <img src="https://img.shields.io/badge/Node.js-v18+-339933?style=flat-square&logo=node.js&logoColor=white" />
  <img src="https://img.shields.io/badge/Arduino-UNO-00979D?style=flat-square&logo=arduino&logoColor=white" />
  <img src="https://img.shields.io/badge/Socket.IO-v4.6-010101?style=flat-square&logo=socket.io&logoColor=white" />
  <img src="https://img.shields.io/badge/Chart.js-v4.4-FF6384?style=flat-square&logo=chart.js&logoColor=white" />
  <img src="https://img.shields.io/badge/License-MIT-blue?style=flat-square" />
</p>

---

## 📋 Table of Contents

- [Overview](#-overview)
- [Features](#-features)
- [System Architecture](#-system-architecture)
- [Hardware Requirements](#-hardware-requirements)
- [Software Requirements](#-software-requirements)
- [Installation](#-installation)
- [Configuration](#-configuration)
- [Usage](#-usage)
- [Project Structure](#-project-structure)
- [How It Works](#-how-it-works)
- [API & Socket Events](#-api--socket-events)
- [Troubleshooting](#-troubleshooting)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🔍 Overview

**Pulse Dashboard** is a full-stack, real-time heart rate monitoring system that connects an **Arduino-based pulse sensor** to a sleek, modern **web dashboard**. It reads live BPM (beats per minute) data from a pulse sensor via serial communication, processes it through a Node.js server, and streams it to a browser using WebSocket technology.

The system is designed for health monitoring, educational demonstrations, and IoT prototyping.

---

## ✨ Features

| Feature                      | Description                                                                        |
| ---------------------------- | ---------------------------------------------------------------------------------- |
| 📊 **Real-Time BPM Display** | Large, animated BPM value with heartbeat animation                                 |
| 📈 **Live Chart**            | Last 30 readings plotted on a dynamic Chart.js line graph with auto-scaling Y-axis |
| 🔴 **High BPM Alert**        | Visual + audio alert when heart rate exceeds 100 BPM                               |
| 📉 **Statistics Panel**      | Live Max, Min, Average BPM and health status indicator                             |
| ⏱ **Session Timer**          | Tracks session start time, duration, and total reading count                       |
| ⏸ **Pause / Resume**         | Pause data collection from the dashboard — sends command to Arduino                |
| 🔄 **Reset Session**         | Full reset of all stats, chart, timer, and Arduino state                           |
| 📥 **CSV Export**            | Export all BPM data with status labels to a `.csv` file                            |
| ⛶ **Fullscreen Mode**        | Toggle fullscreen for distraction-free monitoring                                  |
| 🔊 **Sound Alert**           | Web Audio API beep on high heart rate (throttled to every 5 seconds)               |
| 🔌 **Auto-Reconnect**        | Server automatically retries serial connection up to 10 times                      |
| 📱 **Responsive Design**     | Fully responsive layout for desktop, tablet, and mobile                            |
| 🎨 **Premium Dark UI**       | Glassmorphism-inspired dark theme with neon accents                                |

---

## 🏗 System Architecture

```
┌─────────────┐    Serial (USB)    ┌─────────────┐    WebSocket    ┌─────────────┐
│   Arduino   │ ─────────────────► │  Node.js    │ ──────────────► │  Browser    │
│  + Pulse    │ ◄───────────────── │  Server     │ ◄────────────── │  Dashboard  │
│  Sensor     │   (PAUSE/RESUME/   │ (Express +  │  (Socket.IO)   │ (HTML/CSS/  │
│             │    RESET commands)  │  Socket.IO) │                │  Chart.js)  │
└─────────────┘                    └─────────────┘                └─────────────┘
     A0 pin                         Port 3000                     localhost:3000
```

**Data Flow:**

1. Arduino reads analog signal from pulse sensor on **pin A0**
2. ISR (Interrupt Service Routine) samples at 500Hz, applies EMA filtering, and detects heartbeats
3. BPM values are sent over serial as `BPM:<value>` lines
4. Node.js server parses serial data and broadcasts via Socket.IO
5. Browser receives events and updates the dashboard in real-time

---

## 🔧 Hardware Requirements

| Component           | Specification                                                 |
| ------------------- | ------------------------------------------------------------- |
| **Microcontroller** | Arduino UNO (or compatible ATmega328P board)                  |
| **Pulse Sensor**    | Analog Pulse Sensor (e.g., PulseSensor.com)                   |
| **LEDs**            | Green (pin 7), Yellow (pin 6), Blue (pin 5), Onboard (pin 13) |
| **Buzzer**          | Active buzzer on pin 2                                        |
| **USB Cable**       | Type-A to Type-B (for Arduino UNO)                            |

### Wiring Diagram

| Component           | Arduino Pin |
| ------------------- | ----------- |
| Pulse Sensor Signal | `A0`        |
| Green LED           | `D7`        |
| Yellow LED          | `D6`        |
| Blue LED            | `D5`        |
| Buzzer              | `D2`        |
| Onboard LED         | `D13`       |

---

## 💻 Software Requirements

- **Node.js** v18 or higher
- **npm** (comes with Node.js)
- **Arduino IDE** 1.8+ or 2.x
- **Web Browser** (Chrome, Firefox, Edge — any modern browser)
- **Windows / macOS / Linux** (driver support for USB-to-Serial)

---

## 🚀 Installation

### 1. Clone the Repository

```bash
git clone https://github.com/xismail606/Smart-Heart-Rate-Monitoring-System.git
cd pulse-dashboard
```

### 2. Install Dependencies

```bash
npm install
```

### 3. Upload Arduino Code

1. Open `Arduino/pulse.ino` in the Arduino IDE
2. Select **Board → Arduino UNO**
3. Select the correct **COM Port**
4. Click **Upload**

### 4. Configure Environment

Create or edit the `.env` file in the project root:

```env
COM_PORT=COM5
BAUD_RATE=115200
SERVER_PORT=3000
```

> **Note:** On Linux/macOS, the port may look like `/dev/ttyUSB0` or `/dev/tty.usbmodem*`

### 5. Start the Server

```bash
npm start
```

### 6. Open the Dashboard

Navigate to:

```
http://localhost:3000
```

---

## ⚙ Configuration

All configuration is managed through the `.env` file:

| Variable      | Default  | Description                                     |
| ------------- | -------- | ----------------------------------------------- |
| `COM_PORT`    | `COM5`   | Serial port where Arduino is connected          |
| `BAUD_RATE`   | `115200` | Serial communication speed (must match Arduino) |
| `SERVER_PORT` | `3000`   | HTTP server port for the web dashboard          |

---

## 📖 Usage

### Dashboard Controls

| Button           | Action                                                  |
| ---------------- | ------------------------------------------------------- |
| ⏸ **Pause**      | Pauses data collection; Arduino stops sending BPM       |
| ▶ **Resume**     | Resumes data collection from Arduino                    |
| 🔄 **Reset**     | Clears all data, resets chart, timer, and Arduino state |
| 📥 **Export**    | Downloads all session data as a CSV file                |
| ⛶ **Fullscreen** | Toggles fullscreen mode                                 |

### Status Indicators

| Status        | Meaning                      |
| ------------- | ---------------------------- |
| 🟢 Green dot  | Connected and receiving data |
| 🟡 Yellow dot | Paused or no signal          |
| ⚪ Grey dot   | Disconnected                 |

### Health Status Levels

| BPM Range | Status    | Color          |
| --------- | --------- | -------------- |
| < 60      | Low       | 🟡 Yellow      |
| 60 – 100  | Normal ✓  | 🟢 Green       |
| 101 – 140 | High ⚠    | 🔴 Red         |
| > 140     | Danger 🚨 | 🔴 Red + Alert |

---

## 📁 Project Structure

```
pulse-dashboard/
├── Arduino/
│   └── pulse.ino          # Arduino firmware (sensor reading, ISR, serial protocol)
├── public/
│   ├── index.html         # Main HTML page (dashboard layout)
│   ├── style.css          # Design system (dark theme, animations, responsive)
│   └── app.js             # Client-side logic (Socket.IO, Chart.js, UI controls)
├── server.js              # Node.js server (Express, Socket.IO, SerialPort bridge)
├── Package.json           # Node.js dependencies and scripts
├── .env                   # Environment variables (COM port, baud rate, server port)
└── README.md              # This file
```

---

## ⚙ How It Works

### Arduino (`pulse.ino`)

1. **Calibration** — On startup, reads 100 samples to establish a baseline signal level
2. **ISR (Timer2)** — Fires every 2ms (500Hz) to sample the analog pulse signal
3. **EMA Filter** — Applies Exponential Moving Average (α = 0.25) to smooth the raw signal
4. **Beat Detection** — Detects heartbeats using adaptive threshold crossing with peak/trough tracking
5. **BPM Calculation** — Averages the last 10 inter-beat intervals (IBI) to compute stable BPM
6. **Serial Output** — Sends `BPM:<value>` and `STATUS:<state>` messages over serial
7. **LED Indicators** — Green (normal), Blue (high), Yellow (beat flash), Buzzer (alerts)
8. **Command Handling** — Receives `PAUSE`, `RESUME`, `RESET` commands from the serial port

### Server (`server.js`)

1. **Express** serves static files from `public/`
2. **SerialPort** reads and parses data from Arduino (line-based protocol)
3. **Socket.IO** broadcasts BPM data and status to all connected browsers
4. **Command Relay** — Receives control commands from the browser and writes them to Arduino
5. **Auto-Reconnect** — Retries serial connection up to 10 times with a 3-second delay

### Frontend (`app.js` + `index.html` + `style.css`)

1. **Socket.IO Client** — Connects to the server and listens for `bpm`, `serial-status`, and `arduino-status` events
2. **Chart.js** — Renders a real-time line chart with the last 30 BPM readings
3. **Statistics Engine** — Computes max, min, and running average BPM
4. **Session Management** — Tracks session timer, reading count, and export data
5. **Alert System** — Visual (banner + card glow) and audio (Web Audio API beep) alerts for high BPM
6. **Controls** — Pause/Resume, Reset, Export CSV, and Fullscreen toggle

---

## 🔌 API & Socket Events

### Server → Client Events

| Event            | Payload                                  | Description                                                   |
| ---------------- | ---------------------------------------- | ------------------------------------------------------------- |
| `bpm`            | `{ bpm: number, time: number }`          | New BPM reading from Arduino                                  |
| `serial-status`  | `{ connected: boolean, error?: string }` | Serial port connection status                                 |
| `arduino-status` | `{ status: string, time: number }`       | Arduino status messages (CALIBRATING, READY, NO_SIGNAL, etc.) |

### Client → Server Events

| Event     | Payload                            | Description                        |
| --------- | ---------------------------------- | ---------------------------------- |
| `control` | `"pause"` / `"resume"` / `"reset"` | Send command to Arduino via serial |

### Serial Protocol (Arduino ↔ Server)

| Direction        | Format           | Example        |
| ---------------- | ---------------- | -------------- |
| Arduino → Server | `BPM:<value>`    | `BPM:72`       |
| Arduino → Server | `STATUS:<state>` | `STATUS:READY` |
| Server → Arduino | `<COMMAND>\n`    | `PAUSE\n`      |

---

## 🔧 Troubleshooting

| Problem                   | Solution                                                                               |
| ------------------------- | -------------------------------------------------------------------------------------- |
| **Serial port not found** | Check `COM_PORT` in `.env`. Run `ls /dev/tty*` (Linux/Mac) or Device Manager (Windows) |
| **Max retries reached**   | Ensure Arduino is connected and the correct port is set. Restart the server            |
| **No BPM readings**       | Place finger firmly on the sensor. Wait for calibration to complete                    |
| **Chart not showing**     | Ensure Chart.js CDN is reachable. Check browser console for errors                     |
| **Baud rate mismatch**    | Ensure both `.env` and `pulse.ino` use `115200`                                        |
| **Dashboard not loading** | Verify server is running on the correct port. Check `http://localhost:3000`            |

---

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Commit your changes: `git commit -m "Add my feature"`
4. Push to the branch: `git push origin feature/my-feature`
5. Open a Pull Request

---

## 📄 License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.

---

<p align="center">
  Made with 💓 by <strong>x606</strong>
</p>
