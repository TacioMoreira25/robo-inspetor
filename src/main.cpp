#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "motor_controller.h"
#include "thermal_sensor.h"
#include "camera_stream.h"
#include "telemetry.h"

// =============================================================================
// INSTÂNCIAS DE SUBSISTEMAS DO ROBÔ
// =============================================================================
static MotorController motors;
static ThermalSensor thermalSensor(MLX90614_DEFAULT_ADDR);
static CameraStream camera;
static TelemetryServer telemetryServer;

// Variáveis de temporização não-bloqueante
static uint32_t lastTelemetryMs = 0;
static uint32_t lastThermalReadMs = 0;
static uint32_t lastBatteryReadMs = 0;
static uint32_t lastBuzzerToggleMs = 0;
static bool buzzerState = false;

static ThermalData currentThermalData = {0.0f, 0.0f, false, 0};
static float currentBatteryVoltage = BATTERY_NOMINAL_VOLTAGE;
static uint8_t currentBatteryPct = 100;
static bool batteryLowWarning = false;
static bool batteryCriticalStop = false;

// Máquina de estados operacional
enum class RobotOperationalState {
    BOOT_DIAGNOSTICS,
    NORMAL_OPERATION,
    THERMAL_WARNING_PREVENTIVE, // >= 50 °C ou Bateria < 10.5V (Buzzer intermitente + LED)
    THERMAL_EMERGENCY_STOP      // >= 70 °C ou Bateria < 9.9V (Parada crítica total)
};

static RobotOperationalState currentState = RobotOperationalState::BOOT_DIAGNOSTICS;

// =============================================================================
// LEITURA DO DIVISOR DE TENSÃO DA BATERIA 3S2P (10 kOhm / 3.3 kOhm na GPIO 34)
// =============================================================================
void readBatteryStatus() {
#if !defined(PLACA_ESP32_CAM) || MODO_SIMULACAO
    uint16_t raw_adc = analogRead(PIN_BATTERY_ADC);
    float v_pin = (raw_adc / BATTERY_ADC_MAX) * BATTERY_ADC_VREF;
    currentBatteryVoltage = v_pin * BATTERY_DIVIDER_RATIO;
#endif

    // Cálculo linear de porcentagem entre 9.90V (0%) e 12.60V (100%)
    float pct_f = ((currentBatteryVoltage - BATTERY_CRIT_VOLTAGE) /
                   (BATTERY_MAX_VOLTAGE - BATTERY_CRIT_VOLTAGE)) * 100.0f;
    if (pct_f > 100.0f) pct_f = 100.0f;
    if (pct_f < 0.0f) pct_f = 0.0f;
    currentBatteryPct = (uint8_t)pct_f;

    batteryCriticalStop = (currentBatteryVoltage < BATTERY_CRIT_VOLTAGE);
    batteryLowWarning = (!batteryCriticalStop && (currentBatteryVoltage < BATTERY_WARN_VOLTAGE));
}

// =============================================================================
// ETAPA 1: BOOT E AUTODIAGNÓSTICO (SELF-TEST)
// =============================================================================
void runSelfTest() {
    Serial.println("\n[SISTEMA] Iniciando Autodiagnostico de Hardware...");

    // Inicialização das portas de alerta local (Buzzer e LED)
    pinMode(PIN_ALERT_BUZZER, OUTPUT);
    pinMode(PIN_ALERT_LED, OUTPUT);
    digitalWrite(PIN_ALERT_BUZZER, LOW);
    digitalWrite(PIN_ALERT_LED, LOW);

#if !defined(PLACA_ESP32_CAM) || MODO_SIMULACAO
    pinMode(PIN_BATTERY_ADC, INPUT);
    analogReadResolution(12);
    readBatteryStatus();
    Serial.print("[OK] Monitoramento ADC Bateria 3S2P na GPIO 34: ");
    Serial.print(currentBatteryVoltage, 2);
    Serial.print(" V (");
    Serial.print(currentBatteryPct);
    Serial.println("%).");
#endif

    // Configuração do barramento I2C
    bool i2c_ok = thermalSensor.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_FREQUENCY_HZ);
    if (!i2c_ok) {
        Serial.println("[FALHA] Sensor Termico MLX90614 nao respondeu no endereco 0x5A!");
    } else {
        Serial.println("[OK] Barramento I2C e sensor MLX90614 (GY-906) prontos.");
    }

    // Inicialização da Câmera
    bool cam_ok = camera.begin();
    if (!cam_ok) {
        Serial.println("[AVISO] Modulo de camera indisponivel (operando em modo alternativo).");
    } else {
        Serial.println("[OK] Modulo de visao e streaming de video prontos.");
    }

    // Inicialização da Tração
    motors.begin();
    Serial.println("[OK] Tração L298N configurada (PWM otimizado em 1500 Hz).");

    // Inicialização do Canal PWM dos Faróis (MOSFET D4184 na GPIO 4)
    ledcSetup(PWM_CHANNEL_HEADLIGHT, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_HEADLIGHT_PWM, PWM_CHANNEL_HEADLIGHT);
    ledcWrite(PWM_CHANNEL_HEADLIGHT, 0);
    Serial.println("[OK] Faróis LED (MOSFET D4184 na GPIO 4) prontos.");

    currentState = RobotOperationalState::NORMAL_OPERATION;
}

