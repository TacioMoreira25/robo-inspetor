#pragma once

/*
 * =============================================================================
 * EMPACOTADOR DO PAINEL WEB EM MEMÓRIA FLASH (web_dashboard.h)
 * =============================================================================
 * ARQUIVO GERADO AUTOMATICAMENTE - NÃO EDITAR MANUALMENTE.
 * Origem: web/index.html + web/style.css + web/app.js, unidos pelo script
 * `scripts/sync_html.py`. Armazenado em memória Flash (PROGMEM),
 * preservando a memória RAM dinâmica do microcontrolador.
 */

#include <Arduino.h>

static const char HTML_DASHBOARD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
    <title>UGV Inspetor</title>
    <style>
:root {
    --bg-dark: #090c10;
    --panel-bg: rgba(16, 22, 30, 0.84);
    --panel-border: rgba(56, 189, 248, 0.22);
    --accent: #00f0ff;
    --accent-dim: rgba(0, 240, 255, 0.15);
    --ok: #10b981;
    --warn: #f59e0b;
    --crit: #ef4444;
    --text-main: #f1f5f9;
    --text-muted: #94a3b8;
    --font-ui: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, Helvetica, sans-serif;
    --font-mono: 'JetBrains Mono', 'Fira Code', Consolas, monospace;
}

* { box-sizing: border-box; margin: 0; padding: 0; user-select: none; -webkit-user-select: none; }

body {
    background-color: var(--bg-dark);
    color: var(--text-main);
    font-family: var(--font-ui);
    height: 100vh;
    width: 100vw;
    overflow: hidden;
    display: flex;
    flex-direction: column;
}

/* BARRA SUPERIOR DE STATUS (TOPBAR) */
.topbar {
    height: 48px;
    background: rgba(11, 15, 22, 0.95);
    border-bottom: 1px solid var(--panel-border);
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 0 20px;
    z-index: 20;
    flex-shrink: 0;
}

.brand {
    display: flex;
    align-items: center;
    gap: 12px;
}

.brand-dot {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: var(--ok);
    box-shadow: 0 0 10px var(--ok);
    animation: pulse 2s infinite;
}

@keyframes pulse {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.4; }
}

.brand-title {
    font-family: var(--font-mono);
    font-size: 14px;
    font-weight: 700;
    letter-spacing: 1.5px;
    color: var(--text-main);
}

.brand-sub {
    font-family: var(--font-mono);
    font-size: 11px;
    color: var(--accent);
    background: var(--accent-dim);
    padding: 3px 8px;
    border-radius: 4px;
    border: 1px solid rgba(0, 240, 255, 0.3);
}

.top-metrics {
    display: flex;
    align-items: center;
    gap: 14px;
    font-family: var(--font-mono);
    font-size: 12px;
}

.pill {
    background: rgba(255, 255, 255, 0.04);
    border: 1px solid rgba(255, 255, 255, 0.1);
    padding: 4px 10px;
    border-radius: 4px;
    display: flex;
    align-items: center;
    gap: 6px;
}

.pill-label { color: var(--text-muted); font-size: 10px; }
.pill-val { font-weight: 700; color: var(--text-main); }

