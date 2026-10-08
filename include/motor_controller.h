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
    
    // Inicialização de GPIOs e configuração do gerador de PWM LEDC
    void begin();

    // Atualização de sinais físicos (chamado ciclicamente para processar rampas de aceleração)
    void update();

    // Comandos de movimento com rampa
    void setTargetSpeed(int16_t target_left, int16_t target_right);
    void drive(DriveDirection dir, uint8_t speed);

    // Failsafe e parada imediata (ignora rampa, corta PWM instantaneamente)
    void emergencyStop();

    // Notificação de recepção de pacote (alimenta o watchdog de comunicação)
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
    void applyHBridge(uint8_t pin_in_a, uint8_t pin_in_b, uint8_t pwm_channel, int16_t speed);
};
