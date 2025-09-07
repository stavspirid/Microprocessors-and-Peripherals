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
//dht 11 driver
#include "dht11.h"

#define BUFF_SIZE   128
#define MAX_DIGITS  32
#define MAX_AEM 		5

static Queue   rx_queue;
static char    buff[BUFF_SIZE];
static char    digit_buf[MAX_DIGITS+1];
static bool pass_accepted = false;
static const char password[] = "admin" ;
static bool aem_accepted = false;
static int aem[MAX_AEM];
static uint8_t aem_count;
static bool aem_valid;
static bool menu_print = false;
volatile int mode_changes = 0;
static int print_speed = 2;
volatile int timer_count = 0;
static int print_show = 0; // 0:Temp 1:Hum 2:Both
static char msg[64];
static int temp_arr[3];
static int hum_arr[3];
// 3 values of temp/hum should be available every moment. Thus we have this int cycling 0 through 2 and on each read
// we save on the each arr 
static int arr_idx = 0 ; 
volatile bool led_flag = false;
static int alert_count = 0;
static char buff1[BUFF_SIZE];
static bool new_input = false;
static bool aem_pass =false;

// DHT data struct
data_struct data;


// Timer isr
void timer_1isr(void) {
    timer_count ++ ;
        if(led_flag){
        gpio_toggle(PB_0);
        }
}

// Touch sensor isr
// mode_changes %2 == 0 => mode A 
// mode_changes%2 == 1 => mode B 

void touch_isr(int status){
    (void)status;
    mode_changes++;
        
}

// UART isr
void uart_rx_isr(uint8_t rx) {
    // Check if the received character is a printable ASCII character
    if (rx >= 0x0 && rx <= 0x7F ) {
        // Store the received character
        queue_enqueue(&rx_queue, rx);
    }
    if(rx == '\r'){
        new_input = true;
    }
}

