#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"

/* Equations
   // 12-bit conversion assuming max value == ADC_VREF == 3.3 V
   voltage = result * 3.3f / (1 << 12)  (result is direct ADC output)
   UV
   1 UVI = 26nA
   V = I * R
   R = 100k => 2.6 mV / 1 UVI; R = 1M => 26 mV / 1 UVI; R = 10M => 260 mV / 1 UVI;
   Ideal to use R = 1M or lower if connecting to 3.3V rail
*/

// 7-segment display message buffer
// Declared as static to limit scope to this file only.
/*static char msg[8] = {
    0x3F, // seven-segment value of 0
    0x06, // seven-segment value of 1
    0x5B, // seven-segment value of 2
    0x4F, // seven-segment value of 3
    0x66, // seven-segment value of 4
    0x6D, // seven-segment value of 5
    0x7D, // seven-segment value of 6
    0x07, // seven-segment value of 7
};
extern char font[]; // Font mapping for 7-segment display
static int index = 0; // Current index in the message buffer*/
uint32_t adc_fifo_out = 0;
volatile uint16_t adc_result;

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

/*void init_adc_dma() {
    adc_init();
    adc_gpio_init(26);        // GPIO26 = ADC0
    adc_select_input(0);

    // Setup FIFO
    adc_fifo_setup(
        true,    // enable FIFO
        true,    // enable DMA DREQ
        1,       // DREQ when 1 sample available
        false,   // no error bit
        false    // no 8-bit shift
    );

    int dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, DREQ_ADC);

    dma_channel_configure(
        dma_chan,
        &c,
        &adc_result,     // dest
        &adc_hw->fifo,   // src
        1,               // transfer count
        true             // start immediately
    );

    adc_run(true);
}*/

/*void display_char_print(const char message[]) {
    for (int i = 0; i < 8; i++) {
        msg[i] = font[message[i] & 0xFF];
    }
}

void display_init_pins() {
    // outputs
    sio_hw -> gpio_oe_set = 0x7ff << 10;
    //sio_hw -> gpio_set = 0x4ff << 10;

    for (int pin = 10; pin < 21; pin++) {
            *(io_rw_32 *) hw_xor_alias_untyped((volatile void *) &pads_bank0_hw->io[pin]) = (*&pads_bank0_hw->io[pin] ^ PADS_BANK0_GPIO0_IE_BITS) & (PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS);
            io_bank0_hw->io[pin].ctrl = GPIO_FUNC_SIO << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
            *(io_rw_32 *) hw_clear_alias_untyped((volatile void *) &pads_bank0_hw->io[pin]) = PADS_BANK0_GPIO0_ISO_BITS;
    }

}

void display_isr() {
    //printf("in display_irs\n");

    timer1_hw->intr = 1u << 0; // acknowledges the interrupt

    sio_hw -> gpio_clr = 0x7ff << 10;
    //printf("index: %d\n", index);
    int set_val = (index << 8) | msg[index];
    sio_hw -> gpio_set = (set_val << 10);

    index = (index + 1) & 0x7;

    uint32_t target0 = timer1_hw->timerawl + 3000;
    timer1_hw->alarm[0] = target0;
}

void display_init_timer() {
    hw_set_bits(&timer1_hw->inte, 1u << 0);

    irq_set_exclusive_handler(TIMER1_IRQ_0, display_isr);

    irq_set_enabled(TIMER1_IRQ_0, true);

    uint32_t target0 = timer1_hw->timerawl + 3000;
    timer1_hw->alarm[0] = target0;
}*/

int main() {
    stdio_init_all();

    //display_init_pins();
    //display_init_timer();

    init_adc_dma();
    for(;;) {
        //printf("in main\n");
        float v = (adc_fifo_out * 3.3) / (1u << 12);
        float uvi = v / 0.026f; // using 1M resistor
        //printf("uvi: %4.7f\n", uvi);
        printf("ADC=%d  V=%.3f  UVI=%.2f\n", adc_fifo_out, v, uvi);

        sleep_ms(500);
    }

    /*init_adc();
    for(;;) {
        printf("ADC Result: %d     \r", read_adc());
        // We've found that when we do NOT send a newline character,
        // the output is not flushed immediately, which can cause
        // the output to be delayed or not appear at all in some cases.
        // The fflush function forces a flush.
        fflush(stdout);
        sleep_ms(250);
    }*/

    for(;;);
    return 0;
}