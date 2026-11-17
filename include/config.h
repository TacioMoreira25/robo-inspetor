#pragma once

/*
 * =============================================================================
 * CENTRAL DE CONFIGURAÇÃO DE HARDWARE E PARÂMETROS (config.h)
 * =============================================================================
 * ARQUITETURA E RACIONAL TÉCNICO:
 * Este cabeçalho centraliza o mapeamento físico de pinos (GPIOs), cores dos
 * condutores, frequências de operação e parâmetros de rede, desacoplando as
 * definições de hardware da lógica de controle dos módulos.
 */

#ifndef MODO_SIMULACAO
    #define MODO_SIMULACAO 0
#endif

// =============================================================================
// 1. TABELA OFICIAL DE PINOS E CORES - ESP32 DEVKIT V1 (MASTER)
// =============================================================================
// Controle da Ponte H L298N por 4 canais PWM (Jumpers ENA e ENB fechados na L298N,
// jumper regulador de 5V removido, 5V lógico vindo direto do Step-Down LM2596):
#define PIN_MOTOR_LEFT_IN1          27  // Fio Azul    -> Pino IN 2 da Ponte H L298N (Motor Esq. Frente - invertido via software)
#define PIN_MOTOR_LEFT_IN2          26  // Fio Cinza   -> Pino IN 1 da Ponte H L298N (Motor Esq. Ré - invertido via software)
#define PIN_MOTOR_RIGHT_IN3         14  // Fio Amarelo -> Pino IN 3 da Ponte H L298N (Motor Dir. Sentido A)
#define PIN_MOTOR_RIGHT_IN4         13  // Fio Verde   -> Pino IN 4 da Ponte H L298N (Motor Dir. Sentido B)

// Módulo MOSFET XY-MOS V921 (Acionamento dos 2 Faróis Olho de Águia 12V ligados em OUT+ e OUT-)
#define PIN_HEADLIGHT_MOSFET        25  // Fio Cinza (avulso) -> Furo TRIG/PWM do Módulo MOSFET XY-MOS
#define PIN_HEADLIGHT_PWM           PIN_HEADLIGHT_MOSFET

// Barramento I2C Hardware da ESP32 DevKit V1 (Sensor Térmico MLX90614 + Pull-ups de 4.7k para 3V3)
// Alimentação do sensor: Pino 3V3 (Fio Laranja -> VIN) e GND no Trilho Azul (Fio Azul -> GND)
#define PIN_I2C_SDA                 21  // Fio Amarelo -> Pino SDA do Sensor MLX90614
#define PIN_I2C_SCL                 22  // Fio Verde   -> Pino SCL do Sensor MLX90614

// =============================================================================
// 2. ENDEREÇAMENTO I2C E LIMITES DE SEGURANÇA TÉRMICA (MLX90614 / GY-906)
// =============================================================================
#define MLX90614_DEFAULT_ADDR       0x5A    // Endereço fixo de fábrica do sensor na rede I2C
#define MLX90614_REG_TA             0x06    // Registrador da Temperatura Ambiente
#define MLX90614_REG_TOBJ1          0x07    // Registrador da Temperatura do Objeto (Alvo)
#define I2C_FREQUENCY_HZ            100000  // Velocidade padrão do barramento SMBus/I2C (100 kHz)

#define ALERTA_TEMP_PREVENTIVO_C    50.0f   // >= 50 °C: Alerta amarelo no Painel Web e Serial
#define ALERTA_TEMP_CRITICA_C       70.0f   // >= 70 °C: Corta os motores imediatamente por segurança

// =============================================================================
// 3. PARÂMETROS DE PWM E CONTROLE SUAVE DOS MOTORES (PONTE H L298N)
// =============================================================================
// Frequência de 1500 Hz ideal para os transistores BJT Darlington da L298N
#define PWM_FREQ_HZ                 250     // 250 Hz (Maior torque e rendimento mecânico no L298N)
#define PWM_RESOLUTION_BITS         8       // Escala de 8 bits: 0 (parado) até 255 (100%)

// Canais internos independentes do periférico LEDC da ESP32
#define PWM_CH_LEFT_IN1             0       // Canal PWM para GPIO 26 (IN 1)
#define PWM_CH_LEFT_IN2             1       // Canal PWM para GPIO 27 (IN 2)
#define PWM_CH_RIGHT_IN3            2       // Canal PWM para GPIO 14 (IN 3)
#define PWM_CH_RIGHT_IN4            3       // Canal PWM para GPIO 13 (IN 4)
#define PWM_CH_HEADLIGHT            4       // Canal PWM para GPIO 25 (TRIG/PWM do MOSFET XY-MOS)

// Rampa de Aceleração (Slew Rate Limiter) e Zona Morta (Deadband)
#define MOTOR_RAMP_STEP             64      // Rampa rápida: atinge 100% de torque em 80ms
#define MOTOR_RAMP_INTERVAL_MS      20      // Atualização da rampa a 50 Hz
#define MOTOR_DEADBAND              35      // PWM mínimo para vencer o atrito das esteiras

// =============================================================================
// 4. TEMPORIZADORES E WATCHDOG FAILSAFE (CÃO DE GUARDA DE 1 SEGUNDO)
// =============================================================================
#define FAILSAFE_TIMEOUT_MS         3000    // 1000 ms (1 segundo) sem comando corta os motores
#define TELEMETRY_INTERVAL_MS       500     // Período de atualização de telemetria (500 ms)
#define SENSOR_READ_INTERVAL_MS     100     // Leitura térmica I2C a 10 Hz (100 ms)
#define SERIAL_BAUD_RATE            115200

// =============================================================================
// 5. REDE WI-FI (HOTSPOT DO CELULAR GALAXY A20 COMO ROTEADOR CENTRAL)
// =============================================================================
// Na arquitetura Hotspot, a ESP32 recebe o IP automaticamente via DHCP do celular
// (não usa mais o 192.168.4.1 fixo) e responde também em http://ugv.local
#define WIFI_AP_SSID                "robo_ugv"
#define WIFI_AP_PASS                "12345678"
#define WIFI_AP_CHANNEL             6
#define WIFI_CONNECT_TIMEOUT_MS     8000
#define MDNS_HOSTNAME               "ugv"

#define MASTER_STATIC_IP_ADDR       192, 168, 4, 1
#define SLAVE_CAM_STATIC_IP_ADDR    192, 168, 4, 50
#define NETWORK_GATEWAY_ADDR        192, 168, 4, 1
#define NETWORK_SUBNET_MASK         255, 255, 255, 0

#define CAM_STREAM_URL              "http://192.168.4.50:81/stream"
#define WEB_SERVER_PORT             80
#define STREAM_SERVER_PORT          81
