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
