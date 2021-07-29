#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include <stdint.h>
#include <stdio.h>
#include "hygrometer.h"
#include "util.h"

#define HYGRO_DATA GPIO_ODR_8

unsigned char dataBuffer[5];
int bufferPtr = 0;
int respSignalFlag = 0;
int dataTransmissionFlag = 0;

void _insert_buffer(int bit) {
	if (bufferPtr >= 40)
		return;
	dataBuffer[bufferPtr / 8] &= ~(1 << (bufferPtr % 8));
	dataBuffer[bufferPtr / 8] |= bit << (bufferPtr % 8);
	bufferPtr++;
}

void _clear_buffer() {
	for(int i = 0; i < sizeof(dataBuffer) / sizeof(*dataBuffer); i++) {
		dataBuffer[i] = 0;
	}
	bufferPtr = 0;
}

void hygrometer_init() {
	// enable clock for GPIOA
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	// set PA8 to output mode
	GPIOA->MODER &= ~GPIO_MODER_MODER8_1;
	GPIOA->MODER |= GPIO_MODER_MODER8_0;
	// set data pin to output high
	GPIOA->ODR |= HYGRO_DATA;
	// set EXTI to trigger on falling edge
	EXTI->FTSR |= EXTI_FTSR_TR8;
	// mask interrupt for the EXTI
	EXTI->IMR &= ~EXTI_IMR_MR8;
	_tim6_init();
}

void _falling_edge_handler() {
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

void _rising_edge_handler() {
	if(!respSignalFlag) {
		if(value_check(tim6_stop(), 80, 5))
			respSignalFlag = 1;
	}
	tim6_start();
}

void hygrometer_read(short* temp, short* humidity) {
	// set data pin to output mode
	GPIOA->MODER &= ~GPIO_MODER_MODER8_1;
	GPIOA->MODER |= GPIO_MODER_MODER8_0;
	GPIOA->ODR &= ~HYGRO_DATA;
	micro_wait(20000);
	GPIOA->ODR |= HYGRO_DATA;
	// enable interrupt for the EXTI
	EXTI->IMR |= EXTI_IMR_MR8;
	// set data pin to input mode
	GPIOA->MODER &= ~GPIO_MODER_MODER8;
	// wait for data transmission to complete
	while(bufferPtr < 40);
	micro_wait(50);
	// set data pin to output high
	GPIOA->MODER &= ~GPIO_MODER_MODER8_1;
	GPIOA->MODER |= GPIO_MODER_MODER8_0;
	GPIOA->ODR |= HYGRO_DATA;
	// clear the status flags
	int respSignalFlag = 0;
	int dataTransmissionFlag = 0;
	// interpret data
	*humidity = dataBuffer[0] << 8 | dataBuffer[1];
	*temp = dataBuffer[2] << 8 | dataBuffer[3];
	_clear_buffer();
}
