#include "platform.h"
#include "gpio.h"

uint32_t IRQ_status;
uint32_t IRQ_port_num;
uint32_t IRQ_pin_index;
uint32_t EXTI_port_set;
uint32_t prioritygroup;
uint32_t priority;

static void (*GPIO_callback)(int status);

void gpio_toggle(Pin pin) {
    gpio_set(pin, !gpio_get(pin));
}

void gpio_set(Pin pin, int value) {
    GPIO_TypeDef* p = GET_PORT(pin);
    uint32_t pin_index = GET_PIN_INDEX(pin);
    MODIFY_REG(p->ODR, 1UL << pin_index, (value & 1U) << pin_index);
}

int gpio_get(Pin pin) {
    GPIO_TypeDef* p = GET_PORT(pin);
    uint32_t pin_index = GET_PIN_INDEX(pin);
    return (p->IDR >> pin_index) & 1U;
}

void gpio_set_range(Pin pin_base, int count, int value) {
    GPIO_TypeDef* p = GET_PORT(pin_base);
    uint32_t pin_index = GET_PIN_INDEX(pin_base);
    uint32_t mask = ((1UL << count) - 1UL) << pin_index;
    MODIFY_REG(p->ODR, mask, (value << pin_index) & mask);
}

unsigned int gpio_get_range(Pin pin_base, int count) {
    GPIO_TypeDef* p = GET_PORT(pin_base);
    uint32_t pin_index = GET_PIN_INDEX(pin_base);
    uint32_t mask = ((1U << count) - 1U) << pin_index;
    return (p->IDR & mask) >> pin_index;
}

void gpio_set_mode(Pin pin, PinMode mode) {
    GPIO_TypeDef* p = GET_PORT(pin);
    uint32_t pin_index = GET_PIN_INDEX(pin);

    // enable GPIO port clock
    RCC->AHB1ENR |= 1UL << GET_PORT_INDEX(pin);
    // enable SYSCFG for EXTI
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    // allow debug in low-power
    DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP | DBGMCU_CR_DBG_STOP | DBGMCU_CR_DBG_STANDBY;

    // configure MODER & PUPDR
    switch(mode) {
        case Reset:
        case Input:
            MODIFY_REG(p->MODER, 3UL << (2*pin_index), 0UL << (2*pin_index));
            MODIFY_REG(p->PUPDR, 3UL << (2*pin_index), 0UL << (2*pin_index));
            break;
        case Output:
            MODIFY_REG(p->MODER, 3UL << (2*pin_index), 1UL << (2*pin_index));
            MODIFY_REG(p->PUPDR, 3UL << (2*pin_index), 0UL << (2*pin_index));
            break;
        case PullUp:
            MODIFY_REG(p->MODER, 3UL << (2*pin_index), 0UL << (2*pin_index));
            MODIFY_REG(p->PUPDR, 3UL << (2*pin_index), 1UL << (2*pin_index));
            break;
        case PullDown:
            MODIFY_REG(p->MODER, 3UL << (2*pin_index), 0UL << (2*pin_index));
            MODIFY_REG(p->PUPDR, 3UL << (2*pin_index), 2UL << (2*pin_index));
            break;
    }
}

void gpio_set_trigger(Pin pin, TriggerMode trig) {
    uint32_t pin_index = GET_PIN_INDEX(pin);

    switch(trig){
        case None:
            EXTI->IMR &= ~(1UL << pin_index);
            break;
        case Rising:
            EXTI->IMR |=  (1UL << pin_index);
            EXTI->RTSR |= (1UL << pin_index);
            // clear any previous pending
            EXTI->PR   =  (1UL << pin_index);
            break;
        case Falling:
            EXTI->IMR |=  (1UL << pin_index);
            EXTI->FTSR |= (1UL << pin_index);
            // clear any previous pending
            EXTI->PR   =  (1UL << pin_index);
            break;
    }
}

