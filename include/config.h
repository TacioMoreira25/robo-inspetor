#pragma once

#include <Arduino.h>

// =============================================================================
// SELEÇÃO DE PLATAFORMA E DIRETIVAS DE COMPILAÇÃO
// =============================================================================
// Arquitetura Dual-ESP32:
// - PLACA_ESP32_DEVKIT (Master): Controla Ponte H L298N (4 fios PWM), Sensor I2C MLX90614,
//   Módulo MOSFET Duplo (Faróis 12V) e hospeda o Dashboard Web (IP 192.168.1.100).
// - PLACA_ESP32_CAM (Slave): Isolada de fios de controle nos pinos de boot (12, 13, 14, 15),
//   dedicada 100% ao streaming MJPEG da câmera OV2640 na porta 81 (IP 192.168.1.101).
#ifndef MODO_SIMULACAO
    #define MODO_SIMULACAO 1
#endif

// =============================================================================
// TABELA DEFINITIVA DE PINOUT - ESP32 DEVKIT V1 (MASTER)
// =============================================================================
// Barramento I2C por Hardware (Sensor Térmico MLX90614 / GY-906)
#define PIN_I2C_SDA                 21  // SDA I2C (Pull-up externo de 4.7 kOhm para 3V3)
#define PIN_I2C_SCL                 22  // SCL I2C (Pull-up externo de 4.7 kOhm para 3V3)

// Controle da Ponte H L298N (4 Fios Diretos com Jumpers ENA/ENB Fechados)
#define PIN_MOTOR_LEFT_IN1          26  // IN 1 - Motor Esquerdo A (PWM / Direção)
#define PIN_MOTOR_LEFT_IN2          27  // IN 2 - Motor Esquerdo B (PWM / Direção)
#define PIN_MOTOR_RIGHT_IN3         14  // IN 3 - Motor Direito A  (PWM / Direção)
#define PIN_MOTOR_RIGHT_IN4         13  // IN 4 - Motor Direito B  (PWM / Direção)

// Iluminação Frontal (Módulo MOSFET Duplo - Faróis Olho de Águia 12V)
#define PIN_HEADLIGHT_PWM           25  // SIG / PWM do Módulo MOSFET Duplo

// Porta Serial2 (Reserva Física no Chassi p/ U0T e U0R da ESP32-CAM)
#define PIN_SERIAL2_RX              16  // RX2 (Fio Verde - Opcional/Reserva)
#define PIN_SERIAL2_TX              17  // TX2 (Fio Amarelo - Opcional/Reserva)

// =============================================================================
// PARÂMETROS DO BARRAMENTO I2C E SENSOR GY-906 (MLX90614)
// =============================================================================
#define MLX90614_DEFAULT_ADDR       0x5A
#define MLX90614_REG_TA             0x06   // Temperatura Ambiente da carcaça (RAM)
#define MLX90614_REG_TOBJ1          0x07   // Temperatura do Objeto Infravermelho (RAM)
#define I2C_FREQUENCY_HZ            100000 // 100 kHz (Modo Standard SMBus)

// Limiares térmicos operacionais exibidos no Dashboard Web e JSON Serial
#define ALERTA_TEMP_PREVENTIVO_C    50.0f  // Alerta preventivo na interface e telemetria
#define ALERTA_TEMP_CRITICA_C       70.0f  // Parada crítica de emergência imediata (corte PWM)

// =============================================================================
// PARÂMETROS DE CONTROLE DE POTÊNCIA E PWM (L298N BJT DARLINGTON)
// =============================================================================
// Frequência de 1500 Hz otimizada para os transistores BJT Darlington da L298N
#define PWM_FREQ_HZ                 1500   // 1.5 kHz
#define PWM_RESOLUTION_BITS         8      // Resolução de 8 bits (0 a 255)

// Canais LEDC independentes para PWM de 4 pinos + Farol MOSFET
#define PWM_CH_LEFT_IN1             0
#define PWM_CH_LEFT_IN2             1
#define PWM_CH_RIGHT_IN3            2
#define PWM_CH_RIGHT_IN4            3
#define PWM_CH_HEADLIGHT            4

// Dinâmica e Rampas de aceleração (Slew Rate Limiter)
#define MOTOR_RAMP_STEP             15     // Incremento máximo de PWM por ciclo de rampa
#define MOTOR_RAMP_INTERVAL_MS      20     // Período de atualização da rampa (50 Hz)
#define MOTOR_DEADBAND              35     // Offset para superar atrito estático das esteiras MTB

// =============================================================================
// TEMPORIZADORES, SEGURANÇA E FAILSAFE (WATCHDOG DE COMUNICAÇÃO)
// =============================================================================
#define FAILSAFE_TIMEOUT_MS         1000   // Corte imediato dos motores após 1 segundo sem comando Wi-Fi/Serial
#define TELEMETRY_INTERVAL_MS       500    // Período de atualização de telemetria térmica (2 Hz / 500 ms)
#define SENSOR_READ_INTERVAL_MS     100    // Amostragem térmica I2C interna (10 Hz)
#define SERIAL_BAUD_RATE            115200

// =============================================================================
// REDE WI-FI PRÓPRIA NO ROBÔ (SOFTAP 100% AUTÔNOMO E ANTI-LAG)
// =============================================================================
// A ESP32 DevKit V1 (Master) cria o Wi-Fi "ROBO_INSPETOR_UGV" (IP 192.168.4.1)
// A ESP32-CAM (Slave) conecta nesse Wi-Fi com IP fixo 192.168.4.2
#define WIFI_AP_SSID                "ROBO_INSPETOR_UGV"
#define WIFI_AP_PASS                "12345678"
#define WIFI_AP_CHANNEL             6
#define WIFI_CONNECT_TIMEOUT_MS     8000

// Endereçamento IP Estático na Rede Própria do Robô (192.168.4.x)
#define MASTER_STATIC_IP_ADDR       192, 168, 4, 1
#define SLAVE_CAM_STATIC_IP_ADDR    192, 168, 4, 2
#define NETWORK_GATEWAY_ADDR        192, 168, 4, 1
#define NETWORK_SUBNET_MASK         255, 255, 255, 0

#define CAM_STREAM_URL              "http://192.168.4.2:81/stream"
#define WEB_SERVER_PORT             80
#define STREAM_SERVER_PORT          81
