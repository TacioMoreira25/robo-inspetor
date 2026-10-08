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
    bool watchdog_alert;
    int8_t wifi_rssi;
};

class TelemetryServer {
public:
    TelemetryServer();
    
    // Inicialização da pilha de rede (AP ou STA) e servidores HTTP
    void begin(MotorController *motor_ctrl, CameraStream *cam_stream);
    
    // Processamento de requisições clientes (não-bloqueante)
    void update(const RobotTelemetryState &state);

    // Emissão serial formatada em JSON (para monitor Wokwi e telemetria por cabo)
    void logSerialJson(const RobotTelemetryState &state);

private:
    MotorController *_motor_ref;
    CameraStream *_cam_ref;
    uint32_t _last_telemetry_emit_ms;
    
    void handleRoot();
    void handleStatus(const RobotTelemetryState &state);
    void handleControl();
    void handleMjpegStream();
};
