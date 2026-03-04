/* ══════════════════════════════════════════════
   Pulse Dashboard — Application Logic
   ══════════════════════════════════════════════ */

const socket = io();

// ── Chart Setup ──
const MAX_POINTS = 30;
const labels = [],
  dataPoints = [];
const ctx = document.getElementById("bpmChart").getContext("2d");

const chart = new Chart(ctx, {
  type: "line",
  data: {
    labels,
    datasets: [
      {
        label: "BPM",
        data: dataPoints,
        borderColor: "#ff2d55",
        backgroundColor: "rgba(255,45,85,0.08)",
        borderWidth: 2,
        pointRadius: 3,
        pointBackgroundColor: "#ff2d55",
        pointHoverRadius: 5,
        pointHoverBackgroundColor: "#fff",
        tension: 0.4,
        fill: true,
      },
    ],
  },
  options: {
    responsive: true,
    maintainAspectRatio: false,
    animation: { duration: 400 },
    plugins: {
      legend: { display: false },
      tooltip: {
        backgroundColor: "#111118",
        borderColor: "#ff2d55",
        borderWidth: 1,
        titleColor: "#ff2d55",
        bodyColor: "#e8e8f0",
        titleFont: { family: "Space Mono" },
        bodyFont: { family: "Space Mono" },
        displayColors: false,
        callbacks: {
          label: (ctx) => `${ctx.parsed.y} BPM`,
        },
      },
    },
    scales: {
      x: { display: false },
      y: {
        grid: { color: "rgba(255,255,255,0.04)" },
        ticks: { color: "#5a5a7a", font: { family: "Space Mono", size: 11 } },
        suggestedMin: 50,
        suggestedMax: 120,
      },
    },
  },
});

// ── Temperature Chart Setup ──
const tempLabels = [],
  tempDataPoints = [];
const tempCtx = document.getElementById("tempChart").getContext("2d");

const tempChart = new Chart(tempCtx, {
  type: "line",
  data: {
    labels: tempLabels,
    datasets: [
      {
        label: "Temp °C",
        data: tempDataPoints,
        borderColor: "#ff9500",
        backgroundColor: "rgba(255,149,0,0.08)",
        borderWidth: 2,
        pointRadius: 3,
        pointBackgroundColor: "#ff9500",
        pointHoverRadius: 5,
        pointHoverBackgroundColor: "#fff",
        tension: 0.4,
        fill: true,
      },
    ],
  },
  options: {
    responsive: true,
    maintainAspectRatio: false,
    animation: { duration: 400 },
    plugins: {
      legend: { display: false },
      tooltip: {
        backgroundColor: "#111118",
        borderColor: "#ff9500",
        borderWidth: 1,
        titleColor: "#ff9500",
        bodyColor: "#e8e8f0",
        titleFont: { family: "Space Mono" },
        bodyFont: { family: "Space Mono" },
        displayColors: false,
        callbacks: {
          label: (ctx) => `${ctx.parsed.y}°C`,
        },
      },
    },
    scales: {
      x: { display: false },
      y: {
        grid: { color: "rgba(255,255,255,0.04)" },
        ticks: {
          color: "#5a5a7a",
          font: { family: "Space Mono", size: 11 },
          callback: (v) => v + "°",
        },
        suggestedMin: 30,
        suggestedMax: 42,
      },
    },
  },
});

// ── State ──
let isPaused = false;
let maxBPM = 0,
  minBPM = Infinity;
let allBPMs = [];
let hasData = false,
  lastBPM = null;
let sessionStart = null;
let sessionTimerInterval = null;
let alertSoundEnabled = true;

// ── Temperature State ──
let lastTemp = null;
let maxTemp = -Infinity,
  minTemp = Infinity;
let allTemps = [];

// ── Audio Context for Alert Sound ──
let audioCtx = null;
function playAlertBeep() {
  if (!alertSoundEnabled) return;
  try {
    if (!audioCtx)
      audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    const osc = audioCtx.createOscillator();
    const gain = audioCtx.createGain();
    osc.connect(gain);
    gain.connect(audioCtx.destination);
    osc.frequency.value = 880;
    osc.type = "sine";
    gain.gain.setValueAtTime(0.15, audioCtx.currentTime);
    gain.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + 0.4);
    osc.start(audioCtx.currentTime);
    osc.stop(audioCtx.currentTime + 0.4);
  } catch (e) {
    /* ignore audio errors */
  }
}