/* VIEWPORT CENTRAL (CÂMERA EM TELA CHEIA + HUD FLUTUANTE) */
.cockpit {
    position: relative;
    flex: 1;
    width: 100%;
    height: calc(100vh - 48px);
    background: radial-gradient(circle at center, #131b26 0%, #070a0e 100%);
    display: flex;
    align-items: center;
    justify-content: center;
    overflow: hidden;
}

/* Grade tática sutil ao fundo da câmera */
.cockpit::before {
    content: "";
    position: absolute;
    inset: 0;
    background-image:
        linear-gradient(rgba(0, 240, 255, 0.03) 1px, transparent 1px),
        linear-gradient(90deg, rgba(0, 240, 255, 0.03) 1px, transparent 1px);
    background-size: 40px 40px;
    pointer-events: none;
}

/* MOLDURA CENTRAL DE VÍDEO */
.camera-stage {
    position: relative;
    width: 100%;
    height: 100%;
    display: flex;
    align-items: center;
    justify-content: center;
}

.camera-feed {
    width: 100%;
    height: 100%;
    object-fit: contain;
    z-index: 1;
}

.camera-placeholder {
    position: absolute;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 10px;
    color: var(--text-muted);
    font-family: var(--font-mono);
    text-align: center;
    z-index: 0;
}

.camera-placeholder svg {
    width: 48px;
    height: 48px;
    stroke: var(--accent);
    opacity: 0.5;
}

.btn-reconnect-cam {
    margin-top: 6px;
    background: rgba(0, 240, 255, 0.1);
    border: 1px solid var(--accent);
    color: var(--accent);
    font-family: var(--font-mono);
    font-size: 11px;
    padding: 6px 12px;
    border-radius: 4px;
    cursor: pointer;
    z-index: 5;
}
.btn-reconnect-cam:hover { background: var(--accent); color: #000; }

/* Mira Central HUD (Crosshair) */
.hud-crosshair {
    position: absolute;
    width: 120px;
    height: 120px;
    pointer-events: none;
    z-index: 2;
    opacity: 0.45;
}

/* PAINÉIS FLUTUANTES NOS CANTOS (HUD OVERLAYS) */
.hud-card {
    position: absolute;
    background: var(--panel-bg);
    backdrop-filter: blur(12px);
    -webkit-backdrop-filter: blur(12px);
    border: 1px solid var(--panel-border);
    border-radius: 8px;
    padding: 14px 16px;
    z-index: 10;
    box-shadow: 0 8px 32px rgba(0, 0, 0, 0.55);
}

.hud-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    border-bottom: 1px solid rgba(255, 255, 255, 0.08);
    padding-bottom: 8px;
    margin-bottom: 10px;
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 1px;
    color: var(--accent);
    text-transform: uppercase;
}

/* CANTO SUPERIOR ESQUERDO: MINI-PAINEL DE TERMOGRAFIA */
.hud-thermal {
    top: 16px;
    left: 16px;
    width: 260px;
}

.temp-main-row {
    display: flex;
    align-items: baseline;
    justify-content: space-between;
    margin-bottom: 6px;
}

.temp-obj-label {
    font-size: 11px;
    color: var(--text-muted);
    font-family: var(--font-mono);
}

.temp-obj-val {
    font-family: var(--font-mono);
    font-size: 32px;
    font-weight: 800;
    color: var(--text-main);
    line-height: 1;
}

.temp-bar-track {
    width: 100%;
    height: 6px;
    background: rgba(255, 255, 255, 0.08);
    border-radius: 3px;
    overflow: hidden;
    margin: 8px 0 10px 0;
}

.temp-bar-fill {
    height: 100%;
    width: 25%;
    background: var(--ok);
    transition: width 0.25s ease, background-color 0.25s ease;
}

.temp-sub-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 8px;
    font-family: var(--font-mono);
    font-size: 11px;
    padding-top: 6px;
    border-top: 1px solid rgba(255, 255, 255, 0.06);
}

.sub-item span { display: block; color: var(--text-muted); font-size: 10px; }
.sub-item strong { color: var(--text-main); font-size: 13px; }

/* CANTO SUPERIOR DIREITO: TELEMETRIA DE TRAÇÃO & SISTEMA */
.hud-traction {
    top: 16px;
    right: 16px;
    width: 250px;
}

.motor-gauge {
    margin-bottom: 8px;
}

.motor-gauge-head {
    display: flex;
    justify-content: space-between;
    font-family: var(--font-mono);
    font-size: 11px;
    margin-bottom: 4px;
}

.motor-bar-bg {
    height: 7px;
    background: rgba(255, 255, 255, 0.08);
    border-radius: 3px;
    overflow: hidden;
}

.motor-bar-fill {
    height: 100%;
    width: 0%;
    background: var(--accent);
    transition: width 0.15s ease;
}

/* CANTO INFERIOR ESQUERDO: TECLADO WASD + FARÓIS + AUTO-TESTE */
.hud-controls-left {
    bottom: 16px;
    left: 16px;
    width: 310px;
}

.wasd-container {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    margin-bottom: 12px;
}

