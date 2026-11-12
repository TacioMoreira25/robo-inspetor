#pragma once

/*
 * =============================================================================
 * DRIVER DO SENSOR TÉRMICO INFRAVERMELHO MLX90614 / GY-906 (thermal_sensor.h)
 * =============================================================================
 * RACIONAL TÉCNICO DE COMUNICAÇÃO I2C/SMBUS:
 * Realiza a leitura direta dos registradores internos de RAM do MLX90614 no
 * endereço 0x5A (0x06 para Temperatura Ambiente e 0x07 para Temperatura do Objeto)
 * utilizando o driver nativo Wire.h, garantindo portabilidade e baixa sobrecarga
 * de memória sem dependência de bibliotecas externas.
 */

#include <Arduino.h>

// Estrutura que agrupa o resultado de cada leitura de temperatura
struct ThermalData {
    float ambient_temperature;  // Temperatura do ar dentro/ao redor do robô (°C)
    float object_temperature;   // Temperatura do equipamento/alvo apontado pela lente (°C)
    bool is_valid;              // true = sensor respondeu OK | false = fio solto ou falha I2C
    uint32_t read_timestamp_ms; // Momento da leitura (em milissegundos desde que ligou)
};

class ThermalSensor {
public:
    explicit ThermalSensor(uint8_t i2c_addr = 0x5A);

    // Inicia os pinos SDA (21) e SCL (22) e verifica se o sensor responde no endereço 0x5A
    bool begin(int sda_pin, int scl_pin, uint32_t frequency = 100000);

    // Testa rapidamente se o sensor está vivo e conectado no barramento I2C
    bool ping();

    // Lê os registradores 0x06 e 0x07 e converte de Kelvin para Graus Celsius
    ThermalData read();

private:
    uint8_t _address;
    bool readRegister16(uint8_t reg, uint16_t &raw_value);
};
