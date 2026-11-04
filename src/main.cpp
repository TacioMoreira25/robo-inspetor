#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "motor_controller.h"
#include "thermal_sensor.h"
#include "camera_stream.h"
#include "telemetry.h"

// =============================================================================
// COMPILAÇÃO CONDICIONAL: SLAVE (ESP32-CAM) vs MASTER (ESP32 DEVKIT V1)
// =============================================================================
#if defined(PLACA_ESP32_CAM) && !MODO_SIMULACAO

// -----------------------------------------------------------------------------
// MODO SLAVE (ESP32-CAM - IP 192.168.1.101): Dedicada 100% ao Stream MJPEG (:81)
// -----------------------------------------------------------------------------
static CameraStream slaveCamera;
static TelemetryServer slaveStreamServer;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(200);

    Serial.println("==================================================");
    Serial.println("  ROBO INSPETOR (UGV) - ESP32-CAM SLAVE (VIDEO)   ");
    Serial.println("  Pinos 12, 13, 14, 15 livres - Dedicada ao Stream");
    Serial.println("==================================================");

    if (!slaveCamera.begin()) {
        Serial.println("[FALHA] Erro ao inicializar camera OV2640!");
    } else {
        Serial.println("[OK] Camera OV2640 inicializada.");
    }

    slaveStreamServer.beginSlaveCam(&slaveCamera);
    Serial.println("[STREAM] Servidor MJPEG ativo em http://192.168.1.101:81/stream");
}

void loop() {
    slaveStreamServer.updateSlaveCam();
}

#else

// -----------------------------------------------------------------------------
// MODO MASTER (ESP32 DEVKIT V1 - IP 192.168.1.100): Controle, I2C, MOSFET e Web
// -----------------------------------------------------------------------------
static MotorController motors;
static ThermalSensor thermalSensor(MLX90614_DEFAULT_ADDR);
static TelemetryServer telemetryServer;

static uint32_t lastTelemetryMs = 0;
static uint32_t lastThermalReadMs = 0;
static ThermalData currentThermalData = {0.0f, 0.0f, false, 0};
static uint8_t currentThermalAlert = 0;

enum class RobotOperationalState {
    BOOT_DIAGNOSTICS,
    NORMAL_OPERATION,
    THERMAL_WARNING_PREVENTIVE, // >= 50 °C (Alerta Web/Serial)
    THERMAL_EMERGENCY_STOP      // >= 70 °C (Parada crítica dos motores)
};

static RobotOperationalState currentState = RobotOperationalState::BOOT_DIAGNOSTICS;

void runSelfTest() {
    Serial.println("\n[MASTER] Iniciando Autodiagnostico na ESP32 DevKit V1...");

    // 1. Barramento I2C (GPIO 21 = SDA, GPIO 22 = SCL)
    bool i2c_ok = thermalSensor.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_FREQUENCY_HZ);
    if (!i2c_ok) {
        Serial.println("[FALHA] Sensor MLX90614 nao respondeu no endereco 0x5A!");
    } else {
        Serial.println("[OK] Sensor MLX90614 (GY-906) reconhecido em GPIO 21/22.");
    }

    // 2. Ponte H L298N (4 fios PWM: IN1=26, IN2=27, IN3=14, IN4=13)
    motors.begin();
    Serial.println("[OK] Ponte H L298N configurada em 4 pinos PWM (GPIOs 26, 27, 14, 13 @ 1500 Hz).");

    // 3. Módulo MOSFET Duplo dos Faróis Olho de Águia 12V (GPIO 25)
    pinMode(PIN_HEADLIGHT_PWM, OUTPUT);
    ledcSetup(PWM_CH_HEADLIGHT, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_HEADLIGHT_PWM, PWM_CH_HEADLIGHT);
    ledcWrite(PWM_CH_HEADLIGHT, 0);
    Serial.println("[OK] Modulo MOSFET Duplo (Farois 12V) configurado na GPIO 25.");

    currentState = RobotOperationalState::NORMAL_OPERATION;
}

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(200);

    Serial.println("==================================================");
    Serial.println("  ROBO INSPETOR (UGV) - ESP32 DEVKIT V1 (MASTER)  ");
    Serial.println("  Controle L298N + MLX90614 + MOSFET + Web Server ");
    Serial.println("==================================================");

    runSelfTest();

    telemetryServer.beginMaster(&motors);
    Serial.println("[WEB] Painel Master ativo na porta 80 (Rotas: /, /cmd, /temp, /status).");
}

void loop() {
    uint32_t now = millis();

    // Comandos via Monitor Serial (W, A, S, D, Espaço, L) para bancada e Wokwi
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
        }
    }

    // Atualização de Tração, Rampas e Watchdog Failsafe (1000 ms)
    if (currentState != RobotOperationalState::THERMAL_EMERGENCY_STOP) {
        motors.update();
    } else {
        motors.emergencyStop();
    }

    // Leitura Térmica I2C Não-Bloqueante (a cada 100 ms)
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

    // Atualização do Servidor Web e Emissão de Telemetria JSON (a cada 500 ms)
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

    telemetryServer.updateMaster(telem);

    if ((now - lastTelemetryMs) >= TELEMETRY_INTERVAL_MS) {
        lastTelemetryMs = now;
        telemetryServer.logSerialJson(telem);
    }
}

#endif