.wasd-grid {
    display: grid;
    grid-template-columns: repeat(3, 38px);
    grid-template-rows: repeat(2, 36px);
    gap: 5px;
}

.key-cap {
    background: rgba(255, 255, 255, 0.05);
    border: 1px solid rgba(255, 255, 255, 0.16);
    border-bottom: 3px solid rgba(255, 255, 255, 0.25);
    border-radius: 6px;
    color: var(--text-main);
    font-family: var(--font-mono);
    font-size: 12px;
    font-weight: 700;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    transition: all 0.08s ease;
}

.key-cap:active, .key-cap.pressed {
    background: var(--accent);
    color: #000;
    border-color: var(--accent);
    transform: translateY(2px);
    box-shadow: 0 0 12px rgba(0, 240, 255, 0.5);
}

.key-w { grid-column: 2; grid-row: 1; }
.key-a { grid-column: 1; grid-row: 2; }
.key-s { grid-column: 2; grid-row: 2; }
.key-d { grid-column: 3; grid-row: 2; }

.quick-actions {
    flex: 1;
    display: flex;
    flex-direction: column;
    gap: 6px;
}

.btn-tactical {
    width: 100%;
    background: rgba(255, 255, 255, 0.06);
    border: 1px solid rgba(255, 255, 255, 0.18);
    color: var(--text-main);
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
    padding: 8px 10px;
    border-radius: 6px;
    cursor: pointer;
    display: flex;
    align-items: center;
    justify-content: space-between;
    transition: all 0.15s ease;
}

.btn-tactical:hover {
    border-color: var(--accent);
    background: rgba(0, 240, 255, 0.1);
}

.btn-tactical.active {
    background: var(--ok);
    color: #051b11;
    border-color: var(--ok);
    box-shadow: 0 0 14px rgba(16, 185, 129, 0.45);
}

.btn-stop {
    background: rgba(239, 68, 68, 0.14);
    border-color: rgba(239, 68, 68, 0.4);
    color: #fca5a5;
}
.btn-stop:active, .btn-stop.pressed {
    background: var(--crit);
    color: #fff;
}

.light-control {
    padding-top: 10px;
    border-top: 1px solid rgba(255, 255, 255, 0.08);
}

.light-row {
    display: flex;
    align-items: center;
    gap: 10px;
    margin-top: 6px;
}

input[type=range] {
    -webkit-appearance: none;
    appearance: none;
    flex: 1;
    height: 6px;
    background: rgba(255, 255, 255, 0.12);
    border-radius: 3px;
    outline: none;
}

input[type=range]::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 16px;
    height: 16px;
    border-radius: 50%;
    background: var(--accent);
    cursor: pointer;
    box-shadow: 0 0 8px var(--accent);
}

/* CANTO INFERIOR DIREITO: JOYSTICK ANALÓGICO DE PRECISÃO */
.hud-joystick {
    bottom: 16px;
    right: 16px;
    width: 240px;
    display: flex;
    flex-direction: column;
    align-items: center;
}

.hud-joystick .hud-header { width: 100%; }

canvas#joy {
    background: radial-gradient(circle, rgba(0, 240, 255, 0.06) 0%, rgba(10, 14, 20, 0.9) 100%);
    border: 1px solid rgba(0, 240, 255, 0.3);
    border-radius: 50%;
    touch-action: none;
    cursor: grab;
}
canvas#joy:active { cursor: grabbing; }

.joy-coords {
    margin-top: 8px;
    width: 100%;
    display: flex;
    justify-content: space-between;
    font-family: var(--font-mono);
    font-size: 11px;
    color: var(--text-muted);
}

.status-ok { color: var(--ok) !important; }
.status-warn { color: var(--warn) !important; }
.status-crit { color: var(--crit) !important; }

/* RESPONSIVIDADE PARA CELULAR / TELAS MENORES */
@media (max-width: 900px) {
    body { overflow-y: auto; height: auto; }
    .cockpit {
        flex-direction: column;
        height: auto;
        padding: 12px;
        gap: 12px;
    }
    .camera-stage {
        height: 260px;
        border: 1px solid var(--panel-border);
        border-radius: 8px;
        overflow: hidden;
        background: #000;
    }
    .hud-card {
        position: static;
        width: 100%;
    }
    .top-metrics { display: none; }
}

