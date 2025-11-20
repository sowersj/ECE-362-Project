#include "hardware/uart.h" 
#include "hardware/gpio.h" 
#include <stdio.h>  
#include <string.h>
#include "pico/stdlib.h"
#include "sdcard.h"
#include <stdlib.h>

#define BUFSIZE 100
char serbuf[BUFSIZE];
int seridx = 0;
int newline_seen = 0;

// add this here so that compiler does not complain about implicit function
// in init_uart_irq
void uart_rx_handler();

/*******************************************************************/

void init_uart() {
    // fill in.
    gpio_set_function(0, UART_FUNCSEL_NUM(uart0, 0));
    gpio_set_function(1, UART_FUNCSEL_NUM(uart0, 1)); 
    uart_init(uart0, 115200);
    uart_set_format(uart0, 8, 1, UART_PARITY_NONE); //I changed this from 0 to UART_PARITY_NONE
}

void init_uart_irq() {
    // fill in.
    uart_set_fifo_enabled(uart0, 0);
    uart0_hw->imsc |= (1 << 4);
    //uart_set_irqs_enabled(uart0, 1, 1);//I'm assuming we maybe don't need this, since we've disabled the FIFOs
    irq_set_enabled(33, 1); //enabled the UART interrupt
    irq_set_exclusive_handler(33, uart_rx_handler);
}

void uart_rx_handler() {
    // fill in.
        // fill in.
    uart0_hw->icr |= 1; //double check which one to clear should be 0x10
    if (seridx == BUFSIZE){
        return;
    }
    uint8_t c;
    uart_read_blocking(uart0, &c, 1); //is there a problem with this? They said no SDK functions needed...
    if (c == (0x0A)){
        newline_seen = 1;
    }
    if ((seridx > 0) && (c == 8)){
        uart_putc(uart0, 8);
        uart_putc(uart0, 32);
        uart_putc(uart0, 8);
        seridx = seridx - 1;
        serbuf[seridx] = '\0';
        return;
    }
    else {
        serbuf[seridx] = c;
        uart_putc(uart0, serbuf[seridx]);
        seridx = seridx + 1;
    }
}

int _read(__unused int handle, char *buffer, int length) {
    // fill in.
    while (newline_seen == 0){
        sleep_ms(5);
    }
    newline_seen = 0;
    if (length > seridx){
        strncpy(buffer, serbuf, seridx);
        int temp = seridx;
        seridx = 0;
        return(temp);
    }
    else{
        strncpy(buffer, serbuf, length);
        seridx = 0;
        return(length);
    }
}

int _write(__unused int handle, char *buffer, int length) {
    // fill in.
    uart_write_blocking(uart0, (uint8_t*)buffer, length);
    return(length);
}

/*******************************************************************/

struct commands_t {
    const char *cmd;
    void      (*fn)(int argc, char *argv[]);
};

struct commands_t cmds[] = {
        { "append", append },
        { "cat", cat },
        { "cd", cd },
        { "date", date },
        { "input", input },
        { "ls", ls },
        { "mkdir", mkdir },
        { "mount", mount },
        { "pwd", pwd },
        { "rm", rm },
        { "restart", restart }
};

// This function inserts a string into the input buffer and echoes it to the UART
// but whatever is "typed" by this function can be edited by the user.
void insert_echo_string(const char* str) {
    // Print the string and copy it into serbuf, allowing editing
    seridx = 0;
    newline_seen = 0;
    memset(serbuf, 0, BUFSIZE);

    // Print and fill serbuf with the initial string
    for (int i = 0; str[i] != '\0' && seridx < BUFSIZE - 1; i++) {
        char c = str[i];
        uart_write_blocking(uart0, (uint8_t*)&c, 1);
        serbuf[seridx++] = c;
    }
}

void parse_command(const char* input) {
    size_t len = strlen(input); //new line
    char * copystring; //new line
    copystring = (char *) malloc(len + 1); //new line
    strcpy(copystring, input); //new line

    char *token = strtok(copystring, " "); //we changed "input" to "copystring"
    int argc = 0;
    char *argv[10];
    while (token != NULL && argc < 10) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }

    int i = 0;
    for(; i<sizeof cmds/sizeof cmds[0]; i++) {
        if (strcmp(cmds[i].cmd, argv[0]) == 0) {
            cmds[i].fn(argc, argv);
            break;
        }
    }
    if (i == (sizeof cmds/sizeof cmds[0])) {
        printf("Unknown command: %s\n", argv[0]);
    }
    free(copystring);//New line - need to free memory
}

void command_shell() {
    char input[100];
    memset(input, 0, sizeof(input));

    // Disable buffering for stdout
    setbuf(stdout, NULL);

    printf("\nEnter current ");
    insert_echo_string("date 20250701120000");
    fgets(input, 99, stdin);
    input[strcspn(input, "\r")] = 0; // Remove CR character
    input[strcspn(input, "\n")] = 0; // Remove newline character
    parse_command(input);
    
    printf("SD Card Command Shell");
    printf("\r\nType 'mount' to mount the SD card.\n");
    while (1) {
        printf("\r\n> ");
        fgets(input, sizeof(input), stdin);
        fflush(stdin);
        input[strcspn(input, "\r")] = 0; // Remove CR character
        input[strcspn(input, "\n")] = 0; // Remove newline character
        
        parse_command(input);
    }
}