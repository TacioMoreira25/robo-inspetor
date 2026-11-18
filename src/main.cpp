/*
 * =============================================================================
 * ARQUIVO PRINCIPAL DO FIRMWARE (main.cpp)
 * =============================================================================
 * ARQUITETURA DE COMPILAÇÃO CONDICIONAL (MASTER / SLAVE):
 * A diretiva `#if defined(PLACA_ESP32_CAM)` seleciona o perfil de compilação:
 *
 * 1. Perfil Slave (`env:esp32cam_hardware`):
 *    Ativa o BLOCO 1, dedicando a ESP32-CAM exclusivamente ao streaming MJPEG
 *    no endereço `http://192.168.4.50:81/stream`, mantendo os pinos de boot livres.
 *
 * 2. Perfil Master (`env:esp32_hardware` e `env:wokwi`):
 *    Ativa o BLOCO 2 na ESP32 DevKit V1, responsável pela rede `ROBO_INSPETOR_UGV`
 *    (`192.168.4.1`), servidor Web, leitura térmica I2C (`GPIO 21/22`), controle
 *    do módulo MOSFET (`GPIO 25`) e acionamento PWM de 4 canais da Ponte H L298N
 *    (`GPIO 26, 27, 14, 13`).
 */

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "motor_controller.h"
#include "thermal_sensor.h"
#include "telemetry.h"

// =============================================================================
// CÓDIGO DA ESP32 DEVKIT V1 (MASTER - CÉREBRO CENTRAL DO ROBÔ)
// Vídeo delegado ao celular Android (IP Webcam) conectado na rede do robô.
// =============================================================================
static MotorController motors;
static ThermalSensor thermalSensor(MLX90614_DEFAULT_ADDR);
static TelemetryServer telemetryServer;

// Cronômetros internos para executar tarefas sem travar o processador (sem usar delay)
static uint32_t lastTelemetryMs = 0;
static uint32_t lastThermalReadMs = 0;
static ThermalData currentThermalData = {0.0f, 0.0f, false, 0};
static uint8_t currentThermalAlert = 0;

// Máquina de Estados de Segurança do Robô
enum class RobotOperationalState {
    BOOT_DIAGNOSTICS,           // Fazendo checagem inicial das placas ao ligar
    NORMAL_OPERATION,           // Tudo normal (< 50 °C)
    THERMAL_WARNING_PREVENTIVE, // Alerta Amarelo (>= 50 °C detectados no alvo)
    THERMAL_EMERGENCY_STOP      // Emergência Crítica (>= 70 °C -> Corta os motores!)
};

static RobotOperationalState currentState = RobotOperationalState::BOOT_DIAGNOSTICS;

// Executado 1 vez assim que a ESP32 DevKit V1 liga
void runSelfTest() {
    Serial.println("\n[MASTER] Iniciando Autodiagnostico na ESP32 DevKit V1...");

    // 1. Verifica se o Sensor Térmico MLX90614 responde nos pinos GPIO 21 (SDA) e 22 (SCL)
    bool i2c_ok = thermalSensor.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_FREQUENCY_HZ);
    if (!i2c_ok) {
        Serial.println("[AVISO] Sensor MLX90614 nao respondeu em 0x5A (voce ainda pode testar os motores normalmente).");
    } else {
        Serial.println("[OK] Sensor MLX90614 (GY-906) reconhecido em GPIO 21/22.");
    }

    // 2. Prepara os 4 canais PWM da Ponte H L298N (IN1=26, IN2=27, IN3=14, IN4=13)
    motors.begin();
    Serial.println("[OK] Ponte H L298N configurada em 4 pinos PWM (GPIOs 26, 27, 14, 13 @ 1500 Hz).");

    // 3. Prepara o canal PWM do Módulo MOSFET Duplo dos Faróis 12V (GPIO 25)
    pinMode(PIN_HEADLIGHT_PWM, OUTPUT);
    ledcAttachChannel(PIN_HEADLIGHT_PWM, PWM_FREQ_HZ, PWM_RESOLUTION_BITS, PWM_CH_HEADLIGHT);
    ledcWriteChannel(PWM_CH_HEADLIGHT, 0);
    Serial.println("[OK] Modulo MOSFET Duplo (Farois 12V) configurado na GPIO 25.");
    Serial.println("[DICA] Digite 't' no Monitor Serial (ou clique em 'Testar Motores' no Painel Web) para testar cada canal individualmente!");

    currentState = RobotOperationalState::NORMAL_OPERATION;
}

