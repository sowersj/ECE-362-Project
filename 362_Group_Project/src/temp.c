#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "math.h"

#define I2C_SDA 28
#define I2C_SCL 29
//these are i2c0
#define I2C_inst i2c0

#define SHT30_ADDR 0x44 //addr pin connected to logic low

// 0x2126 to start periodic measurements @ medium repeatability and 1mps
// 0xE000 to fetch data
// 0x3093 to stop periodic measurement

void init_i2c();
void start_measurement();
void fetch_data(uint8_t *data);
void stop_measurement();



int main() {
    stdio_init_all();
    init_i2c();
    start_measurement();

    uint8_t data[4];

    uint16_t temp_b;
    uint16_t humid_b;

    while(1) {
        fetch_data(data);

        temp_b = (data[0] << 8) | data[1];
        humid_b = (data[2] << 8) | data[3];

        float temp = 315.0 * ((float) temp_b) / (pow(2, 16) - 1) - 49;
        float humid = 100.0 * ((float) humid_b) / (pow(2, 16) - 1);

        printf("temp: %.1f°F, hum: %.1f%%\n", temp, humid);
        sleep_ms(2000);
    }
}

void init_i2c() {
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    i2c_init(i2c0, 1000);
}

void start_measurement() {
    uint8_t control[2];
    control[0] = 0x21;
    control[1] = 0x26;

    i2c_write_blocking(I2C_inst, SHT30_ADDR, control, 2, false);
}

// void fetch_data(uint8_t *data) {
//     uint8_t control[2];
//     control[0] = 0xE0;
//     control[1] = 0x00;

//     i2c_write_blocking(I2C_inst, SHT30_ADDR, control, 2, false);

//     i2c_read_blocking(I2C_inst, SHT30_ADDR, data, 4, false);
// }

void fetch_data(uint8_t *data) {
    uint8_t fetch_cmd[2] = {0xE0, 0x00};  // Fetch data command
    
    // Send fetch command
    i2c_write_blocking(I2C_inst, SHT30_ADDR, fetch_cmd, 2, true);  // true = repeated start
    
    // Read 6 bytes: temp_msb, temp_lsb, temp_crc, humid_msb, humid_lsb, humid_crc
    uint8_t buffer[6];
    i2c_read_blocking(I2C_inst, SHT30_ADDR, buffer, 6, false);
    
    // Copy temperature and humidity data (skipping CRC bytes for now)
    data[0] = buffer[0];  // temp MSB
    data[1] = buffer[1];  // temp LSB
    data[2] = buffer[3];  // humid MSB
    data[3] = buffer[4];  // humid LSB
}

void stop_measurement() {
    uint8_t control[2];
    control[0] = 0x30;
    control[1] = 0x93;

    i2c_write_blocking(I2C_inst, SHT30_ADDR, control, 2, false);
}