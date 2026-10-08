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

    ledcSetup(PWM_CHANNEL_LEFT, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_MOTOR_LEFT_PWM, PWM_CHANNEL_LEFT);

    ledcSetup(PWM_CHANNEL_RIGHT, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_MOTOR_RIGHT_PWM, PWM_CHANNEL_RIGHT);

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

void MotorController::applyHBridge(uint8_t pin_in_a, uint8_t pin_in_b, uint8_t pwm_channel, int16_t speed) {
    speed = constrain(speed, -255, 255);

    // Tratamento de zona morta para superar atrito mecânico das esteiras
    if (speed > 0 && speed < MOTOR_DEADBAND) {
        speed = MOTOR_DEADBAND;
    } else if (speed < 0 && speed > -MOTOR_DEADBAND) {
        speed = -MOTOR_DEADBAND;
    }

    uint8_t duty = (uint8_t)abs(speed);

    if (speed > 0) {
        digitalWrite(pin_in_a, HIGH);
        digitalWrite(pin_in_b, LOW);
    } else if (speed < 0) {
        digitalWrite(pin_in_a, LOW);
        digitalWrite(pin_in_b, HIGH);
    } else {
        // Freio passivo / parada
        digitalWrite(pin_in_a, LOW);
        digitalWrite(pin_in_b, LOW);
    }

    ledcWrite(pwm_channel, duty);
}

void MotorController::update() {
    uint32_t now = millis();

    // 1. Verificação contínua do Failsafe (Watchdog de Comunicação)
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
            applyHBridge(PIN_MOTOR_LEFT_IN1, PIN_MOTOR_LEFT_IN2, PWM_CHANNEL_LEFT, _current_left);
        }

        if (_current_right != _target_right) {
            _current_right = computeRamp(_current_right, _target_right);
            applyHBridge(PIN_MOTOR_RIGHT_IN3, PIN_MOTOR_RIGHT_IN4, PWM_CHANNEL_RIGHT, _current_right);
        }
    }
}

void MotorController::setTargetSpeed(int16_t target_left, int16_t target_right) {
    resetWatchdog();
    _target_left = constrain(target_left, -255, 255);
    _target_right = constrain(target_right, -255, 255);
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

    digitalWrite(PIN_MOTOR_LEFT_IN1, LOW);
    digitalWrite(PIN_MOTOR_LEFT_IN2, LOW);
    digitalWrite(PIN_MOTOR_RIGHT_IN3, LOW);
    digitalWrite(PIN_MOTOR_RIGHT_IN4, LOW);

    ledcWrite(PWM_CHANNEL_LEFT, 0);
    ledcWrite(PWM_CHANNEL_RIGHT, 0);
}
