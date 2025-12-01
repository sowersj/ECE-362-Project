#ifndef CHARDISP_H
#define CHARDISP_H

void send_spi_cmd(spi_inst_t* spi, uint16_t value);
void send_spi_data(spi_inst_t* spi, uint16_t value);
void cd_init();
void cd_display1(const int time, const char uv_str, const int temp, const int hum);
void cd_display2(const char *filename_str, const char *uv_str, const char *temp_str, const char *hum_str);

#endif