// ── Session Timer ──
function startSessionTimer() {
  sessionStart = Date.now();
  document.getElementById("sessionStart").textContent = formatTime(
    new Date(sessionStart),
  );

  sessionTimerInterval = setInterval(() => {
    const elapsed = Date.now() - sessionStart;
    const mins = Math.floor(elapsed / 60000);
    const secs = Math.floor((elapsed % 60000) / 1000);
    document.getElementById("sessionDuration").textContent =
      `${String(mins).padStart(2, "0")}:${String(secs).padStart(2, "0")}`;
  }, 1000);
}

function formatTime(d) {
  return `${String(d.getHours()).padStart(2, "0")}:${String(d.getMinutes()).padStart(2, "0")}:${String(d.getSeconds()).padStart(2, "0")}`;
}

// ── Pause / Resume ──
function togglePause() {
  isPaused = !isPaused;
  socket.emit("control", isPaused ? "pause" : "resume");

  const btn = document.getElementById("pauseBtn");
  const dot = document.getElementById("statusDot");
  const badge = document.getElementById("pausedBadge");
  const card = document.getElementById("bpmCard");
  const val = document.getElementById("bpmValue");

  if (isPaused) {
    btn.classList.add("paused");
    document.getElementById("pauseIcon").textContent = "▶";
    document.getElementById("pauseText").textContent = "Resume";
    dot.classList.replace("active", "paused");
    badge.classList.add("visible");
    card.classList.add("paused");
    val.classList.add("paused-color");
    document.getElementById("statusText").textContent = "Paused";
  } else {
    btn.classList.remove("paused");
    document.getElementById("pauseIcon").textContent = "⏸";
    document.getElementById("pauseText").textContent = "Pause";
    dot.classList.replace("paused", "active");
    badge.classList.remove("visible");
    card.classList.remove("paused");
    val.classList.remove("paused-color");
    if (lastBPM !== null) updateDashboard(lastBPM, Date.now());
  }
}

// ── Fullscreen ──
function toggleFullscreen() {
  if (!document.fullscreenElement) {
    document.documentElement.requestFullscreen().catch(() => {});
    document.getElementById("fsIcon").textContent = "🔲";
  } else {
    document.exitFullscreen();
    document.getElementById("fsIcon").textContent = "⛶";
  }
}

document.addEventListener("fullscreenchange", () => {
  document.getElementById("fsIcon").textContent = document.fullscreenElement
    ? "🔲"
    : "⛶";
});

// ── Reset Dashboard ──
function resetDashboard() {
  // Reset state
  maxBPM = 0;
  minBPM = Infinity;
  allBPMs = [];
  hasData = false;
  lastBPM = null;

  // Clear chart
  labels.length = 0;
  dataPoints.length = 0;
  chart.options.scales.y.suggestedMin = 50;
  chart.options.scales.y.suggestedMax = 120;
  chart.update();

  // Reset UI elements
  document.getElementById("bpmValue").textContent = "---";
  document.getElementById("statusValue").textContent = "—";
  document.getElementById("statusValue").className = "stat-value";
  document.getElementById("maxValue").textContent = "—";
  document.getElementById("minValue").textContent = "—";
  document.getElementById("avgValue").textContent = "—";
  document.getElementById("readingCount").textContent = "0";
  document.getElementById("waitingMsg").style.display = "";
  document.getElementById("bpmCard").classList.remove("alert");
  document.getElementById("alertBanner").classList.remove("visible");

  // Reset temperature state
  lastTemp = null;
  maxTemp = -Infinity;
  minTemp = Infinity;
  allTemps = [];
  tempLabels.length = 0;
  tempDataPoints.length = 0;
  tempChart.options.scales.y.suggestedMin = 30;
  tempChart.options.scales.y.suggestedMax = 42;
  tempChart.update();
  document.getElementById("tempValue").textContent = "--.-";
  document.getElementById("tempStatus").textContent = "Waiting for sensor...";
  document.getElementById("tempMaxValue").textContent = "—";
  document.getElementById("tempMinValue").textContent = "—";
  document.getElementById("tempCard").classList.remove("fever");
  document.getElementById("feverBanner").classList.remove("visible");

  // Reset session timer
  if (sessionTimerInterval) clearInterval(sessionTimerInterval);
  sessionTimerInterval = null;
  sessionStart = null;
  document.getElementById("sessionStart").textContent = "--:--:--";
  document.getElementById("sessionDuration").textContent = "00:00";

  // If paused, unpause
  if (isPaused) {
    isPaused = false;
    document.getElementById("pauseBtn").classList.remove("paused");
    document.getElementById("pauseIcon").textContent = "⏸";
    document.getElementById("pauseText").textContent = "Pause";
    document.getElementById("pausedBadge").classList.remove("visible");
    document.getElementById("bpmCard").classList.remove("paused");
    document.getElementById("bpmValue").classList.remove("paused-color");
  }

  // Send reset to server → Arduino
  socket.emit("control", "reset");

  // Update status
  const dot = document.getElementById("statusDot");
  dot.classList.remove("paused");
  dot.classList.add("active");
  document.getElementById("statusText").textContent =
    "Session reset — waiting for data...";
}