/* Telemetria de enlace e correções de quebra de linha nos cabeçalhos */
.pill, .pill-val, .pill-label { white-space: nowrap; }
.hud-header, .hud-header * { white-space: nowrap; }
.hud-panel { min-width: 260px; }
.link-lost {
    display: none; position: fixed; top: 64px; left: 50%; transform: translateX(-50%);
    padding: 10px 22px; border: 1px solid var(--crit); border-radius: 6px;
    background: rgba(239, 68, 68, 0.18); color: var(--crit);
    font-family: var(--font-mono); font-size: 13px; letter-spacing: 1px; z-index: 50;
    animation: blink 1s steps(2) infinite;
}
.link-lost.show { display: block; }
@keyframes blink { 50% { opacity: 0.45; } }

/* ---------------------------------------------------------------------------
 * AJUSTES DE LAYOUT (espaçamento dos cabeçalhos, larguras e estado do vídeo)
 * ------------------------------------------------------------------------- */
.hud-header { gap: 12px; }
.hud-header > span:last-child { flex-shrink: 0; }
.hdr-tag { color: var(--text-muted); font-weight: 500; }

.hud-thermal, .hud-traction, .hud-controls-left { width: 290px; }
.hud-joystick { width: 260px; }

/* Leitura de temperatura: remove fundo/brilho herdado indevidamente */
.temp-obj-val {
    background: none;
    box-shadow: none;
    cursor: default;
    font-size: 30px;
    letter-spacing: 0;
    font-variant-numeric: tabular-nums;
}
.sub-item strong { white-space: nowrap; }

/* Vídeo: imagem e mira só aparecem quando o stream está ativo */
.camera-feed { display: none; }
.camera-stage.live .camera-feed { display: block; }
.camera-stage.live .camera-placeholder { display: none; }
.hud-crosshair { display: none; }
.camera-stage.live .hud-crosshair { display: block; }

