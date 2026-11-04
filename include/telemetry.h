#pragma once

#include <Arduino.h>
#include "thermal_sensor.h"
#include "motor_controller.h"
#include "camera_stream.h"

struct RobotTelemetryState {
    uint32_t uptime_ms;
    int16_t motor_left_speed;
    int16_t motor_right_speed;
    int16_t target_left_speed;
    int16_t target_right_speed;
    ThermalData thermal;
    uint8_t thermal_alert_level; // 0 = OK, 1 = Alerta Preventivo (>=50C), 2 = Emergência (>=70C)
    uint8_t headlight_pwm;
    bool watchdog_alert;
    int8_t wifi_rssi;
};

class TelemetryServer {
public:
    TelemetryServer();

    // Inicialização da rede Wi-Fi (STA com IP Fixo 192.168.1.100 ou Fallback SoftAP) e servidor HTTP
    void beginMaster(MotorController *motor_ctrl);

    // Inicialização dedicada da ESP32-CAM Slave (IP Fixo 192.168.1.101, Porta 81 /stream)
    void beginSlaveCam(CameraStream *cam_stream);

    // Processamento de requisições HTTP do Master
    void updateMaster(const RobotTelemetryState &state);

    // Processamento de requisições HTTP de vídeo da Slave CAM
    void updateSlaveCam();

    // Emissão serial formatada em JSON (a cada 500 ms)
    void logSerialJson(const RobotTelemetryState &state);

    uint8_t getHeadlightBrightness() const;
    void setHeadlightBrightness(uint8_t val);

private:
    MotorController *_motor_ref;
    CameraStream *_cam_ref;
};
