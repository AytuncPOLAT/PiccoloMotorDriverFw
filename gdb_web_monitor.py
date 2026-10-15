#!/usr/bin/env python3
"""
Piccolo Motor Driver - Live GDB Variable & 7-Pole BLDC Motor Visualizer
Reads encoder data (multiturn / rotor angle), electrical angle, and phase currents
from STM32H7 via J-Link GDB Server and renders an interactive 7-pole brushless motor visualization.
"""

import os
import sys
import time
import math
import json
import struct
import socket
import select
import signal
import argparse
import threading
import subprocess
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse

# ---------------------------------------------------------------------------
# Default Configuration & Search Paths
# ---------------------------------------------------------------------------
DEFAULT_DEVICE = "STM32H723VG"
DEFAULT_SPEED_KHZ = 4000
DEFAULT_GDB_PORT = 2331
DEFAULT_HTTP_PORT = 8080
DEFAULT_SAMPLE_RATE_HZ = 25
DEFAULT_ELF = "build/Release/PiccoloMotorDriver.elf"

POLE_PAIRS = 7
COUNTS_PER_REV = 16384  # AS5047 14-bit absolute encoder

GDB_SERVER_PATHS = [
    "/opt/st/stm32cubeide_2.0.0/plugins/com.st.stm32cube.ide.mcu.externaltools.jlink.linux64_2.5.100.202509120932/tools/bin/JLinkGDBServerCLExe",
    "/opt/st/stm32cubeide_2.0.0/plugins/com.st.stm32cube.ide.mcu.externaltools.jlink.linux64_2.5.100.202509120932/tools/bin/JLinkGDBServer",
    "/opt/SEGGER/Flasher/JLinkGDBServerCLExe",
    "/usr/bin/JLinkGDBServerCLExe",
]

NM_PATHS = [
    "/opt/st/stm32cubeide_2.0.0/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.linux64_1.0.100.202509120712/tools/bin/arm-none-eabi-nm",
    "/usr/bin/arm-none-eabi-nm",
]


