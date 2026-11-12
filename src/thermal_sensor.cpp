/*
 * =============================================================================
 * LEITURA DO SENSOR TÉRMICO INFRAVERMELHO MLX90614 (thermal_sensor.cpp)
 * =============================================================================
 * PROTOCOLO SMBUS E CONVERSÃO TERMODINÂMICA:
 * 1. Transação I2C com Repeated Start:
 *    O padrão SMBus do MLX90614 exige a manutenção do barramento ativo entre
 *    a escrita do endereço do registrador (0x06 ou 0x07) e a leitura dos bytes
 *    de resposta via `Wire.endTransmission(false)`.
 * 2. Conversão de Escala (Kelvin para Celsius):
 *    O registrador de 16 bits fornece a leitura em centésimos de Kelvin com
 *    fator de escala de 0.02 K por LSB: T(°C) = (raw * 0.02) - 273.15.
 */

#include "thermal_sensor.h"
#include "config.h"
#include <Wire.h>

ThermalSensor::ThermalSensor(uint8_t i2c_addr) : _address(i2c_addr) {}

bool ThermalSensor::begin(int sda_pin, int scl_pin, uint32_t frequency) {
    // Inicia o barramento I2C nos pinos GPIO 21 (SDA) e GPIO 22 (SCL) a 100 kHz
    bool ok = Wire.begin(sda_pin, scl_pin, frequency);
    if (!ok) {
        return false;
    }
    return ping();
}

// Verifica se o sensor responde com ACK no endereço 0x5A
bool ThermalSensor::ping() {
    Wire.beginTransmission(_address);
    uint8_t error = Wire.endTransmission();
    return (error == 0);
}

// Lê um registrador de 16 bits do MLX90614 via protocolo SMBus
bool ThermalSensor::readRegister16(uint8_t reg, uint16_t &raw_value) {
    Wire.beginTransmission(_address);
    Wire.write(reg);

    // Envia Repeated Start (false) obrigatório para o MLX90614 não abortar a leitura
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    // Pede 3 bytes ao sensor: Byte Baixo (LSB), Byte Alto (MSB) e Verificador (PEC)
    uint8_t bytes_received = Wire.requestFrom((uint8_t)_address, (uint8_t)3);
    if (bytes_received < 2) {
        return false;
    }

    uint8_t lsb = Wire.read();
    uint8_t msb = Wire.read();
    if (Wire.available()) {
        Wire.read(); // Descarta o 3º byte (PEC)
    }

    // Junta os dois bytes de 8 bits em um número inteiro de 16 bits
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

    // O bit mais significativo (0x8000) no MLX90614 indica erro de leitura interna
    if (ok_ta && ok_tobj && !(raw_ta & 0x8000) && !(raw_tobj & 0x8000)) {
        data.ambient_temperature = (raw_ta * 0.02f) - 273.15f;
        data.object_temperature = (raw_tobj * 0.02f) - 273.15f;
        data.is_valid = true;
    }

    return data;
}
