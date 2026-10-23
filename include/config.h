#pragma once

#include <Arduino.h>

// =============================================================================
// SELEÇÃO DE PLATAFORMA E DIRETIVAS DE COMPILAÇÃO
// =============================================================================
#ifndef MODO_SIMULACAO
    #define MODO_SIMULACAO 1
#endif

// =============================================================================
// MAPEAMENTO DE PINOS (GPIOs) - CONFORME ESPECIFICAÇÃO TÉCNICA OFICIAL
// =============================================================================
#if defined(PLACA_ESP32_CAM)

    // Barramento I2C para ESP32-CAM (Sensor MLX90614 / GY-906)
    #define PIN_I2C_SDA             21  // SDA I2C com pull-up externo de 4.7k
    #define PIN_I2C_SCL             22  // SCL I2C com pull-up externo de 4.7k

    // Tração Esquerda (Driver Ponte H L298N - Motores 24V)
    #define PIN_MOTOR_LEFT_PWM      14  // ENA (PWM de controle de velocidade)
    #define PIN_MOTOR_LEFT_IN1      27  // IN1 (Direção A)
    #define PIN_MOTOR_LEFT_IN2      26  // IN2 (Direção B)

    // Tração Direita (Driver Ponte H L298N - Motores 24V)
    #define PIN_MOTOR_RIGHT_PWM     12  // ENB (PWM de controle de velocidade)
    #define PIN_MOTOR_RIGHT_IN3     25  // IN3 (Direção A)
    #define PIN_MOTOR_RIGHT_IN4     33  // IN4 (Direção B)

    // Iluminação Frontal (Módulo MOSFET D4184 - Faróis Olho de Águia 12V Direct 3S)
    #define PIN_HEADLIGHT_PWM        4  // PWM Dimming para controle de intensidade

    // Interface Lógica de Alerta Local (Mini Protoboard)
    #define PIN_ALERT_BUZZER        13  // Transistor BC547 -> Buzzer Ativo 5V
    #define PIN_ALERT_LED           15  // LED Difuso Vermelho 5mm

    // Monitoramento de Bateria (Lido pela ESP32 DevKit V1 / Co-processador na GPIO 34)
    #define PIN_BATTERY_ADC         34

#else // PLACA_ESP32_DEVKIT Padrão (Ambiente Simulado Wokwi / DevKit V1)

    // Barramento I2C Padrão por Hardware
    #define PIN_I2C_SDA             21
    #define PIN_I2C_SCL             22

    // Tração Esquerda
    #define PIN_MOTOR_LEFT_PWM      14  // ENA
    #define PIN_MOTOR_LEFT_IN1      27  // IN1
    #define PIN_MOTOR_LEFT_IN2      26  // IN2

    // Tração Direita
    #define PIN_MOTOR_RIGHT_PWM     12  // ENB
    #define PIN_MOTOR_RIGHT_IN3     25  // IN3
    #define PIN_MOTOR_RIGHT_IN4     33  // IN4

    // Iluminação e Alertas
    #define PIN_HEADLIGHT_PWM        4  // MOSFET Faróis
    #define PIN_ALERT_BUZZER        13  // BC547 Buzzer
    #define PIN_ALERT_LED           15  // LED Alerta

    // Divisor de Tensão Analógico da Bateria 3S (10 kOhm e 3,3 kOhm -> GPIO 34 ADC1_CH6)
    #define PIN_BATTERY_ADC         34

#endif

// =============================================================================
// PARÂMETROS DO MONITORAMENTO DE BATERIA 3S2P (DIVISOR 10k / 3.3k)
// =============================================================================
#define BATTERY_R1_OHMS             10000.0f // Resistor superior ligado ao +12V (Direct 3S)
#define BATTERY_R2_OHMS             3300.0f  // Resistor inferior ligado ao GND
#define BATTERY_DIVIDER_RATIO       ((BATTERY_R1_OHMS + BATTERY_R2_OHMS) / BATTERY_R2_OHMS) // ~4.0303
#define BATTERY_ADC_VREF            3.30f    // Tensão de referência ADC da ESP32
#define BATTERY_ADC_MAX             4095.0f  // Resolução ADC de 12 bits (0 a 4095)

#define BATTERY_MAX_VOLTAGE         12.60f   // 100% de carga (4,20V por célula)
#define BATTERY_NOMINAL_VOLTAGE     11.10f   // Tensão nominal de operação (3,70V por célula)
#define BATTERY_WARN_VOLTAGE        10.50f   // Alerta preventivo de bateria fraca (3,50V/célula ~22%)
#define BATTERY_CRIT_VOLTAGE         9.90f   // Corte crítico de tração para proteção das células (3,30V/célula 0%)

// =============================================================================
// PARÂMETROS DO BARRAMENTO I2C E SENSOR GY-906 (MLX90614)
// =============================================================================
#define MLX90614_DEFAULT_ADDR       0x5A
#define MLX90614_REG_TA             0x06  // Temperatura Ambiente da carcaça (RAM)
#define MLX90614_REG_TOBJ1          0x07  // Temperatura do Objeto Infravermelho (RAM)
#define I2C_FREQUENCY_HZ            100000 // 100 kHz (Modo Standard SMBus)

// Limiares térmicos operacionais conforme especificação
#define ALERTA_TEMP_PREVENTIVO_C    50.0f // Aciona buzzer intermitente e LED vermelho
#define ALERTA_TEMP_CRITICA_C       70.0f // Parada crítica de emergência imediata (corte PWM)

// =============================================================================
// PARÂMETROS DE CONTROLE DE POTÊNCIA E PWM (L298N BJT DARLINGTON)
// =============================================================================
// ALERTA CRÍTICO: 1500 Hz para evitar superaquecimento no L298N BJT
#define PWM_FREQ_HZ                 1500  // 1.5 kHz (Faixa otimizada para BJT Darlington L298N)
#define PWM_RESOLUTION_BITS         8     // Resolução de 8 bits (0 a 255)
#define PWM_CHANNEL_LEFT            0
#define PWM_CHANNEL_RIGHT           1
#define PWM_CHANNEL_HEADLIGHT       2

// Dinâmica e Rampas de aceleração (Slew Rate Limiter)
#define MOTOR_RAMP_STEP             15    // Incremento máximo de PWM por ciclo de rampa
#define MOTOR_RAMP_INTERVAL_MS      20    // Período de atualização da rampa (50 Hz)
#define MOTOR_DEADBAND              35    // Offset para superar atrito das esteiras MTB

// =============================================================================
// TEMPORIZADORES, SEGURANÇA E FAILSAFE (WATCHDOG DE COMUNICAÇÃO)
// =============================================================================
#define FAILSAFE_TIMEOUT_MS         500   // Corte imediato de motores após 500ms sem comando
#define TELEMETRY_INTERVAL_MS       200   // Período de emissão de telemetria JSON (5 Hz)
#define SENSOR_READ_INTERVAL_MS     100   // Amostragem térmica I2C (10 Hz)
#define SERIAL_BAUD_RATE            115200

// =============================================================================
// REDE E COMUNICAÇÃO
// =============================================================================
#define WIFI_AP_SSID                "ROBO_INSPETOR_UGV"
#define WIFI_AP_PASS                "inspetor123"
#define WEB_SERVER_PORT             80
#define STREAM_SERVER_PORT          81