def resolve_binary(candidates, fallback_name):
    for path in candidates:
        if os.path.isfile(path) and os.access(path, os.X_OK):
            return path
    which = subprocess.run(["which", fallback_name], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if which.returncode == 0:
        return which.stdout.strip()
    return candidates[0] if candidates else fallback_name


def resolve_symbols(elf_path):
    symbols = {}
    nm_bin = resolve_binary(NM_PATHS, "arm-none-eabi-nm")
    if not os.path.isfile(elf_path):
        print(f"[Warning] ELF file '{elf_path}' not found. Using default symbols.")
        symbols["GLOBAL_multiturn"] = 0x24007224
        return symbols

    try:
        res = subprocess.run([nm_bin, elf_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
        for line in res.stdout.splitlines():
            parts = line.split()
            if len(parts) == 3:
                addr, _, name = parts
                if name.startswith("GLOBAL_"):
                    symbols[name] = int(addr, 16)
        print(f"[Info] Resolved {len(symbols)} symbols from {elf_path}")
    except Exception as e:
        print(f"[Warning] Failed to run nm: {e}")
        symbols["GLOBAL_multiturn"] = 0x24007224

    return symbols


class JLinkGDBServerManager:
    def __init__(self, device=DEFAULT_DEVICE, speed=DEFAULT_SPEED_KHZ, port=DEFAULT_GDB_PORT):
        self.device = device
        self.speed = speed
        self.port = port
        self.process = None
        self.we_started_server = False

    def is_port_open(self, host="127.0.0.1", port=None):
        port = port or self.port
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(0.4)
            try:
                s.connect((host, port))
                return True
            except (socket.timeout, ConnectionRefusedError, OSError):
                return False

    def start(self):
        if self.is_port_open():
            print(f"[Info] J-Link GDB Server is already active on port {self.port}.")
            return True

        binary = resolve_binary(GDB_SERVER_PATHS, "JLinkGDBServerCLExe")
        cmd = [
            binary,
            "-device", self.device,
            "-if", "SWD",
            "-speed", str(self.speed),
            "-port", str(self.port),
            "-nohalt"
        ]
        print(f"[Info] Starting J-Link GDB Server (port {self.port}, nohalt, {self.device})...")
        self.process = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        self.we_started_server = True

        for _ in range(30):
            time.sleep(0.1)
            if self.is_port_open():
                print(f"[Info] J-Link GDB Server listening on port {self.port}.")
                return True

        print("[Error] Timeout waiting for J-Link GDB Server to start.")
        return False

    def stop(self):
        if self.we_started_server and self.process:
            print("[Info] Terminating J-Link GDB Server...")
            self.process.terminate()
            try:
                self.process.wait(timeout=2.0)
            except subprocess.TimeoutExpired:
                self.process.kill()
            self.process = None


class GDBClient:
    def __init__(self, host="127.0.0.1", port=DEFAULT_GDB_PORT):
        self.host = host
        self.port = port
        self.sock = None
        self.connected = False

    def connect(self):
        try:
            if self.sock:
                self.sock.close()
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(1.0)
            self.sock.connect((self.host, self.port))
            self.connected = True
            return True
        except Exception:
            self.connected = False
            return False

    def disconnect(self):
        if self.sock:
            try:
                self.sock.close()
            except Exception:
                pass
            self.sock = None
        self.connected = False

    @staticmethod
    def _csum(cmd_str):
        return "{:02x}".format(sum(cmd_str.encode("ascii")) % 256)

    def read_memory(self, addr, length):
        if not self.connected and not self.connect():
            return None

        cmd = f"m{addr:x},{length:x}"
        pkt = f"${cmd}#{self._csum(cmd)}".encode("ascii")

        try:
            self.sock.sendall(pkt)
            resp = b""
            start = time.time()
            while b"#" not in resp:
                if time.time() - start > 1.0:
                    raise socket.timeout("Read timeout")
                chunk = self.sock.recv(1024)
                if not chunk:
                    raise ConnectionError("Socket closed")
                resp += chunk

            self.sock.sendall(b"+")
            hex_part = resp.split(b"$")[-1].split(b"#")[0].decode("ascii")
            if hex_part.startswith("E"):
                return None
            return bytes.fromhex(hex_part)
        except Exception:
            self.disconnect()
            return None


class TelemetryCollector:
    def __init__(self, gdb_client, symbols, sample_rate_hz=DEFAULT_SAMPLE_RATE_HZ):
        self.client = gdb_client
        self.symbols = symbols
        self.sample_rate_hz = sample_rate_hz
        self.running = False
        self.thread = None
        self.lock = threading.Lock()

        self.latest_data = {
            "timestamp": time.time(),
            "connected": False,
            "multiturn": 0,
            "revolutions": 0.0,
            "mech_angle_deg": 0.0,
            "mech_angle_rad": 0.0,
            "elec_angle_deg": 0.0,
            "elec_angle_rad": 0.0,
            "pole_pairs": POLE_PAIRS,
            "current_a": 0.0,
            "current_b": 0.0,
            "current_c": 0.0,
            "i_alpha": 0.0,
            "i_beta": 0.0,
            "i_mag": 0.0,
            "i_angle_deg": 0.0,
            "speed": 0,
            "vbus": 0.0,
            "torque_cmd": 0.0,
            "pos_cmd": 0.0,
            "sample_rate_hz": sample_rate_hz,
            "latency_ms": 0.0,
        }

        self.history = []
        self.max_history = 2000
        self.subscribers = []
        self.subs_lock = threading.Lock()

    def set_rate(self, hz):
        self.sample_rate_hz = max(1, min(100, int(hz)))
        with self.lock:
            self.latest_data["sample_rate_hz"] = self.sample_rate_hz

    def add_subscriber(self, q):
        with self.subs_lock:
            self.subscribers.append(q)

    def remove_subscriber(self, q):
        with self.subs_lock:
            if q in self.subscribers:
                self.subscribers.remove(q)

    def start(self):
        self.running = True
        self.thread = threading.Thread(target=self._run, daemon=True)
        self.thread.start()

    def stop(self):
        self.running = False
        if self.thread:
            self.thread.join(timeout=1.0)

    def _run(self):
        base_addr = 0x24007210
        block_len = 56

        while self.running:
            start_ts = time.time()
            data_bytes = self.client.read_memory(base_addr, block_len)

            if data_bytes and len(data_bytes) == block_len:
                try:
                    latency = (time.time() - start_ts) * 1000.0
                    # Scale phase current inputs by 1/10 (0.1)
                    current_scale = 0.1
                    current_c = struct.unpack_from("<f", data_bytes, 0x08)[0] * current_scale
                    current_b = struct.unpack_from("<f", data_bytes, 0x0C)[0] * current_scale
                    current_a = struct.unpack_from("<f", data_bytes, 0x10)[0] * current_scale
                    multiturn = struct.unpack_from("<i", data_bytes, 0x14)[0]
                    speed = struct.unpack_from("<i", data_bytes, 0x18)[0]
                    angle_deg_unused = struct.unpack_from("<f", data_bytes, 0x1C)[0]
                    angle_rad = struct.unpack_from("<f", data_bytes, 0x20)[0]
                    vbus = struct.unpack_from("<f", data_bytes, 0x24)[0]
                    pos_cmd = struct.unpack_from("<f", data_bytes, 0x28)[0]
                    torque_cmd = struct.unpack_from("<f", data_bytes, 0x2C)[0]

                    # 1. Mechanical Angle (from AS5047 multi-turn counts)
                    revs = multiturn / float(COUNTS_PER_REV)
                    pos_within_rev = multiturn % COUNTS_PER_REV
                    mech_deg = (pos_within_rev / float(COUNTS_PER_REV)) * 360.0
                    mech_deg = (mech_deg + 360.0) % 360.0
                    mech_rad = (mech_deg * math.pi) / 180.0

                    # 2. Electrical Angle (from firmware angleInRadians)
                    elec_rad = angle_rad % (2.0 * math.pi)
                    if elec_rad < 0:
                        elec_rad += 2.0 * math.pi
                    elec_deg = (elec_rad * 180.0) / math.pi

                    # 3. Space Vector (Clarke Transform)
                    i_alpha = current_a - 0.5 * (current_b + current_c)
                    i_beta = (math.sqrt(3.0) / 2.0) * (current_b - current_c)
                    i_mag = math.sqrt(i_alpha**2 + i_beta**2)
                    i_angle_rad = math.atan2(i_beta, i_alpha)
                    i_angle_deg = (math.degrees(i_angle_rad) + 360.0) % 360.0

                    now = time.time()
                    snapshot = {
                        "timestamp": now,
                        "connected": True,
                        "multiturn": multiturn,
                        "revolutions": round(revs, 3),
                        "mech_angle_deg": round(mech_deg, 2),
                        "mech_angle_rad": round(mech_rad, 4),
                        "elec_angle_deg": round(elec_deg, 2),
                        "elec_angle_rad": round(elec_rad, 4),
                        "pole_pairs": POLE_PAIRS,
                        "current_a": round(current_a, 3),
                        "current_b": round(current_b, 3),
                        "current_c": round(current_c, 3),
                        "i_alpha": round(i_alpha, 3),
                        "i_beta": round(i_beta, 3),
                        "i_mag": round(i_mag, 3),
                        "i_angle_deg": round(i_angle_deg, 2),
                        "speed": speed,
                        "vbus": round(vbus, 2),
                        "torque_cmd": round(torque_cmd, 3),
                        "pos_cmd": round(pos_cmd, 2),
                        "sample_rate_hz": self.sample_rate_hz,
                        "latency_ms": round(latency, 1),
                    }
                except Exception:
                    snapshot = None
            else:
                now = time.time()
                snapshot = {
                    "timestamp": now,
                    "connected": False,
                    "multiturn": self.latest_data["multiturn"],
                    "revolutions": self.latest_data["revolutions"],
                    "mech_angle_deg": self.latest_data["mech_angle_deg"],
                    "mech_angle_rad": self.latest_data["mech_angle_rad"],
                    "elec_angle_deg": self.latest_data["elec_angle_deg"],
                    "elec_angle_rad": self.latest_data["elec_angle_rad"],
                    "pole_pairs": POLE_PAIRS,
                    "current_a": 0.0,
                    "current_b": 0.0,
                    "current_c": 0.0,
                    "i_alpha": 0.0,
                    "i_beta": 0.0,
                    "i_mag": 0.0,
                    "i_angle_deg": 0.0,
                    "speed": 0,
                    "vbus": 0.0,
                    "torque_cmd": 0.0,
                    "pos_cmd": 0.0,
                    "sample_rate_hz": self.sample_rate_hz,
                    "latency_ms": 0.0,
                }

            if snapshot:
                with self.lock:
                    self.latest_data = snapshot
                    self.history.append((
                        snapshot["timestamp"],
                        snapshot["mech_angle_deg"],
                        snapshot["elec_angle_deg"],
                        snapshot["current_a"],
                        snapshot["current_b"],
                        snapshot["current_c"],
                        snapshot["multiturn"]
                    ))
                    if len(self.history) > self.max_history:
                        self.history.pop(0)

                msg = f"data: {json.dumps(snapshot)}\n\n"
                with self.subs_lock:
                    for q in list(self.subscribers):
                        try:
                            q.put_nowait(msg)
                        except Exception:
                            pass

            interval = 1.0 / self.sample_rate_hz
            elapsed = time.time() - start_ts
            time.sleep(max(0.005, interval - elapsed))


# ---------------------------------------------------------------------------
# Visualizer HTML / CSS / JS Application
# ---------------------------------------------------------------------------
HTML_PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>7-Pole BLDC Motor Real-Time Visualizer</title>
<style>
  :root {
    --bg-primary: #0a0d14;
    --bg-card: #121824;
    --border: #263346;
    --text-main: #f0f6fc;
    --text-muted: #8b9bb4;
    --accent: #38bdf8;
    --green: #22c55e;
    --red: #ef4444;
    --phase-a: #fbbf24;
    --phase-b: #06b6d4;
    --phase-c: #ec4899;
    --font-mono: 'SFMono-Regular', Consolas, 'Liberation Mono', Menlo, monospace;
    --font-sans: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
  }

  * { box-sizing: border-box; margin: 0; padding: 0; }

  body {
    background-color: var(--bg-primary);
    color: var(--text-main);
    font-family: var(--font-sans);
    padding: 16px 20px;
    min-height: 100vh;
  }

  header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 15px;
    margin-bottom: 20px;
    padding-bottom: 12px;
    border-bottom: 1px solid var(--border);
  }

  .title-group h1 {
    font-size: 1.4rem;
    font-weight: 700;
    display: flex;
    align-items: center;
    gap: 10px;
  }

  .badge {
    font-size: 0.75rem;
    padding: 2px 8px;
    border-radius: 10px;
    font-weight: 600;
    background: rgba(56, 189, 248, 0.15);
    color: var(--accent);
    border: 1px solid rgba(56, 189, 248, 0.3);
  }

  .status-bar {
    display: flex;
    align-items: center;
    gap: 10px;
    font-size: 0.8rem;
    font-family: var(--font-mono);
  }

  .pill {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 4px 10px;
    border-radius: 6px;
    background: var(--bg-card);
    border: 1px solid var(--border);
  }

  .status-dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--red);
    box-shadow: 0 0 6px var(--red);
    transition: all 0.3s;
  }

  .status-dot.online {
    background: var(--green);
    box-shadow: 0 0 8px var(--green);
  }

  /* Main Split Layout */
  .layout-grid {
    display: grid;
    grid-template-columns: 500px 1fr;
    gap: 20px;
    margin-bottom: 20px;
  }

  @media (max-width: 1100px) {
    .layout-grid { grid-template-columns: 1fr; }
  }

  .card {
    background: var(--bg-card);
    border: 1px solid var(--border);
    border-radius: 10px;
    padding: 16px 20px;
    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.3);
  }

  .card-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 14px;
  }

  .card-title {
    font-size: 0.85rem;
    text-transform: uppercase;
    letter-spacing: 1px;
    color: var(--text-muted);
    font-weight: 600;
  }

  /* Motor Canvas Wrapper */
  .motor-container {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    position: relative;
  }

  #motorCanvas {
    width: 440px;
    height: 440px;
    display: block;
  }

  .motor-legend {
    display: flex;
    justify-content: center;
    gap: 16px;
    font-size: 0.75rem;
    font-family: var(--font-mono);
    margin-top: 10px;
  }

  .legend-item {
    display: flex;
    align-items: center;
    gap: 6px;
  }

  .legend-color {
    width: 12px;
    height: 12px;
    border-radius: 3px;
  }

  /* Angle Telemetry Cards */
  .angles-panel {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 12px;
    margin-top: 14px;
    width: 100%;
  }

  .angle-card {
    background: rgba(10, 13, 20, 0.8);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 10px 14px;
    text-align: center;
  }

  .angle-card.rotor { border-left: 3px solid var(--accent); }
  .angle-card.elec { border-left: 3px solid var(--green); }

  .angle-card-label {
    font-size: 0.7rem;
    color: var(--text-muted);
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }

  .angle-card-deg {
    font-size: 1.7rem;
    font-weight: 800;
    font-family: var(--font-mono);
    color: var(--text-main);
    line-height: 1.2;
    margin: 4px 0;
  }

  .angle-card-rad {
    font-size: 0.8rem;
    color: var(--text-muted);
    font-family: var(--font-mono);
  }

  /* Phase Currents Monitor */
  .currents-container {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  .current-bar-row {
    display: grid;
    grid-template-columns: 80px 1fr 80px;
    align-items: center;
    gap: 10px;
    font-family: var(--font-mono);
  }

  .phase-label {
    font-size: 0.85rem;
    font-weight: 700;
  }

  .bar-track {
    background: #1b2434;
    height: 20px;
    border-radius: 4px;
    position: relative;
    overflow: hidden;
    border: 1px solid var(--border);
  }

  .bar-zero-line {
    position: absolute;
    top: 0;
    bottom: 0;
    left: 50%;
    width: 2px;
    background: #475569;
    z-index: 2;
  }

  .bar-fill {
    position: absolute;
    top: 0;
    bottom: 0;
    transition: width 0.05s linear, left 0.05s linear;
  }

  .current-val {
    font-size: 0.95rem;
    font-weight: 700;
    text-align: right;
  }

  /* Chart & Tabs */
  .chart-section {
    display: flex;
    flex-direction: column;
    height: 100%;
  }

  .tabs {
    display: flex;
    gap: 8px;
    margin-bottom: 12px;
  }

  .tab-btn {
    background: #1a2332;
    color: var(--text-muted);
    border: 1px solid var(--border);
    padding: 6px 14px;
    border-radius: 6px;
    font-size: 0.8rem;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s;
  }

  .tab-btn.active {
    background: rgba(56, 189, 248, 0.15);
    color: var(--accent);
    border-color: var(--accent);
  }

  .canvas-holder {
    position: relative;
    flex: 1;
    min-height: 260px;
    width: 100%;
  }

  #stripchartCanvas {
    width: 100%;
    height: 100%;
    display: block;
  }

  /* Controls row */
  .controls-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 10px;
    margin-top: 12px;
    padding-top: 10px;
    border-top: 1px solid var(--border);
  }

  button {
    background: #1a2332;
    color: var(--text-main);
    border: 1px solid var(--border);
    padding: 5px 12px;
    border-radius: 6px;
    font-size: 0.8rem;
    cursor: pointer;
    font-weight: 500;
  }
  button:hover { background: #263346; }

  .slider-group {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 0.8rem;
    color: var(--text-muted);
    font-family: var(--font-mono);
  }
  input[type="range"] { accent-color: var(--accent); cursor: pointer; }
</style>
</head>
<body>

<header>
  <div class="title-group">
    <h1>
      Brushless Motor Visualizer
      <span class="badge">7 Pole Pairs (14 Magnets)</span>
    </h1>
  </div>
  <div class="status-bar">
    <div class="pill">
      <div id="statusDot" class="status-dot"></div>
      <span id="statusText">CONNECTING...</span>
    </div>
    <div class="pill">
      <span>GDB:</span>
      <span style="color: var(--accent);">Port 2331</span>
    </div>
    <div class="pill">
      <span>LATENCY:</span>
      <span id="latencyText">-- ms</span>
    </div>
  </div>
</header>

<div class="layout-grid">
  <!-- Left: 7-Pole Motor Cross-Section Canvas -->
  <div class="card">
    <div class="card-header">
      <span class="card-title">Motor Cross-Section & Magnetic Field</span>
      <span style="font-family: var(--font-mono); font-size: 0.75rem; color: var(--accent);" id="speedDisplay">0 RPM</span>
    </div>

    <div class="motor-container">
      <canvas id="motorCanvas" width="440" height="440"></canvas>
      <div class="motor-legend">
        <div class="legend-item"><div class="legend-color" style="background: #ef4444;"></div> North Pole (N)</div>
        <div class="legend-item"><div class="legend-color" style="background: #3b82f6;"></div> South Pole (S)</div>
        <div class="legend-item"><div class="legend-color" style="background: var(--phase-a);"></div> Phase A</div>
        <div class="legend-item"><div class="legend-color" style="background: var(--phase-b);"></div> Phase B</div>
        <div class="legend-item"><div class="legend-color" style="background: var(--phase-c);"></div> Phase C</div>
      </div>
    </div>

    <!-- Rotor & Electrical Angle Readouts -->
    <div class="angles-panel">
      <div class="angle-card rotor">
        <div class="angle-card-label">Rotor Angle (Mechanical)</div>
        <div class="angle-card-deg" id="mechDeg">0.0°</div>
        <div class="angle-card-rad"><span id="mechRad">0.000</span> rad | <span id="revsCount">0.00</span> rev</div>
      </div>
      <div class="angle-card elec">
        <div class="angle-card-label">Electrical Angle (FOC)</div>
        <div class="angle-card-deg" id="elecDeg" style="color: var(--green);">0.0°</div>
        <div class="angle-card-rad"><span id="elecRad">0.000</span> rad | Cycle <span id="elecCycle">1</span>/7</div>
      </div>
    </div>
  </div>

  <!-- Right: Phase Currents & Real-Time Waveforms -->
  <div class="card chart-section">
    <div class="card-header">
      <span class="card-title">Phase Currents ($I_A, I_B, I_C$)</span>
      <div class="slider-group">
        <span>Rate:</span>
        <input type="range" id="rateSlider" min="5" max="50" step="5" value="25">
        <span id="rateLabel">25 Hz</span>
      </div>
    </div>

    <!-- Live Current Gauges -->
    <div class="currents-container">
      <div class="current-bar-row">
        <span class="phase-label" style="color: var(--phase-a);">Phase A</span>
        <div class="bar-track">
          <div class="bar-zero-line"></div>
          <div id="barA" class="bar-fill" style="background: var(--phase-a); left: 50%; width: 0%;"></div>
        </div>
        <span id="valA" class="current-val" style="color: var(--phase-a);">0.00 A</span>
      </div>

      <div class="current-bar-row">
        <span class="phase-label" style="color: var(--phase-b);">Phase B</span>
        <div class="bar-track">
          <div class="bar-zero-line"></div>
          <div id="barB" class="bar-fill" style="background: var(--phase-b); left: 50%; width: 0%;"></div>
        </div>
        <span id="valB" class="current-val" style="color: var(--phase-b);">0.00 A</span>
      </div>

      <div class="current-bar-row">
        <span class="phase-label" style="color: var(--phase-c);">Phase C</span>
        <div class="bar-track">
          <div class="bar-zero-line"></div>
          <div id="barC" class="bar-fill" style="background: var(--phase-c); left: 50%; width: 0%;"></div>
        </div>
        <span id="valC" class="current-val" style="color: var(--phase-c);">0.00 A</span>
      </div>
    </div>

    <!-- Chart Tabs -->
    <div style="margin-top: 18px; display: flex; justify-content: space-between; align-items: center;">
      <div class="tabs">
        <button class="tab-btn active" data-tab="currents">Phase Currents</button>
        <button class="tab-btn" data-tab="angles">Rotor vs Electrical Angle</button>
      </div>
      <div style="font-size: 0.75rem; font-family: var(--font-mono); color: var(--text-muted);" id="vectorMagnitude">
        |Is|: 0.00 A
      </div>
    </div>

    <!-- Canvas Stripchart -->
    <div class="canvas-holder">
      <canvas id="stripchartCanvas"></canvas>
    </div>

    <div class="controls-row">
      <div style="display: flex; gap: 6px;">
        <button id="btnPause">Pause</button>
        <button id="btnClear">Clear</button>
        <button id="btnExport">Export CSV</button>
      </div>
      <div style="display: flex; gap: 6px;">
        <button class="win-btn active" data-sec="5">5s</button>
        <button class="win-btn" data-sec="15">15s</button>
        <button class="win-btn" data-sec="30">30s</button>
      </div>
    </div>
  </div>
</div>

<script>
  // Elements
  const statusDot = document.getElementById('statusDot');
  const statusText = document.getElementById('statusText');
  const latencyText = document.getElementById('latencyText');
  const speedDisplay = document.getElementById('speedDisplay');
  const mechDeg = document.getElementById('mechDeg');
  const mechRad = document.getElementById('mechRad');
  const revsCount = document.getElementById('revsCount');
  const elecDeg = document.getElementById('elecDeg');
  const elecRad = document.getElementById('elecRad');
  const elecCycle = document.getElementById('elecCycle');
  const barA = document.getElementById('barA');
  const barB = document.getElementById('barB');
  const barC = document.getElementById('barC');
  const valA = document.getElementById('valA');
  const valB = document.getElementById('valB');
  const valC = document.getElementById('valC');
  const vectorMagnitude = document.getElementById('vectorMagnitude');
  const rateSlider = document.getElementById('rateSlider');
  const rateLabel = document.getElementById('rateLabel');
  const btnPause = document.getElementById('btnPause');
  const btnClear = document.getElementById('btnClear');
  const btnExport = document.getElementById('btnExport');
  const tabBtns = document.querySelectorAll('.tab-btn');
  const winBtns = document.querySelectorAll('.win-btn');

  // Canvases
  const motorCanvas = document.getElementById('motorCanvas');
  const mCtx = motorCanvas.getContext('2d');
  const chartCanvas = document.getElementById('stripchartCanvas');
  const cCtx = chartCanvas.getContext('2d');

  // High DPI Setup
  function setupHiDPI(canvas, ctx) {
    const dpr = window.devicePixelRatio || 1;
    const rect = canvas.getBoundingClientRect();
    canvas.width = rect.width * dpr;
    canvas.height = rect.height * dpr;
    ctx.scale(dpr, dpr);
  }
  setupHiDPI(motorCanvas, mCtx);
  setupHiDPI(chartCanvas, cCtx);

  window.addEventListener('resize', () => {
    setupHiDPI(motorCanvas, mCtx);
    setupHiDPI(chartCanvas, cCtx);
  });

  // State
  let isPaused = false;
  let activeTab = 'currents'; // 'currents' or 'angles'
  let timeWindowSec = 5;
  let historyBuffer = [];
  const MAX_CURRENT_SCALE = 15.0; // +/- 15A for bar gauge

  // Current Telemetry
  let curMechAngle = 0.0;
  let curElecAngle = 0.0;
  let curIa = 0.0;
  let curIb = 0.0;
  let curIc = 0.0;
  let curIAlpha = 0.0;
  let curIBeta = 0.0;
  let curIMag = 0.0;

  // Tab Switching
  tabBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      tabBtns.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      activeTab = btn.dataset.tab;
    });
  });

  winBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      winBtns.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      timeWindowSec = parseFloat(btn.dataset.sec);
    });
  });

  btnPause.addEventListener('click', () => {
    isPaused = !isPaused;
    btnPause.innerText = isPaused ? 'Resume' : 'Pause';
  });

  btnClear.addEventListener('click', () => { historyBuffer = []; });

  rateSlider.addEventListener('input', (e) => {
    const hz = e.target.value;
    rateLabel.innerText = hz + ' Hz';
    fetch('/api/rate', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ rate: parseInt(hz) })
    }).catch(console.error);
  });

  btnExport.addEventListener('click', () => {
    if (historyBuffer.length === 0) return;
    let csv = "timestamp,mech_angle_deg,elec_angle_deg,current_a,current_b,current_c,multiturn\\n";
    historyBuffer.forEach(p => {
      csv += `${p.t.toFixed(4)},${p.mech.toFixed(2)},${p.elec.toFixed(2)},${p.ia.toFixed(3)},${p.ib.toFixed(3)},${p.ic.toFixed(3)},${p.raw}\\n`;
    });
    const blob = new Blob([csv], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `bldc_motor_telemetry_${Date.now()}.csv`;
    a.click();
    URL.revokeObjectURL(url);
  });

  // SSE Stream Receiver
  const evtSource = new EventSource('/api/stream');
  evtSource.onmessage = (e) => {
    if (isPaused) return;
    try {
      const d = JSON.parse(e.data);
      updateTelemetry(d);
    } catch(err) {}
  };
  evtSource.onerror = () => {
    statusDot.classList.remove('online');
    statusText.innerText = 'DISCONNECTED';
  };

  function updateTelemetry(d) {
    if (d.connected) {
      statusDot.classList.add('online');
      statusText.innerText = 'TARGET RUNNING';
    } else {
      statusDot.classList.remove('online');
      statusText.innerText = 'TARGET DISCONNECTED';
    }

    latencyText.innerText = (d.latency_ms || 0) + ' ms';
    speedDisplay.innerText = (d.speed || 0) + ' RPM';

    curMechAngle = d.mech_angle_deg || 0.0;
    curElecAngle = d.elec_angle_deg || 0.0;
    curIa = d.current_a || 0.0;
    curIb = d.current_b || 0.0;
    curIc = d.current_c || 0.0;
    curIAlpha = d.i_alpha || 0.0;
    curIBeta = d.i_beta || 0.0;
    curIMag = d.i_mag || 0.0;

    // Angle text displays
    mechDeg.innerText = curMechAngle.toFixed(1) + '°';
    mechRad.innerText = (d.mech_angle_rad || 0.0).toFixed(3);
    revsCount.innerText = (d.revolutions || 0.0).toFixed(2);

    elecDeg.innerText = curElecAngle.toFixed(1) + '°';
    elecRad.innerText = (d.elec_angle_rad || 0.0).toFixed(3);
    const cycleNum = Math.floor((curMechAngle / (360.0 / 7))) + 1;
    elecCycle.innerText = Math.min(7, Math.max(1, cycleNum));

    // Phase current bars
    updateCurrentBar(barA, valA, curIa);
    updateCurrentBar(barB, valB, curIb);
    updateCurrentBar(barC, valC, curIc);

    vectorMagnitude.innerText = `|Is|: ${curIMag.toFixed(2)} A (∠${(d.i_angle_deg || 0.0).toFixed(0)}°)`;

    // History buffer
    historyBuffer.push({
      t: d.timestamp,
      mech: curMechAngle,
      elec: curElecAngle,
      ia: curIa,
      ib: curIb,
      ic: curIc,
      raw: d.multiturn
    });

    const cutoff = d.timestamp - 60;
    while (historyBuffer.length > 0 && historyBuffer[0].t < cutoff) {
      historyBuffer.shift();
    }
  }

  function updateCurrentBar(barEl, valEl, amps) {
    valEl.innerText = (amps >= 0 ? '+' : '') + amps.toFixed(2) + ' A';
    const dynamicScale = Math.max(MAX_CURRENT_SCALE, Math.abs(amps) * 1.25);
    const percent = Math.min(50, (Math.abs(amps) / dynamicScale) * 50);
    if (amps >= 0) {
      barEl.style.left = '50%';
      barEl.style.width = percent + '%';
    } else {
      barEl.style.left = (50 - percent) + '%';
      barEl.style.width = percent + '%';
    }
  }

  // -------------------------------------------------------------------------
  // 7-Pole BLDC Motor Rendering (Canvas)
  // -------------------------------------------------------------------------
  function drawMotor() {
    const w = motorCanvas.getBoundingClientRect().width;
    const h = motorCanvas.getBoundingClientRect().height;
    mCtx.clearRect(0, 0, w, h);

    const cx = w / 2;
    const cy = h / 2;
    const rOuter = Math.min(cx, cy) - 10;
    const rStatorYoke = rOuter - 18;
    const rStatorTeeth = rStatorYoke - 36;
    const rAirGap = rStatorTeeth - 8;
    const rRotorMagnets = rAirGap - 6;
    const rRotorIron = rRotorMagnets - 24;
    const rShaft = 22;

    // 1. Outer Frame / Stator Casing
    mCtx.save();
    mCtx.beginPath();
    mCtx.arc(cx, cy, rOuter, 0, 2 * Math.PI);
    mCtx.fillStyle = '#161e2e';
    mCtx.strokeStyle = '#263346';
    mCtx.lineWidth = 4;
    mCtx.fill();
    mCtx.stroke();

    // 2. 12 Stator Teeth & Windings (3-Phase: A, B, C)
    const numTeeth = 12;
    const toothAngle = (2 * Math.PI) / numTeeth;
    const phaseNames = ['A+', 'B+', 'C+', 'A-', 'B-', 'C-'];
    const phaseColors = [
      'rgba(251, 191, 36, ',  // Phase A (Amber)
      'rgba(6, 182, 212, ',   // Phase B (Cyan)
      'rgba(236, 72, 153, '   // Phase C (Pink)
    ];

    for (let i = 0; i < numTeeth; i++) {
      const angle = i * toothAngle;
      const phaseIdx = i % 3;
      const isNegative = Math.floor(i / 3) % 2 === 1;

      // Determine current excitation for coil glow
      let coilCurrent = (phaseIdx === 0) ? curIa : (phaseIdx === 1 ? curIb : curIc);
      if (isNegative) coilCurrent = -coilCurrent;
      const glowAlpha = Math.min(1.0, Math.max(0.15, Math.abs(coilCurrent) / 5.0));

      mCtx.save();
      mCtx.translate(cx, cy);
      mCtx.rotate(angle);

      // Stator tooth iron
      mCtx.beginPath();
      mCtx.rect(-10, rAirGap, 20, rStatorYoke - rAirGap);
      mCtx.fillStyle = '#222d3f';
      mCtx.strokeStyle = '#2c3b52';
      mCtx.lineWidth = 1.5;
      mCtx.fill();
      mCtx.stroke();

      // Coil winding around tooth with dynamic current glow
      mCtx.beginPath();
      mCtx.rect(-14, rAirGap + 8, 28, rStatorYoke - rAirGap - 16);
      mCtx.fillStyle = phaseColors[phaseIdx] + glowAlpha + ')';
      mCtx.strokeStyle = phaseColors[phaseIdx] + '0.9)';
      mCtx.lineWidth = 2;
      mCtx.fill();
      mCtx.stroke();

      // Coil label
      mCtx.save();
      mCtx.translate(0, rStatorYoke - 6);
      mCtx.rotate(Math.PI);
      mCtx.fillStyle = '#f0f6fc';
      mCtx.font = 'bold 9px monospace';
      mCtx.textAlign = 'center';
      const pLabel = phaseIdx === 0 ? 'A' : (phaseIdx === 1 ? 'B' : 'C');
      mCtx.fillText(pLabel + (isNegative ? '⁻' : '⁺'), 0, 3);
      mCtx.restore();

      mCtx.restore();
    }

    // 3. Stator Air Gap Ring
    mCtx.beginPath();
    mCtx.arc(cx, cy, rAirGap, 0, 2 * Math.PI);
    mCtx.strokeStyle = 'rgba(56, 189, 248, 0.2)';
    mCtx.lineWidth = 1;
    mCtx.stroke();

    // 4. ROTOR (Rotates according to Encoder Data: curMechAngle)
    const mechRadAngle = (curMechAngle * Math.PI) / 180.0;
    mCtx.save();
    mCtx.translate(cx, cy);
    mCtx.rotate(mechRadAngle);

    // 14 Permanent Magnets (7 Pole Pairs = 7 North + 7 South)
    const numMagnets = 14;
    const magAngle = (2 * Math.PI) / numMagnets;

    for (let m = 0; m < numMagnets; m++) {
      const aStart = m * magAngle;
      const aEnd = (m + 1) * magAngle;
      const isNorth = (m % 2 === 0);

      mCtx.beginPath();
      mCtx.arc(0, 0, rRotorMagnets, aStart, aEnd);
      mCtx.arc(0, 0, rRotorIron, aEnd, aStart, true);
      mCtx.closePath();

      // Gradient for 3D curved magnet appearance
      const midAngle = (aStart + aEnd) / 2;
      const gx = Math.cos(midAngle) * rRotorMagnets;
      const gy = Math.sin(midAngle) * rRotorMagnets;

      mCtx.fillStyle = isNorth ? '#dc2626' : '#2563eb';
      mCtx.fill();
      mCtx.strokeStyle = '#0a0d14';
      mCtx.lineWidth = 1.5;
      mCtx.stroke();

      // Pole text N / S
      const textR = (rRotorMagnets + rRotorIron) / 2;
      const tx = Math.cos(midAngle) * textR;
      const ty = Math.sin(midAngle) * textR;

      mCtx.save();
      mCtx.translate(tx, ty);
      mCtx.rotate(midAngle + Math.PI / 2);
      mCtx.fillStyle = '#ffffff';
      mCtx.font = 'bold 11px sans-serif';
      mCtx.textAlign = 'center';
      mCtx.textBaseline = 'middle';
      mCtx.fillText(isNorth ? 'N' : 'S', 0, 0);
      mCtx.restore();
    }

    // Rotor Back-Iron Body
    mCtx.beginPath();
    mCtx.arc(0, 0, rRotorIron, 0, 2 * Math.PI);
    mCtx.fillStyle = '#1e293b';
    mCtx.strokeStyle = '#334155';
    mCtx.lineWidth = 2;
    mCtx.fill();
    mCtx.stroke();

    // Mechanical 0° Reference Marker (Notch & Neon Line on Rotor)
    mCtx.beginPath();
    mCtx.moveTo(0, 0);
    mCtx.lineTo(rRotorMagnets + 3, 0);
    mCtx.strokeStyle = '#facc15';
    mCtx.lineWidth = 3;
    mCtx.stroke();

    mCtx.beginPath();
    mCtx.arc(rRotorMagnets, 0, 4, 0, 2 * Math.PI);
    mCtx.fillStyle = '#facc15';
    mCtx.fill();

    // Central Motor Shaft with Keyway
    mCtx.beginPath();
    mCtx.arc(0, 0, rShaft, 0, 2 * Math.PI);
    mCtx.fillStyle = '#475569';
    mCtx.strokeStyle = '#64748b';
    mCtx.lineWidth = 2;
    mCtx.fill();
    mCtx.stroke();

    // Shaft keyway slot
    mCtx.beginPath();
    mCtx.rect(-3, -rShaft, 6, 8);
    mCtx.fillStyle = '#0f172a';
    mCtx.fill();

    mCtx.restore(); // End Rotor rotation

    // 5. Stator Magnetic Flux / Space Vector Vector (Arrow in Stator Frame)
    if (curIMag > 0.1) {
      mCtx.save();
      mCtx.translate(cx, cy);

      const maxVecLen = rAirGap - 10;
      const vecLen = Math.min(maxVecLen, (curIMag / 10.0) * maxVecLen + 15);
      const vecAngle = Math.atan2(curIBeta, curIAlpha);

      mCtx.rotate(vecAngle);
      mCtx.beginPath();
      mCtx.moveTo(0, 0);
      mCtx.lineTo(vecLen, 0);
      mCtx.strokeStyle = '#10b981';
      mCtx.lineWidth = 3.5;
      mCtx.shadowColor = 'rgba(16, 185, 129, 0.8)';
      mCtx.shadowBlur = 8;
      mCtx.stroke();

      // Arrow head
      mCtx.beginPath();
      mCtx.moveTo(vecLen, 0);
      mCtx.lineTo(vecLen - 10, -5);
      mCtx.lineTo(vecLen - 10, 5);
      mCtx.closePath();
      mCtx.fillStyle = '#10b981';
      mCtx.fill();
      mCtx.restore();
    }

    mCtx.restore();
  }

  // -------------------------------------------------------------------------
  // Stripchart Oscilloscope (Canvas)
  // -------------------------------------------------------------------------
  function drawChart() {
    const w = chartCanvas.getBoundingClientRect().width;
    const h = chartCanvas.getBoundingClientRect().height;
    cCtx.clearRect(0, 0, w, h);

    const padL = 50, padR = 20, padT = 15, padB = 25;
    const plotW = w - padL - padR;
    const plotH = h - padT - padB;
    if (plotW <= 0 || plotH <= 0) return;

    const now = (historyBuffer.length > 0) ? historyBuffer[historyBuffer.length - 1].t : Date.now() / 1000;
    const minT = now - timeWindowSec;
    const maxT = now;
    const visible = historyBuffer.filter(p => p.t >= minT && p.t <= maxT);

    // Y Axis Range
    let yMin = -5, yMax = 5;
    if (activeTab === 'currents') {
      if (visible.length > 0) {
        const allVals = visible.flatMap(p => [p.ia, p.ib, p.ic]);
        yMin = Math.min(...allVals);
        yMax = Math.max(...allVals);
      }
      const margin = Math.max(1.0, (yMax - yMin) * 0.2);
      yMin = Math.floor(yMin - margin);
      yMax = Math.ceil(yMax + margin);
    } else {
      yMin = 0;
      yMax = 360;
    }

    // Draw Grid Lines
    cCtx.strokeStyle = '#1e293b';
    cCtx.lineWidth = 1;
    cCtx.fillStyle = '#64748b';
    cCtx.font = '10px monospace';
    cCtx.textAlign = 'right';

    const ySteps = 4;
    for (let i = 0; i <= ySteps; i++) {
      const yVal = yMin + ((yMax - yMin) * (i / ySteps));
      const py = padT + plotH - (plotH * (i / ySteps));
      cCtx.beginPath();
      cCtx.moveTo(padL, py);
      cCtx.lineTo(padL + plotW, py);
      cCtx.stroke();
      cCtx.fillText(activeTab === 'currents' ? yVal.toFixed(1) + 'A' : yVal.toFixed(0) + '°', padL - 6, py + 3);
    }

    // X Axis Grid
    cCtx.textAlign = 'center';
    const xSteps = 5;
    for (let i = 0; i <= xSteps; i++) {
      const px = padL + (plotW * (i / xSteps));
      cCtx.beginPath();
      cCtx.moveTo(px, padT);
      cCtx.lineTo(px, padT + plotH);
      cCtx.stroke();
      const sec = Math.round((i / xSteps - 1) * timeWindowSec);
      cCtx.fillText(sec === 0 ? 'now' : sec + 's', px, padT + plotH + 16);
    }

    if (visible.length < 2) return;

    const toX = (t) => padL + ((t - minT) / timeWindowSec) * plotW;
    const toY = (v) => padT + plotH - ((v - yMin) / (yMax - yMin)) * plotH;

    function plotLine(valFn, color) {
      cCtx.beginPath();
      cCtx.moveTo(toX(visible[0].t), toY(valFn(visible[0])));
      for (let i = 1; i < visible.length; i++) {
        cCtx.lineTo(toX(visible[i].t), toY(valFn(visible[i])));
      }
      cCtx.strokeStyle = color;
      cCtx.lineWidth = 2;
      cCtx.stroke();
    }

    if (activeTab === 'currents') {
      plotLine(p => p.ia, '#fbbf24'); // Phase A (Amber)
      plotLine(p => p.ib, '#06b6d4'); // Phase B (Cyan)
      plotLine(p => p.ic, '#ec4899'); // Phase C (Pink)
    } else {
      plotLine(p => p.mech, '#38bdf8'); // Mechanical (Sky blue)
      plotLine(p => p.elec, '#22c55e'); // Electrical (Green)
    }
  }

  // Animation Loop (60 FPS)
  function animate() {
    drawMotor();
    drawChart();
    requestAnimationFrame(animate);
  }
  requestAnimationFrame(animate);
</script>

</body>
</html>
"""


class WebServerHandler(BaseHTTPRequestHandler):
    collector = None

    def log_message(self, format, *args):
        pass

    def do_GET(self):
        parsed = urlparse(self.path)

        if parsed.path == "/" or parsed.path == "/index.html":
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Cache-Control", "no-cache")
            self.end_headers()
            self.wfile.write(HTML_PAGE.encode("utf-8"))

        elif parsed.path == "/api/data":
            with self.collector.lock:
                data = dict(self.collector.latest_data)
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(json.dumps(data).encode("utf-8"))

        elif parsed.path == "/api/history":
            with self.collector.lock:
                history_copy = list(self.collector.history)
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(json.dumps(history_copy).encode("utf-8"))

        elif parsed.path == "/api/stream":
            self.send_response(200)
            self.send_header("Content-Type", "text/event-stream")
            self.send_header("Cache-Control", "no-cache")
            self.send_header("Connection", "keep-alive")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()

            import queue
            client_queue = queue.Queue(maxsize=100)
            self.collector.add_subscriber(client_queue)

            try:
                with self.collector.lock:
                    init_msg = f"data: {json.dumps(self.collector.latest_data)}\n\n"
                self.wfile.write(init_msg.encode("utf-8"))
                self.wfile.flush()

                while True:
                    try:
                        msg = client_queue.get(timeout=2.0)
                        self.wfile.write(msg.encode("utf-8"))
                        self.wfile.flush()
                    except queue.Empty:
                        self.wfile.write(b": ping\n\n")
                        self.wfile.flush()
            except (BrokenPipeError, ConnectionResetError):
                pass
            finally:
                self.collector.remove_subscriber(client_queue)

        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        parsed = urlparse(self.path)
        if parsed.path == "/api/rate":
            content_len = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(content_len).decode("utf-8")
            try:
                payload = json.loads(body)
                rate = payload.get("rate", DEFAULT_SAMPLE_RATE_HZ)
                self.collector.set_rate(rate)
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"status": "ok", "rate": rate}).encode("utf-8"))
            except Exception:
                self.send_response(400)
                self.end_headers()
        else:
            self.send_response(404)
            self.end_headers()


def main():
    parser = argparse.ArgumentParser(description="7-Pole BLDC Motor Real-Time Visualizer")
    parser.add_argument("--port", type=int, default=DEFAULT_HTTP_PORT, help=f"HTTP Server port (default: {DEFAULT_HTTP_PORT})")
    parser.add_argument("--gdb-port", type=int, default=DEFAULT_GDB_PORT, help=f"GDB Server port (default: {DEFAULT_GDB_PORT})")
    parser.add_argument("--device", default=DEFAULT_DEVICE, help=f"Target device (default: {DEFAULT_DEVICE})")
    parser.add_argument("--speed", type=int, default=DEFAULT_SPEED_KHZ, help=f"SWD speed in kHz (default: {DEFAULT_SPEED_KHZ})")
    parser.add_argument("--rate", type=int, default=DEFAULT_SAMPLE_RATE_HZ, help=f"Sample rate in Hz (default: {DEFAULT_SAMPLE_RATE_HZ})")
    parser.add_argument("--elf", default=DEFAULT_ELF, help=f"Path to firmware ELF (default: {DEFAULT_ELF})")
    parser.add_argument("--no-spawn", action="store_true", help="Do not spawn JLinkGDBServer")
    args = parser.parse_args()

    print("=" * 65)
    print(" 7-Pole Brushless Motor Real-Time Visualizer")
    print("=" * 65)

    symbols = resolve_symbols(args.elf)

    gdb_mgr = None
    if not args.no_spawn:
        gdb_mgr = JLinkGDBServerManager(device=args.device, speed=args.speed, port=args.gdb_port)
        if not gdb_mgr.start():
            print("[Error] Failed to initialize J-Link GDB Server.")
            sys.exit(1)

    gdb_client = GDBClient(port=args.gdb_port)
    if not gdb_client.connect():
        print(f"[Warning] Connecting to GDB server on port {args.gdb_port}...")

    collector = TelemetryCollector(gdb_client, symbols, sample_rate_hz=args.rate)
    collector.start()

    WebServerHandler.collector = collector
    httpd = ThreadingHTTPServer(("0.0.0.0", args.port), WebServerHandler)
    print(f"\n[Ready] Motor Visualizer live at: http://localhost:{args.port}/")
    print(f"[Ready] Pole pairs: {POLE_PAIRS} (14 magnets), Sampling at {args.rate} Hz")
    print("[Ready] Press Ctrl+C to stop.\n")

    def shutdown_signal(sig, frame):
        print("\n[Info] Stopping monitor...")
        collector.stop()
        gdb_client.disconnect()
        if gdb_mgr:
            gdb_mgr.stop()
        try:
            httpd.server_close()
        except Exception:
            pass
        sys.exit(0)

    signal.signal(signal.SIGINT, shutdown_signal)
    signal.signal(signal.SIGTERM, shutdown_signal)

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        shutdown_signal(None, None)


if __name__ == "__main__":
    main()
