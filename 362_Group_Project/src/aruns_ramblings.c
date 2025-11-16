#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/adc.h"

const char keymap[16] = "DCBA#9630852*741";
char key = '\0';
int col = 0;
char uv = 0; //I feel it'll be easier since this is set to be one byte
int temp = 50;
int humidity = 100; 
int sd_counter = 0;
char weather_block[512];
char hist_display[512];

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
    spi_write_blocking(spi1, temp, 1);
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

spi_inst_t *sd = spi1; // the SPI interface to use for the SD card

// Weak definitions for the functions that must be implemented elsewhere
// to allow the SPI interface for the SD card to work.
__attribute((weak)) void init_sdcard_io() {
    puts("init_sdcard_io() not implemented.");
}
__attribute((weak)) void sdcard_io_high_speed(void) {
    puts("sdcard_io_high_speed() not implemented.");
}
__attribute((weak)) void enable_sdcard(void) {
    puts("enable_sdcard() not implemented.");
}
__attribute((weak)) void disable_sdcard(void) {
    puts("disable_sdcard() not implemented.");
}

// Make sure the receive FIFO of the SPI interface is clear.
void spi_clear_rxfifo(spi_inst_t *s) {
    while (spi_is_readable(s)) {
        (void)spi_get_hw(s)->dr;
    }
}

// Write a single byte to the SD card interface and read a value
// back.  Wait until the read/write exchange has completed before
// reading the value from the SPI_DR and returning that value.
uint8_t sdcard_write(uint8_t b)
{
    spi_write_blocking(sd, &b, 1);
    return (uint8_t)spi_get_hw(sd)->dr;
}

// uint8_t sdcard_read(void)
// {
//     uint8_t value = 0xff;
//     spi_read_blocking(sd, 0xff, &value, 1);
//     return value;
// }

// Write 10 bytes of 0xff (80 bits total) to initialize the SD card
// into legacy SPI mode.
void sdcard_init_clock()
{
    for(int i=0; i<10; i++)
        sdcard_write(0xff);
}

// Send a command, argument, and CRC value to the card.
// Wait for a response.  Return the response code.
int sdcard_cmd(uint8_t cmd, uint32_t arg, int crc)
{
    sdcard_write(64 + cmd);
    sdcard_write((arg>>24) & 0xff);
    sdcard_write((arg>>16) & 0xff);
    sdcard_write((arg>>8) & 0xff);
    sdcard_write((arg>>0) & 0xff);
    sdcard_write(crc);
    int value = 0xff;
    int count=0;
    // The card should respond to any command within 8 bytes.
    // We'll wait for 100 to be safe.
    for(; count<100; count++) {
        value = sdcard_write(0xff);
        if (value != 0xff) break;
    }
    return value;
}

// Send an "R3" command.  (See the SD card interface documentation.)
// Return the 32-bit response.
int sdcard_r3(void)
{
    int value = 0;
    value = (value << 8) | sdcard_write(0xff);
    value = (value << 8) | sdcard_write(0xff);
    value = (value << 8) | sdcard_write(0xff);
    value = (value << 8) | sdcard_write(0xff);
    return value;
}

// Read a block of a specified length from the SD card.
int sdcard_readblock(uint8_t buffer[], int len)
{
    int value = 0xff;
    int count=0;
    for(; count<100000; count++) {
        value = sdcard_write(0xff);
        if (value != 0xff) break;
    }
    if (value != 0xfe)
        return value;
    for(int i=0; i<len; i++)
        buffer[i] = sdcard_write(0xff);
    uint8_t __attribute__((unused))crc1 = sdcard_write(0xff);
    uint8_t __attribute__((unused))crc2 = sdcard_write(0xff);
    uint8_t __attribute__((unused))check = sdcard_write(0xff); // Check that this is 0xff
    return 0xfe;
}

// Write a block of a specified length to the SD card.
//TODO: I need to check if changing "BYTE" to uint8_t is ok
int sdcard_writeblock(const uint8_t buffer[], int len)
{
    int value = 0xff;
    value = sdcard_write(0xff); // pause for one byte [expect 0xff]
    value = sdcard_write(0xfe); // start data packet [expect 0xff]
    for(int i=0; i<len; i++)
        value = sdcard_write(buffer[i]);
    value = sdcard_write(0x01); // write the crc [expect 0xff]
    value = sdcard_write(0x01); // write the crc [expect 0xff]
    do {
        value = sdcard_write(0xff); // Get the response [exepct xxx00101 == 0x1f & 0x05]
    } while(value == 0xff);
    int status = value & 0x1f;
    do {
        value = sdcard_write(0xff);
    } while(value != 0xff);
    return status;
}

//--------------------------------------END SD CARD----------------------------------------

void send_spi_cmd(spi_inst_t* spi, uint16_t value) {
    while (spi_is_busy(spi)){
    }
    spi_write16_blocking(spi, &value, 1);
}

void send_spi_data(spi_inst_t* spi, uint16_t value) {
    send_spi_cmd(spi, (value | 0x100));
}

bool sd_read_block(uint32_t block_addr, uint8_t *buffer)
{

}

bool sd_write_block(uint32_t block_addr, const uint8_t *buffer)
{

}

void cd_init() {
    sleep_ms(1);
    send_spi_cmd(spi0, 0b0000111100);
    sleep_us(40);
    send_spi_cmd(spi0, 0b0000001100);
    sleep_us(40);
    send_spi_cmd(spi0, 0b0000000001);
    sleep_ms(2);
    send_spi_cmd(spi0, 0b0000000110);
    sleep_us(40);
}

void cd_display1(const char *str) {
    send_spi_cmd(spi0, 0x80);
    while(*str != '\0'){
        send_spi_data(spi0, *str);
        str++;
    }
}

void cd_display2(const char *str) {
    send_spi_cmd(spi0, 0xC0);
    while(*str != '\0'){
        send_spi_data(spi0, *str);
        str++;
    }
}

void keypad_isr() {
    int row = 0;
    
    for (int i = 2; i < 6; i++){
        //would we change to "and" instead of == because we could possibly have a different interrupt
        if (gpio_get_irq_event_mask(i) == GPIO_IRQ_EDGE_RISE){
            gpio_acknowledge_irq(i, GPIO_IRQ_EDGE_RISE);
            row = i - 2;
            key = keymap[(col*4) + row];
            printf("Pressed: %c\n", key);
            sd_read_block(sd_counter - 1, hist_display); //TODO: needs to change to subtract by "key" value
            printf(hist_display[0]);
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
    if (uv < 100){
        uv += 2;
    }
    else{
        uv = 0;
    }
    humidity += 5;
    temp -= 3;
    //The SECOND part is what we need to send that data to the SD Card
    hw_clear_bits(&timer1_hw->intr, 1u << 0);
    //printf("%d\n", uv); //TODO: There's an issue right now - for some reason it's starting at 8. But, it does increment as expected, every second

    sprintf(weather_block, "%d", uv);

    //TODO: Check on the following two commands. Do we need to call them each time we do a read/write
    enable_sdcard();      
    sdcard_init_clock(); 

    //TODO: ChatGPT says there are additional commands, such as CMD0, CMD8, and ACMD41 to initialize. Is this true?
    sd_write_block(sd_counter, weather_block);
    sd_counter++;

    uint64_t target = timer1_hw->timerawl + 1000000;
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
    init_keypad();
    init_keypad_irq();
    init_chardisp_pins();
    init_timer_irq();
    init_sdcard_io();
    cd_init();
    cd_display1("Humidity today is:  ");
    for (;;) {
        drive_column();
    }
}

