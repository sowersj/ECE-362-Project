#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "chardisp.h"

/***************************************************************** */

void send_spi_cmd(spi_inst_t* spi, uint16_t value) {
    while (spi_get_hw(spi)->sr & 0b1 << 4) {
        tight_loop_contents();
    }
    spi_get_hw(spi)->dr = value;
}

void send_spi_data(spi_inst_t* spi, uint16_t value) {
    send_spi_cmd(spi, value | 0x100);
}

void cd_init() {
    sleep_ms(1);
    send_spi_cmd(spi0, 0b111100);
    sleep_us(40);
    send_spi_cmd(spi0, 0b1100);
    sleep_us(40);
    send_spi_cmd(spi0, 0b1);
    sleep_ms(2);
    send_spi_cmd(spi0, 0b110);
    sleep_us(40);
}

void cd_display1(const int time, const char uv_str, const int temp, const int hum) {
    int uv = uv_str - '0';
    if (uv > 9) uv = 9;
    if (uv < 0) uv = 0;

    char line[17];

    if (time > 999) {
        snprintf(line, sizeof(line), "%2d U:%1dT:%2dH:%2d\0", time, uv, temp, hum);
    } 
    else if (time > 99) {
        snprintf(line, sizeof(line), "%2dU:%1d T:%2d H:%2d\0", time, uv, temp, hum);
    }
    else {
        snprintf(line, sizeof(line), "%2d U:%1d T:%2d H:%2d\0", time, uv, temp, hum);
    }

    send_spi_cmd(spi0, 0b10000000);

    for (int i = 0; i < 16; i++) {
        send_spi_data(spi0, line[i]);
    }
}

void cd_display2(const char *filename_str, const char *uv_str, const char *temp_str, const char *hum_str) {
    int uv = atoi(uv_str);
    int temp = atoi(temp_str);
    int hum = atoi(hum_str);
    int filename = atoi(filename_str);

    char line[17];

    if (filename > 999) {
        snprintf(line, sizeof(line), "%2d %1d  %3d   %2d\0", filename, uv, temp, hum);
    } 
    else if (filename > 99) {
        snprintf(line, sizeof(line), "%2d  %1d  %3d   %2d\0", filename, uv, temp, hum);
    }
    else {
        snprintf(line, sizeof(line), "%2d   %1d  %3d   %2d\0", filename, uv, temp, hum);
    }

    send_spi_cmd(spi0, 0b11000000);
    for (int i = 0; i < 16; i++) {
        send_spi_data(spi0, line[i]);
    }
}

/***************************************************************** */