/* Aviso de perda de conexão com maior contraste */
.link-lost {
    top: 76px; /* abaixo da barra superior, entre os painéis do topo */
    color: #fee2e2;
    background: rgba(127, 29, 29, 0.85);
    font-weight: 700;
}
</style>
</head>
<body>

    <!-- BARRA SUPERIOR DO CONSOLE -->
    <header class="topbar">
        <div class="brand">
            <div class="brand-dot" id="conn_dot"></div>
            <span class="brand-title">UGV INSPETOR</span>
            <span class="brand-sub">MASTER 192.168.4.1</span>
        </div>
        <div class="top-metrics">
            <div class="pill">
                <span class="pill-label">LINK:</span>
                <span class="pill-val" id="link">--</span>
            </div>
            <div class="pill">
                <span class="pill-label">LATÊNCIA:</span>
                <span class="pill-val" id="lat">-- ms</span>
            </div>
            <div class="pill">
                <span class="pill-label">UPTIME:</span>
                <span class="pill-val" id="upt">00:00:00</span>
            </div>
            <div class="pill">
                <span class="pill-label">REDE WI-FI:</span>
                <span class="pill-val">ROBO_INSPETOR_UGV</span>
            </div>
            <div class="pill">
                <span class="pill-label">DISPOSITIVOS:</span>
                <span class="pill-val" id="wcli">0</span>
            </div>
            <div class="pill">
                <span class="pill-label">WATCHDOG (1S):</span>
                <span class="pill-val status-ok" id="wd">STANDBY</span>
            </div>
        </div>
    </header>

    <!-- PALCO CENTRAL DE VÍDEO + OVERLAYS HUD NOS 4 CANTOS -->
    <main class="cockpit">

        <!-- CÂMERA CENTRAL EM DESTAQUE -->
        <div class="camera-stage">
            <div class="camera-placeholder">
                <svg viewBox="0 0 24 24" fill="none" stroke-width="1.5">
                    <path d="M15 10l4.553-2.276A1 1 0 0121 8.618v6.764a1 1 0 01-1.447.894L15 14M5 18h8a2 2 0 002-2V8a2 2 0 00-2-2H5a2 2 0 00-2 2v8a2 2 0 002 2z" stroke-linecap="round" stroke-linejoin="round"/>
                </svg>
                <div>STREAMING ÓPTICO OV2640 (ESP32-CAM SLAVE)</div>
                <div style="font-size:11px;color:#64748b;">Endpoint: http://192.168.4.2:81/stream</div>
                <button class="btn-reconnect-cam" onclick="reloadCamera()">Reconectar Vídeo</button>
            </div>

            <img id="cam_stream" class="camera-feed" src="http://192.168.4.2:81/stream" alt="" onload="onCamLoad()" onerror="onCamError()">

            <!-- Mira Tática Central -->
            <svg class="hud-crosshair" viewBox="0 0 100 100">
                <circle cx="50" cy="50" r="28" stroke="#00f0ff" stroke-width="0.8" fill="none" stroke-dasharray="4 4"/>
                <line x1="50" y1="10" x2="50" y2="32" stroke="#00f0ff" stroke-width="1"/>
                <line x1="50" y1="68" x2="50" y2="90" stroke="#00f0ff" stroke-width="1"/>
                <line x1="10" y1="50" x2="32" y2="50" stroke="#00f0ff" stroke-width="1"/>
                <line x1="68" y1="50" x2="90" y2="50" stroke="#00f0ff" stroke-width="1"/>
                <circle cx="50" cy="50" r="2" fill="#00f0ff"/>
            </svg>
        </div>

        <!-- CANTO SUPERIOR ESQUERDO: MINI-TELA DE TERMOGRAFIA MLX90614 -->
        <section class="hud-card hud-thermal">
            <div class="hud-header">
                <span>TERMOGRAFIA IR</span>
                <span id="i2c_ok" class="status-warn">AGUARDANDO</span>
            </div>
            <div class="temp-main-row">
                <span class="temp-obj-label">ALVO (OBJETO)</span>
                <div><span id="to" class="temp-obj-val">--.-</span> <span style="font-family:var(--font-mono);font-size:14px;color:var(--text-muted);">°C</span></div>
            </div>
            <div class="temp-bar-track">
                <div id="temp_bar" class="temp-bar-fill"></div>
            </div>
            <div class="temp-sub-grid">
                <div class="sub-item">
                    <span>AMBIENTE (CHASSI)</span>
                    <strong><span id="ta">--.-</span> °C</strong>
                </div>
                <div class="sub-item">
                    <span>STATUS TÉRMICO</span>
                    <strong id="ts" class="status-ok">NORMAL</strong>
                </div>
            </div>
        </section>

        <!-- CANTO SUPERIOR DIREITO: DIAGNÓSTICO DE TRAÇÃO E PONTE H -->
        <section class="hud-card hud-traction">
            <div class="hud-header">
                <span>TRAÇÃO L298N</span>
                <span id="mpwm" style="color:var(--text-main);">E:0 | D:0</span>
            </div>
            <div class="motor-gauge">
                <div class="motor-gauge-head">
                    <span style="color:var(--text-muted);">ESTEIRA ESQ (GPIO 26/27)</span>
                    <span id="lbl_mL">0</span>
                </div>
                <div class="motor-bar-bg">
                    <div id="bar_mL" class="motor-bar-fill"></div>
                </div>
            </div>
            <div class="motor-gauge" style="margin-bottom:0;">
                <div class="motor-gauge-head">
                    <span style="color:var(--text-muted);">ESTEIRA DIR (GPIO 14/13)</span>
                    <span id="lbl_mR">0</span>
                </div>
                <div class="motor-bar-bg">
                    <div id="bar_mR" class="motor-bar-fill"></div>
                </div>
            </div>
        </section>

        <!-- CANTO INFERIOR ESQUERDO: TECLADO WASD + FARÓIS + AUTO-TESTE -->
        <section class="hud-card hud-controls-left">
            <div class="hud-header">
                <span>COMANDOS</span>
                <span class="hdr-tag">TECLADO</span>
            </div>

            <div class="wasd-container">
                <div class="wasd-grid">
                    <div class="key-cap key-w" id="key_w" onpointerdown="holdKey('w')" onpointerup="releaseKey('w')">W</div>
                    <div class="key-cap key-a" id="key_a" onpointerdown="holdKey('a')" onpointerup="releaseKey('a')">A</div>
                    <div class="key-cap key-s" id="key_s" onpointerdown="holdKey('s')" onpointerup="releaseKey('s')">S</div>
                    <div class="key-cap key-d" id="key_d" onpointerdown="holdKey('d')" onpointerup="releaseKey('d')">D</div>
                </div>

                <div class="quick-actions">
                    <button class="btn-tactical btn-stop" id="key_space" onclick="emergencyBrake()">
                        <span>FREIO TOTAL</span>
                        <span>[ESPAÇO]</span>
                    </button>
                    <button class="btn-tactical" id="btn_test" onclick="runBenchTest()">
                        <span>AUTO-TESTE BANCADA</span>
                        <span>[T]</span>
                    </button>
                </div>
            </div>

            <div class="light-control">
                <button class="btn-tactical" id="btn_light" onclick="toggleLight()">
                    <span>FARÓIS OLHO DE ÁGUIA (MOSFET)</span>
                    <span id="light_pct">OFF [L]</span>
                </button>
                <div class="light-row">
                    <span style="font-family:var(--font-mono);font-size:10px;color:var(--text-muted);">PWM</span>
                    <input id="rng_light" type="range" min="0" max="255" value="0" oninput="setLight(this.value)">
                </div>
            </div>
        </section>

        <!-- CANTO INFERIOR DIREITO: JOYSTICK VIRTUAL TÁTICO -->
        <section class="hud-card hud-joystick">
            <div class="hud-header">
                <span>JOYSTICK</span>
                <span class="hdr-tag">PROPORCIONAL</span>
            </div>
            <canvas id="joy" width="170" height="170"></canvas>
            <div class="joy-coords">
                <span>EIXO X: <strong id="jx" style="color:var(--text-main);">0</strong></span>
                <span>EIXO Y: <strong id="jy" style="color:var(--text-main);">0</strong></span>
            </div>
        </section>

    </main>

    <div id="link_lost" class="link-lost">CONEXÃO PERDIDA COM O ROBÔ — WATCHDOG ACIONARÁ O FREIO</div>
    <script>
