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
#include <ESPmDNS.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

static WebServer server(WEB_SERVER_PORT);

static MotorController *g_motors = nullptr;
static uint8_t g_headlight_val = 0;
static RobotTelemetryState g_latest_state;
static bool g_mdns_started = false;

// Conecta a ESP32 DevKit V1 no Hotspot Wi-Fi do Celular ("robo_ugv" / "12345678")
static void setupMasterStation() {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(MDNS_HOSTNAME);

    // Otimizações Anti-Lag: desliga economia de energia (PS_NONE), fixa largura HT20
    // (maior imunidade a ruído/reflexão sob a tampa) e usa 15 dBm para não saturar o receptor com o celular em cima.
    WiFi.setSleep(false);
    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);

    WiFi.begin(WIFI_AP_SSID, WIFI_AP_PASS);
    WiFi.setTxPower(WIFI_POWER_15dBm);
    WiFi.setSleep(false);
    esp_wifi_set_ps(WIFI_PS_NONE);

    Serial.printf("[REDE MASTER] Conectando ao Hotspot do celular '%s'...\n", WIFI_AP_SSID);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - t0) < WIFI_CONNECT_TIMEOUT_MS) {
        delay(200);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        if (MDNS.begin(MDNS_HOSTNAME)) {
            g_mdns_started = true;
            MDNS.addService("http", "tcp", WEB_SERVER_PORT);
        }
        Serial.print("[REDE MASTER] Conectado! Painel em: http://");
        Serial.print(WiFi.localIP());
        Serial.println(" ou http://ugv.local");
    } else {
        Serial.println("[REDE MASTER] Hotspot ainda nao encontrado (tentara reconectar em background).");
    }
}

TelemetryServer::TelemetryServer() : _motor_ref(nullptr) {}

uint8_t TelemetryServer::getHeadlightBrightness() const {
    return g_headlight_val;
}

void TelemetryServer::setHeadlightBrightness(uint8_t val) {
    g_headlight_val = val;
    PWM_WRITE_CH(PWM_CH_HEADLIGHT, g_headlight_val);
}