// ── Export CSV ──
function exportCSV() {
  if (allBPMs.length === 0) return;

  let csv = "Index,BPM,Status,Temperature(°C)\n";
  allBPMs.forEach((bpm, i) => {
    let status = "Normal";
    if (bpm < 60) status = "Low";
    else if (bpm > 140) status = "Danger";
    else if (bpm > 100) status = "High";
    const temp = allTemps[i] !== undefined ? allTemps[i] : "N/A";
    csv += `${i + 1},${bpm},${status},${temp}\n`;
  });

  csv += `\nMin BPM,${minBPM === Infinity ? "N/A" : minBPM}\n`;
  csv += `Max BPM,${maxBPM}\n`;
  csv += `Average BPM,${Math.round(allBPMs.reduce((a, b) => a + b, 0) / allBPMs.length)}\n`;
  csv += `Total Readings,${allBPMs.length}\n`;
  if (allTemps.length > 0) {
    csv += `Min Temp,${minTemp === Infinity ? "N/A" : minTemp}°C\n`;
    csv += `Max Temp,${maxTemp === -Infinity ? "N/A" : maxTemp}°C\n`;
    const avgTemp = (allTemps.reduce((a, b) => a + b, 0) / allTemps.length).toFixed(1);
    csv += `Average Temp,${avgTemp}°C\n`;
  }

  const blob = new Blob(["\uFEFF" + csv], { type: "text/csv;charset=utf-8;" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = `pulse-data-${new Date().toISOString().slice(0, 10)}.csv`;
  a.click();
  URL.revokeObjectURL(url);
}

// ── Socket Events ──
socket.on("connect", () => {
  document.getElementById("statusDot").classList.add("active");
  document.getElementById("statusText").textContent = "Connected to server";
});

socket.on("disconnect", () => {
  document.getElementById("statusDot").classList.remove("active", "paused");
  document.getElementById("statusText").textContent = "Disconnected!";
});

socket.on("serial-status", ({ connected, error }) => {
  const dot = document.getElementById("statusDot");
  if (connected) {
    dot.classList.add("active");
    document.getElementById("statusText").textContent = "Sensor connected";
  } else if (error) {
    document.getElementById("statusText").textContent = `Error: ${error}`;
  }
});

// Arduino hardware status messages
socket.on("arduino-status", ({ status }) => {
  const statusMap = {
    CALIBRATING: "Calibrating sensor...",
    READY: "Sensor ready",
    NO_SIGNAL: "No signal — place finger",
    PAUSED: "Arduino paused",
    RESUMED: "Arduino resumed",
    RESET: "Session reset",
    SLOW: "Low heart rate",
    NORMAL: "Normal heart rate",
    HIGH: "High heart rate",
  };
  const dot = document.getElementById("statusDot");
  const msg = statusMap[status] || status;

  if (status === "NO_SIGNAL") {
    dot.classList.remove("active");
    dot.classList.add("paused");
    document.getElementById("statusText").textContent = msg;
  } else if (status === "READY" || status === "RESUMED" || status === "RESET") {
    dot.classList.remove("paused");
    dot.classList.add("active");
    document.getElementById("statusText").textContent = msg;
  } else if (status === "CALIBRATING") {
    document.getElementById("statusText").textContent = msg;
  }
});

socket.on("bpm", ({ bpm, time }) => {
  lastBPM = bpm;
  if (!isPaused) updateDashboard(bpm, time);
});

// ── Temperature Socket Event ──
socket.on("temperature", ({ temp, time }) => {
  lastTemp = temp;
  if (!isPaused) updateTemperature(temp, time);
});

// ── Update Dashboard ──
let lastAlertTime = 0;

function updateDashboard(bpm, time) {
  // Start session timer on first data
  if (!hasData) {
    hasData = true;
    document.getElementById("waitingMsg").style.display = "none";
    startSessionTimer();
  }

  // Animate BPM value
  const bpmEl = document.getElementById("bpmValue");
  bpmEl.textContent = bpm;
  bpmEl.classList.remove("beat");
  void bpmEl.offsetWidth;
  bpmEl.classList.add("beat");

  // Alert for high BPM
  const isHigh = bpm > 100;
  document.getElementById("bpmCard").classList.toggle("alert", isHigh);
  document.getElementById("alertBanner").classList.toggle("visible", isHigh);

  if (isHigh && Date.now() - lastAlertTime > 5000) {
    playAlertBeep();
    lastAlertTime = Date.now();
  }

  // Status
  const s = document.getElementById("statusValue");
  if (bpm < 60) {
    s.textContent = "Low";
    s.className = "stat-value caution";
  } else if (bpm <= 100) {
    s.textContent = "Normal ✓";
    s.className = "stat-value normal";
  } else if (bpm <= 140) {
    s.textContent = "High ⚠";
    s.className = "stat-value high";
  } else {
    s.textContent = "Danger 🚨";
    s.className = "stat-value high";
  }

  // Max / Min
  if (bpm > maxBPM) {
    maxBPM = bpm;
    document.getElementById("maxValue").textContent = maxBPM;
  }
  if (bpm < minBPM) {
    minBPM = bpm;
    document.getElementById("minValue").textContent = minBPM;
  }

  // Average
  allBPMs.push(bpm);
  if (allBPMs.length > 200) allBPMs.shift();
  const avg = Math.round(allBPMs.reduce((a, b) => a + b, 0) / allBPMs.length);
  document.getElementById("avgValue").textContent = avg;

  // Chart — dynamic Y axis
  const currentMin = Math.min(...dataPoints, bpm);
  const currentMax = Math.max(...dataPoints, bpm);
  chart.options.scales.y.suggestedMin = Math.max(30, currentMin - 15);
  chart.options.scales.y.suggestedMax = currentMax + 15;

  const t = new Date(time);
  const label = formatTime(t);
  labels.push(label);
  dataPoints.push(bpm);
  if (labels.length > MAX_POINTS) {
    labels.shift();
    dataPoints.shift();
  }
  chart.update();

  // Reading count
  document.getElementById("readingCount").textContent = allBPMs.length;

  // Status text
  if (!isPaused) {
    document.getElementById("statusText").textContent = `Last update ${label}`;
  }
}

// ── Update Temperature ──
function updateTemperature(temp, time) {
  // Update display value
  const tempEl = document.getElementById("tempValue");
  tempEl.textContent = temp.toFixed(1);

  // Fever detection (> 37.5°C)
  const isFever = temp > 37.5;
  document.getElementById("tempCard").classList.toggle("fever", isFever);
  document.getElementById("feverBanner").classList.toggle("visible", isFever);

  // Temperature status
  const ts = document.getElementById("tempStatus");
  if (temp < 36.0) {
    ts.textContent = "Low";
    ts.className = "temp-status low";
  } else if (temp <= 37.5) {
    ts.textContent = "Normal";
    ts.className = "temp-status normal";
  } else {
    ts.textContent = "Fever ⚠";
    ts.className = "temp-status fever";
  }

  // Max / Min
  if (temp > maxTemp) {
    maxTemp = temp;
    document.getElementById("tempMaxValue").textContent = maxTemp.toFixed(1) + "°";
  }
  if (temp < minTemp) {
    minTemp = temp;
    document.getElementById("tempMinValue").textContent = minTemp.toFixed(1) + "°";
  }

  // Store
  allTemps.push(temp);
  if (allTemps.length > 200) allTemps.shift();

  // Chart — dynamic Y axis
  const cMin = Math.min(...tempDataPoints, temp);
  const cMax = Math.max(...tempDataPoints, temp);
  tempChart.options.scales.y.suggestedMin = Math.max(20, cMin - 2);
  tempChart.options.scales.y.suggestedMax = cMax + 2;

  const t = new Date(time);
  const label = formatTime(t);
  tempLabels.push(label);
  tempDataPoints.push(temp);
  if (tempLabels.length > MAX_POINTS) {
    tempLabels.shift();
    tempDataPoints.shift();
  }
  tempChart.update();
}