let lightOn = false;
const canvas = document.getElementById('joy');
const ctx = canvas.getContext('2d');
const R = canvas.width / 2;
let dragging = false;
let joyX = 0, joyY = 0;
let driveInterval = null;

// Estado do vídeo: a classe "live" no palco exibe a imagem e a mira
function onCamLoad() {
    document.querySelector('.camera-stage').classList.add('live');
}
function onCamError() {
    document.querySelector('.camera-stage').classList.remove('live');
}

function reloadCamera() {
    const img = document.getElementById('cam_stream');
    img.src = 'http://192.168.4.2:81/stream?t=' + Date.now();
}

function drawJoystick(normX, normY) {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Anéis de radar
    ctx.strokeStyle = 'rgba(0, 240, 255, 0.15)';
    ctx.lineWidth = 1;
    [R * 0.35, R * 0.7].forEach(radius => {
        ctx.beginPath();
        ctx.arc(R, R, radius, 0, Math.PI * 2);
        ctx.stroke();
    });

    // Eixos ortogonais
    ctx.beginPath();
    ctx.moveTo(R, 8); ctx.lineTo(R, canvas.height - 8);
    ctx.moveTo(8, R); ctx.lineTo(canvas.width - 8, R);
    ctx.stroke();

    const px = R + normX * (R - 24);
    const py = R - normY * (R - 24);

    // Linha vetorial do centro até o manche
    ctx.beginPath();
    ctx.moveTo(R, R);
    ctx.lineTo(px, py);
    ctx.strokeStyle = 'rgba(0, 240, 255, 0.6)';
    ctx.lineWidth = 2;
    ctx.stroke();

    // Manche (Knob)
    ctx.beginPath();
    ctx.arc(px, py, 20, 0, Math.PI * 2);
    ctx.fillStyle = (normX !== 0 || normY !== 0) ? '#00f0ff' : 'rgba(0, 240, 255, 0.35)';
    ctx.fill();
    ctx.strokeStyle = '#ffffff';
    ctx.lineWidth = 1.5;
    ctx.stroke();
}
drawJoystick(0, 0);