void TelemetryServer::beginMaster(MotorController *motor_ctrl) {
    _motor_ref = motor_ctrl;
    g_motors = motor_ctrl;

    setupMasterStation();

    // Rota "/" -> Entrega o Painel Web (carregado de include/web_dashboard.h)
    server.on("/", HTTP_GET, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "text/html", FPSTR(HTML_DASHBOARD));
    });

    // Rota "/cmd" -> Recebe X e Y do Joystick e o brilho do farol MOSFET (GPIO 25)
    server.on("/cmd", HTTP_GET, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        if (!g_motors) {
            server.send(500, "text/plain", "Erro Motores");
            return;
        }
        int16_t x = server.hasArg("x") ? server.arg("x").toInt() : 0;
        int16_t y = server.hasArg("y") ? server.arg("y").toInt() : 0;
        g_motors->driveJoystick(x, y);

        if (server.hasArg("light")) {
            g_headlight_val = (uint8_t)constrain(server.arg("light").toInt(), 0, 255);
            PWM_WRITE_CH(PWM_CH_HEADLIGHT, g_headlight_val);
        }
        server.send(200, "text/plain", "OK");
    });

    // Rota "/temp" -> Consultada pelo navegador a cada 500ms para atualizar a tela
    server.on("/temp", HTTP_GET, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        StaticJsonDocument<320> doc;
        doc["valid"] = g_latest_state.thermal.is_valid;
        doc["obj"] = g_latest_state.thermal.object_temperature;
        doc["amb"] = g_latest_state.thermal.ambient_temperature;
        doc["alert"] = g_latest_state.thermal_alert_level;
        doc["watchdog"] = g_latest_state.watchdog_alert;
        doc["mL"] = g_latest_state.motor_left_speed;
        doc["mR"] = g_latest_state.motor_right_speed;
        doc["clients"] = g_latest_state.wifi_clients;
        doc["rssi"] = g_latest_state.wifi_rssi;
        doc["ip"] = WiFi.localIP().toString();
        doc["gw"] = WiFi.gatewayIP().toString();
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
        PWM_WRITE_CH(PWM_CH_HEADLIGHT, 255);
        delay(300);
        PWM_WRITE_CH(PWM_CH_HEADLIGHT, g_headlight_val);

        // DESLIGA CANAIS LEDC TEMPORARIAMENTE E TESTA COM NÍVEL LÓGICO PURO (DIGITAL HIGH/LOW)
        PWM_DETACH_PIN(PIN_MOTOR_LEFT_IN1);
        PWM_DETACH_PIN(PIN_MOTOR_LEFT_IN2);
        PWM_DETACH_PIN(PIN_MOTOR_RIGHT_IN3);
        PWM_DETACH_PIN(PIN_MOTOR_RIGHT_IN4);

        pinMode(PIN_MOTOR_LEFT_IN1, OUTPUT);
        pinMode(PIN_MOTOR_LEFT_IN2, OUTPUT);
        pinMode(PIN_MOTOR_RIGHT_IN3, OUTPUT);
        pinMode(PIN_MOTOR_RIGHT_IN4, OUTPUT);

        Serial.println("[2/5] Motor ESQUERDO FRENTE (DIGITAL HIGH 100% PURO)...");
        digitalWrite(PIN_MOTOR_LEFT_IN2, LOW);
        digitalWrite(PIN_MOTOR_LEFT_IN1, HIGH);
        delay(1000);
        digitalWrite(PIN_MOTOR_LEFT_IN1, LOW);
        delay(200);

        Serial.println("[3/5] Motor ESQUERDO RE (DIGITAL HIGH 100% PURO)...");
        digitalWrite(PIN_MOTOR_LEFT_IN1, LOW);
        digitalWrite(PIN_MOTOR_LEFT_IN2, HIGH);
        delay(1000);
        digitalWrite(PIN_MOTOR_LEFT_IN2, LOW);
        delay(200);

        Serial.println("[4/5] Motor DIREITO FRENTE (DIGITAL HIGH 100% PURO)...");
        digitalWrite(PIN_MOTOR_RIGHT_IN4, LOW);
        digitalWrite(PIN_MOTOR_RIGHT_IN3, HIGH);
        delay(1000);
        digitalWrite(PIN_MOTOR_RIGHT_IN3, LOW);
        delay(200);

        Serial.println("[5/5] Motor DIREITO RE (DIGITAL HIGH 100% PURO)...");
        digitalWrite(PIN_MOTOR_RIGHT_IN3, LOW);
        digitalWrite(PIN_MOTOR_RIGHT_IN4, HIGH);
        delay(1000);
        digitalWrite(PIN_MOTOR_RIGHT_IN4, LOW);

        // Reanexa os canais PWM
        PWM_ATTACH_CH(PIN_MOTOR_LEFT_IN1, PWM_FREQ_HZ, PWM_RESOLUTION_BITS, PWM_CH_LEFT_IN1);
        PWM_ATTACH_CH(PIN_MOTOR_LEFT_IN2, PWM_FREQ_HZ, PWM_RESOLUTION_BITS, PWM_CH_LEFT_IN2);
        PWM_ATTACH_CH(PIN_MOTOR_RIGHT_IN3, PWM_FREQ_HZ, PWM_RESOLUTION_BITS, PWM_CH_RIGHT_IN3);
        PWM_ATTACH_CH(PIN_MOTOR_RIGHT_IN4, PWM_FREQ_HZ, PWM_RESOLUTION_BITS, PWM_CH_RIGHT_IN4);
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

void TelemetryServer::updateMaster(const RobotTelemetryState &state) {
    g_latest_state = state;
    if (WiFi.status() == WL_CONNECTED && !g_mdns_started) {
        if (MDNS.begin(MDNS_HOSTNAME)) {
            g_mdns_started = true;
            MDNS.addService("http", "tcp", WEB_SERVER_PORT);
        }
        Serial.print("[REDE MASTER] Conectado ao Hotspot! Acesse: http://");
        Serial.print(WiFi.localIP());
        Serial.println(" ou http://ugv.local");
    } else if (WiFi.status() != WL_CONNECTED) {
        static uint32_t last_reconnect_ms = 0;
        if (millis() - last_reconnect_ms > 8000) {
            last_reconnect_ms = millis();
            WiFi.reconnect();
            WiFi.setSleep(false);
            esp_wifi_set_ps(WIFI_PS_NONE);
            WiFi.setTxPower(WIFI_POWER_15dBm);
        }
    }
    server.handleClient();
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