// =============================================================================
// CONTROLE DE SINALIZAÇÃO LOCAL DE ALERTA
// =============================================================================
void updateAlertSignaling(uint32_t now) {
    if (currentState == RobotOperationalState::THERMAL_WARNING_PREVENTIVE) {
        digitalWrite(PIN_ALERT_LED, HIGH);
        
        // Buzzer intermitente a cada 250 ms
        if (now - lastBuzzerToggleMs >= 250) {
            lastBuzzerToggleMs = now;
            buzzerState = !buzzerState;
            digitalWrite(PIN_ALERT_BUZZER, buzzerState ? HIGH : LOW);
        }
    } else if (currentState == RobotOperationalState::THERMAL_EMERGENCY_STOP) {
        // Alarme contínuo de parada crítica
        digitalWrite(PIN_ALERT_LED, HIGH);
        digitalWrite(PIN_ALERT_BUZZER, HIGH);
    } else {
        // Normal
        digitalWrite(PIN_ALERT_LED, LOW);
        digitalWrite(PIN_ALERT_BUZZER, LOW);
    }
}

// =============================================================================
// SETUP DO FIRMWARE
// =============================================================================
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(200);

    Serial.println("==================================================");
    Serial.println("  ROBO DE INSPECAO CONFINADA (UGV) - ESP32-CAM    ");
    Serial.println("    Firmware Unico com Camada de Abstracao HAL    ");
    Serial.println("==================================================");

    // Executa diagnóstico inicial
    runSelfTest();

    // ETAPA 2: Inicialização de Rede e Servidores Web/Stream
    telemetryServer.begin(&motors, &camera);
    Serial.println("[REDE] SoftAP (ROBO_INSPETOR_UGV) e Servidores HTTP ativos.");
}

// =============================================================================
// LOOP PRINCIPAL NÃO-BLOQUEANTE (EXECUÇÃO DE ETAPAS 3 A 8)
// =============================================================================
void loop() {
    uint32_t now = millis();

    // Processamento de comandos interativos pelo Monitor Serial (para testes no Wokwi)
    if (Serial.available()) {
        char cmd = Serial.read();
        motors.resetWatchdog();
        if (cmd == 'w' || cmd == 'W') {
            Serial.println("[MANUAL] Frente (180)");
            motors.drive(DriveDirection::FORWARD, 180);
        } else if (cmd == 's' || cmd == 'S') {
            Serial.println("[MANUAL] Ré (180)");
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
            static bool light_on = false;
            light_on = !light_on;
            ledcWrite(PWM_CHANNEL_HEADLIGHT, light_on ? 255 : 0);
            Serial.println(light_on ? "[FARÓIS] Ligados (100%)" : "[FARÓIS] Desligados");
        }
    }

    // ETAPAS 3 & 4: Atualização de Tração, Rampas e Watchdog (500 ms)
    if (currentState != RobotOperationalState::THERMAL_EMERGENCY_STOP) {
        motors.update();
    } else {
        motors.emergencyStop();
    }

    // ETAPA 5: Varredura Térmica e de Bateria Contínua Não-Bloqueante (100 ms)
    if ((now - lastThermalReadMs) >= SENSOR_READ_INTERVAL_MS) {
        lastThermalReadMs = now;
        currentThermalData = thermalSensor.read();
        readBatteryStatus();

        bool thermal_crit = (currentThermalData.is_valid && currentThermalData.object_temperature >= ALERTA_TEMP_CRITICA_C);
        bool thermal_warn = (currentThermalData.is_valid && currentThermalData.object_temperature >= ALERTA_TEMP_PREVENTIVO_C);

        if (thermal_crit || batteryCriticalStop) {
            if (currentState != RobotOperationalState::THERMAL_EMERGENCY_STOP) {
                currentState = RobotOperationalState::THERMAL_EMERGENCY_STOP;
                if (thermal_crit) {
                    Serial.println("[ALERTA CRITICO] 70°C ATINGIDOS! Parada de emergencia do UGV.");
                } else {
                    Serial.println("[ALERTA CRITICO] BATERIA < 9.9V! Corte de tracao para protecao das celulas 3S.");
                }
                motors.emergencyStop();
            }
        } else if (thermal_warn || batteryLowWarning) {
            if (currentState != RobotOperationalState::THERMAL_WARNING_PREVENTIVE) {
                currentState = RobotOperationalState::THERMAL_WARNING_PREVENTIVE;
                if (thermal_warn) {
                    Serial.println("[ALERTA PREVENTIVO] 50°C detectados no alvo.");
                } else {
                    Serial.println("[ALERTA PREVENTIVO] Bateria baixa (< 10.5V). Retornar o UGV.");
                }
            }
        } else {
            if (currentState != RobotOperationalState::NORMAL_OPERATION) {
                currentState = RobotOperationalState::NORMAL_OPERATION;
            }
        }
    }

    // Atualização da sinalização sonora/visual
    updateAlertSignaling(now);

    // ETAPA 6: Despacho de Telemetria (200 ms)
    if ((now - lastTelemetryMs) >= TELEMETRY_INTERVAL_MS) {
        lastTelemetryMs = now;

        RobotTelemetryState telem;
        telem.uptime_ms = now;
        telem.motor_left_speed = motors.getCurrentSpeedLeft();
        telem.motor_right_speed = motors.getCurrentSpeedRight();
        telem.target_left_speed = motors.getTargetSpeedLeft();
        telem.target_right_speed = motors.getTargetSpeedRight();
        telem.thermal = currentThermalData;
        telem.battery_voltage = currentBatteryVoltage;
        telem.battery_percentage = currentBatteryPct;
        telem.battery_low_warning = batteryLowWarning;
        telem.battery_critical_stop = batteryCriticalStop;
        telem.watchdog_alert = motors.isWatchdogTriggered();
        telem.wifi_rssi = WiFi.RSSI();

        telemetryServer.update(telem);
        telemetryServer.logSerialJson(telem);
    }
}
