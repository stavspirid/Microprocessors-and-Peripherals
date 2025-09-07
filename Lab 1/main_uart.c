#include "platform.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "uart.h"
#include "queue.h"

#define BUFF_SIZE 128

// Declare your ASM functions
extern int hash(const char *buffer);
extern int addmod(void);
extern int fib(int n);
extern char bit_xor(const char *buffer);

Queue rx_queue;

void uart_rx_isr(uint8_t rx) {
    if (rx >= 0x00 && rx <= 0x7F) {
        queue_enqueue(&rx_queue, rx);
    }
}

int main(void) {
    uint8_t rx_char = 0;
    char input_buffer[BUFF_SIZE];
    char output_buffer[128];
    uint32_t buff_index;

    // Init UART and queue
    queue_init(&rx_queue, BUFF_SIZE);
    uart_init(115200);
    uart_set_rx_callback(uart_rx_isr);
    uart_enable();

    __enable_irq(); // Global interrupts

    uart_print("\r\nSTM32 UART Hash Processor Ready (ASCII Input)\r\n");

    while (1) {
        buff_index = 0;
        uart_print("\r\nEnter your string (ASCII): ");

        // Input collection loop
        do {
            while (!queue_dequeue(&rx_queue, &rx_char))
                __WFI();

            if (rx_char == 0x7F || rx_char == '\b') {
                if (buff_index > 0) {
                    buff_index--;
                    uart_tx('\b');
                    uart_tx(' ');
                    uart_tx('\b');
                }
            } else if (rx_char != '\r' && buff_index < BUFF_SIZE - 1) {
                input_buffer[buff_index++] = (char)rx_char;
                uart_tx(rx_char); // Echo back
            }
        } while (rx_char != '\r');

        input_buffer[buff_index] = '\0'; // Null-terminate for ASM compatibility
        uart_print("\r\n");

        // ? Confirm the received input (char count + content)
        sprintf(output_buffer, "Input received (%lu chars): ", buff_index);
        uart_print(output_buffer);

        for (uint32_t i = 0; i < buff_index; ++i) {
            uart_tx(input_buffer[i]);
        }
        uart_print("\r\n");

        // ?? Call your assembly functions
        int hash_value = hash(input_buffer);
        int mod7 = addmod();
        int fib_result = fib(mod7);
        int xor_result = bit_xor(input_buffer);

        // ?? Output results
        sprintf(output_buffer, "Hash value: %d\r\n", hash_value);
        uart_print(output_buffer);

        sprintf(output_buffer, "Hash mod 7: %d\r\n", mod7);
        uart_print(output_buffer);

        sprintf(output_buffer, "Fibonacci of %d: %d\r\n", mod7, fib_result);
        uart_print(output_buffer);

        sprintf(output_buffer, "Bitwise XOR of input: %u\r\n", xor_result);
        uart_print(output_buffer);
    }
}