function sendCmd(x, y, lightVal) {
    document.getElementById('jx').innerText = x;
    document.getElementById('jy').innerText = y;
    let url = '/cmd?x=' + x + '&y=' + y;
    if (lightVal !== undefined) url += '&light=' + lightVal;
    fetch(url).catch(() => {});
}

function startContinuousSend() {
    if (!driveInterval) {
        driveInterval = setInterval(() => {
            if (joyX !== 0 || joyY !== 0) sendCmd(joyX, joyY);
        }, 150);
    }
}

function stopContinuousSend() {
    if (driveInterval) {
        clearInterval(driveInterval);
        driveInterval = null;
    }
}

function updatePos(clientX, clientY) {
    const rect = canvas.getBoundingClientRect();
    const dx = clientX - (rect.left + R);
    const dy = (rect.top + R) - clientY;
    const maxDist = R - 24;
    const dist = Math.min(maxDist, Math.hypot(dx, dy));
    const angle = Math.atan2(dy, dx);
    const normX = (dist * Math.cos(angle)) / maxDist;
    const normY = (dist * Math.sin(angle)) / maxDist;

    joyX = Math.round(normX * 255);
    joyY = Math.round(normY * 255);
    drawJoystick(normX, normY);
    sendCmd(joyX, joyY);
}

canvas.addEventListener('pointerdown', e => {
    dragging = true;
    updatePos(e.clientX, e.clientY);
    startContinuousSend();
});
window.addEventListener('pointermove', e => {
    if (dragging) updatePos(e.clientX, e.clientY);
});
window.addEventListener('pointerup', () => {
    if (dragging) {
        dragging = false;
        stopContinuousSend();
        joyX = 0; joyY = 0;
        drawJoystick(0, 0);
        sendCmd(0, 0);
    }
});

function updateLightUI(val) {
    lightOn = (val > 0);
    document.getElementById('rng_light').value = val;
    const btn = document.getElementById('btn_light');
    const pct = Math.round((val / 255) * 100);
    btn.className = lightOn ? 'btn-tactical active' : 'btn-tactical';
    document.getElementById('light_pct').innerText = lightOn ? (pct + '% [L]') : 'OFF [L]';
}

function toggleLight() {
    const val = lightOn ? 0 : 255;
    updateLightUI(val);
    sendCmd(joyX, joyY, val);
}

function setLight(val) {
    updateLightUI(parseInt(val, 10));
    sendCmd(joyX, joyY, val);
}

function runBenchTest() {
    const btn = document.getElementById('btn_test');
    btn.classList.add('active');
    setTimeout(() => btn.classList.remove('active'), 2200);
    fetch('/test').catch(() => {});
}

function emergencyBrake() {
    stopContinuousSend();
    joyX = 0; joyY = 0;
    drawJoystick(0, 0);
    sendCmd(0, 0);
}

function holdKey(k) {
    const el = document.getElementById('key_' + k);
    if (el) el.classList.add('pressed');
    if (k === 'w') { joyX = 0; joyY = 200; }
    else if (k === 's') { joyX = 0; joyY = -200; }
    else if (k === 'a') { joyX = -180; joyY = 0; }
    else if (k === 'd') { joyX = 180; joyY = 0; }
    drawJoystick(joyX / 255, joyY / 255);
    sendCmd(joyX, joyY);
    startContinuousSend();
}

function releaseKey(k) {
    const el = document.getElementById('key_' + k);
    if (el) el.classList.remove('pressed');
    if (['w', 'a', 's', 'd'].includes(k)) {
        emergencyBrake();
    }
}

window.addEventListener('keydown', e => {
    if (e.repeat) return;
    const k = e.key.toLowerCase();
    if (['w', 'a', 's', 'd'].includes(k)) holdKey(k);
    else if (k === ' ') {
        document.getElementById('key_space').classList.add('pressed');
        emergencyBrake();
    }
    else if (k === 'l') toggleLight();
    else if (k === 't') runBenchTest();
});

