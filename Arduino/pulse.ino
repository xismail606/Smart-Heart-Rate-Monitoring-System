// ===== Pulse Sensor — Advanced Stable Version (Fixed) =====

// ----- Pins -----
const int pulsePin  = A0;
const int blinkPin  = 13;
const int greenLED  = 7;
const int yellowLED = 6;
const int blueLED   = 5;
const int buzzerPin = 2;

// ----- Pause -----
volatile boolean paused = false;

// ----- EMA filter -----
const float ALPHA = 0.25;
volatile float filteredSignal = 512.0;

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
volatile int thresh = 525;
volatile int amp    = 100;
volatile boolean firstBeat  = true;
volatile boolean secondBeat = false;

const int MIN_IBI = 300;
const int MAX_IBI = 1500;

// ----- Buzzer -----
unsigned long buzzerStartTime = 0;
const unsigned long buzzerDuration = 100;
int  buzzerRemaining = 0;
bool buzzerOn        = false;

// ----- BPM output -----
float bpmSmooth = 0;
bool bpmFirstReading = true;
int  beatCount = 0;
const int SKIP_BEATS = 3;

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

  Serial.println("STATUS:CALIBRATING");

  // Calibration: read baseline
  long sum = 0;
  for (int i = 0; i < 100; i++) {
    sum += analogRead(pulsePin);
    delay(10);
  }
  int baseline = sum / 100;
  filteredSignal = baseline;

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
      filteredSignal  = 512.0f;
      thresh          = 525;
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
      bpmSmooth       = 0;
      BPM             = 0;
      IBI             = 600;
      amp             = 100;
      filteredSignal  = 512.0f;
      thresh          = 525;
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

      Serial.println("STATUS:RESET");
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

    // FIX: handleBuzzer() moved BEFORE any early return
    // so the buzzer is never stuck ON
    handleBuzzer();

    if (localQS) {
      beatCount++;

      // Skip only first few unstable beats
      if (beatCount <= SKIP_BEATS) {
        // NOTE: no return here — handleBuzzer() already called above
      }
      else if (localBPM >= 30 && localBPM <= 200) {

        // ===== BPM smoothing =====
        // FIX: changed ratio from 0.5/0.5 to 0.7/0.3 for more stable display
        if (bpmFirstReading) {
          bpmSmooth       = localBPM;
          bpmFirstReading = false;
        } else {
          bpmSmooth = 0.7f * bpmSmooth + 0.3f * localBPM;
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

// ===== ISR =====
// NOTE: analogRead (~104µs) and float math are expensive inside an ISR on AVR.
// This is a known trade-off for this sensor library approach.
// If timing issues appear, consider moving filtering to loop() and using
// a simple flag + raw sample buffer instead.
ISR(TIMER2_COMPA_vect) {

  if (paused) return;

  // EMA-filtered signal
  int rawSample  = analogRead(pulsePin);
  filteredSignal = ALPHA * rawSample + (1.0f - ALPHA) * filteredSignal;
  Signal         = (int)filteredSignal;

  sampleCounter += 2;
  unsigned long N = sampleCounter - lastBeatTime;

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

      int newIBI = sampleCounter - lastBeatTime;

      if (newIBI < MIN_IBI || newIBI > MAX_IBI) return;

      Pulse = true;

      digitalWrite(blinkPin,  HIGH);
      digitalWrite(yellowLED, HIGH);

      IBI          = newIBI;
      lastBeatTime = sampleCounter;

      if (secondBeat) {
        secondBeat = false;
        for (int i = 0; i <= 9; i++) rate[i] = IBI;
      }

      if (firstBeat) {
        firstBeat  = false;
        secondBeat = true;
        return;
      }

      unsigned long runningTotal = 0;
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

    amp = P - T;

    // FIX: enforce minimum amp to prevent bad thresh calculation
    if (amp < 10) amp = 10;

    // FIX: constrain thresh to valid ADC range
    thresh = constrain(amp / 2 + T, 100, 900);

    P = thresh;
    T = thresh;
  }

  // ---- No signal timeout ----
  if (N > 2500) {
    thresh         = 525;
    P              = 512;
    T              = 512;
    lastBeatTime   = sampleCounter;
    firstBeat      = true;
    secondBeat     = false;
    filteredSignal = 512.0f;
    noSignalFlag   = true;
  }
}
