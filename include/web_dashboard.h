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

.cam-config-bar {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 3px 8px;
    background: rgba(255, 255, 255, 0.04);
    border: 1px solid var(--panel-border);
    border-radius: 4px;
}

.cam-config-lbl {
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
    color: var(--accent);
    white-space: nowrap;
}

.cam-config-bar input {
    width: 155px;
    padding: 3px 6px;
    border-radius: 3px;
    border: 1px solid rgba(255, 255, 255, 0.15);
    background: rgba(9, 12, 16, 0.85);
    color: var(--text-main);
    font-family: var(--font-mono);
    font-size: 11px;
    user-select: text;
    -webkit-user-select: text;
}

.cam-config-bar input:focus {
    outline: none;
    border-color: var(--accent);
}

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
    padding: 10px 12px;
    z-index: 10;
    box-shadow: 0 8px 32px rgba(0, 0, 0, 0.55);
}

.hud-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    border-bottom: 1px solid rgba(255, 255, 255, 0.08);
    padding-bottom: 6px;
    margin-bottom: 8px;
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 1px;
    color: var(--accent);
    text-transform: uppercase;
}

/* CANTO SUPERIOR ESQUERDO: MINI-PAINEL DE TERMOGRAFIA */
.hud-thermal {
    top: 10px;
    left: 10px;
    width: 220px;
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
    top: 10px;
    right: 10px;
    width: 220px;
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
    bottom: 10px;
    left: 10px;
    width: 235px;
}

.wasd-container {
    display: flex;
    flex-direction: column;
    align-items: stretch;
    gap: 8px;
    margin-bottom: 10px;
}

