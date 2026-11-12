#pragma once

/*
 * =============================================================================
 * CONTROLADOR DE TRAÇÃO DIFERENCIAL E PONTE H L298N (motor_controller.h)
 * =============================================================================
 * PROPÓSITO DO MÓDULO:
 * Encapsula a cinemática diferencial das esteiras (conversão dos eixos X e Y do
 * Joystick em velocidades independentes), o controle de rampa de aceleração
 * (Slew Rate Limiter), a compensação de zona morta (Deadband) e o desligamento
 * automático de segurança por perda de comunicação (Watchdog Failsafe).
 */

#include <Arduino.h>

// Direções básicas para controle pelo teclado (W, A, S, D e Espaço)
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

    // Configura os 4 pinos da ESP32 DevKit V1 (GPIOs 26, 27, 14, 13) como saídas PWM de 1500 Hz
    void begin();

    // Chamado continuamente no loop(): aplica a rampa suave e checa se o Failsafe de 1s estourou
    void update();

    // Define a velocidade desejada de cada esteira (-255 = ré máxima, 0 = parado, +255 = frente máxima)
    void setTargetSpeed(int16_t target_left, int16_t target_right);

    // Converte a posição (X, Y) do Joystick Virtual na velocidade individual da roda esquerda e direita
    void driveJoystick(int16_t x, int16_t y);

    // Atalho para movimentação por teclas discretas (W, A, S, D)
    void drive(DriveDirection dir, uint8_t speed);

    // Parada de Emergência: zera imediatamente os 4 pinos PWM sem esperar rampa
    void emergencyStop();

    // Dá um pulso curto em 1 único pino da Ponte H (usado no botão "Testar Motores / Tecla T")
    void pulseSinglePin(uint8_t ch, uint8_t pwm_duty, uint16_t duration_ms);

    // Reinicia o cronômetro de 1 segundo do Watchdog sempre que chega um comando novo do operador
    void resetWatchdog();
    bool isWatchdogTriggered() const;

    // Funções de leitura para mostrar a velocidade atual na tela e no Monitor Serial
    int16_t getCurrentSpeedLeft() const { return _current_left; }
    int16_t getCurrentSpeedRight() const { return _current_right; }
    int16_t getTargetSpeedLeft() const { return _target_left; }
    int16_t getTargetSpeedRight() const { return _target_right; }

private:
    int16_t _target_left;       // Velocidade onde o motor esquerdo QUER chegar
    int16_t _target_right;      // Velocidade onde o motor direito QUER chegar
    int16_t _current_left;      // Velocidade real que está sendo enviada AGORA ao motor esquerdo
    int16_t _current_right;     // Velocidade real que está sendo enviada AGORA ao motor direito

    uint32_t _last_ramp_time_ms; // Guarda o instante do último degrau de aceleração
    uint32_t _last_heartbeat_ms; // Guarda o instante do último comando recebido do usuário
    bool _watchdog_active;       // Indica se o robô está travado por falta de sinal (> 1 segundo)

    int16_t computeRamp(int16_t current, int16_t target);
    void applyHBridge4Pin(uint8_t ch_in_a, uint8_t ch_in_b, int16_t speed);
};
