#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "ff.h"
#include "diskio.h"
#include "sdcard.h"
#include "chardisp.h"

const char keymap[16] = "DCBA#9630852*741";
char key = '\0';
int col = 0;
uint32_t adc_fifo_out = 0; //direct output of adc from uv sensor
char uv = 0; //I feel it'll be easier since this is set to be one byte
int temperature = 50;
int humidity = 100; 
char weather_block[512];
char filename[512];
//char hist_display[512];
int timestamp = 0; //set to 0 at the beginning of each reset
int delete_later = 0;
int keynum = 0;

//temp data
uint8_t data[4];

char a[100];
char b[100];
char c[100];

void init_keypad() {
    uint32_t in_mask = 0x3c;
    uint32_t out_mask = 0xf << (6);
    sio_hw->gpio_oe_clr = in_mask;
    sio_hw->gpio_oe_set = out_mask;
    sio_hw->gpio_oe_set = (1 << 13);
    for (int i = 2; i <= 9; i++){
        *(io_rw_32 *) hw_xor_alias_untyped((volatile void *) &pads_bank0_hw->io[i]) = (pads_bank0_hw->io[i] ^  PADS_BANK0_GPIO0_IE_BITS) & (PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS);
        io_bank0_hw->io[i].ctrl = GPIO_FUNC_SIO << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
        *(io_rw_32 *) hw_clear_alias_untyped((volatile void *) &pads_bank0_hw->io[i]) = PADS_BANK0_GPIO0_ISO_BITS;
    }

    //here I'm also initializing Csn pin.
    *(io_rw_32 *) hw_xor_alias_untyped((volatile void *) &pads_bank0_hw->io[13]) = (pads_bank0_hw->io[13] ^  PADS_BANK0_GPIO0_IE_BITS) & (PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS);
    io_bank0_hw->io[13].ctrl = GPIO_FUNC_SIO << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
    *(io_rw_32 *) hw_clear_alias_untyped((volatile void *) &pads_bank0_hw->io[13]) = PADS_BANK0_GPIO0_ISO_BITS;
}
void drive_column() {
    sio_hw->gpio_set = 1u << (6 + col);
    if (col == 0){
        sio_hw->gpio_clr = (1u << 7) | (1u << 8) | (1u << 9);
    }
    else if (col == 1){
        sio_hw->gpio_clr = (1u << 6) | (1u << 8) | (1u << 9);
    }
    else if (col == 2){
        sio_hw->gpio_clr = (1u << 6) | (1u << 7) | (1u << 9);
    }
    else if (col == 3){
        sio_hw->gpio_clr = (1u << 6) | (1u << 7) | (1u << 8);
    }

    sleep_ms(25);

    if (col < 3){
        col++;
    }
    else if (col == 3){
        col = 0;
    }
}

void init_chardisp_pins() {
    spi_init(spi0, 10000); 
    spi_set_format(spi0, 9, 0, 0, SPI_MSB_FIRST);
    gpio_set_function(17, GPIO_FUNC_SPI);
    gpio_set_function(18, GPIO_FUNC_SPI);
    gpio_set_function(19, GPIO_FUNC_SPI);
}

//---------------------------------------SD CARD------------------------------
void init_spi_sdcard() {
    spi_init(spi1, 400000);
    spi_set_format(spi1, 8, 0, 0, SPI_MSB_FIRST);
    gpio_set_function(12, GPIO_FUNC_SPI);
    gpio_set_function(14, GPIO_FUNC_SPI);
    gpio_set_function(15, GPIO_FUNC_SPI);

    sio_hw->gpio_oe_set = (1 << 13);
    gpio_set_function(13, GPIO_FUNC_SIO);
    sio_hw->gpio_set = (1 << 13);
}

void disable_sdcard(){
    sio_hw->gpio_set = (1 << 13);
    uint8_t temp = 0xff;
    spi_write_blocking(spi1, &temp, 1);
    gpio_set_function(15, GPIO_FUNC_SIO);
    sio_hw->gpio_oe_set = (1 << 15);
    sio_hw->gpio_set = (1 << 15);
}

void enable_sdcard(){
    sio_hw->gpio_clr = (1 << 13);
    gpio_set_function(15, GPIO_FUNC_SPI);
}

void sdcard_io_high_speed(){
    spi_set_baudrate(spi1, 12000000);
}

void init_sdcard_io(){
    init_spi_sdcard();
    disable_sdcard();
}

//--------------------------------------END SD CARD----------------------------------------

//--------------------------------------Begin UV-------------------------------------------
void init_adc() {
    // fill in
    adc_init();

    adc_gpio_init(45); // selects gpio pin used as adc
    adc_select_input(5); // selects channel 5 as adc input
    
}

uint16_t read_adc() {
    // fill in
    return adc_read();
}

void init_adc_freerun() {
    // fill in
    init_adc();

    adc_run(true);
}

void init_dma() {
    // fill in
    dma_hw->ch[0].read_addr = &adc_hw->fifo;
    dma_hw->ch[0].write_addr = &adc_fifo_out;

    dma_hw->ch[0].transfer_count = (1u << 28) | (1u);

    uint32_t temp = (1u << 2) | (48u << 17) | (1u << 0);

    dma_hw->ch[0].ctrl_trig = temp;
}

