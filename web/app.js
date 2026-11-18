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

    const isMachenikeHid = /0079-181c/i.test(pad.id || '') && pad.axes.length >= 8;

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
