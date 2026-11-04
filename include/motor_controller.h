#pragma once

#include <Arduino.h>

enum class DriveDirection {
    STOP,
    FORWARD,
    BACKWARD,
    SPIN_LEFT,
    SPIN_RIGHT,
    PIVOT_LEFT,
    PIVOT_RIGHT
};

class MotorController {
public:
    MotorController();

    // Inicialização dos 4 canais PWM LEDC nos pinos IN1..IN4 (GPIOs 26, 27, 14, 13)
    void begin();

    // Atualização cíclica de rampas de aceleração e verificação do Failsafe (1000 ms)
    void update();

    // Controle direto das esteiras (-255 a +255 para cada lado)
    void setTargetSpeed(int16_t target_left, int16_t target_right);

    // Cinemática diferencial por eixos X e Y do Joystick (-255 a +255)
    void driveJoystick(int16_t x, int16_t y);

    // Comandos discretos de direção (W, A, S, D)
    void drive(DriveDirection dir, uint8_t speed);

    // Parada imediata (corta PWM dos 4 canais instantaneamente)
    void emergencyStop();

    // Alimentação do Watchdog Failsafe
    void resetWatchdog();
    bool isWatchdogTriggered() const;

    // Getters de telemetria
    int16_t getCurrentSpeedLeft() const { return _current_left; }
    int16_t getCurrentSpeedRight() const { return _current_right; }
    int16_t getTargetSpeedLeft() const { return _target_left; }
    int16_t getTargetSpeedRight() const { return _target_right; }

private:
    int16_t _target_left;
    int16_t _target_right;
    int16_t _current_left;
    int16_t _current_right;

    uint32_t _last_ramp_time_ms;
    uint32_t _last_heartbeat_ms;
    bool _watchdog_active;

    int16_t computeRamp(int16_t current, int16_t target);
    void applyHBridge4Pin(uint8_t ch_in_a, uint8_t ch_in_b, int16_t speed);
};
