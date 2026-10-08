#include "thermal_sensor.h"
#include "config.h"
#include <Wire.h>

ThermalSensor::ThermalSensor(uint8_t i2c_addr) : _address(i2c_addr) {}

bool ThermalSensor::begin(int sda_pin, int scl_pin, uint32_t frequency) {
    bool ok = Wire.begin(sda_pin, scl_pin, frequency);
    if (!ok) {
        return false;
    }
    return ping();
}

bool ThermalSensor::ping() {
    Wire.beginTransmission(_address);
    uint8_t error = Wire.endTransmission();
    return (error == 0);
}

bool ThermalSensor::readRegister16(uint8_t reg, uint16_t &raw_value) {
    Wire.beginTransmission(_address);
    Wire.write(reg);
    
    // Mantém o barramento ocupado (Repeated Start / false) para compatibilidade com SMBus
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    // O protocolo MLX90614 retorna: Data LSB (8 bits), Data MSB (8 bits) e PEC (CRC-8)
    uint8_t bytes_received = Wire.requestFrom((uint8_t)_address, (uint8_t)3);
    if (bytes_received < 2) {
        return false;
    }

    uint8_t lsb = Wire.read();
    uint8_t msb = Wire.read();
    if (Wire.available()) {
        Wire.read(); // Leitura e descarte do PEC
    }

    raw_value = (uint16_t)(lsb | (msb << 8));
    return true;
}

ThermalData ThermalSensor::read() {
    ThermalData data;
    data.ambient_temperature = 0.0f;
    data.object_temperature = 0.0f;
    data.is_valid = false;
    data.read_timestamp_ms = millis();

    uint16_t raw_ta = 0;
    uint16_t raw_tobj = 0;

    bool ok_ta = readRegister16(MLX90614_REG_TA, raw_ta);
    bool ok_tobj = readRegister16(MLX90614_REG_TOBJ1, raw_tobj);

    if (ok_ta && ok_tobj) {
        // Conversão oficial Melexis:
        // Temp (Kelvin) = raw_data * 0.02
        // Temp (Celsius) = Temp (Kelvin) - 273.15
        data.ambient_temperature = (raw_ta * 0.02f) - 273.15f;
        data.object_temperature = (raw_tobj * 0.02f) - 273.15f;
        data.is_valid = true;
    }

    return data;
}
