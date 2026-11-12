/*
 * =============================================================================
 * SERVIDOR WEB MASTER, REDE WI-FI PRÓPRIA E STREAM SLAVE (telemetry.cpp)
 * =============================================================================
 * ARQUITETURA DO MÓDULO DE REDE E ROTAS HTTP:
 * A interface gráfica (HTML/CSS/JS) encontra-se isolada em `web/index.html` e
 * empacotada em memória Flash via `include/web_dashboard.h` (`PROGMEM`).
 * Este módulo gerencia exclusivamente o ponto de acesso Wi-Fi, os parâmetros de
 * rádio (`WiFi.setSleep(false)` e `WIFI_POWER_15dBm`) e o despacho das rotas
 * HTTP (`/`, `/cmd`, `/temp`, `/test`, `/status` e `/stream`).
 */

#include "telemetry.h"
#include "config.h"
#include "web_dashboard.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

static WebServer server(WEB_SERVER_PORT);
static WebServer streamServer(STREAM_SERVER_PORT);

static MotorController *g_motors = nullptr;
static CameraStream *g_camera = nullptr;
static uint8_t g_headlight_val = 0;
static RobotTelemetryState g_latest_state;

// Configura a ESP32 DevKit V1 como Ponto de Acesso (SoftAP) com IP fixo 192.168.4.1
static void setupMasterSoftAP(IPAddress local_ip) {
    IPAddress gateway(NETWORK_GATEWAY_ADDR);
    IPAddress subnet(NETWORK_SUBNET_MASK);

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL, 0, 4);

    // Otimizações Anti-Lag (desliga economia de energia do rádio) e Anti-Ruído RF no chassi
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

    // Rota "/" -> Entrega o Painel Web (carregado de include/web_dashboard.h)
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", FPSTR(HTML_DASHBOARD));
    });

    // Rota "/cmd" -> Recebe X e Y do Joystick e o brilho do farol MOSFET (GPIO 25)
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

    // Rota "/temp" -> Consultada pelo navegador a cada 500ms para atualizar a tela
    server.on("/temp", HTTP_GET, []() {
        StaticJsonDocument<256> doc;
        doc["valid"] = g_latest_state.thermal.is_valid;
        doc["obj"] = g_latest_state.thermal.object_temperature;
        doc["amb"] = g_latest_state.thermal.ambient_temperature;
        doc["alert"] = g_latest_state.thermal_alert_level;
        doc["watchdog"] = g_latest_state.watchdog_alert;
        doc["mL"] = g_latest_state.motor_left_speed;
        doc["mR"] = g_latest_state.motor_right_speed;
        doc["clients"] = g_latest_state.wifi_clients;
        doc["uptime"] = millis();
        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });

    // Rota "/test" -> Executa o Auto-Teste individual dos 4 pinos da Ponte H e Farol
    server.on("/test", HTTP_GET, []() {
        if (!g_motors) {
            server.send(500, "text/plain", "Erro Motores");
            return;
        }
        server.send(200, "text/plain", "Iniciando Auto-Teste de Bancada...");
        Serial.println("\n================ [AUTO-TESTE DE BANCADA] ================");
        Serial.println("[1/5] Testando Farois 12V no Modulo MOSFET (GPIO 25)...");
        ledcWrite(PWM_CH_HEADLIGHT, 255);
        delay(300);
        ledcWrite(PWM_CH_HEADLIGHT, g_headlight_val);

        Serial.println("[2/5] Pulso Motor ESQUERDO -> FRENTE (L298N IN1 = GPIO 26)...");
        g_motors->pulseSinglePin(PWM_CH_LEFT_IN1, 160, 400);

        Serial.println("[3/5] Pulso Motor ESQUERDO -> RE (L298N IN2 = GPIO 27)...");
        g_motors->pulseSinglePin(PWM_CH_LEFT_IN2, 160, 400);

        Serial.println("[4/5] Pulso Motor DIREITO -> FRENTE (L298N IN3 = GPIO 14)...");
        g_motors->pulseSinglePin(PWM_CH_RIGHT_IN3, 160, 400);

        Serial.println("[5/5] Pulso Motor DIREITO -> RE (L298N IN4 = GPIO 13)...");
        g_motors->pulseSinglePin(PWM_CH_RIGHT_IN4, 160, 400);
        Serial.println("================ [FIM DO AUTO-TESTE: MOTORES TRAVADOS EM 0] ================\n");
    });

    // Rota "/status" -> Retorna um pacote JSON completo para diagnóstico externo
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

// Configura a ESP32-CAM Slave para conectar no Wi-Fi da Master e transmitir vídeo na porta 81
void TelemetryServer::beginSlaveCam(CameraStream *cam_stream) {
    _cam_ref = cam_stream;
    g_camera = cam_stream;

    IPAddress local_ip(SLAVE_CAM_STATIC_IP_ADDR);
    IPAddress gateway(NETWORK_GATEWAY_ADDR);
    IPAddress subnet(NETWORK_SUBNET_MASK);

    WiFi.mode(WIFI_STA);
    WiFi.config(local_ip, gateway, subnet);
    WiFi.begin(WIFI_AP_SSID, WIFI_AP_PASS);

    // Otimizações Anti-Lag e Anti-Ruído RF dentro do chassi
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_15dBm);

    uint32_t start_ms = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start_ms) < WIFI_CONNECT_TIMEOUT_MS) {
        delay(100);
    }

    // Rota "/stream" (Porta 81) -> Envia quadros JPEG seguidos (Multipart MJPEG)
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
    // Se a câmera perder conexão com a Master por qualquer motivo, reconecta sozinha
    if (WiFi.status() != WL_CONNECTED) {
        static uint32_t last_retry_ms = 0;
        if (millis() - last_retry_ms > 3000) {
            last_retry_ms = millis();
            WiFi.reconnect();
        }
    }
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
