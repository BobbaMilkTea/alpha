#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include "stdint.h"
#include "stdio.h"
#include "include/hygrometer.h"
#include "include/util.h"

unsigned char dataBuffer[5];
int bufferPtr = 0;
int commPhase = 0; // 0: before resp from DHT11, 1: during resp from DHT11, 2: after resp from DHT11 (data transfer)

static void _insert_buffer(int bit) {
	if (bufferPtr >= 40)
		return;
	dataBuffer[bufferPtr / 8] &= ~(1 << (bufferPtr % 8));
	dataBuffer[bufferPtr / 8] |= bit << (bufferPtr % 8);
	bufferPtr++;
}

static void _clear_buffer() {
	for(int i = 0; i < sizeof(dataBuffer) / sizeof(*dataBuffer); i++) {
		dataBuffer[i] = 0;
	}
	bufferPtr = 0;
}

static void _enable_EXTI() {
	// enable clock for system config
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	// enable EXTI for PA0 in system config
	SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;
	// set EXTI to trigger on falling edge
	EXTI->FTSR |= EXTI_FTSR_TR0;
	// mask interrupt for EXTI
	EXTI->IMR &= ~EXTI_IMR_MR0;
	// enable interrupt in NVIC
	NVIC->ISER[0] |= 1 << EXTI0_1_IRQn;
}

void hygrometer_init() {
	// enable clock for GPIOA
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	// set data pin to output mode
	GPIOA->MODER &= ~GPIO_MODER_MODER0_1;
	GPIOA->MODER |= GPIO_MODER_MODER0_0;
	// set data pin to output high
	GPIOA->ODR |= GPIO_ODR_0;
	_enable_EXTI();
	tim6_init();
}

void EXTI0_1_IRQHandler() {
	int micro_elapsed = tim6_stop();
	tim6_start();
	switch(commPhase) {
	case 0: commPhase += 1;
		break;
	case 1: commPhase += value_check(micro_elapsed, 160, 10);
		break;
	case 2:
		if(value_check(micro_elapsed, 77, 5))
			_insert_buffer(0);
		else if(value_check(micro_elapsed, 120, 5))
			_insert_buffer(1);
	}
	EXTI->PR |= EXTI_PR_PR0;
}

void hygrometer_read(short* temp, short* humidity) {
	// send data request
	GPIOA->ODR &= ~GPIO_ODR_0;
	micro_wait(20000);
	GPIOA->ODR |= GPIO_ODR_0;
	// set data pin to input mode
	GPIOA->MODER &= ~GPIO_MODER_MODER0;
	// enable interrupt for the EXTI
	EXTI->IMR |= EXTI_IMR_MR0;
	// wait for data transmission to complete
	while(bufferPtr < 40);
	// mask interrupt when output received
	EXTI->IMR &= ~EXTI_IMR_MR0;
	// set data pin to output mode
	GPIOA->MODER &= ~GPIO_MODER_MODER0_1;
	GPIOA->MODER |= GPIO_MODER_MODER0_0;
	// set data pin to output high
	GPIOA->MODER &= ~GPIO_MODER_MODER0_1;
	GPIOA->MODER |= GPIO_MODER_MODER0_0;
	GPIOA->ODR |= GPIO_ODR_0;
	// interpret data
	*humidity = dataBuffer[0] << 8 | dataBuffer[1];
	*temp = dataBuffer[2] << 8 | dataBuffer[3];
	// reset status
	commPhase = 0;
	_clear_buffer();
}
