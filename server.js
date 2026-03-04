require('dotenv').config();
const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const path = require('path');

const app = express();
const server = http.createServer(app);
const io = new Server(server);

// ── Configuration (from .env or defaults) ──
const PORT_NAME = process.env.COM_PORT || 'COM5';
const BAUD_RATE = parseInt(process.env.BAUD_RATE) || 115200;
const SERVER_PORT = parseInt(process.env.SERVER_PORT) || 3000;
const MAX_RETRIES = 10;
const RETRY_DELAY = 3000;

let serialPort;
let retryCount = 0;

// ── Serial Connection with retry limit ──
function connectSerial() {
  if (retryCount >= MAX_RETRIES) {
    console.error(`❌ Max retries (${MAX_RETRIES}) reached. Giving up on serial connection.`);
    io.emit('serial-status', { connected: false, error: 'Max retries reached' });
    return;
  }

  try {
    serialPort = new SerialPort({ path: PORT_NAME, baudRate: BAUD_RATE });
    const parser = serialPort.pipe(new ReadlineParser({ delimiter: '\r\n' }));

    serialPort.on('open', () => {
      retryCount = 0;
      console.log(`✅ Serial Port open on ${PORT_NAME}`);
      io.emit('serial-status', { connected: true });
    });

    parser.on('data', (line) => {
      line = line.trim();
      if (line.startsWith('BPM:')) {
        const bpm = parseInt(line.replace('BPM:', ''));
        if (!isNaN(bpm) && bpm > 0 && bpm < 300) {
          console.log(`💓 BPM = ${bpm}`);
          io.emit('bpm', { bpm, time: Date.now() });
        }
      } else if (line.startsWith('TEMP:')) {
        const temp = parseFloat(line.replace('TEMP:', ''));
        if (!isNaN(temp) && temp > -50 && temp < 125) {
          console.log(`🌡️  Temp = ${temp}°C`);
          io.emit('temperature', { temp, time: Date.now() });
        }
      } else if (line.startsWith('STATUS:')) {
        const status = line.replace('STATUS:', '').trim();
        console.log(`📡 Arduino Status: ${status}`);
        io.emit('arduino-status', { status, time: Date.now() });
      }
    });

    serialPort.on('error', (err) => {
      console.error('❌ Serial error:', err.message);
      retryCount++;
      console.log(`🔄 Retry ${retryCount}/${MAX_RETRIES} in ${RETRY_DELAY / 1000}s...`);
      setTimeout(connectSerial, RETRY_DELAY);
    });

    serialPort.on('close', () => {
      console.log('🔌 Serial closed - reconnecting...');
      io.emit('serial-status', { connected: false });
      retryCount++;
      setTimeout(connectSerial, RETRY_DELAY);
    });
  } catch (err) {
    console.error('❌ Failed to create serial port:', err.message);
    retryCount++;
    setTimeout(connectSerial, RETRY_DELAY);
  }
}

// ── Static files ──
app.use(express.static(path.join(__dirname, 'public')));

// ── Socket.IO ──
const VALID_COMMANDS = ['pause', 'resume', 'reset'];

io.on('connection', (socket) => {
  console.log('🌐 Browser connected:', socket.id);

  socket.on('control', (cmd) => {
    if (!VALID_COMMANDS.includes(cmd)) {
      console.warn(`⚠ Invalid command received: ${cmd}`);
      return;
    }
    if (serialPort && serialPort.isOpen) {
      serialPort.write(cmd.toUpperCase() + '\n');
      console.log(`${cmd === 'pause' ? '⏸' : cmd === 'reset' ? '🔄' : '▶'} ${cmd.toUpperCase()} sent to Arduino`);
    }
  });

  socket.on('disconnect', () => {
    console.log('🔴 Browser disconnected:', socket.id);
  });
});

server.listen(SERVER_PORT, () => {
  console.log(`🚀 Server running on http://localhost:${SERVER_PORT}`);
  connectSerial();
});