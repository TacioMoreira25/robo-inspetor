/*
 * =============================================================================
 * IMPLEMENTAÇÃO DO CONTROLE DE MOTORES E PONTE H L298N (motor_controller.cpp)
 * =============================================================================
 * RACIONAL TÉCNICO DE ACIONAMENTO E SEGURANÇA:
 * 1. Controle por 4 Canais PWM (GPIOs 26, 27, 14, 13):
 *    Com os jumpers ENA e ENB fechados na placa L298N, utilizam-se 4 canais
 *    independentes do periférico LEDC da ESP32 a 1500 Hz.
 * 2. Rampa de Aceleração (computeRamp):
 *    Suaviza transições bruscas de velocidade, protegendo a transmissão mecânica
 *    dos motores JGB37-520, as polias em PETG e evitando quedas de tensão na bateria 3S.
 * 3. Watchdog Failsafe (1000 ms):
 *    Interrompe imediatamente os 4 sinais PWM caso a recepção de comandos cesse
 *    por período superior a 1 segundo.
 */

#include "motor_controller.h"
#include "config.h"

MotorController::MotorController()
    : _target_left(0),
      _target_right(0),
      _current_left(0),
      _current_right(0),
      _last_ramp_time_ms(0),
      _last_heartbeat_ms(0),
      _watchdog_active(false) {}

void MotorController::begin() {
    // Define os 4 pinos ligados à Ponte H L298N como saídas digitais
    pinMode(PIN_MOTOR_LEFT_IN1, OUTPUT);
    pinMode(PIN_MOTOR_LEFT_IN2, OUTPUT);
    pinMode(PIN_MOTOR_RIGHT_IN3, OUTPUT);
    pinMode(PIN_MOTOR_RIGHT_IN4, OUTPUT);

    // Configura os 4 canais internos de PWM da ESP32
    PWM_ATTACH_CH(PIN_MOTOR_LEFT_IN1, PWM_FREQ_HZ, PWM_RESOLUTION_BITS, PWM_CH_LEFT_IN1);
    PWM_ATTACH_CH(PIN_MOTOR_LEFT_IN2, PWM_FREQ_HZ, PWM_RESOLUTION_BITS, PWM_CH_LEFT_IN2);
    PWM_ATTACH_CH(PIN_MOTOR_RIGHT_IN3, PWM_FREQ_HZ, PWM_RESOLUTION_BITS, PWM_CH_RIGHT_IN3);
    PWM_ATTACH_CH(PIN_MOTOR_RIGHT_IN4, PWM_FREQ_HZ, PWM_RESOLUTION_BITS, PWM_CH_RIGHT_IN4);

    _last_heartbeat_ms = millis();
    emergencyStop(); // Garante que os motores iniciem 100% parados no boot
}

void MotorController::resetWatchdog() {
    _last_heartbeat_ms = millis();
    _watchdog_active = false;
}

bool MotorController::isWatchdogTriggered() const {
    return _watchdog_active;
}

// Aproxima a velocidade atual da velocidade desejada em degraus suaves (15 pontos por ciclo)
int16_t MotorController::computeRamp(int16_t current, int16_t target) {
    if (current < target) {
        current += MOTOR_RAMP_STEP;
        if (current > target) current = target;
    } else if (current > target) {
        current -= MOTOR_RAMP_STEP;
        if (current < target) current = target;
    }
    return current;
}

// Envia o pulso PWM para o par de pinos de um motor na Ponte H L298N
void MotorController::applyHBridge4Pin(uint8_t ch_in_a, uint8_t ch_in_b, int16_t speed) {
    speed = constrain(speed, -255, 255);

    // Zona Morta (Deadband = 35):
    // Se a velocidade pedida for muito baixa (entre 1 e 34), elevamos para 35
    // porque abaixo disso o motor de 24V não vence o atrito estático da esteira.
    if (speed > 0 && speed < MOTOR_DEADBAND) {
        speed = MOTOR_DEADBAND;
    } else if (speed < 0 && speed > -MOTOR_DEADBAND) {
        speed = -MOTOR_DEADBAND;
    }

    uint8_t duty = (uint8_t)abs(speed);

    if (speed > 0) {
        // Giro para FRENTE linear (sem degrau artificial que anula curvas suaves)
        PWM_WRITE_CH(ch_in_a, duty);
        PWM_WRITE_CH(ch_in_b, 0);
    } else if (speed < 0) {
        // Giro para RÉ linear
        PWM_WRITE_CH(ch_in_a, 0);
        PWM_WRITE_CH(ch_in_b, duty);
    } else {
        // PARADO: 0V em ambos os pinos
        PWM_WRITE_CH(ch_in_a, 0);
        PWM_WRITE_CH(ch_in_b, 0);
    }
}

