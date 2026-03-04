  // ===== Libraries =====
  #include <OneWire.h>
  #include <DallasTemperature.h>

  // ===== Pulse Sensor =====

  // ----- Pins -----
  const int pulsePin  = A0;
  const int blinkPin  = 13;
  const int greenLED  = 7;
  const int yellowLED = 6;
  const int blueLED   = 5;
  const int buzzerPin = 2;

  // ===== DS18B20 Temperature Sensor =====
  const int DS18B20_PIN = 4;
  OneWire oneWire(DS18B20_PIN);
  DallasTemperature tempSensor(&oneWire);

  // ----- Temperature variables -----
  float currentTemp       = 0.0;
  unsigned long lastTempRead = 0;
  const unsigned long TEMP_INTERVAL = 2000;  // read every 2 seconds
  bool tempSensorFound    = false;

  // ----- Pause -----
  volatile boolean paused = false;

  // ----- PulseSensor variables -----
  volatile int BPM;
  volatile int Signal;
  volatile int IBI        = 600;
  volatile boolean Pulse  = false;
  volatile boolean QS     = false;
  volatile bool noSignalFlag = false;
  volatile int rate[10];
  volatile unsigned long sampleCounter = 0;
  volatile unsigned long lastBeatTime  = 0;
  volatile int P      = 512;
  volatile int T      = 512;
  volatile int thresh = 512;
  volatile int amp    = 100;
  volatile boolean firstBeat  = true;
  volatile boolean secondBeat = false;



  // ----- Buzzer -----
  unsigned long buzzerStartTime = 0;
  const unsigned long buzzerDuration = 100;
  int  buzzerRemaining = 0;
  bool buzzerOn        = false;

  // ----- BPM output -----
  float bpmSmooth = 0;
  bool bpmFirstReading = true;
  int  beatCount = 0;
  const int SKIP_BEATS = 5;
  int  validReadings = 0;           // counts accepted readings after SKIP_BEATS

  // ===== SETUP =====
  void setup() {
    Serial.begin(115200);
    Serial.setTimeout(100);

    pinMode(blinkPin,  OUTPUT);
    pinMode(greenLED,  OUTPUT);
    pinMode(yellowLED, OUTPUT);
    pinMode(blueLED,   OUTPUT);
    pinMode(buzzerPin, OUTPUT);

    digitalWrite(greenLED,  LOW);
    digitalWrite(yellowLED, LOW);
    digitalWrite(blueLED,   LOW);
    digitalWrite(buzzerPin, LOW);

    analogReference(DEFAULT);

    // ---- DS18B20 Init ----
    tempSensor.begin();
    tempSensorFound = (tempSensor.getDeviceCount() > 0);
    if (tempSensorFound) {
      tempSensor.setResolution(12);
      tempSensor.setWaitForConversion(false);  // non-blocking
    }

    Serial.println("STATUS:CALIBRATING");

    // Calibration: read baseline
    long sum = 0;
    for (int i = 0; i < 100; i++) {
      sum += analogRead(pulsePin);
      delay(10);
    }

    interruptSetup();
    Serial.println("STATUS:READY");
  }

  // ===== LOOP =====
  void loop() {

    // ---- Serial Commands ----
    if (Serial.available() > 0) {
      char cmdBuf[16];
      int len = Serial.readBytesUntil('\n', cmdBuf, sizeof(cmdBuf) - 1);
      cmdBuf[len] = '\0';
      if (len > 0 && cmdBuf[len - 1] == '\r') cmdBuf[len - 1] = '\0';

      if (strcmp(cmdBuf, "PAUSE") == 0) {
        paused = true;
        digitalWrite(greenLED, LOW);
        digitalWrite(yellowLED, LOW);
        digitalWrite(blueLED, LOW);
        digitalWrite(buzzerPin, LOW);
        Serial.println("STATUS:PAUSED");
      }
      else if (strcmp(cmdBuf, "RESUME") == 0) {
        paused = false;
        firstBeat       = true;
        secondBeat      = false;
        bpmFirstReading = true;
        beatCount       = 0;
        validReadings   = 0;
        thresh          = 512;
        P               = 512;
        T               = 512;
        Serial.println("STATUS:RESUMED");
      }

      else if (strcmp(cmdBuf, "RESET") == 0) {
        // Full reset: stop, clear all state, restart
        paused          = false;
        firstBeat       = true;
        secondBeat      = false;
        Pulse           = false;
        QS              = false;
        noSignalFlag    = false;
        bpmFirstReading = true;
        beatCount       = 0;
        validReadings   = 0;
        bpmSmooth       = 0;
        BPM             = 0;
        IBI             = 600;
        amp             = 100;
        thresh          = 512;
        P               = 512;
        T               = 512;
        sampleCounter   = 0;
        lastBeatTime    = 0;
        for (int i = 0; i < 10; i++) rate[i] = 0;

        digitalWrite(greenLED,  LOW);
        digitalWrite(yellowLED, LOW);
        digitalWrite(blueLED,   LOW);
        digitalWrite(buzzerPin, LOW);
        buzzerRemaining = 0;
        buzzerOn        = false;

        // Reset temperature
        currentTemp = 0.0;
        lastTempRead = 0;

        Serial.println("STATUS:RESET");
      }
    }

    // ---- DS18B20 Temperature Reading (every 2s) ----
    if (tempSensorFound && !paused) {
      unsigned long now = millis();
      if (now - lastTempRead >= TEMP_INTERVAL) {
        lastTempRead = now;
        tempSensor.requestTemperatures();
        float t = tempSensor.getTempCByIndex(0);
        if (t != DEVICE_DISCONNECTED_C && t > -50.0 && t < 125.0) {
          currentTemp = t;
          Serial.print("TEMP:");
          Serial.println(currentTemp, 1);
        }
      }
    }

    // ---- Safe read from ISR ----
    bool localQS       = false;
    int  localBPM      = 0;
    bool localNoSignal = false;

    noInterrupts();
    if (QS) {
      localQS  = true;
      localBPM = BPM;
      QS       = false;
    }
    if (noSignalFlag) {
      localNoSignal = true;
      noSignalFlag  = false;
    }
    interrupts();

    if (!paused) {

      //  handleBuzzer() moved BEFORE any early return
      // so the buzzer is never stuck ON
      handleBuzzer();

      if (localQS) {
        beatCount++;

        // Skip first few unstable beats (warm-up period)
        if (beatCount <= SKIP_BEATS) {
          // NOTE: no return here — handleBuzzer() already called above
        }
        else if (localBPM >= 30 && localBPM <= 200) {
          validReadings++;

          // ===== Clamp early readings =====
          // First 3 valid readings: cap at 120 BPM to prevent startup spikes
          int safeBPM = localBPM;
          if (validReadings <= 3 && safeBPM > 120) {
            safeBPM = 120;
          }

          // ===== BPM smoothing with outlier rejection =====
          if (bpmFirstReading) {
            bpmSmooth       = safeBPM;
            bpmFirstReading = false;
          } else {
            // Outlier rejection: if new BPM deviates too much, reduce its weight
            int diff = abs(safeBPM - (int)bpmSmooth);
            float weight;
            if (validReadings <= 5) {
              weight = 0.15f;            // warm-up: very slow adaptation
            } else if (diff > 30) {
              weight = 0.05f;            // outlier: almost ignore it
            } else if (diff > 15) {
              weight = 0.15f;            // moderate change: cautious
            } else {
              weight = 0.3f;             // normal change: standard weight
            }
            bpmSmooth = (1.0f - weight) * bpmSmooth + weight * safeBPM;
          }
          int bpmOut = (int)(bpmSmooth + 0.5f);  // round instead of truncate

          sendBPM(bpmOut);
        }
      }

      if (localNoSignal) {
        Serial.println("STATUS:NO_SIGNAL");
        digitalWrite(greenLED, LOW);
        digitalWrite(blueLED,  LOW);
      }
    }
  }

  // ===== SEND BPM =====
  void sendBPM(int currentBPM) {

    Serial.print("BPM:");
    Serial.println(currentBPM);

    if (currentBPM < 60) {
      Serial.println("STATUS:SLOW");
      digitalWrite(greenLED, LOW);
      digitalWrite(blueLED,  LOW);
      triggerBuzzer(1);
    }
    else if (currentBPM <= 100) {
      Serial.println("STATUS:NORMAL");
      digitalWrite(greenLED, HIGH);
      digitalWrite(blueLED,  LOW);
    }
    else {
      Serial.println("STATUS:HIGH");
      digitalWrite(greenLED, LOW);
      digitalWrite(blueLED,  HIGH);
      triggerBuzzer(2);
    }
  }

  // ===== BUZZER =====
  void triggerBuzzer(int times) {
    if (buzzerRemaining == 0 && times > 0) {
      buzzerRemaining = times;
      buzzerOn        = true;
      digitalWrite(buzzerPin, HIGH);
      buzzerStartTime = millis();
    }
  }

  void handleBuzzer() {
    if (buzzerRemaining <= 0) return;

    unsigned long now = millis();

    if (buzzerOn && (now - buzzerStartTime >= buzzerDuration)) {
      digitalWrite(buzzerPin, LOW);
      buzzerOn        = false;
      buzzerRemaining--;
      buzzerStartTime = now;
    }
    else if (!buzzerOn && buzzerRemaining > 0 && (now - buzzerStartTime >= buzzerDuration)) {
      digitalWrite(buzzerPin, HIGH);
      buzzerOn        = true;
      buzzerStartTime = now;
    }
  }

  // ===== TIMER SETUP =====
  void interruptSetup() {
    TCCR2A = 0x02;
    TCCR2B = 0x06;
    OCR2A  = 0x7C;
    TIMSK2 = 0x02;
  }

  // ===== ISR (matches original working PulseSensor code) =====
  ISR(TIMER2_COMPA_vect) {

    if (paused) return;

    // Read raw signal directly (no filter — works best with this sensor)
    Signal = analogRead(pulsePin);

    sampleCounter += 2;
    int N = sampleCounter - lastBeatTime;

    // ---- Find trough T ----
    if (Signal < thresh && N > (IBI / 5) * 3) {
      if (Signal < T) T = Signal;
    }

    // ---- Find peak P ----
    if (Signal > thresh && Signal > P) {
      P = Signal;
    }

    // ---- Beat detection ----
    if (N > 250) {
      if ((Signal > thresh) && (Pulse == false) && (N > (IBI / 5) * 3)) {

        Pulse = true;

        digitalWrite(blinkPin,  HIGH);
        digitalWrite(yellowLED, HIGH);

        IBI          = sampleCounter - lastBeatTime;
        lastBeatTime = sampleCounter;

        if (secondBeat) {
          secondBeat = false;
          for (int i = 0; i <= 9; i++) rate[i] = IBI;
        }

        if (firstBeat) {
          firstBeat  = false;
          secondBeat = true;
          sei();
          return;
        }

        word runningTotal = 0;
        for (int i = 0; i <= 8; i++) {
          rate[i]       = rate[i + 1];
          runningTotal += rate[i];
        }
        rate[9]       = IBI;
        runningTotal += rate[9];
        runningTotal /= 10;
        BPM = 60000 / runningTotal;
        QS  = true;
      }
    }

    // ---- Reset after beat ----
    if (Signal < thresh && Pulse == true) {
      digitalWrite(blinkPin,  LOW);
      digitalWrite(yellowLED, LOW);
      Pulse = false;

      amp    = P - T;
      thresh = amp / 2 + T;

      P = thresh;
      T = thresh;
    }

    // ---- No signal timeout ----
    if (N > 2500) {
      thresh       = 512;
      P            = 512;
      T            = 512;
      lastBeatTime = sampleCounter;
      firstBeat    = true;
      secondBeat   = false;
      noSignalFlag = true;
    }
  }
