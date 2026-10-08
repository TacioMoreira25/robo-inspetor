#pragma once

#include <Arduino.h>

struct ThermalData {
    float ambient_temperature;
    float object_temperature;
    bool is_valid;
    uint32_t read_timestamp_ms;
};

class ThermalSensor {
public:
    explicit ThermalSensor(uint8_t i2c_addr = 0x5A);
    
    // Inicialização do barramento e verificação de presença
    bool begin(int sda_pin, int scl_pin, uint32_t frequency = 100000);
    
    // Varredura de integridade (Self-test)
    bool ping();
    
    // Leitura não-bloqueante via registradores de RAM
    ThermalData read();

private:
    uint8_t _address;
    bool readRegister16(uint8_t reg, uint16_t &raw_value);
};