void MotorController::update() {
    uint32_t now = millis();

    // 1. Verifica o Watchdog Failsafe (Se passou mais de 1000 ms sem comando novo, trava tudo)
    if ((now - _last_heartbeat_ms) > FAILSAFE_TIMEOUT_MS) {
        if (!_watchdog_active) {
            _watchdog_active = true;
            emergencyStop();
            return;
        }
    }

    // 2. Atualiza a Rampa de Aceleração a cada 20 ms (50 Hz)
    if ((now - _last_ramp_time_ms) >= MOTOR_RAMP_INTERVAL_MS) {
        _last_ramp_time_ms = now;

        // Atualiza a velocidade suavemente
        _current_left = computeRamp(_current_left, _target_left);
        _current_right = computeRamp(_current_right, _target_right);

        // Aplica continuamente o sinal PWM na Ponte H
        applyHBridge4Pin(PWM_CH_LEFT_IN1, PWM_CH_LEFT_IN2, _current_left);
        applyHBridge4Pin(PWM_CH_RIGHT_IN3, PWM_CH_RIGHT_IN4, _current_right);
    }
}

void MotorController::setTargetSpeed(int16_t target_left, int16_t target_right) {
    resetWatchdog(); // Avisa o cão de guarda que o operador está ativo
    _target_left = constrain(target_left, -255, 255);
    _target_right = constrain(target_right, -255, 255);
}

// Cinemática Diferencial de Esteiras (Mistura dos Eixos X e Y do Joystick):
// - Eixo Y controla Avanço (+) e Ré (-)
// - Eixo X controla Curva para Direita (+) e Esquerda (-)
void MotorController::driveJoystick(int16_t x, int16_t y) {
    // Eixo X invertido para corrigir sentido físico de Esquerda/Direita no chassi
    int16_t left = constrain(y - x, -255, 255);
    int16_t right = constrain(y + x, -255, 255);
    setTargetSpeed(left, right);
}

void MotorController::drive(DriveDirection dir, uint8_t speed) {
    resetWatchdog();
    switch (dir) {
        case DriveDirection::FORWARD:
            setTargetSpeed(speed, speed);
            break;
        case DriveDirection::BACKWARD:
            setTargetSpeed(-speed, -speed);
            break;
        case DriveDirection::SPIN_LEFT:
            setTargetSpeed(speed, -speed);
            break;
        case DriveDirection::SPIN_RIGHT:
            setTargetSpeed(-speed, speed);
            break;
        case DriveDirection::PIVOT_LEFT:
            setTargetSpeed(speed, 0);
            break;
        case DriveDirection::PIVOT_RIGHT:
            setTargetSpeed(0, speed);
            break;
        case DriveDirection::STOP:
        default:
            setTargetSpeed(0, 0);
            break;
    }
}

// Corta imediatamente o sinal PWM dos 4 pinos da Ponte H L298N
void MotorController::emergencyStop() {
    _target_left = 0;
    _target_right = 0;
    _current_left = 0;
    _current_right = 0;

    PWM_WRITE_CH(PWM_CH_LEFT_IN1, 0);
    PWM_WRITE_CH(PWM_CH_LEFT_IN2, 0);
    PWM_WRITE_CH(PWM_CH_RIGHT_IN3, 0);
    PWM_WRITE_CH(PWM_CH_RIGHT_IN4, 0);
}

// Envia um pulso curto para 1 pino específico (usado no Auto-Teste de Bancada - Tecla T)
void MotorController::pulseSinglePin(uint8_t ch, uint8_t pwm_duty, uint16_t duration_ms) {
    emergencyStop();
    resetWatchdog();
    PWM_WRITE_CH(ch, pwm_duty);
    delay(duration_ms);
    PWM_WRITE_CH(ch, 0);
    delay(150);
    resetWatchdog();
}
