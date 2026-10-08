#include "telemetry.h"
#include "config.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

static WebServer server(WEB_SERVER_PORT);
static WebServer streamServer(STREAM_SERVER_PORT);

static MotorController *g_motors = nullptr;
static CameraStream *g_camera = nullptr;

static const char HTML_DASHBOARD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>Robô Inspetor UGV - Painel de Controle</title>
    <style>
        body { font-family: monospace; background: #121212; color: #00ff66; padding: 15px; margin: 0; }
        h1 { color: #ffffff; text-align: center; }
        .grid { display: flex; flex-wrap: wrap; gap: 20px; justify-content: center; }
        .card { background: #1e1e1e; border: 1px solid #333; padding: 15px; border-radius: 8px; width: 320px; }
        .btn-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; margin-top: 10px; }
        button { background: #2d2d2d; color: #fff; border: 1px solid #555; padding: 15px; font-size: 16px; border-radius: 6px; cursor: pointer; }
        button:active { background: #00ff66; color: #000; }
        .telemetry-val { font-size: 20px; font-weight: bold; color: #fff; }
        .alert { color: #ff3333; font-weight: bold; }
    </style>
</head>
<body>
    <h1>🤖 Robô Inspetor (UGV)</h1>
    <div class="grid">
        <div class="card">
            <h3>🎥 Fluxo de Câmera</h3>
            <p>Porta de Streaming: 81</p>
            <p><a href="http://192.168.4.1:81/stream" target="_blank" style="color: #00ff66;">Abrir Stream MJPEG</a></p>
        </div>
        <div class="card">
            <h3>🌡️ Telemetria Térmica</h3>
            <p>Ambiente: <span id="ta" class="telemetry-val">--</span> °C</p>
            <p>Alvo (Objeto): <span id="to" class="telemetry-val">--</span> °C</p>
            <p>Watchdog Status: <span id="wd" class="telemetry-val">OK</span></p>
        </div>
        <div class="card">
            <h3>💡 Iluminação Frontal (MOSFET)</h3>
            <p>Faróis Olho de Águia (12V):</p>
            <input type="range" min="0" max="255" value="0" style="width:100%" oninput="fetch('/light?val='+this.value)">
        </div>
        <div class="card">
            <h3>🕹️ Controle Manual</h3>
            <div class="btn-grid">
                <div></div><button onclick="cmd('F')">▲</button><div></div>
                <button onclick="cmd('L')">◀</button><button onclick="cmd('S')">■</button><button onclick="cmd('R')">▶</button>
                <div></div><button onclick="cmd('B')">▼</button><div></div>
            </div>
        </div>
    </div>
    <script>
        function cmd(action) {
            fetch('/control?dir=' + action);
        }
        setInterval(() => {
            fetch('/status').then(r => r.json()).then(d => {
                document.getElementById('ta').innerText = d.thermal.ambient.toFixed(1);
                document.getElementById('to').innerText = d.thermal.object.toFixed(1);
                let wd = document.getElementById('wd');
                if (d.watchdog) {
                    wd.innerText = "FAILSAFE ATIVO";
                    wd.className = "telemetry-val alert";
                } else {
                    wd.innerText = "ATIVO (OK)";
                    wd.className = "telemetry-val";
                }
            }).catch(e => {});
        }, 300);
    </script>
</body>
</html>
)rawliteral";

TelemetryServer::TelemetryServer()
    : _motor_ref(nullptr), _cam_ref(nullptr), _last_telemetry_emit_ms(0) {}

void TelemetryServer::begin(MotorController *motor_ctrl, CameraStream *cam_stream) {
    _motor_ref = motor_ctrl;
    _cam_ref = cam_stream;
    g_motors = motor_ctrl;
    g_camera = cam_stream;

    // Inicialização da interface Wi-Fi como SoftAP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);

    // Rotas do Servidor Web
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", FPSTR(HTML_DASHBOARD));
    });

    server.on("/control", HTTP_GET, []() {
        if (!g_motors) {
            server.send(500, "text/plain", "Motores não inicializados");
            return;
        }

        g_motors->resetWatchdog();
        String dir = server.hasArg("dir") ? server.arg("dir") : "S";
        uint8_t speed = server.hasArg("speed") ? server.arg("speed").toInt() : 180;

        if (dir == "F") g_motors->drive(DriveDirection::FORWARD, speed);
        else if (dir == "B") g_motors->drive(DriveDirection::BACKWARD, speed);
        else if (dir == "L") g_motors->drive(DriveDirection::SPIN_LEFT, speed);
        else if (dir == "R") g_motors->drive(DriveDirection::SPIN_RIGHT, speed);
        else g_motors->drive(DriveDirection::STOP, 0);

        server.send(200, "text/plain", "OK");
    });

    // Rota de controle dos Faróis LED (MOSFET D4184 na GPIO 4)
    server.on("/light", HTTP_GET, []() {
        uint8_t brightness = server.hasArg("val") ? server.arg("val").toInt() : 0;
        ledcWrite(PWM_CHANNEL_HEADLIGHT, brightness);
        server.send(200, "text/plain", "OK");
    });

    server.begin();

    // Rota de Streaming MJPEG
    streamServer.on("/stream", HTTP_GET, []() {
        WiFiClient client = streamServer.client();
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
        client.println();

        const uint8_t *frame_buf = nullptr;
        size_t frame_len = 0;

        for (int i = 0; i < 5 && client.connected(); i++) {
            if (g_camera && g_camera->getFrame(&frame_buf, &frame_len)) {
                client.println("--frame");
                client.println("Content-Type: image/jpeg");
                client.println(String("Content-Length: ") + frame_len);
                client.println();
                client.write(frame_buf, frame_len);
                client.println();
                g_camera->releaseFrame();
            }
            delay(10);
        }
    });

    streamServer.begin();
}

void TelemetryServer::update(const RobotTelemetryState &state) {
    server.handleClient();
    streamServer.handleClient();

    // Rota dinâmica de status JSON
    server.on("/status", HTTP_GET, [&state]() {
        StaticJsonDocument<384> doc;
        doc["uptime"] = state.uptime_ms;
        doc["watchdog"] = state.watchdog_alert;
        doc["rssi"] = state.wifi_rssi;

        JsonObject mot = doc.createNestedObject("motors");
        mot["left"] = state.motor_left_speed;
        mot["right"] = state.motor_right_speed;

        JsonObject th = doc.createNestedObject("thermal");
        th["valid"] = state.thermal.is_valid;
        th["ambient"] = state.thermal.ambient_temperature;
        th["object"] = state.thermal.object_temperature;

        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });
}

void TelemetryServer::logSerialJson(const RobotTelemetryState &state) {
    StaticJsonDocument<384> doc;
    doc["uptime_ms"] = state.uptime_ms;
    doc["watchdog"] = state.watchdog_alert;
    doc["rssi"] = state.wifi_rssi;

    JsonObject mot = doc.createNestedObject("motors");
    mot["left_cur"] = state.motor_left_speed;
    mot["right_cur"] = state.motor_right_speed;
    mot["left_tgt"] = state.target_left_speed;
    mot["right_tgt"] = state.target_right_speed;

    JsonObject th = doc.createNestedObject("thermal");
    th["valid"] = state.thermal.is_valid;
    th["ambient_c"] = state.thermal.ambient_temperature;
    th["object_c"] = state.thermal.object_temperature;

    String jsonOutput;
    serializeJson(doc, jsonOutput);
    Serial.println(jsonOutput);
}
