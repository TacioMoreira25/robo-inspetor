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
    pinMode(PIN_MOTOR_LEFT_IN1, OUTPUT);
    pinMode(PIN_MOTOR_LEFT_IN2, OUTPUT);
    pinMode(PIN_MOTOR_RIGHT_IN3, OUTPUT);
    pinMode(PIN_MOTOR_RIGHT_IN4, OUTPUT);

    // Configuração dos 4 canais PWM independentes (Jumpers ENA/ENB fechados na L298N)
    ledcSetup(PWM_CH_LEFT_IN1, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_MOTOR_LEFT_IN1, PWM_CH_LEFT_IN1);

    ledcSetup(PWM_CH_LEFT_IN2, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_MOTOR_LEFT_IN2, PWM_CH_LEFT_IN2);

    ledcSetup(PWM_CH_RIGHT_IN3, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_MOTOR_RIGHT_IN3, PWM_CH_RIGHT_IN3);

    ledcSetup(PWM_CH_RIGHT_IN4, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_MOTOR_RIGHT_IN4, PWM_CH_RIGHT_IN4);

    _last_heartbeat_ms = millis();
    emergencyStop();
}

void MotorController::resetWatchdog() {
    _last_heartbeat_ms = millis();
    _watchdog_active = false;
}

bool MotorController::isWatchdogTriggered() const {
    return _watchdog_active;
}

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

void MotorController::applyHBridge4Pin(uint8_t ch_in_a, uint8_t ch_in_b, int16_t speed) {
    speed = constrain(speed, -255, 255);

    // Compensação de zona morta para vencer o atrito estático das esteiras
    if (speed > 0 && speed < MOTOR_DEADBAND) {
        speed = MOTOR_DEADBAND;
    } else if (speed < 0 && speed > -MOTOR_DEADBAND) {
        speed = -MOTOR_DEADBAND;
    }

    uint8_t duty = (uint8_t)abs(speed);

    if (speed > 0) {
        ledcWrite(ch_in_a, duty);
        ledcWrite(ch_in_b, 0);
    } else if (speed < 0) {
        ledcWrite(ch_in_a, 0);
        ledcWrite(ch_in_b, duty);
    } else {
        ledcWrite(ch_in_a, 0);
        ledcWrite(ch_in_b, 0);
    }
}

void MotorController::update() {
    uint32_t now = millis();

    // 1. Verificação contínua do Failsafe (1000 ms sem comando corta os motores)
    if ((now - _last_heartbeat_ms) > FAILSAFE_TIMEOUT_MS) {
        if (!_watchdog_active) {
            _watchdog_active = true;
            emergencyStop();
            return;
        }
    }

    // 2. Processamento da Rampa de Aceleração/Desaceleração
    if ((now - _last_ramp_time_ms) >= MOTOR_RAMP_INTERVAL_MS) {
        _last_ramp_time_ms = now;

        if (_current_left != _target_left) {
            _current_left = computeRamp(_current_left, _target_left);
            applyHBridge4Pin(PWM_CH_LEFT_IN1, PWM_CH_LEFT_IN2, _current_left);
        }

        if (_current_right != _target_right) {
            _current_right = computeRamp(_current_right, _target_right);
            applyHBridge4Pin(PWM_CH_RIGHT_IN3, PWM_CH_RIGHT_IN4, _current_right);
        }
    }
}

void MotorController::setTargetSpeed(int16_t target_left, int16_t target_right) {
    resetWatchdog();
    _target_left = constrain(target_left, -255, 255);
    _target_right = constrain(target_right, -255, 255);
}

void MotorController::driveJoystick(int16_t x, int16_t y) {
    // Cinemática diferencial: Y controla avanço/recuo, X controla giro diferencial
    int16_t left = constrain(y + x, -255, 255);
    int16_t right = constrain(y - x, -255, 255);
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
            setTargetSpeed(-speed, speed);
            break;
        case DriveDirection::SPIN_RIGHT:
            setTargetSpeed(speed, -speed);
            break;
        case DriveDirection::PIVOT_LEFT:
            setTargetSpeed(0, speed);
            break;
        case DriveDirection::PIVOT_RIGHT:
            setTargetSpeed(speed, 0);
            break;
        case DriveDirection::STOP:
        default:
            setTargetSpeed(0, 0);
            break;
    }
}

void MotorController::emergencyStop() {
    _target_left = 0;
    _target_right = 0;
    _current_left = 0;
    _current_right = 0;

    ledcWrite(PWM_CH_LEFT_IN1, 0);
    ledcWrite(PWM_CH_LEFT_IN2, 0);
    ledcWrite(PWM_CH_RIGHT_IN3, 0);
    ledcWrite(PWM_CH_RIGHT_IN4, 0);
}