void gpio_set_callback(Pin pin, void (*callback)(int status)) {
    __enable_irq();

    IRQ_port_num   = GET_PORT_INDEX(pin);
    IRQ_pin_index  = GET_PIN_INDEX(pin);
    EXTI_port_set  = IRQ_port_num << ((IRQ_pin_index % 4) * 4);
    GPIO_callback  = callback;

    // map the EXTI line to this port/pin
    switch(IRQ_pin_index) {
        case 0:  SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~0xF)            | EXTI_port_set; break;
        case 1:  SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~0xF0)           | EXTI_port_set; break;
        case 2:  SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~0xF00)          | EXTI_port_set; break;
        case 3:  SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~0xF000)         | EXTI_port_set; break;
        case 4:  SYSCFG->EXTICR[1] = (SYSCFG->EXTICR[1] & ~0xF)            | EXTI_port_set; break;
        case 5: case 6: case 7: case 8: case 9:
                   SYSCFG->EXTICR[IRQ_pin_index/4] = 
                       (SYSCFG->EXTICR[IRQ_pin_index/4] & ~(0xF << (4*(IRQ_pin_index%4))))
                       | EXTI_port_set;
                   break;
        case 10: case 11: case 12: case 13: case 14: case 15:
                   SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & 
                       ~(0xF << (4*(IRQ_pin_index%4)))) | EXTI_port_set;
                   break;
    }

    // clear any pending interrupt
    EXTI->PR = (1UL << IRQ_pin_index);

    // enable and priority NVIC
    EXTI->IMR |= (1UL << IRQ_pin_index);  // ensure unmasked
    switch(IRQ_pin_index) {
        case 0:   NVIC_ClearPendingIRQ(EXTI0_IRQn);    NVIC_SetPriority(EXTI0_IRQn, 3);    NVIC_EnableIRQ(EXTI0_IRQn);    break;
        case 1:   NVIC_ClearPendingIRQ(EXTI1_IRQn);    NVIC_SetPriority(EXTI1_IRQn, 3);    NVIC_EnableIRQ(EXTI1_IRQn);    break;
        case 2:   NVIC_ClearPendingIRQ(EXTI2_IRQn);    NVIC_SetPriority(EXTI2_IRQn, 3);    NVIC_EnableIRQ(EXTI2_IRQn);    break;
        case 3:   NVIC_ClearPendingIRQ(EXTI3_IRQn);    NVIC_SetPriority(EXTI3_IRQn, 3);    NVIC_EnableIRQ(EXTI3_IRQn);    break;
        case 4:   NVIC_ClearPendingIRQ(EXTI4_IRQn);    NVIC_SetPriority(EXTI4_IRQn, 3);    NVIC_EnableIRQ(EXTI4_IRQn);    break;
        case 5: case 6: case 7: case 8: case 9:
                   NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
                   NVIC_SetPriority(EXTI9_5_IRQn, 3);
                   NVIC_EnableIRQ(EXTI9_5_IRQn);
                   break;
        case 10: case 11: case 12: case 13: case 14: case 15:
                   NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
                   NVIC_SetPriority(EXTI15_10_IRQn, 3);
                   NVIC_EnableIRQ(EXTI15_10_IRQn);
                   break;
    }
}

// ——— EXTI Handlers ———
// For each, only fire callback when PR bit is set; clear it by writing '1'

void EXTI0_IRQHandler(void) {
    if (EXTI->PR & (1UL << IRQ_pin_index)) {
        EXTI->PR = (1UL << IRQ_pin_index);
        GPIO_callback(IRQ_pin_index);
    }
}

void EXTI1_IRQHandler(void) {
    if (EXTI->PR & (1UL << IRQ_pin_index)) {
        EXTI->PR = (1UL << IRQ_pin_index);
        GPIO_callback(IRQ_pin_index);
    }
}

void EXTI2_IRQHandler(void) {
    if (EXTI->PR & (1UL << IRQ_pin_index)) {
        EXTI->PR = (1UL << IRQ_pin_index);
        GPIO_callback(IRQ_pin_index);
    }
}

void EXTI3_IRQHandler(void) {
    if (EXTI->PR & (1UL << IRQ_pin_index)) {
        EXTI->PR = (1UL << IRQ_pin_index);
        GPIO_callback(IRQ_pin_index);
    }
}

void EXTI4_IRQHandler(void) {
    if (EXTI->PR & (1UL << IRQ_pin_index)) {
        EXTI->PR = (1UL << IRQ_pin_index);
        GPIO_callback(IRQ_pin_index);
    }
}

void EXTI9_5_IRQHandler(void) {
    if (EXTI->PR & (1UL << IRQ_pin_index)) {
        EXTI->PR = (1UL << IRQ_pin_index);
        GPIO_callback(IRQ_pin_index);
    }
}

void EXTI15_10_IRQHandler(void) {
    if (EXTI->PR & (1UL << IRQ_pin_index)) {
        EXTI->PR = (1UL << IRQ_pin_index);
        GPIO_callback(IRQ_pin_index);
    }
}