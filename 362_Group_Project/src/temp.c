#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#define I2C_SDA 28
#define I2C_SCL 29
//these are i2c0
#define I2C_inst i2c0

#define SHT30_ADDR 0x44 //addr pin connected to logic low

// 0x2024 to start periodic measurements @ medium repeatability and 0.5mps
// 0xE000 to fetch data
// 0x3093 to stop periodic measurement


int main() {
    stdio_init_all();
    init_i2c();
    start_measurement();

    uint8_t data[2];
    fetch_data(data);
    while(1) {
        printf("temp: %d, hum: %d\n", data[0], data[1]);
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
    control[0] = 0x20;
    control[1] = 0x24;

    i2c_write_blocking(I2C_inst, SHT30_ADDR, control, 2, false);
}

void fetch_data(uint8_t *data) {
    uint8_t control[2];
    control[0] = 0xE0;
    control[1] = 0x00;

    i2c_read_blocking(I2C_inst, control, data, 2, false);
}

void stop_measurement() {
    uint8_t control[2];
    control[0] = 0x30;
    control[1] = 0x93;

    i2c_write_blocking(I2C_inst, SHT30_ADDR, control, 2, false);
}