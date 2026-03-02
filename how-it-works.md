# How It Works

## 1. Hardware Setup

1. Connect the **Pulse Sensor** signal wire to **pin A0** on the Arduino UNO.
2. Connect the **Green LED** to **pin D7**, **Yellow LED** to **pin D6**, **Blue LED** to **pin D5**, and the **Buzzer** to **pin D2** (each through a 220Ω resistor).
3. Connect the Arduino to your PC via **USB cable**.

## 2. Upload Arduino Code

1. Open `Arduino/pulse.ino` in the **Arduino IDE**.
2. Select **Board: Arduino UNO** and the correct **COM port**.
3. Click **Upload**.

## 3. Install Dependencies

```bash
npm install
```

## 4. Configure COM Port

Open the `.env` file and set your Arduino's COM port:

```
COM_PORT=COM5
```

## 5. Run the Server

```bash
node server.js
```

## 6. Open the Dashboard

Open your browser and go to:

```
http://localhost:3000
```

## 7. Start Monitoring

1. Place your **finger** on the pulse sensor.
2. Wait a few seconds for **calibration**.
3. The dashboard will start showing your **live heart rate (BPM)**, chart, and statistics.

## 8. Controls

| Button           | Action                                   |
| ---------------- | ---------------------------------------- |
| **⏸ Pause**      | Pauses data collection                   |
| **▶ Resume**     | Resumes data collection                  |
| **🔄 Reset**     | Clears all data and restarts the session |
| **⛶ Fullscreen** | Toggles fullscreen mode                  |
| **📥 Export**    | Downloads session data as a CSV file     |