// Rotina de Check-up Passo a Passo (Acionada pela tecla 't' no Serial Monitor)
// Testa 1 canal por vez durante 0,4 segundo para você validar a montagem física das polias e fios
void runBenchCheckup() {
    Serial.println("\n================ [CHECK-UP COMPLETO DE BANCADA] ================");
    ThermalData td = thermalSensor.read();
    if (td.is_valid) {
        Serial.printf("[1/6] Sensor MLX90614 (GPIO 21/22): VIVO! Amb=%.1fC | Obj=%.1fC\n",
                      td.ambient_temperature, td.object_temperature);
    } else {
        Serial.println("[1/6] Sensor MLX90614 (GPIO 21/22): SEM RESPOSTA! Verifique 3V3, GND, SDA(21), SCL(22) e resistores 4.7k.");
    }

    bool wifiOk = (WiFi.status() == WL_CONNECTED);
    Serial.printf("[2/6] Hotspot '%s': %s (IP: %s | RSSI: %d dBm)\n",
                  WIFI_AP_SSID, wifiOk ? "CONECTADO" : "DESCONECTADO",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());

    Serial.println("[3/6] Piscando Farol 12V no Modulo MOSFET (GPIO 25)...");
    ledcWriteChannel(PWM_CH_HEADLIGHT, 255);
    delay(300);
    ledcWriteChannel(PWM_CH_HEADLIGHT, telemetryServer.getHeadlightBrightness());

    Serial.println("[4/6] Pulso Motor ESQUERDO -> FRENTE (L298N IN1 = GPIO 26)...");
    motors.pulseSinglePin(PWM_CH_LEFT_IN1, 160, 400);

    Serial.println("[5/6] Pulso Motor ESQUERDO -> RE (L298N IN2 = GPIO 27)...");
    motors.pulseSinglePin(PWM_CH_LEFT_IN2, 160, 400);

    Serial.println("[6/6] Pulso Motor DIREITO -> FRENTE (GPIO 14) e RE (GPIO 13)...");
    motors.pulseSinglePin(PWM_CH_RIGHT_IN3, 160, 400);
    motors.pulseSinglePin(PWM_CH_RIGHT_IN4, 160, 400);
    Serial.println("================ [FIM DO CHECK-UP: MOTORES PRONTOS E EM REPOUSO] ================\n");
}

void setup() {
    // Zera os pinos da Ponte H imediatamente no boot (antes do delay) para evitar tranco nos motores
    pinMode(PIN_MOTOR_LEFT_IN1, OUTPUT);  digitalWrite(PIN_MOTOR_LEFT_IN1, LOW);
    pinMode(PIN_MOTOR_LEFT_IN2, OUTPUT);  digitalWrite(PIN_MOTOR_LEFT_IN2, LOW);
    pinMode(PIN_MOTOR_RIGHT_IN3, OUTPUT); digitalWrite(PIN_MOTOR_RIGHT_IN3, LOW);
    pinMode(PIN_MOTOR_RIGHT_IN4, OUTPUT); digitalWrite(PIN_MOTOR_RIGHT_IN4, LOW);

    Serial.begin(SERIAL_BAUD_RATE);
    delay(200);

    Serial.println("==================================================");
    Serial.println("  ROBO INSPETOR (UGV) - ESP32 DEVKIT V1 (MASTER)  ");
    Serial.println("  Controle L298N + MLX90614 + MOSFET + Web Server ");
    Serial.println("==================================================");

    runSelfTest();

    telemetryServer.beginMaster(&motors);
    Serial.println("[WEB] Servidor HTTP ativo (Rotas: /, /cmd, /temp, /test, /status).");
}

