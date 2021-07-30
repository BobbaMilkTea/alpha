#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include "stdint.h"
#include "stdio.h"
#include "include/hygrometer.h"
#include "include/util.h"

unsigned char dataBuffer[5];
int bufferPtr = 0;
int respSignalFlag = 0;
int dataTransmissionFlag = 0;

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
	// set EXTI to trigger on both rising and falling edge
	EXTI->RTSR |= EXTI_RTSR_TR0;
	EXTI->FTSR |= EXTI_FTSR_TR0;
	// mask interrupt for the EXTI
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
	// set data pin to pull up
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR0_1;
	GPIOA->PUPDR |= GPIO_PUPDR_PUPDR0_0;
	// set data pin to output high
	GPIOA->ODR |= GPIO_ODR_0;
	_enable_EXTI();
	tim6_init();
}

static void _falling_edge_handler() {
	if(respSignalFlag && !dataTransmissionFlag) {
		if(value_check(tim6_stop(), 80, 5))
			dataTransmissionFlag = 1;
	}
	else if(respSignalFlag && dataTransmissionFlag) {
		// determine bit logic level
		int micro_elapsed = tim6_stop();
		if(value_check(micro_elapsed, 27, 5))
			_insert_buffer(0);
		else if(value_check(micro_elapsed, 70, 5))
			_insert_buffer(1);
	}
	tim6_start();
}

static void _rising_edge_handler() {
	if(!respSignalFlag) {
		if(value_check(tim6_stop(), 80, 5))
			respSignalFlag = 1;
	}
	tim6_start();
}

void EXTI4_15_IRQnHandler() {
	EXTI->PR |= EXTI_PR_PR0;
	if(GPIOA->IDR & GPIO_IDR_8)
		_rising_edge_handler();
	else
		_falling_edge_handler();
}

void hygrometer_read(short* temp, short* humidity) {
	// enable interrupt for the EXTI
	EXTI->IMR |= EXTI_IMR_MR0;
	GPIOA->ODR &= ~GPIO_ODR_0;
	micro_wait(20000);
	GPIOA->ODR |= GPIO_ODR_0;
	// set data pin to input mode
	GPIOA->MODER &= ~GPIO_MODER_MODER0;
	// wait for data transmission to complete
	while(bufferPtr < 40);
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
	respSignalFlag = 0;
	dataTransmissionFlag = 0;
	_clear_buffer();
	EXTI->IMR &= ~EXTI_IMR_MR0;
}
