#include "telemetry.h"
#include "config.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

static WebServer server(WEB_SERVER_PORT);
static WebServer streamServer(STREAM_SERVER_PORT);

static MotorController *g_motors = nullptr;
static CameraStream *g_camera = nullptr;
static uint8_t g_headlight_val = 0;
static RobotTelemetryState g_latest_state;

static const char HTML_DASHBOARD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>Robô Inspetor UGV - ESP32 DevKit V1 Master</title>
    <style>
        body { font-family: monospace; background: #101216; color: #00ff66; padding: 15px; margin: 0; user-select: none; }
        h1 { color: #ffffff; text-align: center; margin-bottom: 5px; font-size: 22px; }
        .sub { text-align: center; color: #888; margin-bottom: 15px; font-size: 13px; }
        .grid { display: flex; flex-wrap: wrap; gap: 18px; justify-content: center; }
        .card { background: #1a1d24; border: 1px solid #333; padding: 15px; border-radius: 10px; width: 320px; box-sizing: border-box; }
        .card h3 { margin-top: 0; color: #fff; border-bottom: 1px solid #333; padding-bottom: 8px; }
        .video-box { width: 100%; height: 220px; background: #000; border: 1px solid #444; border-radius: 6px; display: flex; align-items: center; justify-content: center; overflow: hidden; }
        .video-box img { width: 100%; height: 100%; object-fit: cover; }
        .telemetry-val { font-size: 20px; font-weight: bold; color: #fff; }
        .alert-warn { color: #ffcc00; font-weight: bold; }
        .alert-crit { color: #ff3333; font-weight: bold; }
        .joy-wrap { display: flex; flex-direction: column; align-items: center; margin-top: 8px; }
        canvas { background: #121418; border: 2px solid #444; border-radius: 50%; touch-action: none; cursor: crosshair; }
        .btn-row { display: flex; gap: 10px; margin-top: 10px; width: 100%; }
        button { flex: 1; background: #282c34; color: #fff; border: 1px solid #555; padding: 12px; font-size: 14px; border-radius: 6px; cursor: pointer; font-family: monospace; }
        button:active, button.active { background: #00ff66; color: #000; font-weight: bold; }
    </style>
</head>
<body>
    <h1>ROBO INSPETOR TERRESTRE (UGV)</h1>
    <div class="sub">Wi-Fi: ROBO_INSPETOR_UGV | Master: 192.168.4.1 | Camera Slave: 192.168.4.2:81</div>
    <div class="grid">
        <div class="card">
            <h3>Camera OV2640 (ESP32-CAM)</h3>
            <div class="video-box">
                <img id="cam_stream" src="http://192.168.4.2:81/stream" alt="Aguardando Stream 192.168.4.2:81...">
            </div>
        </div>
        <div class="card">
            <h3>Termografia (MLX90614 I2C)</h3>
            <p>Temp. Objeto: <span id="to" class="telemetry-val">--</span> °C</p>
            <p>Temp. Ambiente: <span id="ta" class="telemetry-val">--</span> °C</p>
            <p>Status Termico: <span id="ts" class="telemetry-val">NORMAL</span></p>
            <p>Failsafe (1s): <span id="wd" class="telemetry-val">OK</span></p>
            <hr style="border:0;border-top:1px solid #333;margin:12px 0;">
            <h3>Farois Olho de Aguia (GPIO 25)</h3>
            <div class="btn-row">
                <button id="btn_light" onclick="toggleLight()">Ligar / Desligar Farol (L)</button>
            </div>
            <input id="rng_light" type="range" min="0" max="255" value="0" style="width:100%;margin-top:10px;" oninput="setLight(this.value)">
        </div>
        <div class="card">
            <h3>Joystick & Teclas (W,A,S,D)</h3>
            <div class="joy-wrap">
                <canvas id="joy" width="200" height="200"></canvas>
                <p style="margin:8px 0 0 0;color:#aaa;font-size:12px;">X: <span id="jx">0</span> | Y: <span id="jy">0</span></p>
            </div>
        </div>
    </div>
    <script>
        let lightOn = false;
        const canvas = document.getElementById('joy');
        const ctx = canvas.getContext('2d');
        let dragging = false;
        let joyX = 0, joyY = 0;

        function drawJoystick(x, y) {
            ctx.clearRect(0, 0, 200, 200);
            ctx.strokeStyle = '#333';
            ctx.beginPath(); ctx.moveTo(100, 0); ctx.lineTo(100, 200); ctx.moveTo(0, 100); ctx.lineTo(200, 100); ctx.stroke();
            ctx.beginPath(); ctx.arc(100 + x * 0.7, 100 - y * 0.7, 25, 0, Math.PI * 2);
            ctx.fillStyle = '#00ff66'; ctx.fill();
        }
        drawJoystick(0, 0);

        function sendCmd(x, y, lightVal) {
            document.getElementById('jx').innerText = x;
            document.getElementById('jy').innerText = y;
            let url = '/cmd?x=' + x + '&y=' + y;
            if (lightVal !== undefined) url += '&light=' + lightVal;
            fetch(url).catch(() => {});
        }

        function updatePos(clientX, clientY) {
            const rect = canvas.getBoundingClientRect();
            let dx = clientX - (rect.left + 100);
            let dy = (rect.top + 100) - clientY;
            let dist = Math.min(100, Math.hypot(dx, dy));
            let angle = Math.atan2(dy, dx);
            joyX = Math.round((dist * Math.cos(angle)) * 2.55);
            joyY = Math.round((dist * Math.sin(angle)) * 2.55);
            drawJoystick(joyX / 2.55, joyY / 2.55);
            sendCmd(joyX, joyY);
        }

        canvas.addEventListener('pointerdown', e => { dragging = true; updatePos(e.clientX, e.clientY); });
        window.addEventListener('pointermove', e => { if (dragging) updatePos(e.clientX, e.clientY); });
        window.addEventListener('pointerup', () => {
            if (dragging) { dragging = false; joyX = 0; joyY = 0; drawJoystick(0, 0); sendCmd(0, 0); }
        });

        function toggleLight() {
            lightOn = !lightOn;
            let val = lightOn ? 255 : 0;
            document.getElementById('rng_light').value = val;
            document.getElementById('btn_light').className = lightOn ? 'active' : '';
            sendCmd(joyX, joyY, val);
        }

        function setLight(val) {
            lightOn = (val > 0);
            document.getElementById('btn_light').className = lightOn ? 'active' : '';
            sendCmd(joyX, joyY, val);
        }

        window.addEventListener('keydown', e => {
            if (e.repeat) return;
            const k = e.key.toLowerCase();
            if (k === 'w') { joyX = 0; joyY = 200; }
            else if (k === 's') { joyX = 0; joyY = -200; }
            else if (k === 'a') { joyX = -180; joyY = 0; }
            else if (k === 'd') { joyX = 180; joyY = 0; }
            else if (k === ' ') { joyX = 0; joyY = 0; }
            else if (k === 'l') { toggleLight(); return; }
            else return;
            drawJoystick(joyX / 2.55, joyY / 2.55);
            sendCmd(joyX, joyY);
        });

        window.addEventListener('keyup', e => {
            const k = e.key.toLowerCase();
            if (['w', 'a', 's', 'd'].includes(k)) {
                joyX = 0; joyY = 0; drawJoystick(0, 0); sendCmd(0, 0);
            }
        });

        setInterval(() => {
            fetch('/temp').then(r => r.json()).then(d => {
                document.getElementById('to').innerText = d.obj.toFixed(1);
                document.getElementById('ta').innerText = d.amb.toFixed(1);
                let ts = document.getElementById('ts');
                if (d.obj >= 70.0) { ts.innerText = "CRITICO (>=70C PARADA)"; ts.className = "telemetry-val alert-crit"; }
                else if (d.obj >= 50.0) { ts.innerText = "ATENCAO (>=50C)"; ts.className = "telemetry-val alert-warn"; }
                else { ts.innerText = "NORMAL"; ts.className = "telemetry-val"; }
                let wd = document.getElementById('wd');
                if (d.watchdog) { wd.innerText = "FAILSAFE (PARADO)"; wd.className = "telemetry-val alert-warn"; }
                else { wd.innerText = "ATIVO"; wd.className = "telemetry-val"; }
            }).catch(() => {});
        }, 500);
    </script>
</body>
</html>
)rawliteral";

static void setupMasterSoftAP(IPAddress local_ip) {
    IPAddress gateway(NETWORK_GATEWAY_ADDR);
    IPAddress subnet(NETWORK_SUBNET_MASK);

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL, 0, 4);

    // Otimizações Anti-Lag e Anti-Ruído RF no chassi
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_15dBm);

    Serial.print("[REDE MASTER] Wi-Fi ROBO_INSPETOR_UGV ativo em: http://");
    Serial.println(WiFi.softAPIP());
}

TelemetryServer::TelemetryServer() : _motor_ref(nullptr), _cam_ref(nullptr) {}

uint8_t TelemetryServer::getHeadlightBrightness() const {
    return g_headlight_val;
}

void TelemetryServer::setHeadlightBrightness(uint8_t val) {
    g_headlight_val = val;
    ledcWrite(PWM_CH_HEADLIGHT, g_headlight_val);
}

void TelemetryServer::beginMaster(MotorController *motor_ctrl) {
    _motor_ref = motor_ctrl;
    g_motors = motor_ctrl;

    setupMasterSoftAP(IPAddress(MASTER_STATIC_IP_ADDR));

    // Rota Principal do Dashboard
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", FPSTR(HTML_DASHBOARD));
    });

    // Rota /cmd -> Recebe eixos X e Y do Joystick e brilho do farol MOSFET (GPIO 25)
    server.on("/cmd", HTTP_GET, []() {
        if (!g_motors) {
            server.send(500, "text/plain", "Erro Motores");
            return;
        }
        int16_t x = server.hasArg("x") ? server.arg("x").toInt() : 0;
        int16_t y = server.hasArg("y") ? server.arg("y").toInt() : 0;
        g_motors->driveJoystick(x, y);

        if (server.hasArg("light")) {
            g_headlight_val = (uint8_t)constrain(server.arg("light").toInt(), 0, 255);
            ledcWrite(PWM_CH_HEADLIGHT, g_headlight_val);
        }
        server.send(200, "text/plain", "OK");
    });

    // Rota /temp -> Retorna JSON simples {"obj": 34.5, "amb": 26.1} buscado a cada 500ms
    server.on("/temp", HTTP_GET, []() {
        StaticJsonDocument<192> doc;
        doc["obj"] = g_latest_state.thermal.object_temperature;
        doc["amb"] = g_latest_state.thermal.ambient_temperature;
        doc["alert"] = g_latest_state.thermal_alert_level;
        doc["watchdog"] = g_latest_state.watchdog_alert;
        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });

    // Rota /status -> Telemetria completa
    server.on("/status", HTTP_GET, []() {
        StaticJsonDocument<384> doc;
        doc["uptime"] = g_latest_state.uptime_ms;
        doc["watchdog"] = g_latest_state.watchdog_alert;
        doc["light_pwm"] = g_headlight_val;

        JsonObject mot = doc.createNestedObject("motors");
        mot["left"] = g_latest_state.motor_left_speed;
        mot["right"] = g_latest_state.motor_right_speed;

        JsonObject th = doc.createNestedObject("thermal");
        th["valid"] = g_latest_state.thermal.is_valid;
        th["amb"] = g_latest_state.thermal.ambient_temperature;
        th["obj"] = g_latest_state.thermal.object_temperature;
        th["alert"] = g_latest_state.thermal_alert_level;

        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });

    server.begin();
}

void TelemetryServer::beginSlaveCam(CameraStream *cam_stream) {
    _cam_ref = cam_stream;
    g_camera = cam_stream;

    IPAddress local_ip(SLAVE_CAM_STATIC_IP_ADDR);
    IPAddress gateway(NETWORK_GATEWAY_ADDR);
    IPAddress subnet(NETWORK_SUBNET_MASK);

    WiFi.mode(WIFI_STA);
    WiFi.config(local_ip, gateway, subnet);
    WiFi.begin(WIFI_AP_SSID, WIFI_AP_PASS);

    // Otimizações Anti-Lag e Anti-Ruído RF no chassi
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_15dBm);

    uint32_t start_ms = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start_ms) < WIFI_CONNECT_TIMEOUT_MS) {
        delay(100);
    }

    streamServer.on("/stream", HTTP_GET, []() {
        WiFiClient client = streamServer.client();
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
        client.println("Access-Control-Allow-Origin: *");
        client.println();

        const uint8_t *frame_buf = nullptr;
        size_t frame_len = 0;

        while (client.connected()) {
            if (g_camera && g_camera->getFrame(&frame_buf, &frame_len)) {
                client.println("--frame");
                client.println("Content-Type: image/jpeg");
                client.println(String("Content-Length: ") + frame_len);
                client.println();
                client.write(frame_buf, frame_len);
                client.println();
                g_camera->releaseFrame();
            }
            delay(30);
        }
    });

    streamServer.begin();
}

void TelemetryServer::updateMaster(const RobotTelemetryState &state) {
    g_latest_state = state;
    server.handleClient();
}

void TelemetryServer::updateSlaveCam() {
    streamServer.handleClient();
}

void TelemetryServer::logSerialJson(const RobotTelemetryState &state) {
    StaticJsonDocument<384> doc;
    doc["uptime_ms"] = state.uptime_ms;
    doc["watchdog"] = state.watchdog_alert;
    doc["light_pwm"] = state.headlight_pwm;

    JsonObject mot = doc.createNestedObject("motors");
    mot["left_cur"] = state.motor_left_speed;
    mot["right_cur"] = state.motor_right_speed;
    mot["left_tgt"] = state.target_left_speed;
    mot["right_tgt"] = state.target_right_speed;

    JsonObject th = doc.createNestedObject("thermal");
    th["valid"] = state.thermal.is_valid;
    th["amb"] = serialized(String(state.thermal.ambient_temperature, 1));
    th["obj"] = serialized(String(state.thermal.object_temperature, 1));
    th["alert"] = state.thermal_alert_level;

    String jsonOutput;
    serializeJson(doc, jsonOutput);
    Serial.println(jsonOutput);
}
