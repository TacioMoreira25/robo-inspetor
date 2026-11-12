#pragma once

/*
 * =============================================================================
 * SERVIDOR WEB, REDE WI-FI E TELEMETRIA (telemetry.h)
 * =============================================================================
 * ARQUITETURA DE COMUNICAÇÃO MASTER / SLAVE:
 * - Modo Master (ESP32 DevKit V1): `beginMaster()` inicializa o ponto de acesso
 *   Wi-Fi próprio ("ROBO_INSPETOR_UGV", IP 192.168.4.1) e o servidor HTTP na
 *   porta 80 para o Dashboard Web, controle de tração, iluminação e diagnóstico.
 * - Modo Slave (ESP32-CAM): `beginSlaveCam()` conecta à rede da Master com IP
 *   estático 192.168.4.2 e disponibiliza o stream MJPEG contínuo na porta 81.
 */

#include <Arduino.h>
#include "thermal_sensor.h"
#include "motor_controller.h"
#include "camera_stream.h"

// Pacote com todos os dados atuais do robô para exibir no Painel Web e no cabo USB
struct RobotTelemetryState {
    uint32_t uptime_ms;           // Tempo ligado (ms)
    int16_t motor_left_speed;     // PWM atual enviado ao motor esquerdo (-255 a +255)
    int16_t motor_right_speed;    // PWM atual enviado ao motor direito (-255 a +255)
    int16_t target_left_speed;    // PWM alvo do motor esquerdo
    int16_t target_right_speed;   // PWM alvo do motor direito
    ThermalData thermal;          // Temperaturas lidas do sensor MLX90614
    uint8_t thermal_alert_level;  // 0 = Normal, 1 = Atenção (>=50°C), 2 = Emergência (>=70°C)
    uint8_t headlight_pwm;        // Brilho atual dos faróis 12V no MOSFET (0 a 255)
    bool watchdog_alert;          // true = Motores travados pelo Failsafe de 1s
    int8_t wifi_rssi;             // Nível de sinal Wi-Fi
    uint8_t wifi_clients;         // Quantidade de placas/celulares conectados no Wi-Fi do robô
};

class TelemetryServer {
public:
    TelemetryServer();

    // Inicia o Wi-Fi próprio "ROBO_INSPETOR_UGV" (192.168.4.1) e o Painel Web na Master
    void beginMaster(MotorController *motor_ctrl);

    // Conecta a ESP32-CAM no Wi-Fi da Master (192.168.4.2) e inicia o vídeo na porta 81
    void beginSlaveCam(CameraStream *cam_stream);

    // Atende os cliques e movimentos do Joystick no navegador (Master)
    void updateMaster(const RobotTelemetryState &state);

    // Atende o envio contínuo de quadros de vídeo (Slave ESP32-CAM)
    void updateSlaveCam();

    // Imprime o resumo em formato JSON no Monitor Serial do VS Code a cada 500 ms
    void logSerialJson(const RobotTelemetryState &state);

    uint8_t getHeadlightBrightness() const;
    void setHeadlightBrightness(uint8_t val);

private:
    MotorController *_motor_ref;
    CameraStream *_cam_ref;
};
