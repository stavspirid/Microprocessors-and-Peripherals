#include "platform.h"
#include "uart.h"
#include "queue.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <timer.h>
#include <gpio.h>
#include <delay.h>

#define BUFF_SIZE         128
#define MAX_DIGITS        32
#define DIGIT_INTERVAL_MS 500   // 0.5s between digits
#define BLINK_INTERVAL_MS 200   // 0.2s blink period
#define ASCII_OFFSET 			48 		// offset of number to character in ASCII coding

static Queue   	rx_queue;
static char    	buff[BUFF_SIZE];
static char    	digit_buf[MAX_DIGITS+1];
static uint8_t 	digit_count;
static bool    	timer_flag;
static bool    	led_enabled = true;
static int     	button_count = 0;
static bool 		new_input;
static bool			repeat_mode; 

// Timer ISR
void timer_0_5isr(void) {
    timer_flag = true;
}

// Button ISR
void button_isr(int status) {
    (void)status;
    button_count++;
    if (button_count%2 == 1) {
        led_enabled = false;
        char msg[64];
        sprintf(msg, "Interrupt: Button pressed. LED locked. Count = %d\r\n", button_count);
        uart_print(msg);
    } else {
        led_enabled = true;
        char msg[64];
        sprintf(msg, "Interrupt: Button pressed. LED unlocked. Count = %d\r\n", button_count);
        uart_print(msg);
    }
}

// UART isr
void uart_rx_isr(uint8_t c) {
    if ((c >= '0' && c <= '9') || c == '-' || c == '\r' || c == 0x7F) {
        queue_enqueue(&rx_queue, c);
        if (c == '\r') {
            timer_flag = false; 	// Sync first digit
						repeat_mode = false;
						new_input = true;
        }
    }
}

int main(void) {
   
    uint8_t rx;
    uint32_t idx;
	
	// Timer 0.5s 
    timer_set_callback(timer_0_5isr);
    timer_init(500000);
    timer_enable();
 
    // UART Setup
    queue_init(&rx_queue, BUFF_SIZE);
    uart_init(115200);
    uart_set_rx_callback(uart_rx_isr);
    uart_enable();
    NVIC_SetPriority( USART2_IRQn, 2 );			// UART priority < Button priority

    // Button Setup
    gpio_set_mode(P_SW, PullUp);
    gpio_set_trigger(P_SW, Falling);
    gpio_set_callback(P_SW, button_isr);
	NVIC_SetPriority( EXTI15_10_IRQn, 0 );	// Button priority > UART priority

	// LED Enable
	gpio_set_mode(PA_5, Output);
    
    uart_print("\r\n");

    while (1) {
        uart_print("Input: ");
				idx = 0;
        memset(buff, 0, BUFF_SIZE);	// Reset buffer
            
        do {
            while (!queue_dequeue(&rx_queue, &rx))
                __WFI();
            if (rx == 0x7F && idx) {
                idx--; uart_tx(0x7F);
            } else if (rx == '\r') {
                uart_print("\r\n");
            } else {
                buff[idx++] = rx;
                uart_tx(rx);
            }
        } while (rx != '\r' && idx < BUFF_SIZE);
				
				buff[idx] = '\0';

        // Check for - on end
        repeat_mode = (idx > 0 && buff[idx-1] == '-');
        if (repeat_mode)
					buff[--idx] = '\0';

        // Save digits 
        digit_count = 0;
        for (uint32_t i=0; buff[i] && digit_count < MAX_DIGITS; i++) {
            if (buff[i] >= '0' && buff[i] <= '9' || buff[i] == '-')
                digit_buf[digit_count++] = buff[i];
        }
                
				// Save '\0' at the end of digits
        digit_buf[digit_count] = '\0';
                
        if (!digit_count) {
            uart_print("No digits to analyze.\r\n");
            continue;
        }

        timer_flag = false;
        new_input = false;

        // Process digits (with optional repeat)    
        do {
					for (uint8_t i=0; i<digit_count; i++) {
						if(new_input){
								continue;
						}              
						while (!timer_flag) __WFI();
						timer_flag = false;

						uint8_t d = digit_buf[i];
						char msg[64];
						
						
						// Dash
						if (d == '-'){
								sprintf(msg, "Digit Dash\r\n");
								uart_print(msg);
						}
						// Odd Number
						else if ((d - '0')%2 == 1) {
								sprintf(msg, "Digit %u -> Toggle LED\r\n", buff[i]-ASCII_OFFSET);
								uart_print(msg);
								if(led_enabled){
									gpio_toggle(PA_5);
								}
						} 
						// Even Number
						else {
							sprintf(msg, "Digit %u -> Blink LED\r\n", buff[i]-ASCII_OFFSET);
							uart_print(msg);
							uint32_t elapsed = 0;
							while (elapsed < DIGIT_INTERVAL_MS) {
								if(led_enabled){
									gpio_toggle(PA_5);
								}
								delay_ms(BLINK_INTERVAL_MS);
								elapsed += BLINK_INTERVAL_MS;
							}	
						}
					}
				}while (repeat_mode);
                    
				if(new_input)
						uart_print("Sequence interrupted. Waiting for new number...\r\n");
        else
						uart_print("End of sequence. Waiting for new number...\r\n");
    }
    return 0;
}