window.addEventListener('keyup', e => {
    const k = e.key.toLowerCase();
    if (['w', 'a', 's', 'd'].includes(k)) releaseKey(k);
    else if (k === ' ') document.getElementById('key_space').classList.remove('pressed');
});

// Telemetria de enlace: latência (RTT do polling), perda de conexão e uptime
let failCount = 0;
function fmtUptime(ms) {
    const s = Math.floor(ms / 1000);
    const p = n => String(n).padStart(2, '0');
    return p(Math.floor(s / 3600)) + ':' + p(Math.floor(s / 60) % 60) + ':' + p(s % 60);
}
function setLink(ok, rtt) {
    const link = document.getElementById('link');
    const lat = document.getElementById('lat');
    document.getElementById('link_lost').classList.toggle('show', !ok);
    if (!ok) {
        link.innerText = 'OFFLINE'; link.className = 'pill-val status-crit';
        lat.innerText = '-- ms'; lat.className = 'pill-val';
        return;
    }
    link.innerText = 'ONLINE'; link.className = 'pill-val status-ok';
    lat.innerText = rtt + ' ms';
    lat.className = 'pill-val ' + (rtt < 80 ? 'status-ok' : rtt < 200 ? 'status-warn' : 'status-crit');
}

setInterval(() => {
    const t0 = performance.now();
    fetch('/temp', { signal: AbortSignal.timeout(1500) }).then(r => r.json()).then(d => {
        failCount = 0;
        setLink(true, Math.round(performance.now() - t0));
        if (d.uptime !== undefined) document.getElementById('upt').innerText = fmtUptime(d.uptime);
        const i2cEl = document.getElementById('i2c_ok');
        if (d.valid) {
            i2cEl.innerText = 'ONLINE (0x5A)';
            i2cEl.className = 'status-ok';
        } else {
            i2cEl.innerText = 'SEM SINAL I2C';
            i2cEl.className = 'status-crit';
        }

        document.getElementById('to').innerText = d.obj.toFixed(1);
        document.getElementById('ta').innerText = d.amb.toFixed(1);

        // Barra térmica proporcional (0 a 100°C)
        const tempBar = document.getElementById('temp_bar');
        const pctTemp = Math.max(5, Math.min(100, d.obj));
        tempBar.style.width = pctTemp + '%';

        const ts = document.getElementById('ts');
        if (d.obj >= 70.0) {
            ts.innerText = 'CRÍTICO (>=70°C)';
            ts.className = 'status-crit';
            tempBar.style.backgroundColor = 'var(--crit)';
        } else if (d.obj >= 50.0) {
            ts.innerText = 'ALERTA (>=50°C)';
            ts.className = 'status-warn';
            tempBar.style.backgroundColor = 'var(--warn)';
        } else {
            ts.innerText = 'NORMAL';
            ts.className = 'status-ok';
            tempBar.style.backgroundColor = 'var(--ok)';
        }

        // Barras de PWM dos motores
        const mL = d.mL || 0;
        const mR = d.mR || 0;
        document.getElementById('mpwm').innerText = 'E:' + mL + ' | D:' + mR;
        document.getElementById('lbl_mL').innerText = mL;
        document.getElementById('lbl_mR').innerText = mR;
        document.getElementById('bar_mL').style.width = (Math.abs(mL) / 255 * 100) + '%';
        document.getElementById('bar_mR').style.width = (Math.abs(mR) / 255 * 100) + '%';

        document.getElementById('wcli').innerText = d.clients + (d.clients >= 2 ? ' (CAM+OP)' : '');

        const wd = document.getElementById('wd');
        if (d.watchdog) {
            wd.innerText = 'FREIO ATIVO';
            wd.className = 'pill-val status-warn';
        } else {
            wd.innerText = 'EM TRAÇÃO';
            wd.className = 'pill-val status-ok';
        }
    }).catch(() => {
        if (++failCount >= 3) setLink(false);
    });
}, 500);
</script>
</body>
</html>

)rawliteral";
