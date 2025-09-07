/*!
 * \file      dht11.h
 * \brief     
 
 */

#ifndef DHT11_H
#define DHT11_H

#include "platform.h"
#include <gpio.h>
#include "delay.h"
#include "uart.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define DHT11_Pin PA_1   // A1

typedef struct {
    uint8_t temp_int;
    uint8_t temp_dec;
    uint8_t hum_int;
    uint8_t hum_dec;
    uint8_t check_sum;
} data_struct;

uint64_t DHT11_bit_read(void);

extern void delay_cycles(uint32_t ms);

data_struct DHT11_read_data(void);
#endif // DHT11_H