void init_adc_dma() {
    // fill in
    init_dma();
    init_adc_freerun();

    hw_write_masked(&adc_hw->fcs, (1u << ADC_FCS_EN_LSB) | (1u << ADC_FCS_DREQ_EN_LSB), ADC_FCS_EN_BITS | ADC_FCS_DREQ_EN_BITS);
}
//--------------------------------------End UV---------------------------------------------

//--------------------------------------Start Temp---------------------------------------------
#define I2C_SDA 28
#define I2C_SCL 29
//these are i2c0
#define I2C_inst i2c0

#define SHT30_ADDR 0x44 //addr pin connected to logic low

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

//--------------------------------------End Temp---------------------------------------------

void keypad_isr() {
    int row = 0;
    int afinalval = 0;
    
    for (int i = 2; i < 6; i++){
        //would we change to "and" instead of == because we could possibly have a different interrupt
        if (gpio_get_irq_event_mask(i) == GPIO_IRQ_EDGE_RISE){
            gpio_acknowledge_irq(i, GPIO_IRQ_EDGE_RISE);
            row = i - 2;
            key = keymap[(col*4) + row];
            //printf("Pressed: %c\n", key);

            //logic: we will read from the file who's name is timestamp - key
            keynum = key - '0';
            int temp1 = timestamp - keynum;
            if (temp1 < 0){
                temp1 = 1;
            }

            sprintf(filename, "%d", temp1);
            cat(filename, a, b, c);
            printf("The file named %s, which was %d seconds ago, has a UV of: %d, a humidity of: %d, and a temperature of: %d.\n", filename, keynum, atoi(a), atoi(b), atoi(c));
            cd_display2(filename, a, b, c);
        }
    }
}

void init_keypad_irq() {
    irq_set_enabled(IO_IRQ_BANK0, true);
    gpio_add_raw_irq_handler_masked(((1u << 2) | (1u << 3) | (1u << 4) | (1u << 5)), keypad_isr);

    gpio_set_irq_enabled(2, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(3, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(4, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(5, GPIO_IRQ_EDGE_RISE, true);
}

void timer_isr(){
    //The FIRST part of this function is going to be reading in data on all the peripherals
    //CHECK ON THIS! The timer isr has priority over the keypad isr, so I think we should send this frequency to be something REALLY LOW (like 0.2 Hz - fires every 5 seconds) to make sure the interrupts don't clash. Or does this not matter (i.e. since main isn't running while the timer isr is running, does the old keypad press get saved?)
    //we can save each data point to a global variable called "humidity", "temp", and "uv" - currently ints but should be changed to strings. 
    //For now, for testing purposes, I'm going to increment the value on each timer hit to simulate a new value being read in
    /*if (uv < 100){ FOR UNINTEGRATED TESTING ONLY
        uv += 3;
    }
    else{
        uv = 0;
    }
    humidity += 3;
    temperature -= 3;*/

    float v = (adc_fifo_out * 0.42) / (1u << 12);
    float uvi = v / 0.1; // using 1M resistor
    uv = (char)((int) uvi);

    //collect temp info
    fetch_data(data);
    uint16_t temp_b = (data[0] << 8) | data[1];
    uint16_t humid_b = (data[2] << 8) | data[3];
    temperature = 315.0 * ((float) temp_b) / (pow(2, 16) - 1) - 49;
    humidity = 100.0 * ((float) humid_b) / (pow(2, 16) - 1);

    //The SECOND part is what we need to send that data to the SD Card
    hw_clear_bits(&timer1_hw->intr, 1u << 0);
    //printf("%d\n", uv); //TODO: There's an issue right now - for some reason it's starting at 8. But, it does increment as expected, every second

    sprintf(weather_block, "%d\n%d\n%d\n%d\n", uv, humidity, temperature);
    sprintf(filename, "%d", timestamp);

    //Check if file with name of timestamp has already been created. If it has, remove it
    if (input(filename, weather_block)){
        rm(filename);
        input(filename, weather_block);
    }

    timestamp++;

    uint64_t target = timer1_hw->timerawl + 1000000; //TODO: Check on what time this represents.
    timer1_hw->alarm[0] = (uint32_t) target;

    //The THIRD part of this function is sending data to LCD: at the moment, this is commented out so we only see the value on the LCD when a button is pressed 
    //cd_display2(&humidity); //The number of bits sent here is wrong, so we'll have to modify that.
}

void init_timer_irq(){
    hw_set_bits(&timer1_hw->inte, (1u << 0));
    irq_set_exclusive_handler(timer_hardware_alarm_get_irq_num(timer1_hw, 0), timer_isr);
    irq_set_enabled(timer_hardware_alarm_get_irq_num(timer1_hw, 0), true);
    uint64_t target = timer1_hw->timerawl + 1000; //this makes it go off approximately once every second
    timer1_hw->alarm[0] = (uint32_t) target;
}

int main(){
    stdio_init_all();
    init_chardisp_pins();
    cd_init();
    init_i2c();
    start_measurement();
    init_keypad();
    init_keypad_irq();
    init_timer_irq();
    init_sdcard_io();
    init_adc_dma();
    mount();
    sleep_ms(100);
    // sprintf(filename, "%d", 1099);
    // sprintf(weather_block, "%d", 6);
    // rm(filename);
    // sleep_ms(20);
    // input(filename, weather_block);
    // sleep_ms(20);
    // delete_later = cat(filename);
    // printf("Value: %c\n", delete_later);
    //cd_init();
    //cd_display1("Humidity today is:  ");
    for (;;) {
        cd_display1(uv, temperature, humidity);
        drive_column();
    }
}