int main(void) {
                
    uint8_t rx;
    uint32_t idx;
        
    // UART 
    queue_init(&rx_queue, BUFF_SIZE);
    uart_init(115200);
    uart_set_rx_callback(uart_rx_isr);
    uart_enable();

    // Timer 1s 
    timer_set_callback(timer_1isr);
    timer_init(1000000);
    timer_enable();

        
    // Touch sensor A2 
    gpio_set_mode(PA_4, PullDown);
    gpio_set_trigger(PA_4, Rising);
    gpio_set_callback(PA_4, touch_isr);
        
    //Led Enable A3
    gpio_set_mode(PB_0, Output);
        
    uart_print("\r\n");
    uart_print("Enter password: ");
    uart_print("\r\n");

    while (1) {
                
        idx = 0;
        // clear buffer buff
        memset(buff, 0, BUFF_SIZE);
            
        // UART
        do {
            while (!queue_dequeue(&rx_queue, &rx))
                __WFI();
            if (rx == 0x7F && idx) {
                idx--; uart_tx(0x7F);
            } else if (rx == '\r') {
                uart_print("\r\n");
            } else {
                buff1[idx++] = rx;
                uart_tx(rx);
            }
        } while (rx != '\r' && idx < BUFF_SIZE);
				
				buff1[idx] = '\0';
						
				int buff_add = 0;
				// Copy buff to another buffer
				for(int i = 0; buff1[i] && i < BUFF_SIZE; i++){
						buff[buff_add++] = buff1[i];
				}
				buff[buff_add] = '\0';
				
				// AEM
        if(pass_accepted){
						if(!aem_accepted){
								aem_valid = true;
								aem_count = 0;
								if(!buff[0]){
										aem_valid = false;
										uart_print("You entered an empty string, not an AEM. Try again!\r\n");
										break;
								}
								for (uint32_t i=0; buff[i] ; i++) {
										if (buff[i] >= '0' && buff[i] <= '9'){
														aem[aem_count++] = buff[i] - '0';
										}
										else{
												aem_valid = false;
												uart_print("You did not enter an AEM, try again.\r\n");
												break;
										}
								}
								if(aem_valid){
								aem_accepted = true;
								}
						}
				}
                            
				// Password logic
				if (!pass_accepted) {
						if (strcmp(buff, password) == 0) {
								pass_accepted = true;
								uart_print("Enter your AEM.\r\n");
						} 
						else {
								uart_print("Wrong password. Try again\r\n");
						}
				}
                
				//MENU PRINT
				if(pass_accepted && aem_accepted && !menu_print){
						uart_print("MENU\r\n");
						uart_print("a: Read/Print speed up (max speed 2sec per print) \r\n");
						uart_print("b: Read/Print speed down (min speed 10s per print)\r\n");
						uart_print("c: Change of printing between Temp/Hum/Both\r\n");
						uart_print("d: Print of last values and system status\r\n");
						menu_print = true;
				}            
                
				new_input = false;
				// REST LOGIC
				if(pass_accepted && aem_accepted && menu_print){
						//menu logic
						if(buff[1] == '\0' && aem_pass){
								if(buff[0] == 'a'){
										if(print_speed >2)
												print_speed --;
								}
								else if (buff[0] == 'b'){
										if(print_speed <10)
												print_speed ++;
								}
								else if (buff[0] == 'c'){
										print_show ++;
										print_show = print_show%3;
										if (print_show %3 == 0) {
											print_speed = aem[aem_count-1] + aem[aem_count];		// TODO: Change indexing
										} 
								}
								else if (buff[0] == 'd'){
										sprintf(msg, "Last value of temperature: %u \r\n", data.temp_dec);
										uart_print(msg);
										sprintf(msg, "Last value of humidity: %u %% \r\n", data.hum_dec);
										uart_print(msg);
										if(mode_changes %2 == 0){
												uart_print("Current opperating mode: A \r\n");
										}
										else{
												uart_print("Current opperating mode: B \r\n");
										}
								}
								else{
										uart_print("You entered a letter that is not supported \r\n");
								}
						}
								// string status as input from UART 
								if(strcmp("status",buff)==0){
										if(mode_changes %2 == 0){
												uart_print("Current opperating mode: A \r\n");
										}
										else{
												uart_print("Current opperatingmode: B \r\n");
										}
										sprintf(msg, "Last value of Temperature: %u \r\n", data.temp_int );
										uart_print(msg);
										sprintf(msg, "Last value of Humidity: %u %%\r\n", data.hum_int);
										uart_print(msg);
										sprintf(msg, "Number of printing changes: %u \r\n", mode_changes);
										uart_print(msg);
								}
								// any other input
								if(strcmp("status",buff)!= 0 && buff[1] != '\0' && aem_pass){
										uart_print("You entered an input that is not supported \r\n" );
								}
								aem_pass = true;
								
								// reading from dht 11 logic
								do{
										if((temp_arr[0] > 35 && temp_arr[1] >35 && temp_arr[2] > 35) || (hum_arr[0] > 80 && hum_arr[1] > 80 && hum_arr[2] > 80)){
												uart_print("DANGER , perfoming software reset \r\n");
												NVIC_SystemReset();
										}
								
										// Printing speed
										while(timer_count < print_speed){
												if(new_input){
														break;
												}
												__WFI();
										}
										timer_count = 0;
								
										if(!new_input){
												data = DHT11_read_data();
												temp_arr[arr_idx] = data.temp_int;
												hum_arr[arr_idx] = data.hum_int;
												arr_idx ++;
												arr_idx = arr_idx %3;
											
												// printing after reading
												if(print_show % 3 == 0){			// only Temperature
														sprintf(msg,"Temperature is %u \r\n",data.temp_int);
														uart_print(msg);
												}
												else if(print_show %3 == 1){	//only Humidity
														sprintf(msg,"Humidity is %u %%\r\n", data.hum_int);
														uart_print(msg);
												}
												else{													// Both
														sprintf(msg,"Temperature is %u \r\n", data.temp_int);
														uart_print(msg);
														sprintf(msg,"Humidity is %u %%\r\n", data.hum_int);
														uart_print(msg);
												}
												
												//MODE B
												if(mode_changes % 2 == 1){
														if(data.temp_int > 25 || data.hum_int > 60){
																// uart_print("In the loop of led \r\n");
																led_flag = true;
																alert_count = 0;
														}
														else{
																if(led_flag && (data.temp_int < 25 && data.hum_int < 60)){
																		alert_count ++;
																		alert_count = alert_count % 5;
																		if(alert_count == 0){
																				led_flag = false;
																				// turn the led off
																				gpio_set(PB_0,0);
																		}
																}
														}
												}
										}
						}while(!new_input);
				}
                
    }
    return 0;
}