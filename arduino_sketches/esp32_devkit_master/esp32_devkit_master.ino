/*
 * =============================================================================
 * ROBÔ INSPETOR TERRESTRE (UGV) - ESP32 DEVKIT V1 (MASTER)
 * Arquivo: esp32_devkit_master.ino
 * =============================================================================
 * Responsabilidades:
 * - Criar a Rede Wi-Fi Própria do Robô ("ROBO_INSPETOR_UGV", Senha "12345678", IP 192.168.4.1)
 * - Hospedar Interface Web (HTML/CSS/JS em PROGMEM) na porta 80 (http://192.168.4.1)
 * - Incorporar stream MJPEG da ESP32-CAM Slave (http://192.168.4.2:81/stream)
 * - Aplicar otimizações anti-lag (WiFi.setSleep(false)) e anti-ruído (WIFI_POWER_15dBm)
 * - Controlar Ponte H L298N via 4 pinos PWM (IN1=26, IN2=27, IN3=14, IN4=13)
 * - Ler Sensor Térmico MLX90614 via I2C (SDA=21, SCL=22) e expor rota /temp
 * - Acionar Módulo MOSFET Duplo dos Faróis Olho de Águia 12V (GPIO 25)
 * - Executar Watchdog Failsafe de 1000 ms (para motores se perder pacotes Wi-Fi)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

// =============================================================================
// 1. TABELA DEFINITIVA DE PINOUT (ESP32 DEVKIT V1 - MASTER)
// =============================================================================
#define PIN_MOTOR_LEFT_IN1    26  // Ponte H L298N - IN 1 (Motor Esq. A)
#define PIN_MOTOR_LEFT_IN2    27  // Ponte H L298N - IN 2 (Motor Esq. B)
#define PIN_MOTOR_RIGHT_IN3   14  // Ponte H L298N - IN 3 (Motor Dir. A)
#define PIN_MOTOR_RIGHT_IN4   13  // Ponte H L298N - IN 4 (Motor Dir. B)

#define PIN_I2C_SDA           21  // Sensor MLX90614 - SDA (Pull-up 4.7k p/ 3V3)
#define PIN_I2C_SCL           22  // Sensor MLX90614 - SCL (Pull-up 4.7k p/ 3V3)
#define MLX90614_ADDR       0x5A

#define PIN_MOSFET_LIGHT      25  // Módulo MOSFET Duplo - SIG / PWM (Faróis 12V)
#define PIN_SERIAL2_RX        16  // Reserva física no chassi (Fio Verde)
#define PIN_SERIAL2_TX        17  // Reserva física no chassi (Fio Amarelo)

// Canais PWM LEDC (1500 Hz, 8 bits)
#define PWM_FREQ_HZ         1500
#define PWM_RES_BITS           8
#define CH_IN1                 0
#define CH_IN2                 1
#define CH_IN3                 2
#define CH_IN4                 3
#define CH_LIGHT               4

#define FAILSAFE_TIMEOUT_MS 1000  // 1 segundo sem comando Wi-Fi corta os motores

// =============================================================================
// 2. CONFIGURAÇÃO DA REDE WI-FI PRÓPRIA DO ROBÔ (SOFTAP 192.168.4.1)
// =============================================================================
const char* AP_SSID   = "ROBO_INSPETOR_UGV";
const char* AP_PASS   = "12345678";
const int   AP_CHAN   = 6;

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

// Estado global
uint32_t lastCommandMs = 0;
float objTempC = 25.0f;
float ambTempC = 25.0f;
uint8_t lightPwm = 0;

// =============================================================================
// 3. INTERFACE WEB (HTML/CSS/JS EM PROGMEM)
// =============================================================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>Robô Inspetor UGV - Painel Master</title>
    <style>
        body { font-family: monospace; background: #101216; color: #00ff66; padding: 15px; margin: 0; user-select: none; }
        h1 { color: #fff; text-align: center; font-size: 20px; margin-bottom: 5px; }
        .grid { display: flex; flex-wrap: wrap; gap: 15px; justify-content: center; }
        .card { background: #1a1d24; border: 1px solid #333; padding: 15px; border-radius: 8px; width: 320px; }
        .video-box { width: 100%; height: 220px; background: #000; border: 1px solid #444; display: flex; align-items: center; justify-content: center; }
        .video-box img { width: 100%; height: 100%; object-fit: cover; }
        .val { font-size: 20px; font-weight: bold; color: #fff; }
        canvas { background: #121418; border: 2px solid #444; border-radius: 50%; touch-action: none; display: block; margin: 0 auto; }
        button { width: 100%; background: #282c34; color: #fff; border: 1px solid #555; padding: 12px; border-radius: 6px; cursor: pointer; margin-top: 8px; }
        button.active { background: #00ff66; color: #000; font-weight: bold; }
    </style>
</head>
<body>
    <h1>ROBO INSPETOR UGV - MASTER (192.168.4.1)</h1>
    <div class="grid">
        <div class="card">
            <h3>Video Ao Vivo (ESP32-CAM Slave)</h3>
            <div class="video-box">
                <img src="http://192.168.4.2:81/stream" alt="Stream 192.168.4.2:81">
            </div>
        </div>
        <div class="card">
            <h3>Sensor Termico MLX90614</h3>
            <p>Objeto: <span id="obj" class="val">--</span> °C</p>
            <p>Ambiente: <span id="amb" class="val">--</span> °C</p>
            <button id="btnL" onclick="toggleLight()">Farois Olho de Aguia (Tecla L)</button>
        </div>
        <div class="card">
            <h3>Joystick Virtual & W,A,S,D</h3>
            <canvas id="joy" width="200" height="200"></canvas>
        </div>
    </div>
    <script>
        let light = 0, joyX = 0, joyY = 0, drag = false;
        const c = document.getElementById('joy'), ctx = c.getContext('2d');
        function draw(x, y) {
            ctx.clearRect(0,0,200,200);
            ctx.beginPath(); ctx.arc(100 + x*0.7, 100 - y*0.7, 25, 0, Math.PI*2);
            ctx.fillStyle = '#00ff66'; ctx.fill();
        }
        draw(0,0);
        function send(x, y) { fetch('/cmd?x='+x+'&y='+y+'&light='+light).catch(()=>{}); }
        function move(cx, cy) {
            const r = c.getBoundingClientRect();
            let dx = cx - (r.left + 100), dy = (r.top + 100) - cy;
            let d = Math.min(100, Math.hypot(dx, dy)), a = Math.atan2(dy, dx);
            joyX = Math.round(d * Math.cos(a) * 2.55); joyY = Math.round(d * Math.sin(a) * 2.55);
            draw(joyX/2.55, joyY/2.55); send(joyX, joyY);
        }
        c.onpointerdown = e => { drag = true; move(e.clientX, e.clientY); };
        window.onpointermove = e => { if (drag) move(e.clientX, e.clientY); };
        window.onpointerup = () => { if (drag) { drag = false; joyX = 0; joyY = 0; draw(0,0); send(0,0); } };
        function toggleLight() {
            light = light ? 0 : 255;
            document.getElementById('btnL').className = light ? 'active' : '';
            send(joyX, joyY);
        }
        window.onkeydown = e => {
            if (e.repeat) return;
            const k = e.key.toLowerCase();
            if (k==='w') { joyX=0; joyY=200; } else if (k==='s') { joyX=0; joyY=-200; }
            else if (k==='a') { joyX=-180; joyY=0; } else if (k==='d') { joyX=180; joyY=0; }
            else if (k==='l') { toggleLight(); return; } else return;
            draw(joyX/2.55, joyY/2.55); send(joyX, joyY);
        };
        window.onkeyup = e => {
            if (['w','a','s','d'].includes(e.key.toLowerCase())) { joyX=0; joyY=0; draw(0,0); send(0,0); }
        };
        setInterval(() => {
            fetch('/temp').then(r => r.json()).then(d => {
                document.getElementById('obj').innerText = d.obj.toFixed(1);
                document.getElementById('amb').innerText = d.amb.toFixed(1);
            }).catch(()=>{});
        }, 500);
    </script>
</body>
</html>
)rawliteral";

// =============================================================================
// 4. CONTROLE DE TRAÇÃO DIFERENCIAL E FAILSAFE
// =============================================================================
void pararMotores() {
    ledcWrite(CH_IN1, 0);
    ledcWrite(CH_IN2, 0);
    ledcWrite(CH_IN3, 0);
    ledcWrite(CH_IN4, 0);
}

void acionarLado(uint8_t chA, uint8_t chB, int16_t vel) {
    vel = constrain(vel, -255, 255);
    if (vel > 0) {
        ledcWrite(chA, vel);
        ledcWrite(chB, 0);
    } else if (vel < 0) {
        ledcWrite(chA, 0);
        ledcWrite(chB, -vel);
    } else {
        ledcWrite(chA, 0);
        ledcWrite(chB, 0);
    }
}

void aplicarCinematicaDiferencial(int16_t x, int16_t y) {
    lastCommandMs = millis();
    int16_t esq = constrain(y + x, -255, 255);
    int16_t dir = constrain(y - x, -255, 255);
    acionarLado(CH_IN1, CH_IN2, esq);
    acionarLado(CH_IN3, CH_IN4, dir);
}

// =============================================================================
// 5. LEITURA DIRETA I2C DO SENSOR MLX90614 (REGISTRADORES 0x06 E 0x07)
// =============================================================================
float lerRegistradorMLX(uint8_t reg) {
    Wire.beginTransmission(MLX90614_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return NAN;
    if (Wire.requestFrom((uint8_t)MLX90614_ADDR, (uint8_t)3) < 2) return NAN;
    uint16_t raw = Wire.read() | (Wire.read() << 8);
    Wire.read(); // PEC
    return (raw * 0.02f) - 273.15f;
}

void atualizarTemperaturas() {
    float tAmb = lerRegistradorMLX(0x06);
    float tObj = lerRegistradorMLX(0x07);
    if (!isnan(tAmb)) ambTempC = tAmb;
    if (!isnan(tObj)) objTempC = tObj;
}

// =============================================================================
// 6. SETUP E LOOP PRINCIPAL
// =============================================================================
void setup() {
    Serial.begin(115200);
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000);

    ledcSetup(CH_IN1, PWM_FREQ_HZ, PWM_RES_BITS); ledcAttachPin(PIN_MOTOR_LEFT_IN1, CH_IN1);
    ledcSetup(CH_IN2, PWM_FREQ_HZ, PWM_RES_BITS); ledcAttachPin(PIN_MOTOR_LEFT_IN2, CH_IN2);
    ledcSetup(CH_IN3, PWM_FREQ_HZ, PWM_RES_BITS); ledcAttachPin(PIN_MOTOR_RIGHT_IN3, CH_IN3);
    ledcSetup(CH_IN4, PWM_FREQ_HZ, PWM_RES_BITS); ledcAttachPin(PIN_MOTOR_RIGHT_IN4, CH_IN4);
    ledcSetup(CH_LIGHT, PWM_FREQ_HZ, PWM_RES_BITS); ledcAttachPin(PIN_MOSFET_LIGHT, CH_LIGHT);
    pararMotores();

    // Cria a rede Wi-Fi própria do robô ("ROBO_INSPETOR_UGV") com otimizações Anti-Lag e Anti-Ruído
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP(AP_SSID, AP_PASS, AP_CHAN, 0, 4);
    WiFi.setSleep(false);                  // Desativa Modem Sleep (reduz latência de ~400ms para ~15ms)
    WiFi.setTxPower(WIFI_POWER_15dBm);     // Evita saturação RF entre as 2 placas no chassi

    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", FPSTR(INDEX_HTML));
    });

    server.on("/cmd", HTTP_GET, []() {
        int16_t x = server.hasArg("x") ? server.arg("x").toInt() : 0;
        int16_t y = server.hasArg("y") ? server.arg("y").toInt() : 0;
        if (server.hasArg("light")) {
            lightPwm = constrain(server.arg("light").toInt(), 0, 255);
            ledcWrite(CH_LIGHT, lightPwm);
        }
        aplicarCinematicaDiferencial(x, y);
        server.send(200, "text/plain", "OK");
    });

    server.on("/temp", HTTP_GET, []() {
        String json = "{\"obj\":" + String(objTempC, 1) + ",\"amb\":" + String(ambTempC, 1) + "}";
        server.send(200, "application/json", json);
    });

    server.begin();
    lastCommandMs = millis();
}

void loop() {
    server.handleClient();

    // Failsafe de 1 segundo ou perda de Wi-Fi
    if ((millis() - lastCommandMs) > FAILSAFE_TIMEOUT_MS) {
        pararMotores();
    }

    static uint32_t lastTempMs = 0;
    if (millis() - lastTempMs >= 200) {
        lastTempMs = millis();
        atualizarTemperaturas();
        if (objTempC >= 70.0f) {
            pararMotores();
        }
    }
}