void loop() {
    uint32_t now = millis();

    // 1. Leitura de Comandos pelo Cabo USB (Monitor Serial: W, A, S, D, Espaço, L e T)
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 'w' || cmd == 'W') {
            Serial.println("[MANUAL] Frente (180)");
            motors.drive(DriveDirection::FORWARD, 180);
        } else if (cmd == 's' || cmd == 'S') {
            Serial.println("[MANUAL] Re (-180)");
            motors.drive(DriveDirection::BACKWARD, 180);
        } else if (cmd == 'a' || cmd == 'A') {
            Serial.println("[MANUAL] Giro Esquerda (160)");
            motors.drive(DriveDirection::SPIN_LEFT, 160);
        } else if (cmd == 'd' || cmd == 'D') {
            Serial.println("[MANUAL] Giro Direita (160)");
            motors.drive(DriveDirection::SPIN_RIGHT, 160);
        } else if (cmd == ' ') {
            Serial.println("[MANUAL] Parada Total");
            motors.drive(DriveDirection::STOP, 0);
        } else if (cmd == 'l' || cmd == 'L') {
            uint8_t next_val = (telemetryServer.getHeadlightBrightness() > 0) ? 0 : 255;
            telemetryServer.setHeadlightBrightness(next_val);
            Serial.println(next_val > 0 ? "[FAROIS] MOSFET GPIO 25 Ligado (100%)" : "[FAROIS] MOSFET GPIO 25 Desligado");
        } else if (cmd == 't' || cmd == 'T') {
            runBenchCheckup();
        }
    }

    // 2. Atualização dos Motores (aplica rampa suave ou corta tudo se estiver em emergência térmica)
    if (currentState != RobotOperationalState::THERMAL_EMERGENCY_STOP) {
        motors.update();
    } else {
        motors.emergencyStop();
    }

    // 3. Leitura do Sensor de Temperatura a cada 100 ms (10 vezes por segundo)
    if ((now - lastThermalReadMs) >= SENSOR_READ_INTERVAL_MS) {
        lastThermalReadMs = now;
        currentThermalData = thermalSensor.read();

        if (currentThermalData.is_valid) {
            float temp_obj = currentThermalData.object_temperature;

            if (temp_obj >= ALERTA_TEMP_CRITICA_C) {
                currentThermalAlert = 2;
                if (currentState != RobotOperationalState::THERMAL_EMERGENCY_STOP) {
                    currentState = RobotOperationalState::THERMAL_EMERGENCY_STOP;
                    Serial.println("[ALERTA CRITICO] 70°C ATINGIDOS! Parada imediata dos motores.");
                    motors.emergencyStop();
                }
            } else if (temp_obj >= ALERTA_TEMP_PREVENTIVO_C) {
                currentThermalAlert = 1;
                if (currentState != RobotOperationalState::THERMAL_WARNING_PREVENTIVE) {
                    currentState = RobotOperationalState::THERMAL_WARNING_PREVENTIVE;
                    Serial.println("[ALERTA PREVENTIVO] 50°C detectados no alvo.");
                }
            } else {
                currentThermalAlert = 0;
                if (currentState != RobotOperationalState::NORMAL_OPERATION) {
                    currentState = RobotOperationalState::NORMAL_OPERATION;
                }
            }
        }
    }

    // 4. Atualiza o Servidor Web (http://192.168.4.1) e imprime telemetria JSON a cada 500 ms
    RobotTelemetryState telem;
    telem.uptime_ms = now;
    telem.motor_left_speed = motors.getCurrentSpeedLeft();
    telem.motor_right_speed = motors.getCurrentSpeedRight();
    telem.target_left_speed = motors.getTargetSpeedLeft();
    telem.target_right_speed = motors.getTargetSpeedRight();
    telem.thermal = currentThermalData;
    telem.thermal_alert_level = currentThermalAlert;
    telem.headlight_pwm = telemetryServer.getHeadlightBrightness();
    telem.watchdog_alert = motors.isWatchdogTriggered();
    telem.wifi_rssi = WiFi.RSSI();
    telem.wifi_clients = (WiFi.status() == WL_CONNECTED) ? 1 : 0;

    telemetryServer.updateMaster(telem);

    if ((now - lastTelemetryMs) >= TELEMETRY_INTERVAL_MS) {
        lastTelemetryMs = now;
        telemetryServer.logSerialJson(telem);
    }
}
