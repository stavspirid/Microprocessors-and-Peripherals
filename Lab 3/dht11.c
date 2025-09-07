#include "dht11.h"

uint64_t DHT11_bit_read(void){
    // Start & Check Response
    int response = 1;
    uint8_t counter = 0;
    gpio_set_mode(DHT11_Pin , Output);
    gpio_set(DHT11_Pin, 0);
    delay_ms(18);    // wait at least 18 ms
    gpio_set_mode(DHT11_Pin , PullUp);
    while(gpio_get(DHT11_Pin)){
        counter++;
        if (counter == 41){
            response = 0;
            break;
        }
        delay_us(1);
    }
    //gpio_set(DHT11_Pin, 1);
    //delay_us(20);    // wait at least 20 us
    //gpio_set_mode(DHT11_Pin , Input);
    delay_us(160);
    
    // read data
    counter = 0;
    uint64_t data = 0;
    for(int i=0; i<40; i++){
        while(!gpio_get(DHT11_Pin)){
            counter++;
            if (counter == 81){
                response = 0;
                break;
            }
        }
        counter = 0;
        delay_us(32);
        if (gpio_get(DHT11_Pin)){
            // received 1
            data += 1;
            while(gpio_get(DHT11_Pin)){
                counter++;
                if (counter == 81){
                    response = 0;
                    break;
                }
            }
        }
        data <<= 1;
    }
    data += response;
    return data;
}

data_struct DHT11_read_data(void){
    uint64_t data = 0;
    data_struct split_data;
    data = DHT11_bit_read();
    int response = 0x01 & data;
    data >>= 1;
    if(response){
        split_data.check_sum = 0xFF & data;
        data >>= 8;
        split_data.temp_dec = 0xFF & data;
        data >>= 8;
        split_data.temp_int = 0xFF & data;
        data >>= 8;
        split_data.hum_dec = 0xFF & data;
        data >>= 8;
        split_data.hum_int = 0xFF & data;
    }
    else {
        uart_print("DHT11 is unresponsive. Data invalid.\r\n");
    }
    return split_data;
}