.wasd-grid {
    display: grid;
    grid-template-columns: repeat(3, 34px);
    grid-template-rows: repeat(2, 32px);
    gap: 4px;
    justify-content: center;
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
    bottom: 10px;
    right: 10px;
    width: 210px;
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
            <span class="brand-sub" id="esp_ip_lbl">UGV.LOCAL</span>
        </div>
        <div class="cam-config-bar">
            <span class="cam-config-lbl">IP CELULAR</span>
            <input id="cam_ip_input" type="text" placeholder="Ex: 192.168.43.1" onkeydown="if(event.key==='Enter') updateCameraUrl()">
            <button class="btn-reconnect-cam" style="margin-top:0;padding:3px 8px;font-size:10px;" onclick="updateCameraUrl()">Conectar</button>
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
                <span class="pill-label">HOTSPOT:</span>
                <span class="pill-val">robo_ugv</span>
            </div>
            <div class="pill">
                <span class="pill-label">SINAL ESP:</span>
                <span class="pill-val" id="wcli">-- dBm</span>
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
                <div>VÍDEO AO VIVO (CELULAR ANDROID / IP WEBCAM)</div>
                <div id="cam_endpoint_lbl" style="font-size:11px;color:#64748b;">Endpoint: http://192.168.4.2:8080/video</div>
                <button class="btn-reconnect-cam" onclick="reloadCamera()">Reconectar Vídeo</button>
            </div>

            <img id="cam_stream" class="camera-feed" src="" alt="" onload="onCamLoad()" onerror="onCamError()">

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
                <span class="hdr-tag" id="joy_mode">PROPORCIONAL</span>
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

const DEFAULT_CAM_ADDR = '192.168.43.1:8080/video';
let currentCamUrl = 'http://' + DEFAULT_CAM_ADDR;
let camAutoDetected = false;
const API_BASE = (location.protocol === 'file:') ? 'http://ugv.local' : '';

function normalizeCamUrl(raw) {
    let v = (raw || '').trim();
    if (!v) v = DEFAULT_CAM_ADDR;
    if (!/^https?:\/\//i.test(v)) v = 'http://' + v;
    // Se o usuário digitou só o IP (ex: 192.168.43.1), completa porta e rota padrão do IP Webcam
    if (/^https?:\/\/[^/:]+$/i.test(v)) v += ':8080/video';
    else if (/^https?:\/\/[^/]+:\d+$/i.test(v)) v += '/video';
    return v;
}

function applyCameraUrl(raw, bustCache) {
    currentCamUrl = normalizeCamUrl(raw);
    const img = document.getElementById('cam_stream');
    const lbl = document.getElementById('cam_endpoint_lbl');
    if (lbl) lbl.innerText = 'Endpoint: ' + currentCamUrl;
    if (img) img.src = currentCamUrl + (bustCache ? ('?t=' + Date.now()) : '');
}

function updateCameraUrl() {
    const input = document.getElementById('cam_ip_input');
    const raw = input ? input.value.trim() : '';
    const finalUrl = normalizeCamUrl(raw || DEFAULT_CAM_ADDR);
    camAutoDetected = true;
    try { localStorage.setItem('ugv_cam_url', finalUrl); } catch (_) {}
    if (input) input.value = finalUrl.replace(/^https?:\/\//i, '');
    applyCameraUrl(finalUrl, true);
}

window.addEventListener('DOMContentLoaded', () => {
    let saved = '';
    try { saved = localStorage.getItem('ugv_cam_url') || ''; } catch (_) {}
    if (saved) camAutoDetected = true;
    const addr = saved || DEFAULT_CAM_ADDR;
    const input = document.getElementById('cam_ip_input');
    if (input) input.value = addr.replace(/^https?:\/\//i, '');
    applyCameraUrl(addr, false);
});

// Estado do vídeo: a classe "live" no palco exibe a imagem e a mira
function onCamLoad() {
    document.querySelector('.camera-stage').classList.add('live');
}
function onCamError() {
    document.querySelector('.camera-stage').classList.remove('live');
}

function reloadCamera() {
    applyCameraUrl(currentCamUrl, true);
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

let cmdInFlight = false;
let pendingCmd = null;
let lastSentX = 0, lastSentY = 0;

function dispatchCmd(x, y, lightVal) {
    cmdInFlight = true;
    lastSentX = x;
    lastSentY = y;
    let url = API_BASE + '/cmd?x=' + x + '&y=' + y;
    if (lightVal !== undefined) url += '&light=' + lightVal;
    const opts = (typeof AbortSignal !== 'undefined' && AbortSignal.timeout)
        ? { signal: AbortSignal.timeout(300) } : undefined;
    fetch(url, opts).catch(() => {}).finally(() => {
        cmdInFlight = false;
        if (pendingCmd) {
            const next = pendingCmd;
            pendingCmd = null;
            dispatchCmd(next.x, next.y, next.lightVal);
        }
    });
}

function sendCmd(x, y, lightVal) {
    document.getElementById('jx').innerText = x;
    document.getElementById('jy').innerText = y;
    if (cmdInFlight) {
        pendingCmd = {
            x,
            y,
            lightVal: (lightVal !== undefined) ? lightVal : (pendingCmd ? pendingCmd.lightVal : undefined)
        };
        return;
    }
    dispatchCmd(x, y, lightVal);
}

function startContinuousSend() {
    if (!driveInterval) {
        driveInterval = setInterval(() => {
            if (joyX !== 0 || joyY !== 0) sendCmd(joyX, joyY);
        }, 120);
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

    const newX = Math.round(normX * 255);
    const newY = Math.round(normY * 255);
    drawJoystick(normX, normY);
    if (newX !== joyX || newY !== joyY) {
        joyX = newX;
        joyY = newY;
        sendCmd(joyX, joyY);
    }
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
    fetch(API_BASE + '/test').catch(() => {});
}

function emergencyBrake() {
    pressedKeys.clear();
    ['w', 'a', 's', 'd'].forEach(k => {
        const el = document.getElementById('key_' + k);
        if (el) el.classList.remove('pressed');
    });
    stopContinuousSend();
    joyX = 0; joyY = 0;
    drawJoystick(0, 0);
    sendCmd(0, 0);
}

// Suporte multi-tecla (W+A / W+D faz curva andando sem parar o robô)
const pressedKeys = new Set();

function applyKeysDrive() {
    const fwd = pressedKeys.has('w') ? 1 : 0;
    const rev = pressedKeys.has('s') ? 1 : 0;
    const left = pressedKeys.has('a') ? 1 : 0;
    const right = pressedKeys.has('d') ? 1 : 0;

    const dirY = fwd - rev;
    const dirX = right - left;

    if (dirY === 0 && dirX === 0) {
        stopContinuousSend();
        joyX = 0; joyY = 0;
        drawJoystick(0, 0);
        sendCmd(0, 0);
        return;
    }

    joyY = dirY * 255;
    // Em movimento (W/S + A/D), usa X=140 para curva em arco suave sem parar/inverter a roda interna
    joyX = (dirY !== 0) ? (dirX * 140) : (dirX * 255);
    drawJoystick(joyX / 255, joyY / 255);
    sendCmd(joyX, joyY);
    startContinuousSend();
}

function holdKey(k) {
    pressedKeys.add(k);
    const el = document.getElementById('key_' + k);
    if (el) el.classList.add('pressed');
    applyKeysDrive();
}

function releaseKey(k) {
    pressedKeys.delete(k);
    const el = document.getElementById('key_' + k);
    if (el) el.classList.remove('pressed');
    applyKeysDrive();
}

const KEY_MAP = {
    'w': 'w', 'arrowup': 'w',
    's': 's', 'arrowdown': 's',
    'a': 'a', 'arrowleft': 'a',
    'd': 'd', 'arrowright': 'd'
};

window.addEventListener('keydown', e => {
    if (document.activeElement && document.activeElement.tagName === 'INPUT') return;
    if (e.repeat) return;
    const raw = e.key.toLowerCase();
    if (KEY_MAP[raw]) {
        if (raw.startsWith('arrow')) e.preventDefault();
        const jm = document.getElementById('joy_mode');
        if (jm) jm.innerText = 'TECLA: ' + raw.toUpperCase();
        holdKey(KEY_MAP[raw]);
    } else if (raw === ' ') {
        e.preventDefault();
        document.getElementById('key_space').classList.add('pressed');
        emergencyBrake();
    }
    else if (raw === 'l') toggleLight();
    else if (raw === 't') runBenchTest();
});

window.addEventListener('keyup', e => {
    if (document.activeElement && document.activeElement.tagName === 'INPUT') return;
    const raw = e.key.toLowerCase();
    if (KEY_MAP[raw]) releaseKey(KEY_MAP[raw]);
    else if (raw === ' ') document.getElementById('key_space').classList.remove('pressed');
});

// Suporte nativo a Controle / Gamepad (USB, Dongle 2.4G ou Bluetooth no navegador)
// Analógico direito e eixo Y do analógico esquerdo ficam 100% desativados.
let gpActive = false;
let gpWasZero = true;
let gpPrevBtnBrake = false;
let gpPrevBtnLight = false;
let gpWasMovingForward = false;
let gpLastForwardMs = 0;
let gpLtWasPressed = false;
let gpLtBrakeUntil = 0;

window.addEventListener('gamepadconnected', () => {
    const tag = document.getElementById('joy_mode');
    if (tag) tag.innerText = 'CONTROLE OK';
});

window.addEventListener('gamepaddisconnected', () => {
    const tag = document.getElementById('joy_mode');
    if (tag) tag.innerText = 'PROPORCIONAL';
});

setInterval(() => {
    const tag = document.getElementById('joy_mode');
    if (!navigator.getGamepads) {
        if (tag && tag.innerText !== 'GP BLOQ (HTTP)') tag.innerText = 'GP BLOQ (HTTP)';
        return;
    }
    const pads = navigator.getGamepads();
    let pad = null;
    for (let i = 0; i < pads.length; i++) {
        const p = pads[i];
        if (!p || !p.connected) continue;
        const hasInput = p.buttons.some(b => b && (b.pressed || b.value > 0.15)) ||
                         p.axes.some(a => Math.abs(a || 0) > 0.2);
        if (hasInput) { pad = p; break; }
        if (!pad || (pad.mapping !== 'standard' && p.mapping === 'standard')) pad = p;
    }
    if (!pad) return;

    const modeLbl = (pad.mapping === 'standard') ? 'CONTROLE [STD]' : 'CONTROLE [RAW]';
    if (tag && !gpActive && tag.innerText !== modeLbl) tag.innerText = modeLbl;

    const isMachenikeHid = /0079-181c/i.test(pad.id || '') && pad.mapping !== 'standard' && pad.axes.length >= 8;

    // Botão 4 (LB/L1) = Freio total (mantém parado enquanto segurar); Botão Y/Triângulo = Faróis
    // (No Machenike 0079-181c, X e Y vêm invertidos: b[3]=X e b[2]=Y)
    const btnBrake = !!(pad.buttons[4] && pad.buttons[4].pressed);
    if (btnBrake && !gpPrevBtnBrake) emergencyBrake();
    gpPrevBtnBrake = btnBrake;
    if (btnBrake) {
        if (tag) tag.innerText = 'CONTROLE [FREIO]';
        return;
    }

    const lightBtnIdx = isMachenikeHid ? 2 : 3;
    const btnLight = !!(pad.buttons[lightBtnIdx] && pad.buttons[lightBtnIdx].pressed);
    if (btnLight && !gpPrevBtnLight) toggleLight();
    gpPrevBtnLight = btnLight;

    // Não sobrescreve se o usuário estiver arrastando na tela ou segurando WASD/Setas
    if (dragging || pressedKeys.size > 0) return;

    const deadzone = 0.15;

    // 1. Acelerador / Freio+Ré nos gatilhos (RT/LT) OU nas Setinhas Cima/Baixo (D-Pad)
    const nowMs = Date.now();
    let rt = 0, lt = 0;
    if (isMachenikeHid) {
        // No Machenike G5Pro (0079-181c), o analógico direito vaza em b[6]/b[7] com ax[6]/ax[7]=0.
        // RT real leva ax[6]>0.15 e ax[7]<-0.15; LT real leva ax[6]<-0.15 e ax[7]>0.15.
        const ax6 = pad.axes[6] || 0;
        const ax7 = pad.axes[7] || 0;
        if (ax6 > 0.15 && ax7 < -0.15) {
            const b7 = pad.buttons[7] ? pad.buttons[7].value : 0;
            rt = b7 > 0.1 ? b7 : ax6;
        } else if (ax6 < -0.15 && ax7 > 0.15) {
            const b6 = pad.buttons[6] ? pad.buttons[6].value : 0;
            lt = b6 > 0.1 ? b6 : -ax6;
        }
    } else {
        rt = pad.buttons[7] ? (pad.buttons[7].value || (pad.buttons[7].pressed ? 1 : 0)) : 0;
        lt = pad.buttons[6] ? (pad.buttons[6].value || (pad.buttons[6].pressed ? 1 : 0)) : 0;
        if (rt < 0.12) rt = 0;
        if (lt < 0.12) lt = 0;
    }
    const dpadUp = (pad.buttons[12] && pad.buttons[12].pressed) ? 1 : 0;
    const dpadDown = (pad.buttons[13] && pad.buttons[13].pressed) ? 1 : 0;

    let rawY = 0;
    if (dpadUp || dpadDown) {
        rawY = dpadUp - dpadDown;
        if (rawY > 0) { gpWasMovingForward = true; gpLastForwardMs = nowMs; }
        gpLtWasPressed = false;
        gpLtBrakeUntil = 0;
    } else if (rt > 0.05) {
        rawY = rt - lt;
        gpWasMovingForward = true;
        gpLastForwardMs = nowMs;
        gpLtWasPressed = (lt > 0.05);
        gpLtBrakeUntil = 0;
    } else if (lt > 0.05) {
        if (!gpLtWasPressed) {
            gpLtWasPressed = true;
            if (gpWasMovingForward) {
                gpLtBrakeUntil = nowMs + 320;
                gpWasMovingForward = false;
            } else {
                gpLtBrakeUntil = 0;
            }
        }
        rawY = (nowMs < gpLtBrakeUntil) ? 0 : -lt;
    } else {
        gpLtWasPressed = false;
        gpLtBrakeUntil = 0;
        if (gpWasMovingForward && (nowMs - gpLastForwardMs) > 500) {
            gpWasMovingForward = false;
        }
    }

    // 2. Direção (Volante): APENAS Analógico Esquerdo X (axes[0]) ou D-Pad Esquerda/Direita
    const dpadLeft = (pad.buttons[14] && pad.buttons[14].pressed) ? 1 : 0;
    const dpadRight = (pad.buttons[15] && pad.buttons[15].pressed) ? 1 : 0;
    let rawX = 0;
    if (dpadLeft || dpadRight) {
        rawX = dpadRight - dpadLeft;
    } else {
        rawX = pad.axes[0] || 0;
        if (Math.abs(rawX) < deadzone) rawX = 0;
    }

    // Em movimento (acelerando/ré), curva em arco suave (x até 150) para não parar a roda interna; parado, gira no eixo (255)
    const maxTurn = (Math.abs(rawY) > 0.1) ? 150 : 255;
    const gx = Math.round(rawX * maxTurn);
    const gy = Math.round(rawY * 255);

    if (gx !== 0 || gy !== 0) {
        gpActive = true;
        gpWasZero = false;
        joyX = gx;
        joyY = gy;
        if (tag) {
            const src = [];
            if (dpadUp || dpadDown || dpadLeft || dpadRight) src.push('DP');
            if (rt > 0.05) src.push('RT');
            if (lt > 0.05) src.push('LT');
            if (Math.abs(pad.axes[0] || 0) >= deadzone && !dpadLeft && !dpadRight) src.push('AX0');
            tag.innerText = (pad.mapping === 'standard' ? 'STD ' : 'RAW ') + src.join('+');
        }
        drawJoystick(gx / 255, rawY);
        sendCmd(joyX, joyY);
    } else if (gpActive && !gpWasZero) {
        gpWasZero = true;
        gpActive = false;
        joyX = 0; joyY = 0;
        if (tag) tag.innerText = modeLbl;
        drawJoystick(0, 0);
        sendCmd(0, 0);
    }
}, 80);

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

let tempInFlight = false;
setInterval(() => {
    if (tempInFlight) return;
    tempInFlight = true;
    const t0 = performance.now();
    fetch(API_BASE + '/temp', { signal: AbortSignal.timeout(1200) }).then(r => r.json()).then(d => {
        failCount = 0;
        setLink(true, Math.round(performance.now() - t0));
        if (d.uptime !== undefined) document.getElementById('upt').innerText = fmtUptime(d.uptime);
        if (d.ip) {
            const ipLbl = document.getElementById('esp_ip_lbl');
            if (ipLbl) ipLbl.innerText = 'ESP ' + d.ip;
        }
        // No Hotspot do Galaxy A20, o próprio celular é o Gateway (d.gw, ex: 192.168.43.1): preenche a câmera automaticamente
        if (d.gw && d.gw !== '0.0.0.0' && !camAutoDetected) {
            camAutoDetected = true;
            const input = document.getElementById('cam_ip_input');
            const gwCam = d.gw + ':8080/video';
            if (input) input.value = gwCam;
            applyCameraUrl(gwCam, false);
        }
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

        document.getElementById('wcli').innerText = (d.rssi !== undefined) ? (d.rssi + ' dBm') : (d.clients || 0);

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
    }).finally(() => {
        tempInFlight = false;
    });
}, 500);
</script>
</body>
</html>

)rawliteral";
