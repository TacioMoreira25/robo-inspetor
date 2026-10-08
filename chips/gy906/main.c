#include "wokwi-api.h"

#define MLX90614_I2C_ADDR 0x5A
#define REG_TA            0x06
#define REG_TOBJ1         0x07

typedef struct {
    pin_t pin_scl;
    pin_t pin_sda;
    i2c_dev_t i2c;
    uint8_t current_reg;
    uint8_t read_byte_index;
    uint32_t attr_amb;
    uint32_t attr_obj;
} chip_state_t;

static chip_state_t g_chip;

static uint16_t celsius_to_raw(float celsius) {
    float kelvin = celsius + 273.15f;
    if (kelvin < 0.0f) kelvin = 0.0f;
    return (uint16_t)(kelvin * 50.0f);
}

static bool on_i2c_connect(void *user_data, uint32_t address, bool read) {
    chip_state_t *chip = (chip_state_t *)user_data;
    if (address != MLX90614_I2C_ADDR) {
        return false;
    }
    chip->read_byte_index = 0;
    return true; // ACK
}

static uint8_t on_i2c_read(void *user_data) {
    chip_state_t *chip = (chip_state_t *)user_data;
    float temp_c = 25.0f;

    if (chip->current_reg == REG_TA) {
        temp_c = attr_read_float(chip->attr_amb);
    } else if (chip->current_reg == REG_TOBJ1) {
        temp_c = attr_read_float(chip->attr_obj);
    }

    uint16_t raw_val = celsius_to_raw(temp_c);
    uint8_t byte_to_send = 0;

    if (chip->read_byte_index == 0) {
        byte_to_send = (uint8_t)(raw_val & 0xFF);         // Low Byte
    } else if (chip->read_byte_index == 1) {
        byte_to_send = (uint8_t)((raw_val >> 8) & 0xFF);  // High Byte
    } else {
        byte_to_send = 0x00;                              // PEC
    }

    chip->read_byte_index++;
    return byte_to_send;
}

static bool on_i2c_write(void *user_data, uint8_t data) {
    chip_state_t *chip = (chip_state_t *)user_data;
    chip->current_reg = data;
    return true; // ACK
}

static void on_i2c_disconnect(void *user_data) {
    chip_state_t *chip = (chip_state_t *)user_data;
    chip->read_byte_index = 0;
}

void chip_init(void) {
    g_chip.current_reg = 0;
    g_chip.read_byte_index = 0;
    g_chip.attr_amb = attr_init_float("ambient_temp", 25.0f);
    g_chip.attr_obj = attr_init_float("object_temp", 36.5f);

    const i2c_config_t i2c_config = {
        .user_data = &g_chip,
        .address = MLX90614_I2C_ADDR,
        .scl = pin_init("SCL", INPUT),
        .sda = pin_init("SDA", INPUT),
        .connect = on_i2c_connect,
        .read = on_i2c_read,
        .write = on_i2c_write,
        .disconnect = on_i2c_disconnect,
    };

    g_chip.i2c = i2c_init(&i2c_config);
}
