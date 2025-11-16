#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ff.h"
#include "diskio.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "hardware/uart.h"


/*******************************************************************/

#define SD_MISO 12
#define SD_CS 13
#define SD_SCK 14
#define SD_MOSI 15

/*******************************************************************/

void init_spi_sdcard() {
    // fill in.
    spi_init(spi1, 400000);
    spi_set_format(spi1, 8, 0, 0, SPI_MSB_FIRST);
    gpio_set_function(12, GPIO_FUNC_SPI);
    gpio_set_function(14, GPIO_FUNC_SPI);
    gpio_set_function(15, GPIO_FUNC_SPI);

    sio_hw->gpio_oe_set = (1 << 13);
    gpio_set_function(13, GPIO_FUNC_SIO);
    sio_hw->gpio_set = (1 << 13);
}

void disable_sdcard() {
    // fill in.
    sio_hw->gpio_set = (1 << 13);
    uint8_t temp = 0xff;
    spi_write_blocking(spi1, &temp, 1);
    gpio_set_function(15, GPIO_FUNC_SIO);
    sio_hw->gpio_oe_set = (1 << 15);
    sio_hw->gpio_set = (1 << 15);
}

void enable_sdcard() {
    // fill in.
    sio_hw->gpio_clr = (1 << 13);
    gpio_set_function(15, GPIO_FUNC_SPI);
}

void sdcard_io_high_speed() {
    // fill in.
    spi_set_baudrate(spi1, 12000000);
}

void init_sdcard_io() {
    // fill in.
    init_spi_sdcard();
    disable_sdcard();
}

/*******************************************************************/

void init_uart();
void init_uart_irq();
void date(int argc, char *argv[]);
void command_shell();

int main() {
    // Initialize the standard input/output library
    init_uart();
    init_uart_irq();
    
    init_sdcard_io();
    
    // SD card functions will initialize everything.
    command_shell();

    for(;;);
}