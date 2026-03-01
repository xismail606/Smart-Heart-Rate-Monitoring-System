// ===== Pulse Sensor Advanced Stable Version =====

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
const float ALPHA = 0.15;   // Lower = smoother signal, less noise
volatile float filteredSignal = 512.0;
volatile int rawSample = 512;

// ----- PulseSensor variables -----
volatile int BPM;
volatile int Signal;
volatile int IBI        = 600;
volatile boolean Pulse  = false;
volatile boolean QS     = false;
volatile bool noSignalFlag = false;
volatile int rate[10];            // 10 beats instead of 15 — faster convergence
volatile unsigned long sampleCounter = 0;
volatile unsigned long lastBeatTime  = 0;
volatile int P      = 512;
volatile int T      = 512;
volatile int thresh = 512;
volatile int amp    = 100;
volatile boolean firstBeat  = true;
volatile boolean secondBeat = false;

const int MIN_IBI = 500;          // Max BPM ~120 (blocks double-beat detection)
const int MAX_IBI = 1200;         // Min BPM ~50

// ----- Buzzer -----
unsigned long buzzerStartTime = 0;
const unsigned long buzzerDuration = 100;
int  buzzerRemaining = 0;
bool buzzerOn        = false;

// ----- BPM output -----
float bpmSmooth = 0;
bool bpmFirstReading = true;
int  beatCount = 0;               // Count beats to skip first few
const int SKIP_BEATS = 8;         // Skip first 8 beats (let algorithm stabilize)
volatile int lastIBI = 600;       // For beat-to-beat validation

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

  long sum = 0;
  int mn = 1023, mx = 0;

  for (int i = 0; i < 200; i++) {
    int v = analogRead(pulsePin);
    sum += v;
    if (v < mn) mn = v;
    if (v > mx) mx = v;
    delay(10);
  }

  int baseline = sum / 200;
  int range    = mx - mn;

  thresh         = baseline + max((int)(range / 2), (int)30);
  P              = baseline + range / 2;
  T              = baseline;
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
    // Trim trailing \r
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
      firstBeat  = true;
      secondBeat = false;
      bpmFirstReading = true;
      beatCount = 0;
      Serial.println("STATUS:RESUMED");
    }
  }

  // ---- Safe read from ISR ----
  bool localQS = false;
  int  localBPM = 0;
  bool localNoSignal = false;

  noInterrupts();
  if (QS) {
    localQS = true;
    localBPM = BPM;
    QS = false;
  }
  if (noSignalFlag) {
    localNoSignal = true;
    noSignalFlag = false;
  }
  interrupts();

  if (!paused) {

    if (localQS) {
      beatCount++;

      // Skip first few beats — they are always unreliable
      if (beatCount <= SKIP_BEATS) {
        // Don't output, just let the rate array stabilize
        return;
      }

      if (localBPM >= 40 && localBPM <= 200) {

        // ===== BPM smoothing =====
        if (bpmFirstReading) {
          bpmSmooth = localBPM;
          bpmFirstReading = false;
        } else {
          bpmSmooth = 0.3 * bpmSmooth + 0.7 * localBPM;
        }
        int bpmOut = (int)bpmSmooth;

        sendBPM(bpmOut);
      }
    }

    if (localNoSignal) {
      Serial.println("STATUS:NO_SIGNAL");
      digitalWrite(greenLED, LOW);
      digitalWrite(blueLED, LOW);
    }

    handleBuzzer();
  }
}

// ===== SEND BPM =====
void sendBPM(int currentBPM) {

  Serial.print("BPM:");
  Serial.println(currentBPM);

  if (currentBPM < 60) {
    Serial.println("STATUS:SLOW");
    digitalWrite(greenLED, LOW);
    digitalWrite(blueLED, LOW);
    triggerBuzzer(1);
  }
  else if (currentBPM <= 100) {
    Serial.println("STATUS:NORMAL");
    digitalWrite(greenLED, HIGH);
    digitalWrite(blueLED, LOW);
  }
  else {
    Serial.println("STATUS:HIGH");
    digitalWrite(greenLED, LOW);
    digitalWrite(blueLED, HIGH);
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
    buzzerOn = false;
    buzzerRemaining--;
    buzzerStartTime = now;
  }
  else if (!buzzerOn && (now - buzzerStartTime >= buzzerDuration)) {
    if (buzzerRemaining > 0) {
      digitalWrite(buzzerPin, HIGH);
      buzzerOn = true;
      buzzerStartTime = now;
    }
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
ISR(TIMER2_COMPA_vect) {

  if (paused) return;

  rawSample = analogRead(pulsePin);

  filteredSignal = ALPHA * rawSample + (1.0 - ALPHA) * filteredSignal;
  Signal = (int)filteredSignal;

  sampleCounter += 2;
  unsigned long N = sampleCounter - lastBeatTime;

  if (Signal < thresh && N > (IBI / 5) * 4)
    if (Signal < T) T = Signal;

  if (Signal > thresh && Signal > P)
    P = Signal;

  if (N > 250) {
    if ((Signal > thresh) && (Pulse == false) && (N > (IBI / 5) * 4)) {

      int newIBI = sampleCounter - lastBeatTime;
      if (newIBI < MIN_IBI || newIBI > MAX_IBI) return;

      // Beat-to-beat validation: reject if IBI changed > 40% from last
      if (lastIBI > 0) {
        int diff = abs(newIBI - lastIBI);
        if (diff > (lastIBI * 4 / 10)) {
          // Suspicious beat — accept IBI but don't flag QS yet
          lastIBI = newIBI;
          lastBeatTime = sampleCounter;
          return;
        }
      }

      Pulse = true;

      digitalWrite(blinkPin, HIGH);
      digitalWrite(yellowLED, HIGH);

      IBI = newIBI;
      lastIBI = newIBI;
      lastBeatTime = sampleCounter;

      if (secondBeat) {
        secondBeat = false;
        for (int i = 0; i <= 9; i++) rate[i] = IBI;
      }

      if (firstBeat) {
        firstBeat = false;
        secondBeat = true;
        return;
      }

      unsigned long runningTotal = 0;
      for (int i = 0; i <= 8; i++) {
        rate[i] = rate[i + 1];
        runningTotal += rate[i];
      }

      rate[9] = IBI;
      runningTotal += rate[9];
      runningTotal /= 10;
      BPM = 60000 / runningTotal;
      QS  = true;
    }
  }

  if (Signal < thresh && Pulse == true) {
    digitalWrite(blinkPin, LOW);
    digitalWrite(yellowLED, LOW);
    Pulse = false;

    amp = P - T;
    amp = constrain(amp, 15, 700);
    thresh = amp / 2 + T;
    thresh = constrain(thresh, 100, 900);

    P = thresh;
    T = thresh;
  }

  if (N > 2500) {
    thresh = 512;
    P = 512;
    T = 512;
    lastBeatTime = sampleCounter;
    firstBeat = true;
    secondBeat = false;
    filteredSignal = 512.0;
    lastIBI = 600;
    noSignalFlag = true;
  }
}